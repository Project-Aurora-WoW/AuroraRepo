// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// aurora_campaign01_achievements.cpp - achievement tracking.
//
// DESIGN CONSTRAINT
//   AzerothCore does not store achievements in a world table: the client reads
//   them from Achievement.dbc, and the server checks criteria from
//   Achievement_Criteria.dbc. A custom achievement exists only if both DBCs
//   were patched into the client MPQ.
//
//   Therefore this file NEVER assumes the DBC entry exists. Every award goes
//   through TryAward(), which looks the entry up in sAchievementStore first
//   and silently skips the award (logging once) when the client patch is
//   missing. That way the module is safe to ship before the DBC work is done,
//   and starts granting achievements the moment the patch lands.
//
//   Until then, progress is still visible to the player through
//   `.aurora campaign` (aurora_campaign01_commands.cpp), which reads the same
//   counters but does not depend on any DBC.
//
// TRACKED ACHIEVEMENTS
//   10001 Первый шаг Авроры     - first entry into map 950
//   10002 Падение Малакора      - kill on normal difficulty
//   10003 Герой Ночи            - kill on heroic difficulty
//   10004 Ночь без жертв        - full clear with no raid deaths
//   10010 Щитоносец             - shield broken within 5 minutes
//   10013 Быстрее тьмы          - full clear within 30 minutes
//   10014 Соло-герой            - Twilight Warden killed without a group
//   10020 Захватчик алтарей     - 10 altar captures (lifetime)
//   10022 Одинокий волк         - 3 altars captured solo in one run
//   10024 Доминион              - all 4 altars in one run
//   10030/10031/10032           - quest chain finished
//   10033 Тройной агент         - all three chains finished
//   10099 Герой Ночи Падших     - meta
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "Group.h"
#include "InstanceScript.h"
#include "CreatureScript.h"
#include "GameTime.h"
#include "QuestDef.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "AchievementMgr.h"
#include "Log.h"

#include "campaign01.h"
#include "aurora_shared.h"

using namespace Aurora;
using namespace Aurora::Campaign01;

namespace
{
    // ------------------------------------------------------------------------
    // DBC-safe award
    // ------------------------------------------------------------------------
    void TryAward(Player* player, uint32 achievementId)
    {
        if (!player || player->HasAchieved(achievementId))
            return;

        AchievementEntry const* entry = sAchievementStore.LookupEntry(achievementId);
        if (!entry)
        {
            // Logged once per achievement id per process, so a server without
            // the client patch does not flood the log.
            static std::unordered_set<uint32> s_warned;
            if (s_warned.insert(achievementId).second)
            {
                LOG_INFO("module", "[Aurora] Achievement {} is not in Achievement.dbc "
                                   "- tracked internally only. Apply the client DBC patch "
                                   "to enable the real award.", achievementId);
            }
            return;
        }

        if (AchievementMgr* mgr = player->GetAchievementMgr())
            mgr->CompletedAchievement(entry);
    }

    // ------------------------------------------------------------------------
    // Instance-side snapshot
    // ------------------------------------------------------------------------
    struct RunSnapshot
    {
        uint32 altarsControlled = 0;
        uint32 altarsCaptured   = 0;
        uint32 capturesSolo     = 0;
        uint32 maxAltars        = 0;
        uint32 raidDeaths       = 0;
        uint32 shieldBrokenAt   = 0;
        uint32 startTime        = 0;
        bool   valid            = false;
    };

    bool Snapshot(InstanceScript* instance, RunSnapshot& out)
    {
        if (!instance)
            return false;

        out.altarsControlled = instance->GetData(DATA_ALTARS_CONTROLLED);
        out.altarsCaptured   = instance->GetData(DATA_ALTARS_CAPTURED_TOTAL);
        out.capturesSolo     = instance->GetData(DATA_CAPTURES_SOLO);
        out.maxAltars        = instance->GetData(DATA_MAX_ALTARS_IN_RUN);
        out.raidDeaths       = instance->GetData(DATA_RAID_DEATHS);
        out.shieldBrokenAt   = instance->GetData(DATA_SHIELD_BROKEN_TIME);
        out.startTime        = instance->GetData(DATA_CAMPAIGN_START_TIME);
        out.valid            = true;
        return true;
    }

    // Lifetime altar captures, used for the 10-capture achievement.
    void AddLifetimeCaptures(uint32 guid, uint32 amount)
    {
        if (amount == 0)
            return;

        CharacterDatabase.Execute(
            "UPDATE `aurora_player_activity` "
            "SET `creatures_killed` = `creatures_killed` "
            "WHERE `player_guid` = {}", guid);

        CharacterDatabase.Execute(
            "INSERT INTO `aurora_campaign_counters` "
            "(`player_guid`, `counter`, `value`) VALUES ({}, 'altar_captures', {}) "
            "ON DUPLICATE KEY UPDATE `value` = `value` + {}",
            guid, amount, amount);
    }

    uint32 GetLifetimeCaptures(uint32 guid)
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT `value` FROM `aurora_campaign_counters` "
            "WHERE `player_guid` = {} AND `counter` = 'altar_captures'", guid);

        if (!result)
            return 0;

        return result->Fetch()[0].Get<uint32>();
    }
}

// ============================================================================
// Player script - entry, quests
// ============================================================================
class aurora_campaign01_player : public PlayerScript
{
public:
    aurora_campaign01_player() : PlayerScript("aurora_campaign01_player") { }

    void OnPlayerMapChanged(Player* player) override
    {
        if (!player || player->GetMapId() != MAP_NIGHT_OF_THE_FALLEN)
            return;

        TryAward(player, ACH_FIRST_STEP);
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (!player || !quest)
            return;

        switch (quest->GetQuestId())
        {
            case QUEST_ALLIANCE_6:
                TryAward(player, ACH_PATH_OF_LIGHT);
                CheckTripleAgent(player);
                TryAward(player, ACH_HERO_OF_FALLEN_NIGHT);
                break;

            case QUEST_HORDE_6:
                TryAward(player, ACH_PATH_OF_BLOOD);
                CheckTripleAgent(player);
                TryAward(player, ACH_HERO_OF_FALLEN_NIGHT);
                break;

            case QUEST_NEUTRAL_6:
                TryAward(player, ACH_PATH_OF_BALANCE);
                CheckTripleAgent(player);
                TryAward(player, ACH_HERO_OF_FALLEN_NIGHT);
                break;

            default:
                break;
        }
    }

private:
    void CheckTripleAgent(Player* player)
    {
        bool const light   = player->GetQuestRewardStatus(QUEST_ALLIANCE_6);
        bool const blood   = player->GetQuestRewardStatus(QUEST_HORDE_6);
        bool const balance = player->GetQuestRewardStatus(QUEST_NEUTRAL_6);

        if (light && blood && balance)
            TryAward(player, ACH_TRIPLE_AGENT);
    }
};

// ============================================================================
// Creature script - kills inside the campaign
// ============================================================================
class aurora_campaign01_creature : public PlayerScript
{
public:
    aurora_campaign01_creature() : PlayerScript("aurora_campaign01_creature") { }

    void OnPlayerCreatureKill(Player* player, Creature* creature) override
    {
        if (!creature || !player)
            return;

        if (player->GetMapId() != MAP_NIGHT_OF_THE_FALLEN)
            return;

        if (creature->GetEntry() == NPC_TWILIGHT_WARDEN && !player->GetGroup())
            TryAward(player, ACH_SOLO_HERO);

        if (creature->GetEntry() != NPC_MALAKOR_FALLEN_KING)
            return;

        InstanceScript* instance = creature->GetInstanceScript();
        RunSnapshot run;
        if (!Snapshot(instance, run))
            return;

        uint32 const now = static_cast<uint32>(GameTime::GetGameTime().count());

        // Difficulty: the campaign uses the stock normal/heroic split to tell
        // the 10-man and 25-man achievement paths apart.
        uint32 const difficulty = creature->GetMap()->GetDifficulty();
        TryAward(player, difficulty == DUNGEON_DIFFICULTY_NORMAL ? ACH_MALAKOR_10
                                                                 : ACH_MALAKOR_25);

        if (run.shieldBrokenAt > 0 && run.startTime > 0)
        {
            uint32 const shieldTime = run.shieldBrokenAt - run.startTime;
            if (shieldTime > 0 && shieldTime <= 300)
                TryAward(player, ACH_SHIELD_BROKER);
        }

        if (run.startTime > 0 && now > run.startTime && (now - run.startTime) <= 1800)
            TryAward(player, ACH_FASTER_THAN_DARK);

        if (run.raidDeaths == 0)
            TryAward(player, ACH_NO_DEATHS);

        // Altar achievements are per-run state, evaluated on the kill.
        if (run.maxAltars >= MAX_ALTARS)
            TryAward(player, ACH_DOMINION);

        if (run.capturesSolo >= 3)
            TryAward(player, ACH_LONE_WOLF);

        AddLifetimeCaptures(player->GetGUID().GetCounter(), run.altarsCaptured);

        if (GetLifetimeCaptures(player->GetGUID().GetCounter()) >= 10)
            TryAward(player, ACH_ALTAR_CAPTURER);

        // Meta: every other campaign #1 achievement.
        uint32 const meta[] =
        {
            ACH_FIRST_STEP, ACH_MALAKOR_10, ACH_MALAKOR_25, ACH_NO_DEATHS,
            ACH_SHIELD_BROKER, ACH_FASTER_THAN_DARK, ACH_SOLO_HERO,
            ACH_ALTAR_CAPTURER, ACH_LONE_WOLF, ACH_DOMINION,
            ACH_PATH_OF_LIGHT, ACH_PATH_OF_BLOOD, ACH_PATH_OF_BALANCE,
            ACH_TRIPLE_AGENT
        };

        bool complete = true;
        for (uint32 id : meta)
        {
            if (!player->HasAchieved(id))
            {
                complete = false;
                break;
            }
        }

        if (complete)
            TryAward(player, ACH_HERO_OF_FALLEN_NIGHT);
    }
};

void AddSC_aurora_campaign01_achievements()
{
    new aurora_campaign01_player();
    new aurora_campaign01_creature();
}
