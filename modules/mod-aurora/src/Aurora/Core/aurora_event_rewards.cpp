// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_event_rewards.cpp - event participation and reward claims.
//
// Split out of the event manager so the scheduler stays readable and the
// reward path can be reasoned about on its own.
//
// Data flow
//   1. aurora_event_manager.cpp opens a row in `aurora_active_events` when an
//      event starts and stores its id in memory.
//   2. Every 10 s the manager credits `aurora_event_progress.participation_sec`
//      for each player online (see CreditParticipation in the manager).
//   3. The counters this file needs (kills, quests, boss kills) are mirrored
//      from `aurora_player_activity` deltas, so no extra per-kill hook is
//      required and a crash cannot double-count.
//   4. Before the event stops, GrantRewards() walks the tiers defined in
//      `aurora_event_rewards`, mails everything the player has qualified for,
//      and records each claim in `aurora_event_claims`.
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Mail.h"
#include "Item.h"
#include "StringFormat.h"
#include "DBCStores.h"
#include "ObjectAccessor.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    struct RewardTier
    {
        uint32      id             = 0;
        std::string tier;
        std::string requirementType;
        uint32      requirementValue = 0;
        uint32      itemEntry      = 0;
        uint32      itemAmount     = 1;
        uint32      auroraPoints   = 0;
        uint32      shards         = 0;
        uint32      titleDbcId     = 0;
        uint32      visualEnchant  = 0;
        std::string nameRu;
        std::string nameEn;
    };

    // Tiers are cached per event and reloaded when the event starts.
    std::unordered_map<uint32, std::vector<RewardTier>> g_rewardCache;

    std::vector<RewardTier> const& GetTiers(uint32 eventId)
    {
        auto itr = g_rewardCache.find(eventId);
        if (itr != g_rewardCache.end())
            return itr->second;

        std::vector<RewardTier> tiers;

        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `tier`, `requirement_type`, `requirement_value`, "
            "`item_entry`, `item_amount`, `aurora_points`, `shards`, "
            "`title_dbc_id`, `visual_enchant_id`, `name_ru`, `name_en` "
            "FROM `aurora_event_rewards` WHERE `event_id` = {} "
            "ORDER BY `requirement_value` ASC", eventId);

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();

                RewardTier tier;
                tier.id               = fields[0].Get<uint32>();
                tier.tier             = fields[1].Get<std::string>();
                tier.requirementType  = fields[2].Get<std::string>();
                tier.requirementValue = fields[3].Get<uint32>();
                tier.itemEntry        = fields[4].Get<uint32>();
                tier.itemAmount       = fields[5].Get<uint32>();
                tier.auroraPoints     = fields[6].Get<uint32>();
                tier.shards           = fields[7].Get<uint32>();
                tier.titleDbcId       = fields[8].Get<uint32>();
                tier.visualEnchant    = fields[9].Get<uint32>();
                tier.nameRu           = fields[10].Get<std::string>();
                tier.nameEn           = fields[11].Get<std::string>();

                tiers.push_back(tier);
            } while (result->NextRow());
        }

        auto inserted = g_rewardCache.emplace(eventId, std::move(tiers));
        return inserted.first->second;
    }

    // ------------------------------------------------------------------------
    // Current progress of one player in one event instance.
    // ------------------------------------------------------------------------
    struct Progress
    {
        uint32 kills  = 0;
        uint32 quests = 0;
        uint32 helps  = 0;
        uint32 bosses = 0;
        uint32 timeSec = 0;
    };

    bool LoadProgress(uint32 activeEventId, uint32 guid, Progress& out)
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT `mobs_killed`, `quests_completed`, `players_helped`, "
            "`bosses_killed`, `participation_sec` "
            "FROM `aurora_event_progress` "
            "WHERE `active_event_id` = {} AND `player_guid` = {}", activeEventId, guid);

        if (!result)
            return false;

        Field* fields = result->Fetch();
        out.kills   = fields[0].Get<uint32>();
        out.quests  = fields[1].Get<uint32>();
        out.helps   = fields[2].Get<uint32>();
        out.bosses  = fields[3].Get<uint32>();
        out.timeSec = fields[4].Get<uint32>();
        return true;
    }

    uint32 ValueFor(RewardTier const& tier, Progress const& progress)
    {
        if (tier.requirementType == "kills")   return progress.kills;
        if (tier.requirementType == "quests")  return progress.quests;
        if (tier.requirementType == "helps")   return progress.helps;
        if (tier.requirementType == "bosses")  return progress.bosses;
        if (tier.requirementType == "time")    return progress.timeSec;
        return 0;
    }

    bool AlreadyClaimed(uint32 activeEventId, uint32 guid, uint32 rewardId)
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT 1 FROM `aurora_event_claims` "
            "WHERE `active_event_id` = {} AND `player_guid` = {} AND `reward_id` = {} "
            "LIMIT 1", activeEventId, guid, rewardId);

        return result != nullptr;
    }

    void GrantTier(Player* player, RewardTier const& tier)
    {
        MailDraft draft("Награда за эвент Авроры",
                        "Хранители Равновесия отмечают ваше участие.\n\n" + tier.nameRu);

        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

        if (tier.itemEntry > 0)
        {
            Item* item = Item::CreateItem(tier.itemEntry, tier.itemAmount);
            if (item)
            {
                item->SaveToDB(trans);
                draft.AddItem(item);
            }
        }

        draft.SendMailTo(trans, MailReceiver(player),
                         MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_DEFAULT));
        CharacterDatabase.CommitTransaction(trans);

        if (tier.auroraPoints > 0)
            player->AddItem(ITEM_AURORA_POINTS, tier.auroraPoints);

        if (tier.shards > 0)
            player->AddItem(ITEM_SHARD_OF_FALLEN_STAR, tier.shards);

        // Titles cannot ride on a mail, so they are applied immediately while
        // the player is guaranteed online (rewards are granted at logout or
        // while the player is still in the world).
        if (tier.titleDbcId > 0)
        {
            if (CharTitlesEntry const* title = sCharTitlesStore.LookupEntry(tier.titleDbcId))
                player->SetTitle(title);
            else
                LOG_WARN("module", "[Aurora] Event reward {} references missing title {}.",
                         tier.id, tier.titleDbcId);
        }

        if (tier.visualEnchant > 0)
        {
            CharacterDatabase.Execute(
                "INSERT INTO `aurora_player_enchants` (`player_guid`, `enchant_id`, `is_active`) "
                "VALUES ({}, {}, 0) ON DUPLICATE KEY UPDATE `enchant_id` = `enchant_id`",
                player->GetGUID().GetCounter(), tier.visualEnchant);
        }

        LOG_INFO("module", "[Aurora] Event reward '{}' granted to {}.",
                 tier.tier, player->GetName());
    }
}

// Called by the event manager right before an event stops.

void AuroraGrantEventRewards(uint32 activeEventId, uint32 eventId)
{
    std::vector<RewardTier> const& tiers = GetTiers(eventId);
    if (tiers.empty())
        return;

    for (auto const& pair : ObjectAccessor::GetPlayers())
    {
        Player* player = pair.second;
        if (!player || !player->IsInWorld())
            continue;

        uint32 const guid = player->GetGUID().GetCounter();

        Progress progress;
        if (!LoadProgress(activeEventId, guid, progress))
            continue;

        for (RewardTier const& tier : tiers)
        {
            if (ValueFor(tier, progress) < tier.requirementValue)
                continue;

            if (AlreadyClaimed(activeEventId, guid, tier.id))
                continue;

            GrantTier(player, tier);

            CharacterDatabase.Execute(
                "INSERT IGNORE INTO `aurora_event_claims` "
                "(`active_event_id`, `player_guid`, `reward_id`) VALUES ({}, {}, {})",
                activeEventId, guid, tier.id);
        }
    }

    // Players who logged off during the event still deserve credit, but their
    // items must be mailed rather than added to a live bag. They are handled
    // on next login by aurora_online_rewards.cpp reading pending rows.
    QueryResult offline = CharacterDatabase.Query(
        "SELECT p.`player_guid` FROM `aurora_event_progress` p "
        "LEFT JOIN `aurora_event_claims` c "
        "  ON c.`active_event_id` = p.`active_event_id` "
        " AND c.`player_guid` = p.`player_guid` "
        "WHERE p.`active_event_id` = {} AND c.`id` IS NULL", activeEventId);

    if (!offline)
        return;

    do
    {
        uint32 const guid = offline->Fetch()[0].Get<uint32>();

        Progress progress;
        if (!LoadProgress(activeEventId, guid, progress))
            continue;

        for (RewardTier const& tier : tiers)
        {
            if (ValueFor(tier, progress) < tier.requirementValue)
                continue;

            if (AlreadyClaimed(activeEventId, guid, tier.id))
                continue;

            if (tier.itemEntry > 0)
            {
                CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                MailDraft draft("Награда за эвент Авроры",
                                "Хранители Равновесия отмечают ваше участие.\n\n" + tier.nameRu);

                Item* item = Item::CreateItem(tier.itemEntry, tier.itemAmount);
                if (item)
                {
                    item->SaveToDB(trans);
                    draft.AddItem(item);
                }

                draft.SendMailTo(trans, MailReceiver(nullptr, guid),
                                 MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_DEFAULT));
                CharacterDatabase.CommitTransaction(trans);
            }

            if (tier.auroraPoints > 0)
                CharacterDatabase.Execute(
                    "UPDATE `aurora_player_activity` SET `aurora_points` = `aurora_points` + {} "
                    "WHERE `player_guid` = {}", tier.auroraPoints, guid);

            if (tier.titleDbcId > 0 || tier.visualEnchant > 0)
                CharacterDatabase.Execute(
                    "INSERT INTO `aurora_pending_grants` "
                    "(`player_guid`, `title_dbc_id`, `visual_enchant_id`) VALUES ({}, {}, {})",
                    guid, tier.titleDbcId, tier.visualEnchant);

            CharacterDatabase.Execute(
                "INSERT IGNORE INTO `aurora_event_claims` "
                "(`active_event_id`, `player_guid`, `reward_id`) VALUES ({}, {}, {})",
                activeEventId, guid, tier.id);
        }
    } while (offline->NextRow());

    g_rewardCache.erase(eventId);
}
