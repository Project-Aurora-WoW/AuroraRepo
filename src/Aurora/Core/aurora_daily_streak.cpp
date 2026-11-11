// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_daily_streak.cpp - consecutive-login streak.
//
// Rules
//   * A login on a new calendar day extends the streak by one day.
//   * A login on the same day changes nothing; the day is already counted.
//   * A gap of two or more full days resets the streak to 1 on the next login.
//   * The reward for day N is mailed automatically when the player logs in
//     that day, so nothing is lost if the player logs out instantly.
//   * The cycle wraps at 7: day 8 is treated as day 1 and the chain is reset.
//
// Why server-side and not a client timer
//   The client cannot be trusted for a reward that has currency value, and the
//   addon may not be installed at all. The whole streak lives in the DB and
//   is advanced exactly once per login.
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Mail.h"
#include "Item.h"
#include "StringFormat.h"
#include "DBCStores.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    constexpr uint8 STREAK_CYCLE = 7;

    struct StreakReward
    {
        uint8  day          = 0;
        uint32 itemEntry    = 0;
        uint32 itemAmount   = 1;
        uint32 auroraPoints = 0;
        uint32 shards       = 0;
    };

    std::array<StreakReward, STREAK_CYCLE> g_rewards;
    bool g_rewardsLoaded = false;

    std::tm LocalToday()
    {
        time_t now = time(nullptr);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        return localTime;
    }

    std::string DateString(std::tm const& value)
    {
        char buffer[16];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &value);
        return buffer;
    }

    // Difference in whole calendar days between two YYYY-MM-DD strings.
    // Using mktime on midnight-normalised tm avoids DST edge cases.
    int32 DaysBetween(std::string const& from, std::string const& to)
    {
        if (from.empty())
            return 9999;

        std::tm a{}, b{};
        std::sscanf(from.c_str(), "%d-%d-%d", &a.tm_year, &a.tm_mon, &a.tm_mday);
        std::sscanf(to.c_str(), "%d-%d-%d", &b.tm_year, &b.tm_mon, &b.tm_mday);

        a.tm_year -= 1900; a.tm_mon -= 1; a.tm_hour = 12; a.tm_isdst = -1;
        b.tm_year -= 1900; b.tm_mon -= 1; b.tm_hour = 12; b.tm_isdst = -1;

        time_t const ta = mktime(&a);
        time_t const tb = mktime(&b);

        if (ta == static_cast<time_t>(-1) || tb == static_cast<time_t>(-1))
            return 9999;

        double const difference = std::difftime(tb, ta);
        return static_cast<int32>(std::floor(difference / 86400.0 + 0.5));
    }

    void LoadRewards()
    {
        if (g_rewardsLoaded)
            return;

        g_rewardsLoaded = true;

        QueryResult result = WorldDatabase.Query(
            "SELECT `day`, `item_entry`, `item_amount`, `aurora_points`, `shards` "
            "FROM `aurora_streak_rewards` WHERE `day` BETWEEN 1 AND {} ORDER BY `day`",
            static_cast<uint32>(STREAK_CYCLE));

        if (!result)
        {
            LOG_ERROR("module", "[Aurora] `aurora_streak_rewards` is empty - streak rewards "
                                "will grant nothing.");
            return;
        }

        do
        {
            Field* fields = result->Fetch();
            uint8 day = static_cast<uint8>(fields[0].Get<uint32>());
            if (day < 1 || day > STREAK_CYCLE)
                continue;

            StreakReward& reward = g_rewards[day - 1];
            reward.day          = day;
            reward.itemEntry    = fields[1].Get<uint32>();
            reward.itemAmount   = fields[2].Get<uint32>();
            reward.auroraPoints = fields[3].Get<uint32>();
            reward.shards       = fields[4].Get<uint32>();
        } while (result->NextRow());
    }

    void GrantStreakReward(Player* player, StreakReward const& reward)
    {
        if (reward.itemEntry > 0)
        {
            CharacterDatabaseTransaction transMail = CharacterDatabase.BeginTransaction();
            MailDraft draft("Награда за серию входов", "Ваша серия входов продолжается. Так держать!");
            if (reward.itemEntry > 0)
            {
                Item* item = Item::CreateItem(reward.itemEntry, reward.itemAmount);
                if (item)
                {
                    item->SaveToDB(transMail);
                    draft.AddItem(item);
                }
            }
            draft.SendMailTo(transMail, MailReceiver(player), MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_DEFAULT));
            CharacterDatabase.CommitTransaction(transMail);
        }

        if (reward.auroraPoints > 0)
            player->AddItem(ITEM_AURORA_POINTS, reward.auroraPoints);

        if (reward.shards > 0)
            player->AddItem(ITEM_SHARD_OF_FALLEN_STAR, reward.shards);
    }
}

class aurora_daily_streak : public PlayerScript
{
public:
    aurora_daily_streak() : PlayerScript("aurora_daily_streak") { }

    void OnPlayerLogin(Player* player) override
    {
        if (!player)
            return;

        LoadRewards();

        uint32 const guid = player->GetGUID().GetCounter();
        std::tm const today = LocalToday();
        std::string const todayString = DateString(today);

        QueryResult result = CharacterDatabase.Query(
            "SELECT `current_streak`, `max_streak`, `last_login_date` "
            "FROM `aurora_daily_streak` WHERE `player_guid` = {}", guid);

        if (!result)
        {
            // First ever login: create the row and start at day 1.
            CharacterDatabase.Execute(
                "INSERT INTO `aurora_daily_streak` "
                "(`player_guid`, `current_streak`, `max_streak`, `last_login_date`) "
                "VALUES ({}, 1, 1, '{}')", guid, todayString);

            AdvanceAndReward(player, 1);
            return;
        }

        Field* fields = result->Fetch();
        uint8 currentStreak  = static_cast<uint8>(fields[0].Get<uint32>());
        uint8 maxStreak      = static_cast<uint8>(fields[1].Get<uint32>());
        std::string lastLogin = fields[2].IsNull() ? "" : fields[2].Get<std::string>();

        int32 const gap = DaysBetween(lastLogin, todayString);

        if (gap == 0)
        {
            // Same day: streak already counted, do not pay again.
            return;
        }

        uint8 newStreak = 1;
        if (gap == 1)
        {
            newStreak = static_cast<uint8>((currentStreak % STREAK_CYCLE) + 1);
        }
        // gap >= 2 -> chain broken, restart at 1 (already set above).

        if (newStreak > maxStreak)
            maxStreak = newStreak;

        CharacterDatabase.Execute(
            "UPDATE `aurora_daily_streak` SET `current_streak` = {}, `max_streak` = {}, "
            "`last_login_date` = '{}', `last_claim_date` = '{}' WHERE `player_guid` = {}",
            newStreak, maxStreak, todayString, todayString, guid);

        AdvanceAndReward(player, newStreak);
    }

private:
    void AdvanceAndReward(Player* player, uint8 streakDay)
    {
        if (streakDay < 1 || streakDay > STREAK_CYCLE)
            return;

        StreakReward const& reward = g_rewards[streakDay - 1];
        if (reward.day == 0)
            return;   // no row configured for this day

        GrantStreakReward(player, reward);

        CharacterDatabase.Execute(
            "UPDATE `aurora_daily_streak` SET `last_claim_date` = CURDATE() "
            "WHERE `player_guid` = {}", player->GetGUID().GetCounter());
    }
};

void AddSC_aurora_daily_streak()
{
    new aurora_daily_streak();
}
