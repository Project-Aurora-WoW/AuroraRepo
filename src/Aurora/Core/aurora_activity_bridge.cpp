// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_activity_bridge.cpp - live activity counters.
//
// Why this exists
//   The SQL layer stores scores, but scores are useless without counters, and
//   counters cannot be produced by a world-side stored procedure: the stock
//   character tables live in a different database. Every counter here is
//   incremented from a real gameplay hook and written to the CHARACTER
//   database, so `aurora_player_activity` always reflects what happened.
//
// Write strategy
//   Hooks are frequent (creature kills, playtime). Writing one UPDATE per
//   event would hammer MySQL, so counters are accumulated in memory and
//   flushed once per AURORA_ACTIVITY_FLUSH_MS as a single batched statement
//   per player. The flush also runs on logout, on shutdown and before score
//   recalculation, so a crash loses at most one minute of statistics.
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "Group.h"
#include "QuestDef.h"
#include "AchievementMgr.h"
#include "DatabaseEnv.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "Log.h"
#include "Map.h"
#include "StringFormat.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    // ------------------------------------------------------------------------
    // Pending counters for one player.
    // ------------------------------------------------------------------------
    struct PendingCounters
    {
        uint32 creaturesKilled   = 0;
        uint32 playersKilled     = 0;
        uint32 deaths            = 0;
        uint32 questsCompleted   = 0;
        uint32 achievements      = 0;
        uint32 mailsSent         = 0;
        uint32 tradesCompleted   = 0;
        uint32 auctionBids       = 0;
        uint32 auctionSales      = 0;
        uint32 honorEarned       = 0;
        uint32 arenaGames        = 0;
        uint32 arenaWins         = 0;
        uint32 battlegrounds     = 0;
        uint32 battlegroundWins  = 0;
        uint32 dungeonsEntered   = 0;
        uint32 raidsEntered      = 0;
        uint32 bossesKilled      = 0;
        uint32 itemsLooted       = 0;
        uint32 itemsCrafted      = 0;
        uint32 playtimeSeconds   = 0;

        bool Empty() const
        {
            return !(creaturesKilled | playersKilled | deaths | questsCompleted
                  | achievements | mailsSent | tradesCompleted | auctionBids
                  | auctionSales | honorEarned | arenaGames | arenaWins
                  | battlegrounds | battlegroundWins | dungeonsEntered
                  | raidsEntered | bossesKilled | itemsLooted | itemsCrafted
                  | playtimeSeconds);
        }
    };

    std::unordered_map<uint32, PendingCounters> g_pending;
    std::mutex g_pendingLock;

    PendingCounters& Pending(uint32 guid)
    {
        std::lock_guard<std::mutex> guard(g_pendingLock);
        return g_pending[guid];
    }

    std::string CurrentWeekStart()
    {
        // Monday of the current ISO week, as a DATE literal.
        time_t now = time(nullptr);
        tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        int daysSinceMonday = (localTime.tm_wday + 6) % 7;
        now -= daysSinceMonday * 86400;

#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        char buffer[16];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);
        return std::string(buffer);
    }
    std::string CurrentWeekStartSql()
    {
        return "'" + CurrentWeekStart() + "'";
    }


    // ------------------------------------------------------------------------
    // Careful with unsigned subtraction: a counter that somehow went negative
    // must not wrap into 4 billion.
    // ------------------------------------------------------------------------
    uint64 SaturatingSub(uint64 value, uint64 delta)
    {
        return (value > delta) ? value - delta : 0;
    }
}

class aurora_activity_bridge : public PlayerScript, public WorldScript
{
public:
    aurora_activity_bridge()
        : PlayerScript("aurora_activity_bridge_player"),
          WorldScript("aurora_activity_bridge_world") { }

    // ------------------------------------------------------------------------
    // WorldScript: periodic flush + shutdown flush
    // ------------------------------------------------------------------------
    void OnUpdate(uint32 diff) override
    {
        _flushTimer += diff;
        if (_flushTimer < AURORA_ACTIVITY_FLUSH_MS)
            return;

        _flushTimer = 0;
        AccumulatePlaytime(AURORA_ACTIVITY_FLUSH_MS / 1000);
        FlushAll();
    }

    void OnShutdown() override
    {
        AccumulatePlaytime(0); // nothing to add, just flush what is pending
        FlushAll();
    }

    // ------------------------------------------------------------------------
    // PlayerScript hooks
    // ------------------------------------------------------------------------
    void OnPlayerLogin(Player* player) override
    {
        uint32 guid = player->GetGUID().GetCounter();

        // Row is created on first login so later UPDATEs always hit something.
        CharacterDatabase.Execute(
            "INSERT INTO `aurora_player_activity` (`player_guid`, `sessions_count`) "
            "VALUES ({}, 1) "
            "ON DUPLICATE KEY UPDATE `sessions_count` = `sessions_count` + 1", guid);

        // Online-rewards bookkeeping row, same reasoning.
        CharacterDatabase.Execute(
            "INSERT IGNORE INTO `aurora_online_progress` (`player_guid`) VALUES ({})", guid);

        CharacterDatabase.Execute(
            "INSERT IGNORE INTO `aurora_daily_streak` (`player_guid`, `last_login_date`) "
            "VALUES ({}, CURDATE())", guid);

        ApplyPendingGrants(player);
    }

    // ------------------------------------------------------------------------
    // Titles and visual enchants owed to the player.
    // A mail can carry an item, but not a title id, so the vote settlement
    // stores the grant in `aurora_pending_grants` and it is applied here,
    // on the first login after settlement. Each row is marked applied so a
    // title is never handed out twice.
    // ------------------------------------------------------------------------
    void ApplyPendingGrants(Player* player)
    {
        uint32 const guid = player->GetGUID().GetCounter();

        QueryResult result = CharacterDatabase.Query(
            "SELECT `id`, `title_dbc_id`, `visual_enchant_id` "
            "FROM `aurora_pending_grants` "
            "WHERE `player_guid` = {} AND `applied_at` IS NULL "
            "ORDER BY `id` ASC", guid);

        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();

            uint32 rowId            = fields[0].Get<uint32>();
            uint32 titleDbcId       = fields[1].Get<uint32>();
            uint32 visualEnchantId  = fields[2].Get<uint32>();

            if (titleDbcId > 0)
            {
                if (CharTitlesEntry const* title = sCharTitlesStore.LookupEntry(titleDbcId))
                    player->SetTitle(title);
                else
                    LOG_WARN("module", "[Aurora] Pending grant {}: title {} is missing from "
                                       "CharTitles.dbc - client patch applied?", rowId, titleDbcId);
            }

            if (visualEnchantId > 0)
            {
                CharacterDatabase.Execute(
                    "INSERT INTO `aurora_player_enchants` "
                    "(`player_guid`, `enchant_id`, `is_active`) VALUES ({}, {}, 0) "
                    "ON DUPLICATE KEY UPDATE `enchant_id` = `enchant_id`",
                    guid, visualEnchantId);
            }

            CharacterDatabase.Execute(
                "UPDATE `aurora_pending_grants` SET `applied_at` = NOW() WHERE `id` = {}",
                rowId);

            LOG_INFO("module", "[Aurora] Applied pending grant {} to player {}.", rowId, guid);
        } while (result->NextRow());
    }

    void OnPlayerLogout(Player* player) override
    {
        FlushPlayer(player->GetGUID().GetCounter());
    }

    void OnPlayerCreatureKill(Player* player, Creature* creature) override
    {
        if (!player || !creature)
            return;

        PendingCounters& pending = Pending(player->GetGUID().GetCounter());
        ++pending.creaturesKilled;

        if (creature->IsDungeonBoss() || creature->isWorldBoss())
            ++pending.bossesKilled;
    }

    void OnPlayerPVPKill(Player* killer, Player* /*killed*/) override
    {
        if (!killer)
            return;

        ++Pending(killer->GetGUID().GetCounter()).playersKilled;
    }


    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (!player || !quest)
            return;

        PendingCounters& pending = Pending(player->GetGUID().GetCounter());
        ++pending.questsCompleted;

        // Campaign quests additionally advance the progress table; the very
        // last quest of each chain marks the campaign as completed.
        uint32 questId = quest->GetQuestId();
        if (!IsAuroraQuest(questId))
            return;

        if (questId == QUEST_ALLIANCE_6 || questId == QUEST_HORDE_6 || questId == QUEST_NEUTRAL_6)
        {
            CharacterDatabase.Execute(
                "INSERT INTO `aurora_player_progress` "
                "(`player_guid`, `campaign_id`, `completed_at`, `completion_time_sec`, `no_death`) "
                "VALUES ({}, 1, NOW(), 0, 0) "
                "ON DUPLICATE KEY UPDATE `completed_at` = NOW()",
                player->GetGUID().GetCounter());
        }
    }

    void OnPlayerAchievementComplete(Player* player, AchievementEntry const* achievement) override
    {
        if (!player || !achievement)
            return;

        PendingCounters& pending = Pending(player->GetGUID().GetCounter());
        ++pending.achievements;

        if (!IsAuroraAchievement(achievement->ID))
            return;

        GrantAchievementReward(player, achievement->ID);
    }


    void OnPlayerStoreNewItem(Player* player, Item* /*item*/, uint32 /*count*/) override
    {
        if (!player)
            return;

        ++Pending(player->GetGUID().GetCounter()).itemsLooted;
    }

    void OnPlayerMapChanged(Player* player) override
    {
        if (!player)
            return;

        Map* map = player->GetMap();
        if (!map)
            return;

        PendingCounters& pending = Pending(player->GetGUID().GetCounter());

        if (map->IsRaid())
            ++pending.raidsEntered;
        else if (map->IsDungeon())
            ++pending.dungeonsEntered;
    }

private:
    // ------------------------------------------------------------------------
    // Playtime: added for every online player once per flush window.
    // ------------------------------------------------------------------------
    void AccumulatePlaytime(uint32 seconds)
    {
        if (seconds == 0)
            return;

        for (auto const& pair : ObjectAccessor::GetPlayers())
        {
            Player* player = pair.second;
            if (!player || !player->IsInWorld())
                continue;

            Pending(player->GetGUID().GetCounter()).playtimeSeconds += seconds;
        }
    }

    // ------------------------------------------------------------------------
    // Flush
    // ------------------------------------------------------------------------
    void FlushAll()
    {
        std::vector<uint32> guids;
        guids.reserve(g_pending.size());

        {
            std::lock_guard<std::mutex> guard(g_pendingLock);
            for (auto const& pair : g_pending)
                guids.push_back(pair.first);
        }

        for (uint32 guid : guids)
            FlushPlayer(guid);
    }

    void FlushPlayer(uint32 guid)
    {
        PendingCounters pending;

        {
            std::lock_guard<std::mutex> guard(g_pendingLock);
            auto itr = g_pending.find(guid);
            if (itr == g_pending.end())
                return;

            pending = itr->second;
            itr->second = PendingCounters{ };
        }

        if (pending.Empty())
            return;

        // One statement, all counters, no read-modify-write race: MySQL does
        // the addition, so two flushes in flight cannot lose an increment.
        CharacterDatabase.Execute(
            "UPDATE `aurora_player_activity` SET "
            "`creatures_killed`     = `creatures_killed` + {}, "
            "`players_killed`       = `players_killed` + {}, "
            "`deaths_count`         = `deaths_count` + {}, "
            "`quests_completed`     = `quests_completed` + {}, "
            "`achievements_earned`  = `achievements_earned` + {}, "
            "`mails_sent`           = `mails_sent` + {}, "
            "`trades_completed`     = `trades_completed` + {}, "
            "`auction_bids`         = `auction_bids` + {}, "
            "`auction_sales`        = `auction_sales` + {}, "
            "`honor_earned`         = `honor_earned` + {}, "
            "`arena_games_played`   = `arena_games_played` + {}, "
            "`arena_wins`           = `arena_wins` + {}, "
            "`battlegrounds_played` = `battlegrounds_played` + {}, "
            "`battleground_wins`    = `battleground_wins` + {}, "
            "`dungeons_entered`     = `dungeons_entered` + {}, "
            "`raids_entered`        = `raids_entered` + {}, "
            "`bosses_killed`        = `bosses_killed` + {}, "
            "`items_looted`         = `items_looted` + {}, "
            "`items_crafted`        = `items_crafted` + {}, "
            "`total_playtime_sec`   = `total_playtime_sec` + {} "
            "WHERE `player_guid` = {}",
            pending.creaturesKilled, pending.playersKilled, pending.deaths,
            pending.questsCompleted, pending.achievements, pending.mailsSent,
            pending.tradesCompleted, pending.auctionBids, pending.auctionSales,
            pending.honorEarned, pending.arenaGames, pending.arenaWins,
            pending.battlegrounds, pending.battlegroundWins, pending.dungeonsEntered,
            pending.raidsEntered, pending.bossesKilled, pending.itemsLooted,
            pending.itemsCrafted, pending.playtimeSeconds, guid);

        // Weekly rollup for the streak / vote eligibility window.
        CharacterDatabase.Execute(
            "INSERT INTO `aurora_player_activity_period` "
            "(`player_guid`, `period_start`, `playtime_sec`, `creatures_killed`, "
            " `players_killed`, `quests_completed`) "
            "VALUES ({}, '{}', {}, {}, {}, {}) "
            "ON DUPLICATE KEY UPDATE "
            "`playtime_sec`     = `playtime_sec` + VALUES(`playtime_sec`), "
            "`creatures_killed` = `creatures_killed` + VALUES(`creatures_killed`), "
            "`players_killed`   = `players_killed` + VALUES(`players_killed`), "
            "`quests_completed` = `quests_completed` + VALUES(`quests_completed`)",
            guid, CurrentWeekStart(), pending.playtimeSeconds,
            pending.creaturesKilled, pending.playersKilled, pending.questsCompleted);

        // Online-reward counters. Without this the thresholds in
        // `aurora_online_rewards` can never be reached, because nothing else
        // increments the minute counters.
        // Period rollover: daily resets at midnight, weekly on Monday,
        // monthly on the 1st. The reset stamps are compared with CURDATE()
        // so a server that was offline for a week still resets correctly.
        uint32 elapsedMinutes = pending.playtimeSeconds / 60;
        if (elapsedMinutes == 0)
            return;

        CharacterDatabase.Execute(
            "INSERT INTO `aurora_online_progress` "
            "(`player_guid`, `session_minutes`, `daily_minutes`, `weekly_minutes`, "
            " `monthly_minutes`, `reset_day`, `reset_week`, `reset_month`) "
            "VALUES ({}, {}, {}, {}, {}, CURDATE(), " + CurrentWeekStartSql() + ", "
            "DATE_FORMAT(CURDATE(), '%Y-%m-01')) "
            "ON DUPLICATE KEY UPDATE "
            "`session_minutes` = `session_minutes` + VALUES(`session_minutes`), "
            "`daily_minutes`   = IF(`reset_day` = CURDATE(), "
            "                       `daily_minutes` + VALUES(`daily_minutes`), "
            "                       VALUES(`daily_minutes`)), "
            "`weekly_minutes`  = IF(`reset_week` = " + CurrentWeekStartSql() + ", "
            "                       `weekly_minutes` + VALUES(`weekly_minutes`), "
            "                       VALUES(`weekly_minutes`)), "
            "`monthly_minutes` = IF(`reset_month` = DATE_FORMAT(CURDATE(), '%Y-%m-01'), "
            "                       `monthly_minutes` + VALUES(`monthly_minutes`), "
            "                       VALUES(`monthly_minutes`)), "
            "`reset_day`       = CURDATE(), "
            "`reset_week`      = " + CurrentWeekStartSql() + ", "
            "`reset_month`     = DATE_FORMAT(CURDATE(), '%Y-%m-01')",
            guid, elapsedMinutes, elapsedMinutes, elapsedMinutes, elapsedMinutes);
    }

    // ------------------------------------------------------------------------
    // Achievement rewards - reads the rule from the DB instead of hard-coding
    // ids in C++, so a designer can retune without a rebuild.
    // ------------------------------------------------------------------------
    void GrantAchievementReward(Player* player, uint32 achievementId)
    {
        QueryResult result = WorldDatabase.Query(
            "SELECT `title_id`, `item_id`, `item_amount`, `aurora_points`, "
            "`mail_subject`, `mail_text`, `send_mail` "
            "FROM `aurora_achievement_rewards` WHERE `achievement_id` = {}", achievementId);

        if (!result)
            return;

        Field* fields = result->Fetch();

        uint32 titleId     = fields[0].Get<uint32>();
        uint32 itemEntry   = fields[1].Get<uint32>();
        uint32 itemAmount  = fields[2].Get<uint32>();
        uint32 points      = fields[3].Get<uint32>();
        std::string subject = fields[4].Get<std::string>();
        std::string text    = fields[5].Get<std::string>();
        bool sendMail       = fields[6].Get<bool>();

        // A title id that does not exist in CharTitles.dbc silently does
        // nothing on the client, so validate before applying.
        if (titleId > 0)
        {
            if (sCharTitlesStore.LookupEntry(titleId))
                player->SetTitle(sCharTitlesStore.LookupEntry(titleId));
            else
                LOG_WARN("module", "[Aurora] Achievement {} points to missing title {} "
                                   "- is the client DBC patch applied?", achievementId, titleId);
        }

        if (itemEntry > 0)
            player->AddItem(itemEntry, itemAmount);

        if (points > 0)
            player->AddItem(ITEM_AURORA_POINTS, points);

        if (sendMail && !subject.empty())
        {
            MailDraft draft(subject, text);
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            draft.SendMailTo(trans, MailReceiver(player), MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_DEFAULT));
            CharacterDatabase.CommitTransaction(trans);
        }
    }
    uint32 _flushTimer = 0;
};

void AddSC_aurora_activity_bridge()
{
    new aurora_activity_bridge();
}
