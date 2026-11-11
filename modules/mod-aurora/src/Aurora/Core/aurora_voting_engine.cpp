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
#include "Item.h"
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

        // Items are mailed through a real transaction; the receiver is
        // resolved by low GUID so an offline winner still gets the mail.
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        MailDraft draft(subject, body);

        if (itemEntry > 0)
        {
            Item* item = Item::CreateItem(itemEntry, itemAmount);
            if (item)
            {
                item->SaveToDB(trans);
                draft.AddItem(item);
            }
        }
        // Titles cannot ride on a mail item: store the pending grant and
        // apply it on next login (PlayerScript reads the table).
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

        draft.SendMailTo(trans, MailReceiver(nullptr, guid), MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_DEFAULT));
        CharacterDatabase.CommitTransaction(trans);
    }
}

class aurora_voting_engine : public WorldScript
{
public:
    aurora_voting_engine() : WorldScript("aurora_voting_engine") { }

    void OnUpdate(uint32 diff) override
    {
        if (!IsVotingEnabled())
            return;

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
        }
    }

private:
    void RecalculateScores()
    {
        std::string const period = PeriodForOffsetMonths(0);
        CharacterDatabase.Execute("CALL sp_AuroraRecalculateScores(3600)");
        CharacterDatabase.Execute("CALL sp_AuroraRebuildCandidates('{}', 50)", period);
        LOG_INFO("module", "[Aurora] Scores recalculated for period {}.", period);
    }

    void ExpireVotes()
    {
        CharacterDatabase.Execute("CALL sp_AuroraExpireVotes(7)");
    }

    // ------------------------------------------------------------------------
    // Weekly settlement.
    //
    // FinalizeVotes() runs the stored procedure that turns the closed period's
    // `aurora_vote_winners` into rows, then this code pays every winner out of
    // `aurora_vote_rewards`. The matrix lives in the world DB so a designer can
    // retune a prize without touching C++.
    // ------------------------------------------------------------------------
    void WeekFinalize()
    {
        std::string const closed = PeriodForOffsetMonths(1);

        CharacterDatabase.Execute("CALL sp_AuroraFinalizeVotes('{}')", closed);

        QueryResult winners = CharacterDatabase.Query(
            "SELECT `category`, `rank`, `player_guid` FROM `aurora_vote_winners` "
            "WHERE `period` = '{}' AND `reward_claimed` = 0 ORDER BY `category`, `rank`",
            closed);

        if (!winners)
        {
            LOG_INFO("module", "[Aurora] No unclaimed winners for period {}.", closed);
            return;
        }

        uint32 paid = 0;

        do
        {
            Field* fields        = winners->Fetch();
            std::string category = fields[0].Get<std::string>();
            uint32 rank          = fields[1].Get<uint32>();
            uint32 guid          = fields[2].Get<uint32>();

            QueryResult reward = WorldDatabase.Query(
                "SELECT `title_dbc_id`, `item_entry`, `item_amount`, "
                "`aurora_points`, `visual_enchant_id` "
                "FROM `aurora_vote_rewards` WHERE `category` = '{}' AND `rank` = {}",
                category, rank);

            if (reward)
            {
                Field* r = reward->Fetch();

                MailVoteReward(guid, rank, category.c_str(),
                               r[0].Get<uint32>(), r[1].Get<uint32>(),
                               r[2].Get<uint32>(), r[3].Get<uint32>(),
                               r[4].Get<uint32>());

                ++paid;
            }
            else
            {
                LOG_WARN("module", "[Aurora] No reward row for {} rank {} "
                                   "- winner {} paid nothing.", category, rank, guid);
            }

            CharacterDatabase.Execute(
                "UPDATE `aurora_vote_winners` SET `reward_claimed` = 1 "
                "WHERE `period` = '{}' AND `category` = '{}' AND `rank` = {}",
                closed, category, rank);
        } while (winners->NextRow());

        LOG_INFO("module", "[Aurora] Vote settlement for {} paid {} winner(s).", closed, paid);
    }

    uint32 _scoreTimer  = 0;
    uint32 _expireTimer = 0;
    uint32 _weekTimer   = 0;
};

void AddSC_aurora_voting_engine()
{
    new aurora_voting_engine();
}
