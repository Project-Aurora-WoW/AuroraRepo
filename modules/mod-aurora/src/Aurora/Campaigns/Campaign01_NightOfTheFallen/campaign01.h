#ifndef AURORA_CAMPAIGN01_H
#define AURORA_CAMPAIGN01_H

// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// Shared declarations for the instance script and its actors.
// AzerothCore master (3.3.5a).
// ============================================================================

#include "aurora_shared.h"
#include "Position.h"

namespace Aurora
{
namespace Campaign01
{
    // ------------------------------------------------------------------------
    // Encounter states kept inside the InstanceScript.
    // ------------------------------------------------------------------------
    enum AltarState : uint8
    {
        ALTAR_NEUTRAL   = 0,
        ALTAR_ALLIANCE  = 1,
        ALTAR_HORDE     = 2
    };

    // World state ids used by the client addon (mirrored in sql/03).
    enum Campaign01WorldStates : uint32
    {
        WS_ALTARS_CONTROLLED = 30050,
        WS_BOSS_SHIELD       = 30051
    };

    // Altar GameObject spawn guids, in the fixed N / S / W / E order.
    static constexpr uint32 AltarSpawnGuids[4] =
    {
        900101, 900102, 900103, 900104
    };

    // Altar display positions, used by the capture channel and by the
    // "captured" visual effect.
    static constexpr float AltarPositions[4][3] =
    {
        {  100.0f,  500.0f, 50.0f },   // North
        {  100.0f, -500.0f, 50.0f },   // South
        { -400.0f,    0.0f, 50.0f },   // West
        {  400.0f,    0.0f, 50.0f }    // East
    };

    static constexpr uint8  MAX_ALTARS          = 4;
    static constexpr uint8  ALTARS_FOR_SHIELD   = 3;
    static constexpr uint32 CAPTURE_TIME_MS     = 10000;
    static constexpr uint32 CAPTURE_RADIUS      = 20.0f;
    static constexpr uint32 ENRAGE_AFTER_MS     = 600000;

    // ------------------------------------------------------------------------
    // Shield penalty per captured altar, from the design table:
    //   0 -> immune, 1 -> -10% HP, 2 -> -25% HP, 3 -> shield off -50% armor,
    //   4 -> -75% armor +100% damage
    // ------------------------------------------------------------------------
    struct AltarEffect
    {
        float healthPercent;    // multiplier applied to max health
        float armorMultiplier;  // 1.0 = untouched
        float damageMultiplier; // 1.0 = untouched
        bool  shieldUp;
    };

    AltarEffect const& GetEffect(uint8 altarsControlled);

    // ------------------------------------------------------------------------
    // Helpers shared between the instance script and the actors.
    // ------------------------------------------------------------------------
    bool  IsAltarEntry(uint32 entry);
    int8  GetAltarIndexByPosition(float x, float y);
    char const* GetAltarStateName(AltarState state);

    // ------------------------------------------------------------------------
    // Run statistics, implemented in aurora_campaign01_progress.cpp.
    // Declared here so the instance script does not include Player.h.
    // ------------------------------------------------------------------------
    void AuroraRecordAltarCapture(Player* player, InstanceScript* instance);
    void AuroraRecordRaidDeath(InstanceScript* instance);
    void AuroraPushWorldStates(InstanceScript* instance, uint32 altarsControlled, bool shieldBroken);
}
}

#endif // AURORA_CAMPAIGN01_H
