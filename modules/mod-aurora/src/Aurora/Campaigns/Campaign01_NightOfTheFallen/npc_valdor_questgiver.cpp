// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// npc_valdor_questgiver.cpp - Archmage Valdor, keeper of the gate.
//
// Valdor is the crossroads of the campaign: both the Alliance and the Horde
// chains pass through him (quests 90001 / 90011 point at him), and he is the
// NPC who hands out the instance entrance explanation.
//
// Menu item wording lives in `aurora_gossip_texts` (action = 'chain'/'leave'),
// so it can be edited without a rebuild: `.aurora reload gossip`.
// ============================================================================

#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include "GossipDef.h"
#include "QuestDef.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "StringFormat.h"

#include "campaign01.h"

#include <mutex>
#include <unordered_map>
#include <vector>

using namespace Aurora;

void AuroraLoadGossipTexts();

namespace
{
    // Quests that Valdor completes / advances.
    constexpr uint32 QUEST_ALLIANCE_ARRIVAL = 90001;
    constexpr uint32 QUEST_HORDE_ARRIVAL    = 90011;

    // Greeting text id from `npc_text` (project range 7000+).
    constexpr uint32 GOSSIP_TEXT_VALDOR = 7004;

    constexpr uint32 ACTION_OPEN_QUESTS = GOSSIP_ACTION_INFO_DEF + 1;
    constexpr uint32 ACTION_LEAVE       = GOSSIP_ACTION_INFO_DEF + 2;

    // ------------------------------------------------------------------------
    // Per-NPC menu text cache, shared with the other campaign givers.
    // ------------------------------------------------------------------------
    struct GossipText
    {
        std::string action;
        std::string textRu;
        std::string textEn;
        uint8       icon;
    };

    std::unordered_map<uint32, std::vector<GossipText>> sValdorCache;
    std::mutex sValdorMutex;

    bool IsRussianLocale(Player const* player)
    {
        if (!player || !player->GetSession())
            return true;

        return player->GetSession()->GetSessionDbcLocale() == LOCALE_ruRU;
    }

    uint32 ActionFromString(std::string const& action)
    {
        if (action == "chain")
            return ACTION_OPEN_QUESTS;

        return ACTION_LEAVE;
    }

    std::string const& Label(Player* player, GossipText const& item)
    {
        return IsRussianLocale(player) ? item.textRu : item.textEn;
    }

    // Fallback wording used when `aurora_gossip_texts` has no row for Valdor,
    // so the NPC still shows a usable menu on a fresh database.
    std::vector<GossipText> DefaultItems()
    {
        return {
            { "chain", "Проводи меня в Ночь Падших.", "Send me into the Night of the Fallen.", 0 },
            { "leave", "Пока что нет.",              "Not yet.",                          0 },
        };
    }
}

class npc_valdor_questgiver : public CreatureScript
{
public:
    npc_valdor_questgiver() : CreatureScript("npc_valdor_questgiver") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        // Whether the player still needs the gate explained: they are on the
        // active chain and have not finished the arrival step yet.
        bool const onAlliance = player->GetQuestStatus(QUEST_ALLIANCE_ARRIVAL) != QUEST_STATUS_NONE;
        bool const onHorde    = player->GetQuestStatus(QUEST_HORDE_ARRIVAL) != QUEST_STATUS_NONE;

        std::vector<GossipText> items;
        {
            std::lock_guard<std::mutex> guard(sValdorMutex);
            auto itr = sValdorCache.find(creature->GetEntry());
            if (itr != sValdorCache.end())
                items = itr->second;
        }

        if (items.empty())
            items = DefaultItems();

        for (GossipText const& item : items)
        {
            // The pointer to the gate is only offered while the player is on
            // one of the two arrival quests.
            if (item.action == "chain" && !onAlliance && !onHorde)
                continue;

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, Label(player, item).c_str(),
                             GOSSIP_SENDER_MAIN, ActionFromString(item.action));
        }

        SendGossipMenuFor(player, GOSSIP_TEXT_VALDOR, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);

        if (action == ACTION_OPEN_QUESTS)
        {
            // Rebuilding the quest menu is what actually opens the quest list.
            player->PrepareQuestMenu(creature->GetGUID());
            SendGossipMenuFor(player, GOSSIP_TEXT_VALDOR, creature->GetGUID());
            return true;
        }

        CloseGossipMenuFor(player);
        return true;
    }
};

void AddSC_npc_valdor_questgiver()
{
    new npc_valdor_questgiver();
}
