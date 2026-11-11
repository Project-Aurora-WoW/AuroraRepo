// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// npc_campaign01_questgivers.cpp - the three quest chain givers.
//
// Quest lines live in the world DB (creature_queststarter / creature_questender).
// This script only renders the greeting and offers the chain entry as a gossip
// action, so a player who already finished the chain is not shown the hint.
// ============================================================================

#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include "GossipDef.h"
#include "QuestDef.h"

#include "campaign01.h"

using namespace Aurora;

namespace
{
    // First quest of each chain. The rest follow through RewardNextQuest.
    constexpr uint32 QUEST_CHAIN_ALLIANCE_FIRST = 90001;
    constexpr uint32 QUEST_CHAIN_HORDE_FIRST    = 90011;
    constexpr uint32 QUEST_CHAIN_NEUTRAL_FIRST  = 90021;

    // Gossip text ids from `npc_text` (project range 7000+).
    constexpr uint32 GOSSIP_TEXT_LYONDREL = 7001;
    constexpr uint32 GOSSIP_TEXT_WARGOK   = 7002;
    constexpr uint32 GOSSIP_TEXT_EONIS    = 7003;

    constexpr uint32 ACTION_OPEN_CHAIN = GOSSIP_ACTION_INFO_DEF + 1;
    constexpr uint32 ACTION_LEAVE      = GOSSIP_ACTION_INFO_DEF + 2;
}

// ---------------------------------------------------------------------------
// Alliance giver
// ---------------------------------------------------------------------------
class npc_lyondrel_questgiver : public CreatureScript
{
public:
    npc_lyondrel_questgiver() : CreatureScript("npc_lyondrel_questgiver") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (!player->GetQuestRewardStatus(QUEST_CHAIN_ALLIANCE_FIRST))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Расскажи о Ночи Падших.",
                             GOSSIP_SENDER_MAIN, ACTION_OPEN_CHAIN);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Прощай.", GOSSIP_SENDER_MAIN, ACTION_LEAVE);

        SendGossipMenuFor(player, GOSSIP_TEXT_LYONDREL, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);

        if (action == ACTION_OPEN_CHAIN)
            CloseGossipMenuFor(player);
        else
            CloseGossipMenuFor(player);

        return true;
    }
};

// ---------------------------------------------------------------------------
// Horde giver
// ---------------------------------------------------------------------------
class npc_wargok_questgiver : public CreatureScript
{
public:
    npc_wargok_questgiver() : CreatureScript("npc_wargok_questgiver") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (!player->GetQuestRewardStatus(QUEST_CHAIN_HORDE_FIRST))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Ночь ждёт. Идём.",
                             GOSSIP_SENDER_MAIN, ACTION_OPEN_CHAIN);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Прощай.", GOSSIP_SENDER_MAIN, ACTION_LEAVE);

        SendGossipMenuFor(player, GOSSIP_TEXT_WARGOK, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        CloseGossipMenuFor(player);
        return true;
    }
};

// ---------------------------------------------------------------------------
// Neutral giver
// ---------------------------------------------------------------------------
class npc_eonis_questgiver : public CreatureScript
{
public:
    npc_eonis_questgiver() : CreatureScript("npc_eonis_questgiver") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (!player->GetQuestRewardStatus(QUEST_CHAIN_NEUTRAL_FIRST))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Равновесие зовёт.",
                             GOSSIP_SENDER_MAIN, ACTION_OPEN_CHAIN);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Прощай.", GOSSIP_SENDER_MAIN, ACTION_LEAVE);

        SendGossipMenuFor(player, GOSSIP_TEXT_EONIS, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        CloseGossipMenuFor(player);
        return true;
    }
};

void AddSC_npc_campaign01_questgivers()
{
    new npc_lyondrel_questgiver();
    new npc_wargok_questgiver();
    new npc_eonis_questgiver();
}
