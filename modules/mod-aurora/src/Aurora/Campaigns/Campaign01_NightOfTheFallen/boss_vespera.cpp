// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// boss_vespera.cpp - Lady Vespera, co-boss.
//
// Design intent
//   Vespera is NOT a second shield-gated boss. She is a support caster who
//   keeps the altar wardens alive: while the wardens survive, the altars stay
//   locked, so the PvP phase cannot be skipped by pulling her.
//   Killing Vespera removes the warden healing and speeds the capture phase.
// ============================================================================

#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"

#include "campaign01.h"

using namespace Aurora;

namespace
{
    enum VesperaSay
    {
        SAY_AGGRO   = 0,
        SAY_HEAL    = 1,
        SAY_DEATH   = 2
    };

    enum VesperaSpells
    {
        SPELL_VESPERA_BOLT      = SPELL_SHADOW_BOLT,
        SPELL_VESPERA_HEAL      = 500013,
        SPELL_VESPERA_WEB       = 500014
    };

    enum VesperaEvents
    {
        EVENT_BOLT          = 1,
        EVENT_HEAL_WARDEN   = 2,
        EVENT_WEB           = 3,
        EVENT_CHECK_WARDENS = 4
    };

    constexpr uint32 BOLT_INTERVAL        = 2500;
    constexpr uint32 HEAL_INTERVAL        = 12000;
    constexpr uint32 WEB_INTERVAL         = 20000;
    constexpr uint32 WARDEN_CHECK_INTERVAL = 2000;
    constexpr float  WARDEN_SEARCH_RANGE  = 200.0f;
}

class boss_vespera : public CreatureScript
{
public:
    boss_vespera() : CreatureScript("boss_vespera") { }

    struct boss_vesperaAI : public BossAI
    {
        explicit boss_vesperaAI(Creature* creature)
            : BossAI(creature, Aurora::DATA_LADY_VESPERA) { }

        void Reset() override
        {
            events.Reset();
            _wardens.clear();
            _lastWardenScan = 0;
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            instance->SetBossState(Aurora::DATA_LADY_VESPERA, IN_PROGRESS);

            events.ScheduleEvent(EVENT_BOLT, Milliseconds(BOLT_INTERVAL));
            events.ScheduleEvent(EVENT_HEAL_WARDEN, Milliseconds(HEAL_INTERVAL));
            events.ScheduleEvent(EVENT_WEB, Milliseconds(WEB_INTERVAL));
            events.ScheduleEvent(EVENT_CHECK_WARDENS, Milliseconds(WARDEN_CHECK_INTERVAL));
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BOLT:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true))
                            DoCast(target, SPELL_VESPERA_BOLT);
                        events.ScheduleEvent(EVENT_BOLT, Milliseconds(BOLT_INTERVAL));
                        break;

                    case EVENT_HEAL_WARDEN:
                        HealWardens();
                        events.ScheduleEvent(EVENT_HEAL_WARDEN, Milliseconds(HEAL_INTERVAL));
                        break;

                    case EVENT_WEB:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true))
                            DoCast(target, SPELL_VESPERA_WEB);
                        events.ScheduleEvent(EVENT_WEB, Milliseconds(WEB_INTERVAL));
                        break;

                    case EVENT_CHECK_WARDENS:
                        // No living warden left: the altars are fully open,
                        // so Vespera stops wasting time on healing.
                        if (_wardens.empty() || !HasLivingWarden())
                        {
                            events.CancelEvent(EVENT_HEAL_WARDEN);
                            break;
                        }
                        events.ScheduleEvent(EVENT_CHECK_WARDENS, Milliseconds(WARDEN_CHECK_INTERVAL));
                        break;

                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            instance->SetBossState(Aurora::DATA_LADY_VESPERA, DONE);

            // Her death does not unlock the altars by itself: the wardens must
            // still be killed. It only removes the healing pressure.
            events.Reset();
        }

    private:
        void HealWardens()
        {
            RefreshWardenList();
            if (_wardens.empty())
                return;

            Talk(SAY_HEAL);

            for (ObjectGuid const& guid : _wardens)
            {
                if (Creature* warden = ObjectAccessor::GetCreature(*me, guid))
                    if (warden->IsAlive() && warden->GetHealth() < warden->GetMaxHealth())
                        DoCast(warden, SPELL_VESPERA_HEAL);
            }
        }

        void RefreshWardenList()
        {
            _wardens.clear();

            std::list<Creature*> found;
            me->GetCreatureListWithEntryInGrid(found, Aurora::NPC_TWILIGHT_WARDEN,
                                               WARDEN_SEARCH_RANGE);

            for (Creature* warden : found)
                if (warden && warden->IsAlive())
                    _wardens.push_back(warden->GetGUID());
        }

        bool HasLivingWarden() const
        {
            for (ObjectGuid const& guid : _wardens)
                if (Creature* warden = ObjectAccessor::GetCreature(*me, guid))
                    if (warden->IsAlive())
                        return true;

            return false;
        }

        std::vector<ObjectGuid> _wardens;
        uint32 _lastWardenScan;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_vesperaAI(creature);
    }
};

void AddSC_boss_vespera()
{
    new boss_vespera();
}
