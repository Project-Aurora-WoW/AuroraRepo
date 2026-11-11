// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// npc_twilight_warden.cpp - altar guardian.
//
// Rules
//   * Passive until attacked; guards its altar and never leaves it far.
//   * Leashing: if pulled beyond the leash radius it returns, heals to full
//     and resets its altar capture progress.
//   * Killing a warden unlocks the altar (handled by the instance script).
//
// The warden deliberately does NOT aggro on proximity: the altar must be
// capture-gated by killing the warden, not by accidental proximity pulls.
// ============================================================================

#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "ScriptedCreature.h"

#include "campaign01.h"

using namespace Aurora;

namespace
{
    enum WardenSpells
    {
        SPELL_WARDEN_STRIKE  = 500015,
        SPELL_WARDEN_CURSE   = 500016
    };

    enum WardenEvents
    {
        EVENT_STRIKE = 1,
        EVENT_CURSE  = 2,
        EVENT_LEASH  = 3
    };

    constexpr uint32 STRIKE_INTERVAL = 5000;
    constexpr uint32 CURSE_INTERVAL  = 15000;
    constexpr uint32 LEASH_CHECK     = 1000;
    constexpr float  LEASH_RADIUS    = 60.0f;

    enum WardenSay
    {
        SAY_AGGRO = 0,
        SAY_DEATH = 1,
        SAY_RESET = 2
    };
}

class npc_twilight_warden : public CreatureScript
{
public:
    npc_twilight_warden() : CreatureScript("npc_twilight_warden") { }

    struct npc_twilight_wardenAI : public ScriptedAI
    {
        explicit npc_twilight_wardenAI(Creature* creature)
            : ScriptedAI(creature),
              _homeX(creature->GetHomePosition().GetPositionX()),
              _homeY(creature->GetHomePosition().GetPositionY()),
              _homeZ(creature->GetHomePosition().GetPositionZ()) { }

        void Reset() override
        {
            events.Reset();

            // Altar index is resolved from home position so a warden that was
            // pulled and reset still knows which altar it belongs to.
            _altarIndex = Campaign01::GetAltarIndexByPosition(_homeX, _homeY);

            me->SetReactState(REACT_DEFENSIVE);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);

            events.ScheduleEvent(EVENT_STRIKE, STRIKE_INTERVAL);
            events.ScheduleEvent(EVENT_CURSE, CURSE_INTERVAL);
            events.ScheduleEvent(EVENT_LEASH, LEASH_CHECK);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_STRIKE:
                        if (me->GetVictim())
                            DoCastVictim(SPELL_WARDEN_STRIKE);
                        events.ScheduleEvent(EVENT_STRIKE, STRIKE_INTERVAL);
                        break;

                    case EVENT_CURSE:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true))
                            DoCast(target, SPELL_WARDEN_CURSE);
                        events.ScheduleEvent(EVENT_CURSE, CURSE_INTERVAL);
                        break;

                    case EVENT_LEASH:
                        if (IsBeyondLeash())
                        {
                            Talk(SAY_RESET);
                            EnterEvadeMode(EvadeReason::Other);
                            break;
                        }
                        events.ScheduleEvent(EVENT_LEASH, LEASH_CHECK);
                        break;

                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            events.Reset();
            // The instance script releases the altar in OnCreatureDeath.
        }

    private:
        bool IsBeyondLeash() const
        {
            float dx = me->GetPositionX() - _homeX;
            float dy = me->GetPositionY() - _homeY;
            return (dx * dx + dy * dy) > (LEASH_RADIUS * LEASH_RADIUS);
        }

        EventMap events;
        float _homeX;
        float _homeY;
        float _homeZ;
        int8  _altarIndex = -1;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_twilight_wardenAI(creature);
    }
};

void AddSC_npc_twilight_warden()
{
    new npc_twilight_warden();
}
