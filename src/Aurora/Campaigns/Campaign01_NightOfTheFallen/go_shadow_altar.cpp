// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// go_shadow_altar.cpp - Altar of Twilight.
//
// The capture logic itself lives in the InstanceScript, which polls player
// positions once per second. The GameObject only provides the interaction
// surface and readable feedback, so two nearly identical capture
// implementations cannot disagree.
// ============================================================================

#include "ScriptMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "Creature.h"
#include "Chat.h"
#include "InstanceScript.h"

#include "campaign01.h"

using namespace Aurora;
using namespace Aurora::Campaign01;

namespace
{
    char const* TextFor(Player* player, char const* ru, char const* en)
    {
        std::string locale = player->GetSession()->GetSessionDbcLocale();
        bool const russian = (locale != "enUS" && locale != "enGB");
        return russian ? ru : en;
    }
}

class go_shadow_altar : public GameObjectScript
{
public:
    go_shadow_altar() : GameObjectScript("go_shadow_altar") { }

    struct go_shadow_altarAI : public GameObjectAI
    {
        explicit go_shadow_altarAI(GameObject* gameObject) : GameObjectAI(gameObject) { }

        bool GossipHello(Player* player) override
        {
            if (me->GetMapId() != MAP_NIGHT_OF_THE_FALLEN)
                return false;

            int8 index = GetAltarIndexByPosition(me->GetPositionX(), me->GetPositionY());
            if (index < 0)
                return false;

            InstanceScript* instance = me->GetInstanceScript();
            if (!instance)
                return false;

            // Wardens must die first: report the lock instead of a silent fail.
            if (Creature* warden = FindWarden())
            {
                if (warden->IsAlive())
                {
                    ChatHandler(player->GetSession()).SendSysMessage(
                        TextFor(player,
                                "|cffff0000[Aurora]|r Страж Сумерек ещё жив — алтарь запечатан.",
                                "|cffff0000[Aurora]|r The Twilight Warden still lives - the altar is sealed."));
                    return true;
                }
            }

            uint32 controlled = instance->GetData(DATA_ALTARS_CONTROLLED);

            ChatHandler(player->GetSession()).PSendSysMessage(
                TextFor(player,
                        "|cffFFD100[Aurora]|r Алтарь свободен. Захвачено алтарей: %u из %u.",
                        "|cffFFD100[Aurora]|r The altar is open. Altars controlled: %u of %u."),
                controlled, MAX_ALTARS);

            ChatHandler(player->GetSession()).SendSysMessage(
                TextFor(player,
                        "Встаньте на алтарь и удерживайте позицию 10 секунд.",
                        "Stand on the altar and hold the position for 10 seconds."));

            return true;
        }

    private:
        Creature* FindWarden()
        {
            float x = me->GetPositionX();
            float y = me->GetPositionY();

            std::list<Creature*> wardens;
            me->GetCreatureListWithEntryInGrid(wardens, NPC_TWILIGHT_WARDEN, 30.0f);

            Creature* nearest = nullptr;
            float bestDistance = 900.0f;

            for (Creature* warden : wardens)
            {
                if (!warden)
                    continue;

                float dx = warden->GetPositionX() - x;
                float dy = warden->GetPositionY() - y;
                float distance = dx * dx + dy * dy;

                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    nearest = warden;
                }
            }

            return nearest;
        }
    };

    GameObjectAI* GetAI(GameObject* gameObject) const override
    {
        return new go_shadow_altarAI(gameObject);
    }
};

void AddSC_go_shadow_altar()
{
    new go_shadow_altar();
}
