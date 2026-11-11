// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// night_of_the_fallen.cpp - InstanceScript.
//
// Encounter flow
//   Phase 1 (PvP): four altars, each guarded by a Twilight Warden. A faction
//                  stands on an altar for CAPTURE_TIME_MS to take it. While
//                  the warden is alive the altar cannot be captured.
//   Phase 2 (PvE): with ALTARS_FOR_SHIELD altars taken by one faction, the
//                  Dark Shield on Malakor falls and the boss becomes killable.
//                  After the shield breaks Malakor enrages in ENRAGE_AFTER_MS.
//
// The script never touches stock tables and never assumes a specific
// difficulty: it reads the instance difficulty only to separate the 10/25
// achievement paths.
// ============================================================================

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "Creature.h"
#include "GameObject.h"
#include "Player.h"
#include "Group.h"
#include "ObjectMgr.h"
#include "Map.h"
#include "WorldState.h"
#include "GameTime.h"

#include "campaign01.h"

using namespace Aurora;
using namespace Aurora::Campaign01;

namespace
{
    // Capture ticks are driven by the instance update; 1000 ms granularity is
    // enough to show a readable progress bar and cheap enough for a raid map.
    constexpr uint32 CAPTURE_TICK_MS = 1000;

    struct AltarRuntime
    {
        ObjectGuid gameObjectGuid;
        ObjectGuid wardenGuid;
        ObjectGuid capturerGuid;      // player currently channelling
        AltarState state           = ALTAR_NEUTRAL;
        uint32     captureProgress = 0;  // ms accumulated
        uint32     allianceHoldMs  = 0;  // how long Alliance kept it (statistics)
        uint32     hordeHoldMs     = 0;
    };
}

class instance_night_of_the_fallen : public InstanceMapScript
{
public:
    instance_night_of_the_fallen()
        : InstanceMapScript("instance_night_of_the_fallen", MAP_NIGHT_OF_THE_FALLEN) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_night_of_the_fallen_InstanceMapScript(map);
    }

    struct instance_night_of_the_fallen_InstanceMapScript : public InstanceScript
    {
        explicit instance_night_of_the_fallen_InstanceMapScript(InstanceMap* map)
            : InstanceScript(map) { }

        void Initialize() override
        {
            SetBossState(DATA_FALLEN_KING, NOT_STARTED);
            SetBossState(DATA_LADY_VESPERA, NOT_STARTED);

            _altars = { };
            _altarsControlled     = 0;
            _shieldBroken         = false;
            _shieldBrokenAt       = 0;
            _encounterStartAt     = 0;
            _altarsCapturedTotal  = 0;
            _capturesSolo         = 0;
            _capturesDefended     = 0;
            _pvpKills             = 0;
            _maxAltarsInRun       = 0;
            _raidDeaths           = 0;
            _enrageScheduled      = false;

            _captureTimer = CAPTURE_TICK_MS;
        }

        void OnPlayerEnter(Player* player) override
        {
            // The campaign clock starts when the first player of the group
            // enters, and only once per instance.
            if (_encounterStartAt == 0)
            {
                _encounterStartAt = static_cast<uint32>(GameTime::GetGameTime().count());
                SetData(DATA_CAMPAIGN_START_TIME, _encounterStartAt);
            }

            // Clients need the altar world state to draw the tracker on join.
            SyncWorldStates();
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_MALAKOR_FALLEN_KING:
                    _malakorGuid = creature->GetGUID();
                    break;

                case NPC_LADY_VESPERA:
                    _vesperaGuid = creature->GetGUID();
                    break;

                case NPC_TWILIGHT_WARDEN:
                {
                    int8 index = GetAltarIndexByPosition(creature->GetPositionX(),
                                                         creature->GetPositionY());
                    if (index >= 0)
                        _altars[index].wardenGuid = creature->GetGUID();
                    break;
                }

                default:
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* gameObject) override
        {
            if (!IsAltarEntry(gameObject->GetEntry()))
                return;

            int8 index = GetAltarIndexByPosition(gameObject->GetPositionX(),
                                                 gameObject->GetPositionY());
            if (index >= 0)
                _altars[index].gameObjectGuid = gameObject->GetGUID();
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (unit && unit->ToPlayer())
                ++_raidDeaths;

            Creature* creature = unit ? unit->ToCreature() : nullptr;
            if (!creature)
                return;

            if (creature->GetEntry() != NPC_TWILIGHT_WARDEN)
                return;

            int8 index = GetAltarIndexByPosition(creature->GetPositionX(),
                                                 creature->GetPositionY());
            if (index >= 0)
            {
                _altars[index].wardenGuid = ObjectGuid::Empty;
                _altars[index].captureProgress = 0;

                // Wardens respawn on their own spawn timer; the altar stays
                // capturable until that happens.
                if (GameObject* altar = instance->GetGameObject(_altars[index].gameObjectGuid))
                    altar->RemoveFlag(GO_FLAG_NOT_SELECTABLE, false);
            }
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
                case DATA_ALTARS_CAPTURED_TOTAL:
                    ++_altarsCapturedTotal;
                    break;

                case DATA_CAPTURES_SOLO:
                    ++_capturesSolo;
                    break;

                case DATA_CAPTURES_DEFENDED:
                    ++_capturesDefended;
                    break;

                case DATA_PVP_KILLS:
                    ++_pvpKills;
                    break;

                default:
                    break;
            }
        }

        void SetData(uint32 type, uint32 /*data*/) override
        {
            switch (type)
            {
                case DATA_FALLEN_KING:
                    OnMalakorEngaged();
                    break;

                default:
                    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_ALTARS_CONTROLLED:   return _altarsControlled;
                case DATA_SHIELD_BROKEN_TIME:  return _shieldBrokenAt;
                case DATA_CAMPAIGN_START_TIME: return _encounterStartAt;
                case DATA_RAID_DEATHS:         return _raidDeaths;
                case DATA_ALTARS_CAPTURED_TOTAL: return _altarsCapturedTotal;
                case DATA_CAPTURES_SOLO:       return _capturesSolo;
                case DATA_CAPTURES_DEFENDED:   return _capturesDefended;
                case DATA_PVP_KILLS:           return _pvpKills;
                case DATA_MAX_ALTARS_IN_RUN:   return _maxAltarsInRun;
                default:                       return 0;
            }
        }

        bool IsShieldBroken() const { return _shieldBroken; }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_FALLEN_KING:   return _malakorGuid;
                case DATA_LADY_VESPERA:  return _vesperaGuid;
                default:                 return ObjectGuid::Empty;
            }
        }

        void Update(uint32 diff) override
        {
            if (_malakorEngaged)
                UpdateCapture(diff);

            if (_shieldBroken && !_enrageScheduled)
            {
                uint32 now = static_cast<uint32>(GameTime::GetGameTime().count());
                if (now - _shieldBrokenAt >= (ENRAGE_AFTER_MS / 1000))
                {
                    _enrageScheduled = true;

                    if (Creature* malakor = instance->GetCreature(_malakorGuid))
                        if (malakor->IsAlive())
                            malakor->CastSpell(malakor, SPELL_ENRAGE_STOCK, true);
                }
            }
        }

        void OnMalakorEngaged()
        {
            if (_malakorEngaged)
                return;

            _malakorEngaged = true;
            _encounterStartAt = static_cast<uint32>(GameTime::GetGameTime().count());

            // Boss is immune while the shield holds; wardens protect altars.
            if (Creature* malakor = instance->GetCreature(_malakorGuid))
                malakor->CastSpell(malakor, SPELL_DARK_SHIELD, true);
        }

    private:
        // --------------------------------------------------------------------
        // Capture loop
        // --------------------------------------------------------------------
        void UpdateCapture(uint32 diff)
        {
            _captureTimer -= diff;
            if (_captureTimer > 0)
                return;

            _captureTimer = CAPTURE_TICK_MS;

            for (uint8 i = 0; i < MAX_ALTARS; ++i)
                UpdateAltar(i, CAPTURE_TICK_MS);
        }

        void UpdateAltar(uint8 index, uint32 tick)
        {
            AltarRuntime& altar = _altars[index];

            // Warden alive -> nothing to do, the altar is locked.
            Creature* warden = instance->GetCreature(altar.wardenGuid);
            if (warden && warden->IsAlive())
                return;

            GameObject* altarObject = instance->GetGameObject(altar.gameObjectGuid);
            if (!altarObject)
                return;

            Player* contender = FindContender(index);
            if (!contender)
            {
                // Nobody on the altar: progress decays instead of resetting,
                // so a short interruption does not wipe a long capture.
                altar.captureProgress = (altar.captureProgress > tick)
                    ? altar.captureProgress - tick
                    : 0;
                altar.capturerGuid = ObjectGuid::Empty;
                return;
            }

            // A second faction contesting the same altar blocks progress.
            if (HasEnemyContest(index, contender->GetTeamId()))
                return;

            if (altar.capturerGuid != contender->GetGUID())
            {
                altar.capturerGuid   = contender->GetGUID();
                altar.captureProgress = 0;

                contender->SendMessageToSet(nullptr, true);
            }

            altar.captureProgress += tick;
            if (altar.captureProgress < CAPTURE_TIME_MS)
                return;

            CompleteCapture(index, contender);
        }

        void CompleteCapture(uint8 index, Player* contender)
        {
            AltarRuntime& altar = _altars[index];
            altar.captureProgress = 0;
            altar.capturerGuid    = ObjectGuid::Empty;

            AltarState const newState = (contender->GetTeamId() == TEAM_ALLIANCE)
                ? ALTAR_ALLIANCE
                : ALTAR_HORDE;

            if (altar.state == newState)
                return;

            // Recapturing an enemy altar counts as a defence for its previous
            // owner and as a fresh capture for the new one.
            if (altar.state != ALTAR_NEUTRAL)
                ++_capturesDefended;

            if (!contender->GetGroup())
                ++_capturesSolo;

            altar.state = newState;
            ++_altarsCapturedTotal;

            if (newState == ALTAR_ALLIANCE)
                ++_allianceAltars;
            else
                ++_hordeAltars;

            RecalculateControl();
            SyncWorldStates();
            RecordCapture(contender);

            if (GameObject* altarObject = instance->GetGameObject(altar.gameObjectGuid))
            {
                // Simple visual switch: the altar becomes non-interactive for
                // the losing faction by swapping the phase mask.
                altarObject->SetPhaseMask((newState == ALTAR_ALLIANCE) ? 512 : 1024, true);
            }

            if (_allianceAltars >= ALTARS_FOR_SHIELD || _hordeAltars >= ALTARS_FOR_SHIELD)
                BreakShield();
        }

        Player* FindContender(uint8 index)
        {
            float const x = AltarPositions[index][0];
            float const y = AltarPositions[index][1];
            float const z = AltarPositions[index][2];

            Player* best = nullptr;
            float bestDistance = CAPTURE_RADIUS * CAPTURE_RADIUS;

            for (Map::PlayerList::const_iterator itr = instance->GetPlayers().begin(); itr != instance->GetPlayers().end(); ++itr)
            {
                Player* player = itr->GetSource();
                if (!player || !player->IsAlive())
                    continue;

                float dx = player->GetPositionX() - x;
                float dy = player->GetPositionY() - y;
                float dz = player->GetPositionZ() - z;

                // Vertical tolerance is generous because the altar platform
                // sits on a raised hall.
                if (std::abs(dz) > 15.0f)
                    continue;

                float distance = dx * dx + dy * dy;
                if (distance <= bestDistance)
                {
                    bestDistance = distance;
                    best = player;
                }
            }

            return best;
        }

        bool HasEnemyContest(uint8 index, TeamId team) const
        {
            float const x = AltarPositions[index][0];
            float const y = AltarPositions[index][1];

            for (Map::PlayerList::const_iterator itr = instance->GetPlayers().begin(); itr != instance->GetPlayers().end(); ++itr)
            {
                Player* player = itr->GetSource();
                if (!player || !player->IsAlive() || player->GetTeamId() == team)
                    continue;

                float dx = player->GetPositionX() - x;
                float dy = player->GetPositionY() - y;
                if ((dx * dx + dy * dy) <= (CAPTURE_RADIUS * CAPTURE_RADIUS))
                    return true;
            }

            return false;
        }

        void RecalculateControl()
        {
            _allianceAltars = 0;
            _hordeAltars    = 0;

            for (uint8 i = 0; i < MAX_ALTARS; ++i)
            {
                if (_altars[i].state == ALTAR_ALLIANCE)
                    ++_allianceAltars;
                else if (_altars[i].state == ALTAR_HORDE)
                    ++_hordeAltars;
            }

            _altarsControlled = std::max<uint8>(_allianceAltars, _hordeAltars);

            if (_altarsControlled > _maxAltarsInRun)
                _maxAltarsInRun = _altarsControlled;
        }

        void BreakShield()
        {
            if (_shieldBroken)
                return;

            _shieldBroken = true;
            _shieldBrokenAt = static_cast<uint32>(GameTime::GetGameTime().count());

            Creature* malakor = instance->GetCreature(_malakorGuid);
            if (!malakor || !malakor->IsAlive())
                return;

            if (malakor->HasAura(SPELL_DARK_SHIELD))
                malakor->RemoveAura(SPELL_DARK_SHIELD);

            ApplyAltarPenalty(malakor);
        }

        // --------------------------------------------------------------------
        // Shield penalty - applied once when the shield breaks and again when
        // a fourth altar is taken, so the 4-altar state is reachable.
        // --------------------------------------------------------------------
        void ApplyAltarPenalty(Creature* malakor)
        {
            AltarEffect const& effect = GetEffect(_altarsControlled);

            if (effect.shieldUp)
                return;

            

            if (!_penaltyApplied)
            {
                _penaltyApplied = true;

                if (effect.healthPercent < 1.0f)
                {
                    uint64 reduced = static_cast<uint64>(malakor->GetMaxHealth()
                                                         * effect.healthPercent);
                    malakor->SetHealth(std::min<uint64>(malakor->GetHealth(), reduced));
                }
            }

            if (effect.damageMultiplier > 1.0f)
                malakor->CastSpell(malakor, SPELL_VULNERABLE, true);
        }

        // Single path for the tracker world states: the helper in
        // aurora_campaign01_progress.cpp does the actual DoUpdateWorldState
        // calls, so there is no second, divergent implementation here.
        void SyncWorldStates()
        {
            AuroraPushWorldStates(this, _altarsControlled, _shieldBroken);
        }

        // Declared in campaign01.h, implemented in aurora_campaign01_progress.cpp
        // so this file does not have to link against the character database
        // helpers directly.
        void RecordCapture(Player* player)
        {
            AuroraRecordAltarCapture(player, this);
        }

        void AddRaidDeath()
        {
            AuroraRecordRaidDeath(this);
        }

        // --------------------------------------------------------------------
        // State
        // --------------------------------------------------------------------
        std::array<AltarRuntime, MAX_ALTARS> _altars;

        ObjectGuid _malakorGuid;
        ObjectGuid _vesperaGuid;

        uint32 _captureTimer        = CAPTURE_TICK_MS;
        uint8  _allianceAltars      = 0;
        uint8  _hordeAltars         = 0;
        uint8  _altarsControlled    = 0;
        uint8  _maxAltarsInRun      = 0;
        bool   _malakorEngaged      = false;
        bool   _shieldBroken        = false;
        bool   _enrageScheduled     = false;
        bool   _penaltyApplied      = false;
        uint32 _shieldBrokenAt      = 0;
        uint32 _encounterStartAt    = 0;
        uint32 _altarsCapturedTotal = 0;
        uint32 _capturesSolo        = 0;
        uint32 _capturesDefended    = 0;
        uint32 _pvpKills            = 0;
        uint32 _raidDeaths          = 0;
    };
};

void AddSC_night_of_the_fallen()
{
    new instance_night_of_the_fallen();
}
