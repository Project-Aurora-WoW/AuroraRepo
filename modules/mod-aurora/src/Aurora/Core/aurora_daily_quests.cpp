// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_daily_quests.cpp - daily commissions.
//
// Flow
//   1. On login the player receives DAILY_QUESTS_PER_DAY commissions from the
//      pool in `aurora_daily_quest_pool`, deterministic per (guid, date) so a
//      reconnect does not reroll the set.
//   2. Progress is driven by the activity bridge counters: each commission
//      names an activity type and a target value.
//   3. `.aurora daily` prints the active set and the current progress.
//   4. A finished commission mails its reward on the next tick.
//
// Storage: `aurora_daily_quests` in the CHARACTER database, one row per
// (player, date, slot). Rewards are mailed, never added to a live bag, so a
// full inventory cannot swallow them.
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Mail.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "StringFormat.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    constexpr uint32 CHECK_INTERVAL_MS = 30000;   // scan players every 30 s
    constexpr uint8  DAILY_QUESTS_PER_DAY = 3;

    struct QuestDefinition
    {
        uint32      id        = 0;
        std::string activity;      // creature_kill / quest_complete / boss_kill / playtime_minute
        uint32      target    = 1;
        uint32      itemEntry = 0;
        uint32      itemAmount = 1;
        uint32      auroraPoints = 0;
        uint32      shards    = 0;
        std::string titleRu;
        std::string titleEn;
    };

    std::vector<QuestDefinition> g_pool;

    std::string TodayKey()
    {
        time_t now = time(nullptr);
        tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        char buffer[16];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);
        return buffer;
    }

    void LoadPool()
    {
        g_pool.clear();

        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `activity`, `target`, `item_entry`, `item_amount`, "
            "`aurora_points`, `shards`, `title_ru`, `title_en` "
            "FROM `aurora_daily_quest_pool` WHERE `enabled` = 1 ORDER BY `id`");

        if (!result)
        {
            LOG_WARN("module", "[Aurora] `aurora_daily_quest_pool` is empty - "
                               "daily commissions are disabled.");
            return;
        }

        do
        {
            Field* fields = result->Fetch();

            QuestDefinition def;
            def.id           = fields[0].Get<uint32>();
            def.activity     = fields[1].Get<std::string>();
            def.target       = fields[2].Get<uint32>();
            def.itemEntry    = fields[3].Get<uint32>();
            def.itemAmount   = fields[4].Get<uint32>();
            def.auroraPoints = fields[5].Get<uint32>();
            def.shards       = fields[6].Get<uint32>();
            def.titleRu      = fields[7].Get<std::string>();
            def.titleEn      = fields[8].Get<std::string>();

            g_pool.push_back(def);
        } while (result->NextRow());

        LOG_INFO("module", "[Aurora] Daily quest pool loaded: {} entries.", g_pool.size());
    }

    // Deterministic pick: same player, same day -> same three commissions.
    void EnsureDailySet(Player* player)
    {
        if (g_pool.empty())
            return;

        uint32 const guid = player->GetGUID().GetCounter();
        std::string const day = TodayKey();

        QueryResult existing = CharacterDatabase.Query(
            "SELECT `slot`, `quest_id` FROM `aurora_daily_quests` "
            "WHERE `player_guid` = {} AND `quest_date` = '{}'", guid, day);

        if (existing)
            return;   // the set for today already exists

        uint32 seed = guid * 2654435761u ^
                      static_cast<uint32>(std::hash<std::string>{ }(day) & 0xFFFFFFFFu);

        std::vector<uint32> picked;
        for (uint8 slot = 0; slot < DAILY_QUESTS_PER_DAY && picked.size() < g_pool.size(); ++slot)
        {
            seed = seed * 1664525u + 1013904223u;
            uint32 const index = seed % static_cast<uint32>(g_pool.size());

            // Linear probe keeps the three commissions distinct.
            uint32 probe = index;
            while (std::find(picked.begin(), picked.end(), g_pool[probe].id) != picked.end())
                probe = (probe + 1) % static_cast<uint32>(g_pool.size());

            picked.push_back(g_pool[probe].id);

            CharacterDatabase.Execute(
                "INSERT IGNORE INTO `aurora_daily_quests` "
                "(`player_guid`, `quest_date`, `slot`, `quest_id`, `progress`) "
                "VALUES ({}, '{}', {}, {}, 0)",
                guid, day, static_cast<uint32>(slot), g_pool[probe].id);
        }
    }

    uint32 ProgressFor(uint32 guid, std::string const& activity)
    {
        // The activity bridge keeps lifetime and per-period counters; the daily
        // figure is derived from the per-period row for today's week.
        QueryResult result = CharacterDatabase.Query(
            "SELECT `playtime_sec`, `creatures_killed`, `players_killed`, `quests_completed` "
            "FROM `aurora_player_activity_period` "
            "WHERE `player_guid` = {} ORDER BY `period_start` DESC LIMIT 1", guid);

        if (!result)
            return 0;

        Field* fields = result->Fetch();
        if (activity == "creature_kill")
            return fields[1].Get<uint32>();
        if (activity == "player_kill")
            return fields[2].Get<uint32>();
        if (activity == "quest_complete")
            return fields[3].Get<uint32>();
        if (activity == "playtime_minute")
            return static_cast<uint32>(fields[0].Get<uint64>() / 60);

        return 0;
    }

    void MailPendingRewards(Player* player)
    {
        uint32 const guid = player->GetGUID().GetCounter();
        std::string const day = TodayKey();

        QueryResult result = CharacterDatabase.Query(
            "SELECT q.`slot`, q.`quest_id` FROM `aurora_daily_quests` q "
            "WHERE q.`player_guid` = {} AND q.`quest_date` = '{}' "
            "AND q.`completed_at` IS NULL", guid, day);

        if (!result)
            return;

        std::vector<uint32> done;

        do
        {
            Field* fields = result->Fetch();
            uint32 const questId = fields[1].Get<uint32>();

            auto itr = std::find_if(g_pool.begin(), g_pool.end(),
                                    [questId](QuestDefinition const& def) { return def.id == questId; });
            if (itr == g_pool.end())
                continue;

            QuestDefinition const& def = *itr;

            QueryResult started = CharacterDatabase.Query(
                "SELECT `progress` FROM `aurora_daily_quests` "
                "WHERE `player_guid` = {} AND `quest_date` = '{}' AND `slot` = {}",
                guid, day, fields[0].Get<uint32>());

            uint32 const baseline = started ? started->Fetch()[0].Get<uint32>() : 0;
            uint32 const current  = ProgressFor(guid, def.activity);

            if (current < baseline + def.target)
                continue;

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            MailDraft draft(def.titleRu, "Ежедневное поручение выполнено. Хранители Равновесия "
                                        "благодарят вас за труд.");
            if (def.itemEntry > 0)
            {
                if (Item* item = Item::CreateItem(def.itemEntry, def.itemAmount))
                {
                    item->SaveToDB(trans);
                    draft.AddItem(item);
                }
            }
            if (def.auroraPoints > 0)
            {
                if (Item* points = Item::CreateItem(ITEM_AURORA_POINTS, def.auroraPoints))
                {
                    points->SaveToDB(trans);
                    draft.AddItem(points);
                }
            }
            if (def.shards > 0)
            {
                if (Item* shards = Item::CreateItem(ITEM_SHARD_OF_FALLEN_STAR, def.shards))
                {
                    shards->SaveToDB(trans);
                    draft.AddItem(shards);
                }
            }
            draft.SendMailTo(trans, MailReceiver(player),
                             MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_DEFAULT));
            CharacterDatabase.CommitTransaction(trans);

            CharacterDatabase.Execute(
                "UPDATE `aurora_daily_quests` SET `completed_at` = NOW() "
                "WHERE `player_guid` = {} AND `quest_date` = '{}' AND `slot` = {}",
                guid, day, fields[0].Get<uint32>());

            done.push_back(questId);
        } while (result->NextRow());

        for (uint32 questId : done)
            LOG_INFO("module", "[Aurora] Daily commission {} finished for {}.",
                     questId, player->GetName());
    }
}

class aurora_daily_quests : public WorldScript, public PlayerScript
{
public:
    aurora_daily_quests()
        : WorldScript("aurora_daily_quests_world"),
          PlayerScript("aurora_daily_quests_player") { }

    void OnStartup() override
    {
        LoadPool();
    }

    void OnPlayerLogin(Player* player) override
    {
        if (!player || !IsProjectEnabled() || g_pool.empty())
            return;

        EnsureDailySet(player);
        MailPendingRewards(player);
    }

    void OnUpdate(uint32 diff) override
    {
        if (!IsProjectEnabled() || g_pool.empty())
            return;

        _timer += diff;
        if (_timer < CHECK_INTERVAL_MS)
            return;

        _timer = 0;

        for (auto const& pair : ObjectAccessor::GetPlayers())
        {
            Player* player = pair.second;
            if (!player || !player->IsInWorld())
                continue;

            EnsureDailySet(player);
            MailPendingRewards(player);
        }
    }

private:
    uint32 _timer = 0;
};

void AddSC_aurora_daily_quests()
{
    new aurora_daily_quests();
}
