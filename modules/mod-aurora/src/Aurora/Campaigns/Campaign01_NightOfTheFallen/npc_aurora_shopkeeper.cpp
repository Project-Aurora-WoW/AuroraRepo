// ============================================================================
// PROJECT AURORA v7.1.0
// npc_aurora_shopkeeper.cpp - Aurora shop keeper gossip window.
//
// The vendor list in `npc_vendor` shows the catalogue, but prices there are
// zero: the real check happens server-side in `.shop buy`. This script turns
// the gossip window into a readable price list so a player sees what a purchase
// actually costs before issuing the command.
//
// Wording comes from `aurora_gossip_texts` (actions: 'shop', 'leave').
// ============================================================================

#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include "GossipDef.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "StringFormat.h"

#include "aurora_shared.h"

#include <mutex>
#include <unordered_map>
#include <vector>

using namespace Aurora;

void AuroraLoadGossipTexts();

namespace
{
    constexpr uint32 GOSSIP_TEXT_SHOPKEEPER = 7004;
    constexpr uint32 ACTION_SHOW_CATALOGUE   = GOSSIP_ACTION_INFO_DEF + 1;
    constexpr uint32 ACTION_LEAVE            = GOSSIP_ACTION_INFO_DEF + 2;

    struct ShopEntryView
    {
        uint32      id          = 0;
        uint32      itemEntry   = 0;
        uint32      itemAmount  = 1;
        uint32      costShards  = 0;
        uint32      costAurora  = 0;
        uint32      costGold    = 0;
        std::string nameRu;
        std::string nameEn;
    };

    struct MenuItem
    {
        std::string action;
        std::string textRu;
        std::string textEn;
    };

    std::unordered_map<uint32, std::vector<MenuItem>> sShopGossipCache;
    std::mutex sShopGossipMutex;

    bool IsRussianLocale(Player const* player)
    {
        if (!player || !player->GetSession())
            return true;

        return player->GetSession()->GetSessionDbcLocale() == LOCALE_ruRU;
    }

    uint32 ActionFromString(std::string const& action)
    {
        return action == "shop" ? ACTION_SHOW_CATALOGUE : ACTION_LEAVE;
    }

    std::string const& Label(Player* player, MenuItem const& item)
    {
        return IsRussianLocale(player) ? item.textRu : item.textEn;
    }

    std::vector<MenuItem> DefaultItems()
    {
        return {
            { "shop",  "Покажи товары.",   "Show me your wares." },
            { "leave", "Уйти.",            "Leave." },
        };
    }

    // Price listing. The `.shop buy <id>` command is the only way to actually
    // purchase, so every line leads with the offer id the player must type.
    void SendCatalogue(Player* player)
    {
        ChatHandler handler(player->GetSession());
        bool const ru = IsRussianLocale(player);

        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `item_entry`, `item_amount`, `cost_shards`, `cost_aurora`, "
            "`cost_gold`, `name_ru`, `name_en` "
            "FROM `aurora_shop_items` WHERE `enabled` = 1 ORDER BY `category`, `id`");

        if (!result)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                ru ? "Витрина пуста." : "The shop has no offers.");
            return;
        }

        handler.PSendSysMessage("|cffFFD100=== %s ===|r",
            ru ? "МАГАЗИН АВРОРЫ" : "AURORA SHOP");

        do
        {
            Field* f = result->Fetch();
            ShopEntryView e;
            e.id         = f[0].Get<uint32>();
            e.itemEntry  = f[1].Get<uint32>();
            e.itemAmount = f[2].Get<uint32>();
            e.costShards = f[3].Get<uint32>();
            e.costAurora = f[4].Get<uint32>();
            e.costGold   = f[5].Get<uint64>();
            e.nameRu     = f[6].Get<std::string>();
            e.nameEn     = f[7].Get<std::string>();

            std::string const& name = ru ? e.nameRu : e.nameEn;

            // Build the price in the order a player thinks about it.
            std::string price;
            if (e.costShards)
                price += Acore::StringFormat("{} {}", e.costShards,
                         ru ? "осколков" : "shards");
            if (e.costAurora)
            {
                if (!price.empty())
                    price += ", ";
                price += Acore::StringFormat("{} {}", e.costAurora,
                         ru ? "очков Авроры" : "Aurora points");
            }
            if (e.costGold)
            {
                if (!price.empty())
                    price += ", ";
                price += Acore::StringFormat("{}g {}s",
                         e.costGold / 10000, (e.costGold % 10000) / 100);
            }
            if (price.empty())
                price = ru ? "бесплатно" : "free";

            if (e.itemAmount > 1)
                handler.PSendSysMessage("|cffffff00#{}|r {} x{} |cff888888({})|r",
                                        e.id, name.c_str(), e.itemAmount, price.c_str());
            else
                handler.PSendSysMessage("|cffffff00#{}|r {} |cff888888({})|r",
                                        e.id, name.c_str(), price.c_str());
        } while (result->NextRow());

        handler.PSendSysMessage("|cff888888%s|r",
            ru ? "Купить: .shop buy <номер>" : "To buy: .shop buy <id>");
    }
}

class npc_aurora_shopkeeper : public CreatureScript
{
public:
    npc_aurora_shopkeeper() : CreatureScript("npc_aurora_shopkeeper") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        // Keep the native vendor window reachable: it lists the same items.
        if (creature->IsVendor())
            player->GetSession()->SendListInventory(creature->GetGUID());

        std::vector<MenuItem> items;
        {
            std::lock_guard<std::mutex> guard(sShopGossipMutex);
            auto itr = sShopGossipCache.find(creature->GetEntry());
            if (itr != sShopGossipCache.end())
                items = itr->second;
        }

        if (items.empty())
            items = DefaultItems();

        for (MenuItem const& item : items)
        {
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, Label(player, item).c_str(),
                             GOSSIP_SENDER_MAIN, ActionFromString(item.action));
        }

        SendGossipMenuFor(player, GOSSIP_TEXT_SHOPKEEPER, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);

        if (action == ACTION_SHOW_CATALOGUE)
        {
            SendCatalogue(player);
            CloseGossipMenuFor(player);
            return true;
        }

        CloseGossipMenuFor(player);
        return true;
    }
};

void AddSC_npc_aurora_shopkeeper()
{
    new npc_aurora_shopkeeper();
}
