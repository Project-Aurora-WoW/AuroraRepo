// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_voting_engine.cpp - score recalculation and vote lifecycle.
//
// What runs, and when
//   hourly   CALL sp_AuroraRecalculateScores(3600)
//            CALL sp_AuroraRebuildCandidates(CURRENT_PERIOD, 50)
//   weekly   CALL sp_AuroraFinalizeVotes(previous period)
//            then mail the rewards returned by sp_AuroraGrantVoteRewards
//   daily    CALL sp_AuroraExpireVotes(7)
//
// Why a world script and not the DB scheduler
//   The procedures need the character database, which a world-side MySQL
//   EVENT cannot reach, and enabling the event scheduler requires SUPER.
//   Doing it from the world process also lets the reward mailing use the
//   real MailDraft path instead of raw SQL inserts into `mail`.
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Mail.h"
#include "DBCStores.h"
#include "StringFormat.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    constexpr uint32 SCORE_TICK_MS   = 3600000;  // 1 hour
    constexpr uint32 EXPIRE_TICK_MS  = 86400000; // 24 hours
    constexpr uint32 WEEK_MS         = 604800000;

    std::string PeriodForOffsetMonths(int months)
    {
        time_t now = time(nullptr);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        int month = localTime.tm_mon + 1 - months;
        int year  = localTime.tm_year + 1900;

        while (month <= 0)
        {
            month += 12;
            --year;
        }

        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%04d-%02d", year, month);
        return buffer;
    }

    void MailVoteReward(uint32 guid, uint32 rank, char const* category,
                        uint32 titleDbcId, uint32 itemEntry, uint32 itemAmount,
                        uint32 auroraPoints, uint32 visualEnchantId)
    {
        char subject[128];
        std::snprintf(subject, sizeof(subject),
                      "Аврора: награда за голосование (%s, место %u)", category, rank);

        std::string body =
            "Хранители Равновесия благодарят вас за участие в голосовании.\n\n"
            "Категория: " + std::string(category) + "\n"
            "Место: " + std::to_string(rank) + "\n";

        MailDraft draft(subject, body);

        if (itemEntry > 0)
            draft.AddItem(itemEntry, itemAmount);

        // Titles and currency cannot be attached to a mail item, so they are
        // stored in the mail text and applied by the player script when the
        // mail is opened. The DB columns below carry the pending grant.
        if (titleDbcId > 0)
            CharacterDatabase.Execute(
                "INSERT IGNORE INTO `aurora_pending_grants` "
                "(`player_guid`, `title_dbc_id`, `visual_enchant_id`, `created_at`) "
                "VALUES ({}, {}, {}, NOW())", guid, titleDbcId, visualEnchantId);

        if (auroraPoints > 0)
        {
            CharacterDatabase.Execute(
                "UPDATE `aurora_player_activity` SET `aurora_points` = `aurora_points` + {} "
                "WHERE `player_guid` = {}", auroraPoints, guid);
        }

        draft.SendMailToCustomReceiver(guid, "Aurora");
    }
}

class aurora_voting_engine : public WorldScript
{
public:
    aurora_voting_engine() : WorldScript("aurora_voting_engine") { }

    void OnStartup() override
    {
        LOG_INFO("module", "[Aurora] Voting engine armed: hourly scores, weekly settlement.");
    }

    void OnUpdate(uint32 diff) override
    {
        _scoreTimer += diff;
        if (_scoreTimer >= SCORE_TICK_MS)
        {
            _scoreTimer = 0;
            RecalculateScores();
        }

        _expireTimer += diff;
        if (_expireTimer >= EXPIRE_TICK_MS)
        {
            _expireTimer = 0;
            ExpireVotes();
            TrySettleWeek();
        }
    }

private:
    void RecalculateScores()
    {
        std::string const period = PeriodForOffsetMonths(0);

        // All five procedures live on the CHARACTER database: that is where
        // aurora_player_activity, aurora_vote_candidates, aurora_votes and
        // aurora_vote_winners are created (see sql/01_aurora_schema.sql,
        // section 2 and sql/04_character_procedures.sql).
        RefreshWeightMirror();

        CharacterDatabase.Execute("CALL sp_AuroraRecalculateScores(3600)");
        CharacterDatabase.Execute("CALL sp_AuroraRebuildCandidates('{}', 50)", period);

        LOG_INFO("module", "[Aurora] Scores recalculated for period {}.", period);
    }

    // The scoring weights are world data, but the character-side procedures
    // cannot read across schemas. Mirror them into `aurora_v_weights` so a
    // designer keeps editing exactly one place: world.aurora_activity_weights.
    void RefreshWeightMirror()
    {
        QueryResult result = WorldDatabase.Query(
            "SELECT `activity`, `base_points` FROM `aurora_activity_weights`");

        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();
            std::string activity = fields[0].Get<std::string>();
            uint32 points = fields[1].Get<uint32>();

            CharacterDatabase.Execute(
                "INSERT INTO `aurora_v_weights` (`activity`, `base_points`) VALUES ('{}', {}) "
                "ON DUPLICATE KEY UPDATE `base_points` = {}", activity, points, points);
        } while (result->NextRow());
    }

    void ExpireVotes()
    {
        CharacterDatabase.Execute("CALL sp_AuroraExpireVotes(7)");
    }

    // Settlement runs once per calendar week. The state is stored in the DB
    // rather than in memory so a restart mid-week does not settle twice.
    void TrySettleWeek()
    {
        QueryResult lastRun = CharacterDatabase.Query(
            "SELECT `settled_period` FROM `aurora_vote_settlement` ORDER BY `settled_at` DESC LIMIT 1");

        std::string const previous = PeriodForOffsetMonths(1);

        if (lastRun)
        {
            std::string settled = lastRun->Fetch()[0].Get<std::string>();
            if (settled == previous)
                return;
        }

        CharacterDatabase.Execute("CALL sp_AuroraFinalizeVotes('{}')", previous);

        QueryResult rewards = CharacterDatabase.Query("CALL sp_AuroraGrantVoteRewards('{}')",
                                                      previous);
        uint32 granted = 0;

        if (rewards)
        {
            do
            {
                Field* fields = rewards->Fetch();
                std::string category = fields[0].Get<std::string>();
                uint32 rank          = fields[1].Get<uint32>();
                uint32 guid          = fields[2].Get<uint32>();
                uint32 titleId       = fields[3].Get<uint32>();
                uint32 itemEntry     = fields[4].Get<uint32>();
                uint32 itemAmount    = fields[5].Get<uint32>();
                uint32 points        = fields[6].Get<uint32>();
                uint32 visualEnchant = fields[7].Get<uint32>();

                MailVoteReward(guid, rank, category.c_str(), titleId, itemEntry,
                               itemAmount, points, visualEnchant);
                ++granted;
            } while (rewards->NextRow());
        }

        CharacterDatabase.Execute(
            "UPDATE `aurora_vote_winners` SET `reward_claimed` = 1 WHERE `period` = '{}'",
            previous);

        CharacterDatabase.Execute(
            "INSERT INTO `aurora_vote_settlement` (`settled_period`, `settled_at`) "
            "VALUES ('{}', NOW())", previous);

        LOG_INFO("module", "[Aurora] Vote settlement for {} complete: {} rewards mailed.",
                 previous, granted);
    }

    uint32 _scoreTimer = 0;
    uint32 _expireTimer = 0;
};

void AddSC_aurora_voting_engine()
{
    new aurora_voting_engine();
}
