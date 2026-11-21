// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_item_sets.cpp - campaign item set bonuses, server side.
//
// Why the server does this:
//   Set bonuses in 3.3.5a normally live in the client DBC (ItemSet.dbc), and
//   the core reads them from `item_template`.`setID`. Neither is available
//   here: the DBC is a client file that is not in this repository, and this
//   world schema has no `setID` column. The bonuses are therefore applied by
//   the server: equipped pieces are counted and a passive aura is added or
//   removed. Only the client tooltip line is missing; the effect is real.
//
// Membership comes from `aurora_item_set_items` (2026_11_10_13.sql), so a
// designer can retune a set without a rebuild or a core change.
//
// The hook is `PlayerScript::OnPlayerEquip`, checked against the AzerothCore
// script API as used by mod-ale (ALE_SC.cpp):
//   void OnPlayerEquip(Player* player, Item* it, uint8 bag, uint8 slot,
//                      bool update) override
// It fires for both equipping and unequipping, so the bonus is re-evaluated
// the moment the equipment changes.
// ============================================================================

#include <algorithm>
#include <array>
#include <vector>

#include "ScriptMgr.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "SpellMgr.h"
#include "StringFormat.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    constexpr uint8 MAX_BONUSES = 3;

    struct SetBonus
    {
        uint32 pieces = 0;
        uint32 spell  = 0;
    };

    struct ItemSetDefinition
    {
        uint32      id = 0;
        std::string nameRu;
        std::string nameEn;
        std::vector<uint32>               items;
        std::array<SetBonus, MAX_BONUSES> bonuses{ };
    };

    std::vector<ItemSetDefinition> g_sets;

    void LoadSets()
    {
        g_sets.clear();

        QueryResult sets = WorldDatabase.Query(
            "SELECT `id`, `name_ru`, `name_en` FROM `aurora_item_sets` ORDER BY `id`");

        if (!sets)
        {
            LOG_WARN("module", "[Aurora] `aurora_item_sets` is empty - set bonuses are off.");
            return;
        }

        do
        {
            Field* fields = sets->Fetch();

            ItemSetDefinition def;
            def.id     = fields[0].Get<uint32>();
            def.nameRu = fields[1].Get<std::string>();
            def.nameEn = fields[2].Get<std::string>();

            g_sets.push_back(def);
        } while (sets->NextRow());

        QueryResult items = WorldDatabase.Query(
            "SELECT `set_id`, `item_entry` FROM `aurora_item_set_items` "
            "ORDER BY `set_id`, `item_entry`");

        if (items)
        {
            do
            {
                Field* fields = items->Fetch();
                uint32 const setId = fields[0].Get<uint32>();
                uint32 const entry = fields[1].Get<uint32>();

                for (ItemSetDefinition& def : g_sets)
                    if (def.id == setId)
                        def.items.push_back(entry);
            } while (items->NextRow());
        }

        QueryResult bonuses = WorldDatabase.Query(
            "SELECT `set_id`, `pieces`, `spell_id` FROM `aurora_item_set_bonuses` "
            "ORDER BY `set_id`, `pieces`");

        if (bonuses)
        {
            do
            {
                Field* fields = bonuses->Fetch();
                uint32 const setId  = fields[0].Get<uint32>();
                uint32 const pieces = fields[1].Get<uint32>();
                uint32 const spell  = fields[2].Get<uint32>();

                for (ItemSetDefinition& def : g_sets)
                {
                    if (def.id != setId)
                        continue;

                    for (SetBonus& bonus : def.bonuses)
                    {
                        if (bonus.pieces == 0)
                        {
                            bonus.pieces = pieces;
                            bonus.spell  = spell;
                            break;
                        }
                    }
                }
            } while (bonuses->NextRow());
        }

        for (ItemSetDefinition const& def : g_sets)
            if (def.items.empty())
                LOG_WARN("module", "[Aurora] Item set {} has no items - its bonuses can never fire.",
                         def.id);

        LOG_INFO("module", "[Aurora] Item sets loaded: {} definitions.",
                 static_cast<uint32>(g_sets.size()));
    }

    uint8 CountEquipped(Player* player, ItemSetDefinition const& def)
    {
        uint8 worn = 0;

        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item)
                continue;

            uint32 const entry = item->GetEntry();
            if (std::find(def.items.begin(), def.items.end(), entry) != def.items.end())
                ++worn;
        }

        return worn;
    }

    void ApplySets(Player* player)
    {
        if (!player || g_sets.empty())
            return;

        for (ItemSetDefinition const& def : g_sets)
        {
            uint8 const worn = CountEquipped(player, def);

            for (SetBonus const& bonus : def.bonuses)
            {
                if (bonus.pieces == 0 || bonus.spell == 0)
                    continue;

                bool const shouldHave = worn >= bonus.pieces;
                bool const has        = player->HasAura(bonus.spell);

                if (shouldHave && !has)
                {
                    // A missing spell row means 2026_11_10_12.sql was not
                    // applied; skip instead of logging on every equip.
                    if (!sSpellMgr->GetSpellInfo(bonus.spell))
                    {
                        LOG_WARN("module", "[Aurora] Set {} bonus spell {} is missing - "
                                           "apply db-world 2026_11_10_12.sql.",
                                 def.id, bonus.spell);
                        continue;
                    }

                    player->CastSpell(player, bonus.spell, true);
                }
                else if (!shouldHave && has)
                {
                    player->RemoveAura(bonus.spell);
                }
            }
        }
    }
}

class aurora_item_sets : public WorldScript, public PlayerScript
{
public:
    aurora_item_sets()
        : WorldScript("aurora_item_sets_world"),
          PlayerScript("aurora_item_sets_player") { }

    void OnStartup() override
    {
        LoadSets();
    }

    void OnPlayerLogin(Player* player) override
    {
        if (!IsProjectEnabled())
            return;

        ApplySets(player);
    }

    // Fires for equip and unequip; `update` is ignored on purpose - both
    // transitions need the same recount.
    void OnPlayerEquip(Player* player, Item* /*item*/, uint8 /*bag*/, uint8 /*slot*/,
                       bool /*update*/) override
    {
        if (!IsProjectEnabled())
            return;

        ApplySets(player);
    }
};

void AddSC_aurora_item_sets()
{
    new aurora_item_sets();
}
