// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// npc_shadow_minion.cpp - the summoned shadow minion.
//
// Malakor summons these with SPELL_SUMMON_MINION. They are disposable pets:
//   * aggressive toward the raid, but never run far from their summoner;
//   * cast a weak shadow bolt on a slow timer instead of pure melee so a pack
//     of them still reads as "shadow magic" rather than a wall of auto-attacks;
//   * despawn when their owner dies, so a wiped raid does not leave stragglers.
// ============================================================================

#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"

#include "campaign01.h"

using namespace Aurora;

namespace
{
    enum MinionSpells
    {
        SPELL_MINION_BOLT = 500010   // SPELL_SHADOW_BOLT from aurora_shared.h
    };

    enum MinionEvents
    {
        EVENT_BOLT  = 1,
        EVENT_CHECK = 2
    };

    constexpr uint32 BOLT_FIRST_MS  = 2000;
    constexpr uint32 BOLT_REPEAT_MS = 6000;
    constexpr uint32 CHECK_MS       = 1000;
    constexpr float  LEASH_RADIUS   = 45.0f;
}

class npc_shadow_minion : public CreatureScript
{
public:
    npc_shadow_minion() : CreatureScript("npc_shadow_minion") { }

    struct npc_shadow_minionAI : public ScriptedAI
    {
        explicit npc_shadow_minionAI(Creature* creature)
            : ScriptedAI(creature),
              _spawnX(creature->GetPositionX()),
              _spawnY(creature->GetPositionY()) { }

        void Reset() override
        {
            events.Reset();
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            events.ScheduleEvent(EVENT_BOLT, Milliseconds(BOLT_FIRST_MS));
            events.ScheduleEvent(EVENT_CHECK, Milliseconds(CHECK_MS));
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            // A minion whose summoner is gone has nothing to fight for.
            if (Unit* owner = me->GetOwner())
            {
                if (!owner->IsAlive())
                {
                    me->DespawnOrUnsummon();
                    return;
                }
            }

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BOLT:
                        if (Unit* victim = me->GetVictim())
                            DoCast(victim, SPELL_MINION_BOLT);
                        events.ScheduleEvent(EVENT_BOLT, Milliseconds(BOLT_REPEAT_MS));
                        break;

                    case EVENT_CHECK:
                        // Follow the summoner instead of wandering the map.
                        if (IsBeyondLeash())
                        {
                            EnterEvadeMode(EVADE_REASON_OTHER);
                            break;
                        }
                        events.ScheduleEvent(EVENT_CHECK, Milliseconds(CHECK_MS));
                        break;

                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

    private:
        bool IsBeyondLeash() const
        {
            float dx = me->GetPositionX() - _spawnX;
            float dy = me->GetPositionY() - _spawnY;
            return (dx * dx + dy * dy) > (LEASH_RADIUS * LEASH_RADIUS);
        }

        EventMap events;
        float _spawnX;
        float _spawnY;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shadow_minionAI(creature);
    }
};

void AddSC_npc_shadow_minion()
{
    new npc_shadow_minion();
}
