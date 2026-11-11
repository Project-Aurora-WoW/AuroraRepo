// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// boss_malakor.cpp - Malakor, the Fallen King.
//
// Behaviour
//   * Passive and immune until the encounter starts: SPELL_DARK_SHIELD is
//     applied by the instance script and is the only source of immunity.
//   * While the shield holds the boss does nothing but watch: no melee, no
//     spells. This makes the PvP phase the real gate.
//   * The moment the shield drops (instance script removes it), the boss AI
//     detects the lost aura within one tick and starts the PvE rotation.
//   * Enrage is scheduled by the instance script ENRAGE_AFTER_MS after the
//     shield break, not by the AI, so it survives a server restart wording.
// ============================================================================

#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "Unit.h"

#include "campaign01.h"

using namespace Aurora;

namespace
{
    enum MalakorSay
    {
        SAY_AGGRO           = 0,
        SAY_SHIELD_BROKEN   = 1,
        SAY_ENRAGE          = 2,
        SAY_DEATH           = 3,
        SAY_MINION_SUMMON   = 4
    };

    enum MalakorSpells
    {
        SPELL_SHADOW_BOLT_AURORA    = SPELL_SHADOW_BOLT,
        SPELL_SUMMON_MINION_AURORA  = SPELL_SUMMON_MINION,
        SPELL_DARKNESS_AURA_AURORA  = SPELL_DARKNESS_AURA
    };

    enum MalakorEvents
    {
        EVENT_SHADOW_BOLT       = 1,
        EVENT_SUMMON_MINION     = 2,
        EVENT_DARKNESS_AURA     = 3,
        EVENT_CHECK_SHIELD      = 4
    };

    // Timers, milliseconds.
    constexpr uint32 SHADOW_BOLT_INTERVAL   = 3000;
    constexpr uint32 SUMMON_MINION_INTERVAL = 30000;
    constexpr uint32 DARKNESS_AURA_INTERVAL = 45000;
    constexpr uint32 SHIELD_CHECK_INTERVAL  = 500;

    constexpr uint8 MINIONS_PER_WAVE = 3;
}

class boss_malakor : public CreatureScript
{
public:
    boss_malakor() : CreatureScript("boss_malakor") { }

    struct boss_malakorAI : public BossAI
    {
        explicit boss_malakorAI(Creature* creature)
            : BossAI(creature, Aurora::DATA_FALLEN_KING), _shieldBroken(false) { }

        void Reset() override
        {
            _shieldBroken = false;

            me->SetReactState(REACT_PASSIVE);
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);

            if (!me->HasAura(SPELL_DARK_SHIELD))
                me->CastSpell(me, SPELL_DARK_SHIELD, true);

            events.Reset();
            events.ScheduleEvent(EVENT_CHECK_SHIELD, SHIELD_CHECK_INTERVAL);

            _summons.clear();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            instance->SetBossState(Aurora::DATA_FALLEN_KING, IN_PROGRESS);
            instance->SetData(Aurora::DATA_FALLEN_KING, 0);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHECK_SHIELD:
                        HandleShieldCheck();
                        events.ScheduleEvent(EVENT_CHECK_SHIELD, SHIELD_CHECK_INTERVAL);
                        break;

                    case EVENT_SHADOW_BOLT:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true))
                            DoCast(target, SPELL_SHADOW_BOLT_AURORA);
                        events.ScheduleEvent(EVENT_SHADOW_BOLT, SHADOW_BOLT_INTERVAL);
                        break;

                    case EVENT_SUMMON_MINION:
                        SummonWave();
                        events.ScheduleEvent(EVENT_SUMMON_MINION, SUMMON_MINION_INTERVAL);
                        break;

                    case EVENT_DARKNESS_AURA:
                        DoCast(me, SPELL_DARKNESS_AURA_AURORA, true);
                        events.ScheduleEvent(EVENT_DARKNESS_AURA, DARKNESS_AURA_INTERVAL);
                        break;

                    default:
                        break;
                }
            }

            if (!_shieldBroken || !UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            instance->SetBossState(Aurora::DATA_FALLEN_KING, DONE);
            DespawnMinions();
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            DespawnMinions();
            _shieldBroken = false;
            BossAI::EnterEvadeMode(EvadeReason::Other);
        }

    private:
        void HandleShieldCheck()
        {
            if (_shieldBroken)
                return;

            if (me->HasAura(SPELL_DARK_SHIELD))
                return;

            // Shield dropped: the instance script already removed the aura,
            // either because 3 altars were captured or on a manual GM call.
            _shieldBroken = true;

            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE);

            Talk(SAY_SHIELD_BROKEN);

            events.ScheduleEvent(EVENT_SHADOW_BOLT, SHADOW_BOLT_INTERVAL);
            events.ScheduleEvent(EVENT_SUMMON_MINION, SUMMON_MINION_INTERVAL);
            events.ScheduleEvent(EVENT_DARKNESS_AURA, DARKNESS_AURA_INTERVAL);

            if (!me->IsInCombat())
                me->SetInCombatWithZone();
        }

        void SummonWave()
        {
            Talk(SAY_MINION_SUMMON);

            for (uint8 i = 0; i < MINIONS_PER_WAVE; ++i)
            {
                float x = me->GetPositionX() + frand(-8.0f, 8.0f);
                float y = me->GetPositionY() + frand(-8.0f, 8.0f);

                if (TempSummon* minion = me->SummonCreature(Aurora::NPC_SHADOW_MINION, x, y,
                                                            me->GetPositionZ(), 0.0f,
                                                            TEMPSUMMON_CORPSE_TIMED_DESPAWN,
                                                            30000))
                {
                    _summons.push_back(minion->GetGUID());

                    if (Unit* victim = SelectTarget(SelectTargetMethod::Random, 0))
                        minion->AI()->AttackStart(victim);
                }
            }
        }

        void DespawnMinions()
        {
            for (ObjectGuid const& guid : _summons)
                if (Creature* minion = ObjectAccessor::GetCreature(*me, guid))
                    minion->DespawnOrUnsummon();

            _summons.clear();
        }

        bool _shieldBroken;
        std::vector<ObjectGuid> _summons;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_malakorAI(creature);
    }
};

void AddSC_boss_malakor()
{
    new boss_malakor();
}
