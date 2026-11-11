// ============================================================================
// PROJECT AURORA v7.1.0 - Campaign #1 "Night of the Fallen"
// npc_campaign01_questgivers.cpp - the three quest chain givers.
//
// Quest lines live in the world DB (creature_queststarter / creature_questender).
// This script renders the greeting, offers the chain entry, and on selection
// actually opens the quest list instead of just closing the window.
//
// Menu item texts come from `aurora_gossip_texts`, so wording can be changed
// without rebuilding the server. The table is cached and reloaded when the
// world starts; `.aurora reload gossip` refreshes it live.
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

    // ------------------------------------------------------------------------
    // Gossip text cache: creature_entry -> ordered list of menu items.
    // Read once from `aurora_gossip_texts` and refreshed on demand.
    // ------------------------------------------------------------------------
    struct GossipText
    {
        std::string action;
        std::string textRu;
        std::string textEn;
        uint8       icon;
    };

    std::unordered_map<uint32, std::vector<GossipText>> sGossipCache;
    std::mutex sGossipMutex;
    bool sGossipLoaded = false;

    bool IsRussianLocale(Player const* player)
    {
        if (!player || !player->GetSession())
            return true;

        return player->GetSession()->GetSessionDbcLocale() == LOCALE_ruRU;
    }

    // Action ids are stable: the row order defines which action the client
    // sends back, so 'chain' and 'leave' map onto fixed constants.
    uint32 ActionFromString(std::string const& action)
    {
        if (action == "chain")
            return ACTION_OPEN_CHAIN;

        return ACTION_LEAVE;
    }

    std::string const& GossipLabel(Player* player, GossipText const& item)
    {
        return IsRussianLocale(player) ? item.textRu : item.textEn;
    }

    // ------------------------------------------------------------------------
    // Shared gossip handling for all three chain givers.
    // ------------------------------------------------------------------------
    void BuildGossipMenu(Player* player, Creature* creature, uint32 greetingTextId,
                         uint32 firstQuestId)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        std::vector<GossipText> items;
        {
            std::lock_guard<std::mutex> guard(sGossipMutex);
            auto itr = sGossipCache.find(creature->GetEntry());
            if (itr != sGossipCache.end())
                items = itr->second;
        }

        bool chainFinished = player->GetQuestRewardStatus(firstQuestId);

        for (GossipText const& item : items)
        {
            // A player who already completed the chain is not offered it again.
            if (item.action == "chain" && chainFinished)
                continue;

            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                             GossipLabel(player, item).c_str(),
                             GOSSIP_SENDER_MAIN, ActionFromString(item.action));
        }

        SendGossipMenuFor(player, greetingTextId, creature->GetGUID());
    }

    bool HandleGossipSelect(Player* player, Creature* creature, uint32 action,
                            uint32 greetingTextId, uint32 firstQuestId)
    {
        ClearGossipMenuFor(player);

        if (action == ACTION_OPEN_CHAIN)
        {
            // Rebuilding PrepareQuestMenu is what attaches the NPC's quests to
            // the gossip menu, so resending the menu now shows the quest list.
            // The old code only closed the window and never opened anything.
            BuildGossipMenu(player, creature, greetingTextId, firstQuestId);
            return true;
        }

        CloseGossipMenuFor(player);
        return true;
    }
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
        BuildGossipMenu(player, creature, GOSSIP_TEXT_LYONDREL, QUEST_CHAIN_ALLIANCE_FIRST);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        return HandleGossipSelect(player, creature, action, GOSSIP_TEXT_LYONDREL, QUEST_CHAIN_ALLIANCE_FIRST);
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
        BuildGossipMenu(player, creature, GOSSIP_TEXT_WARGOK, QUEST_CHAIN_HORDE_FIRST);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        return HandleGossipSelect(player, creature, action, GOSSIP_TEXT_WARGOK, QUEST_CHAIN_HORDE_FIRST);
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
        BuildGossipMenu(player, creature, GOSSIP_TEXT_EONIS, QUEST_CHAIN_NEUTRAL_FIRST);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        return HandleGossipSelect(player, creature, action, GOSSIP_TEXT_EONIS, QUEST_CHAIN_NEUTRAL_FIRST);
    }
};

// ---------------------------------------------------------------------------
// Loader - called from the bootstrap WorldScript and from `.aurora reload`.
// ---------------------------------------------------------------------------
void AuroraLoadGossipTexts()
{
    std::unordered_map<uint32, std::vector<GossipText>> loaded;

    QueryResult result = WorldDatabase.Query(
        "SELECT `creature_entry`, `action`, `text_ru`, `text_en`, `icon` "
        "FROM `aurora_gossip_texts` WHERE `enabled` = 1 "
        "ORDER BY `creature_entry` ASC, `sort_order` ASC, `id` ASC");

    if (!result)
    {
        LOG_WARN("module", "[Aurora] `aurora_gossip_texts` is empty - quest givers "
                          "will show only the default quest menu.");
    }
    else
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 entry  = fields[0].Get<uint32>();

            GossipText item;
            item.action = fields[1].Get<std::string>();
            item.textRu = fields[2].Get<std::string>();
            item.textEn = fields[3].Get<std::string>();
            item.icon   = fields[4].Get<uint8>();

            loaded[entry].push_back(std::move(item));
        } while (result->NextRow());
    }

    size_t total = 0;
    for (auto const& pair : loaded)
        total += pair.second.size();

    {
        std::lock_guard<std::mutex> guard(sGossipMutex);
        sGossipCache.swap(loaded);
        sGossipLoaded = true;
    }

    LOG_INFO("module", "[Aurora] Gossip texts loaded: {} entries for {} creatures.",
             total, sGossipCache.size());
}

void AddSC_npc_campaign01_questgivers()
{
    new npc_lyondrel_questgiver();
    new npc_wargok_questgiver();
    new npc_eonis_questgiver();
}
