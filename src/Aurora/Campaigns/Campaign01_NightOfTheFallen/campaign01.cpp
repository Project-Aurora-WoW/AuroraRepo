// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// campaign01.cpp - implementations of the shared declarations.
// ============================================================================

#include "campaign01.h"

namespace Aurora
{
namespace Campaign01
{
    namespace
    {
        // Index 0..4 = number of altars controlled by one faction.
        AltarEffect const s_effects[5] =
        {
            { 1.00f, 1.00f, 1.00f, true  },   // 0 altars - full immunity
            { 0.90f, 1.00f, 1.00f, true  },   // 1 altar  - -10% HP
            { 0.75f, 1.00f, 1.00f, true  },   // 2 altars - -25% HP
            { 1.00f, 0.50f, 1.00f, false },   // 3 altars - shield off, -50% armor
            { 1.00f, 0.25f, 2.00f, false }    // 4 altars - -75% armor, +100% damage
        };

        AltarEffect const s_unreachable = { 1.0f, 1.0f, 1.0f, true };
    }

    AltarEffect const& GetEffect(uint8 altarsControlled)
    {
        if (altarsControlled > MAX_ALTARS)
            return s_unreachable;

        return s_effects[altarsControlled];
    }

    bool IsAltarEntry(uint32 entry)
    {
        return entry == GO_SHADOW_ALTAR;
    }

    int8 GetAltarIndexByPosition(float x, float y)
    {
        // Altars are 500+ yards apart, so a nearest-match on 2D distance is
        // unambiguous even with client interpolation jitter.
        int8 best = -1;
        float bestDistance = 999999.0f;

        for (uint8 i = 0; i < MAX_ALTARS; ++i)
        {
            float dx = x - AltarPositions[i][0];
            float dy = y - AltarPositions[i][1];
            float distance = dx * dx + dy * dy;

            if (distance < bestDistance)
            {
                bestDistance = distance;
                best = static_cast<int8>(i);
            }
        }

        // Reject the match when the player is nowhere near an altar.
        return (bestDistance > 6400.0f) ? -1 : best;
    }

    char const* GetAltarStateName(AltarState state)
    {
        switch (state)
        {
            case ALTAR_ALLIANCE: return "Alliance";
            case ALTAR_HORDE:    return "Horde";
            default:             return "Neutral";
        }
    }
}
}
