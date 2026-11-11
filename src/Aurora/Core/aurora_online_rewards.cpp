// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_online_rewards.cpp - playtime rewards.
//
// Flow
//   1. Online time is accumulated per player by the activity bridge
//      (`aurora_online_progress.daily_minutes`).
//   2. This module periodically compares the accumulated time against the
//      thresholds in `aurora_online_rewards` and mails whatever is owed.
//   3. A claim row is written to `aurora_online_claims` so a reward is never
//      paid twice inside the same period (daily / weekly / monthly / once).
//
// Reset rules
//   daily   -> counters reset at the first tick after midnight
//   weekly  -> counters reset on Monday
//   monthly -> counters reset on the 1st
// The counters themselves are incremented by the activity bridge; this file
// only rolls them over and spends them.
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Mail.h"
#include "ObjectAccessor.h"
#include "StringFormat.h"
#include "DBCStores.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    constexpr uint32 CHECK_INTERVAL_MS = 60000;  // check every minute
    constexpr uint32 MIN_SESSION_MS    = 60000;  // one minute of playtime to start counting

    struct RewardDefinition
    {
        uint32      id             = 0;
        uint32      minutesRequired = 0;
        uint32      itemEntry      = 0;
        uint32      itemAmount     = 1;
        uint32      auroraPoints   = 0;
        uint32      shards         = 0;
        uint32      titleDbcId     = 0;
        std::string resetPeriod;
        std::string subjectRu;
        std::string subjectEn;
        std::string textRu;
        std::string textEn;
    };

    std::vector<RewardDefinition> g_rewards;

    std::string CurrentPeriodKey(std::string const& resetPeriod)
    {
        time_t now = time(nullptr);
        tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        char buffer[32];

        if (resetPeriod == "once")
            return "once";

        if (resetPeriod == "monthly")
        {
            std::strftime(buffer, sizeof(buffer), "%Y-%m", &localTime);
            return buffer;
        }

        if (resetPeriod == "weekly")
        {
            int const daysSinceMonday = (localTime.tm_wday + 6) % 7;
            now -= daysSinceMonday * 86400;
#ifdef _WIN32
            localtime_s(&localTime, &now);
#else
            localtime_r(&now, &localTime);
#endif
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);
            return buffer;
        }

        // default: daily
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);
        return buffer;
    }

    bool AlreadyClaimed(uint32 guid, uint32 rewardId, std::string const& periodKey)
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT 1 FROM `aurora_online_claims` "
            "WHERE `player_guid` = {} AND `reward_id` = {} AND `claim_period` = '{}' "
            "LIMIT 1", guid, rewardId, periodKey);

        return result != nullptr;
    }

    void MarkClaimed(uint32 guid, uint32 rewardId, std::string const& periodKey)
    {
        CharacterDatabase.Execute(
            "INSERT IGNORE INTO `aurora_online_claims` "
            "(`player_guid`, `reward_id`, `claim_period`) VALUES ({}, {}, '{}')",
            guid, rewardId, periodKey);
    }

    void GrantReward(Player* player, RewardDefinition const& reward)
    {
        if (reward.titleDbcId > 0)
        {
            if (CharTitlesEntry const* title = sCharTitlesStore.LookupEntry(reward.titleDbcId))
                player->SetTitle(title);
            else
                LOG_WARN("module", "[Aurora] Online reward {} references missing title {} "
                                   "- client DBC patch not applied?", reward.id, reward.titleDbcId);
        }

        if (reward.itemEntry > 0)
        {
            // Mail the item instead of dropping it in the bag: the player may
            // be at full capacity and must not lose a reward while offline.
            MailDraft draft(reward.subjectRu, reward.textRu);
            draft.AddItem(reward.itemEntry, reward.itemAmount);
            draft.SendMailTo(MailReceiver(player),
                             MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_DEFAULT));
        }

        if (reward.auroraPoints > 0)
            player->AddItem(ITEM_AURORA_POINTS, reward.auroraPoints);

        if (reward.shards > 0)
            player->AddItem(ITEM_SHARD_OF_FALLEN_STAR, reward.shards);
    }
}

class aurora_online_rewards : public WorldScript
{
public:
    aurora_online_rewards() : WorldScript("aurora_online_rewards") { }

    void OnStartup() override
    {
        LoadRewards();
        LOG_INFO("module", "[Aurora] Online rewards loaded: {} thresholds.", g_rewards.size());
    }

    void OnUpdate(uint32 diff) override
    {
        _timer += diff;
        if (_timer < CHECK_INTERVAL_MS)
            return;

        _timer = 0;
        Tick();
    }

private:
    void LoadRewards()
    {
        g_rewards.clear();

        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `minutes_required`, `item_entry`, `item_amount`, "
            "`aurora_points`, `shards`, `title_dbc_id`, `reset_period`, "
            "`mail_subject_ru`, `mail_subject_en`, `mail_text_ru`, `mail_text_en` "
            "FROM `aurora_online_rewards` WHERE `enabled` = 1 "
            "ORDER BY `minutes_required` ASC");

        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();

            RewardDefinition reward;
            reward.id              = fields[0].Get<uint32>();
            reward.minutesRequired = fields[1].Get<uint32>();
            reward.itemEntry       = fields[2].Get<uint32>();
            reward.itemAmount      = fields[3].Get<uint32>();
            reward.auroraPoints    = fields[4].Get<uint32>();
            reward.shards          = fields[5].Get<uint32>();
            reward.titleDbcId      = fields[6].Get<uint32>();
            reward.resetPeriod     = fields[7].Get<std::string>();
            reward.subjectRu       = fields[8].Get<std::string>();
            reward.subjectEn       = fields[9].Get<std::string>();
            reward.textRu          = fields[10].Get<std::string>();
            reward.textEn          = fields[11].Get<std::string>();

            g_rewards.push_back(reward);
        } while (result->NextRow());
    }

    void Tick()
    {
        if (g_rewards.empty())
            return;

        for (auto const& pair : ObjectAccessor::GetPlayers())
        {
            Player* player = pair.second;
            if (!player || !player->IsInWorld())
                continue;

            ProcessPlayer(player);
        }
    }

    void ProcessPlayer(Player* player)
    {
        uint32 const guid = player->GetGUID().GetCounter();

        QueryResult progress = CharacterDatabase.Query(
            "SELECT `daily_minutes`, `weekly_minutes`, `monthly_minutes` "
            "FROM `aurora_online_progress` WHERE `player_guid` = {}", guid);

        if (!progress)
            return;

        Field* fields = progress->Fetch();
        uint32 dailyMinutes   = fields[0].Get<uint32>();
        uint32 weeklyMinutes  = fields[1].Get<uint32>();
        uint32 monthlyMinutes = fields[2].Get<uint32>();

        for (RewardDefinition const& reward : g_rewards)
        {
            uint32 available = dailyMinutes;
            if (reward.resetPeriod == "weekly")
                available = weeklyMinutes;
            else if (reward.resetPeriod == "monthly" || reward.resetPeriod == "once")
                available = monthlyMinutes;

            if (available < reward.minutesRequired)
                continue;

            std::string const periodKey = CurrentPeriodKey(reward.resetPeriod);

            if (AlreadyClaimed(guid, reward.id, periodKey))
                continue;

            GrantReward(player, reward);
            MarkClaimed(guid, reward.id, periodKey);
        }
    }

    uint32 _timer = 0;
};

void AddSC_aurora_online_rewards()
{
    new aurora_online_rewards();
}
