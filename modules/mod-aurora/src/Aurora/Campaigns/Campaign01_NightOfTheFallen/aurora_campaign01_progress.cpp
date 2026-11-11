// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// aurora_campaign01_progress.cpp - run statistics fed by the InstanceScript.
//
// Split out of night_of_the_fallen.cpp so the encounter script stays free of
// cross-header includes (Player.h pulls in a large part of the core).
//
// What it does
//   * counts altar captures per player, both for the current run and for life;
//   * counts raid deaths, which the "no deaths" and meta achievements need;
//   * writes the altar counters to the world state so the client tracker
//     (WS_ALTARS_CONTROLLED = 30050, WS_BOSS_SHIELD = 30051) draws real data.
//
// All functions are no-ops when the matching rows are missing, so a server
// without the Aurora tables still runs the instance.
// ============================================================================

#include "Player.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "DatabaseEnv.h"
#include "Log.h"

#include "campaign01.h"
#include "aurora_shared.h"

using namespace Aurora;
using namespace Aurora::Campaign01;

namespace
{
    // One row per player per capture, rolled up by the SQL itself.
    void StoreCapture(Player* player)
    {
        if (!player)
            return;

        uint32 const guid = player->GetGUID().GetCounter();

        CharacterDatabase.Execute(
            "INSERT INTO `aurora_campaign_counters` "
            "(`player_guid`, `counter`, `value`) VALUES ({}, 'altar_captures', 1) "
            "ON DUPLICATE KEY UPDATE `value` = `value` + 1", guid);

        // The instance script keeps the per-run number; the character row
        // needs to exist so `.aurora campaign` and the 10-capture achievement
        // have something to read before the boss dies.
        CharacterDatabase.Execute(
            "INSERT IGNORE INTO `aurora_player_activity` (`player_guid`) VALUES ({})", guid);
    }
}

namespace Aurora
{
namespace Campaign01
{
    void AuroraRecordAltarCapture(Player* player, InstanceScript* instance)
    {
        StoreCapture(player);

        if (!instance)
            return;

        instance->DoAction(DATA_ALTARS_CAPTURED_TOTAL);

        if (player && !player->GetGroup())
            instance->DoAction(DATA_CAPTURES_SOLO);

        LOG_DEBUG("module", "[Aurora] Altar captured by {} (map {}).",
                  player ? player->GetName() : "?", MAP_NIGHT_OF_THE_FALLEN);
    }

    void AuroraRecordRaidDeath(InstanceScript* instance)
    {
        if (instance)
            instance->SetData(DATA_RAID_DEATHS, instance->GetData(DATA_RAID_DEATHS) + 1);
    }

    void AuroraPushWorldStates(InstanceScript* instance, uint32 altarsControlled, bool shieldBroken)
    {
        if (!instance)
            return;

        instance->DoUpdateWorldState(WS_ALTARS_CONTROLLED, altarsControlled);
        instance->DoUpdateWorldState(WS_BOSS_SHIELD, shieldBroken ? 0 : 1);
    }
}
}
