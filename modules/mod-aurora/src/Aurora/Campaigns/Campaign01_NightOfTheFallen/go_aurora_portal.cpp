// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1
// go_aurora_portal.cpp - hub portal into "Night of the Fallen".
//
// Behaviour
//   * Right-click opens a gossip window with the campaign status and an
//     "Enter" entry.
//   * Entry is gated by one hour of playtime (Activity-First rule), which is
//     read from `aurora_player_activity` - the same counter the voting system
//     uses, so the requirement can never disagree between features.
//   * The target map and coordinates come from `aurora_campaign_locations`,
//     not from the gossip menu list, so pointing a portal at another campaign
//     is a data change.
//   * A player already inside the campaign gets a "leave" option instead of
//     being teleported onto themselves.
// ============================================================================

#include "ScriptMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Language.h"
#include "MapMgr.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    struct PortalTarget
    {
        uint32 campaignId = 0;
        int32  mapId      = -1;
        float  x = 0.0f;
        float  y = 0.0f;
        float  z = 0.0f;
        float  o = 0.0f;
        std::string hubZone;
        std::string nameRu;
        std::string nameEn;
    };

    bool ResolvePortal(uint32 portalEntry, PortalTarget& out)
    {
        QueryResult result = WorldDatabase.Query(
            "SELECT l.`campaign_id`, l.`map_id`, l.`hub_x`, l.`hub_y`, l.`hub_z`, "
            "       l.`hub_o`, l.`hub_zone`, c.`display_name_ru`, c.`display_name_en` "
            "FROM `aurora_campaign_locations` l "
            "JOIN `aurora_campaigns` c ON c.`id` = l.`campaign_id` "
            "WHERE l.`portal_entry` = {} LIMIT 1", portalEntry);

        if (!result)
            return false;

        Field* fields = result->Fetch();
        out.campaignId = fields[0].Get<uint32>();
        out.mapId      = static_cast<int32>(fields[1].Get<uint32>());
        out.x          = fields[2].Get<float>();
        out.y          = fields[3].Get<float>();
        out.z          = fields[4].Get<float>();
        out.o          = fields[5].Get<float>();
        out.hubZone    = fields[6].Get<std::string>();
        out.nameRu     = fields[7].Get<std::string>();
        out.nameEn     = fields[8].Get<std::string>();
        return true;
    }

    uint64 GetPlaytime(uint32 guid)
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT `total_playtime_sec` FROM `aurora_player_activity` WHERE `player_guid` = {}",
            guid);

        if (!result)
            return 0;

        return result->Fetch()[0].Get<uint64>();
    }

    bool IsRussian(Player const* player)
    {
        return player->GetSession()->GetSessionDbcLocale() == LOCALE_ruRU;
    }
}

class go_aurora_portal : public GameObjectScript
{
public:
    go_aurora_portal() : GameObjectScript("go_aurora_portal") { }

    struct go_aurora_portalAI : public GameObjectAI
    {
        explicit go_aurora_portalAI(GameObject* go) : GameObjectAI(go) { }

        bool GossipHello(Player* player, bool /*reportUse*/) override
        {
            PortalTarget target;
            if (!ResolvePortal(me->GetEntry(), target))
            {
                ChatHandler(player->GetSession()).SendSysMessage(
                    "|cffff0000[Aurora]|r Портал не привязан к кампании.");
                return true;
            }

            bool const russian = IsRussian(player);
            ChatHandler handler(player->GetSession());

            handler.PSendSysMessage("|cffFFD100=== %s ===|r",
                russian ? target.nameRu.c_str() : target.nameEn.c_str());
            handler.PSendSysMessage("|cff888888Хаб: %s|r", target.hubZone.c_str());

            // Already inside the campaign instance: offer the way back.
            if (static_cast<int32>(player->GetMapId()) == target.mapId)
            {
                handler.SendSysMessage(
                    "Вы уже внутри кампании. Возврат в хаб — через камень телепортации.");
                return true;
            }

            uint64 const playtime = GetPlaytime(player->GetGUID().GetCounter());
            if (playtime < AURORA_MIN_PLAYTIME_SEC)
            {
                uint64 const left = (AURORA_MIN_PLAYTIME_SEC - playtime) / 60;
                handler.PSendSysMessage(
                    "|cffff0000[Aurora]|r Для входа нужно 1 час игры. Осталось %u мин.", left);
                return true;
            }

            player->PlayerTalkClass->ClearMenus();
            player->PlayerTalkClass->SendGossipMenu(907, me->GetGUID());
            return true;
        }

        bool GossipSelect(Player* player, uint32 /*sender*/, uint32 /*action*/) override
        {
            PortalTarget target;
            if (!ResolvePortal(me->GetEntry(), target))
                return true;

            if (target.mapId < 0)
            {
                ChatHandler(player->GetSession()).SendSysMessage(
                    "|cffff0000[Aurora]|r Карта кампании не загружена на сервере.");
                LOG_ERROR("module", "[Aurora] Portal {} points to map {} which is not loaded.",
                          me->GetEntry(), target.mapId);
                return true;
            }

            player->TeleportTo(static_cast<uint32>(target.mapId), target.x, target.y,
                               target.z, target.o);
            player->PlayerTalkClass->SendCloseGossip();

            ChatHandler(player->GetSession()).SendSysMessage(
                "|cff00ff00[Aurora]|r Добро пожаловать в кампанию.");
            return true;
        }
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_aurora_portalAI(go);
    }
};

void AddSC_go_aurora_portal()
{
    new go_aurora_portal();
}
