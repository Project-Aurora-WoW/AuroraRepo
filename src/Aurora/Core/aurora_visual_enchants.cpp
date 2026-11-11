// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_visual_enchants.cpp - cosmetic enchant system.
//
// What a "visual enchant" is
//   A purely client-side aura applied to the player so a weapon or the whole
//   character glows. Nothing here changes stats, so it cannot affect balance.
//
// Ownership model
//   `aurora_visual_enchants`      - definitions (spell_visual_id, rarity, source)
//   `aurora_player_enchants`      - which enchants the player owns
//   `is_active`                   - exactly one row is active per player
//
// Commands
//   .aurora visuals            - list everything the player owns
//   .aurora visuals use <id>   - activate an owned enchant
//   .aurora visuals off        - remove the active visual
//
// The aura is reapplied on login because auras do not survive a relog when
// they were cast by a script flag, and it is removed on logout so the next
// character on the same account does not inherit it.
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Chat.h"
#include "CommandScript.h"
#include "StringFormat.h"
#include "SpellAuras.h"

#include "aurora_shared.h"

using namespace Aurora;
using namespace Acore::ChatCommands;

namespace
{
    // The visual is delivered as an aura cast with `triggered` = true, which
    // means no cast animation, no mana cost and no interruption.
    void ApplyVisual(Player* player, uint32 spellId)
    {
        if (spellId == 0)
            return;

        if (!sSpellMgr->GetSpellInfo(spellId))
        {
            LOG_WARN("module", "[Aurora] Visual enchant spell {} is missing from Spell.dbc "
                               "- client patch not applied?", spellId);
            return;
        }

        player->RemoveAurasDueToSpell(spellId);
        player->CastSpell(player, spellId, true);
    }

    uint32 GetEnchantSpell(uint32 enchantId)
    {
        QueryResult result = WorldDatabase.Query(
            "SELECT `spell_visual_id` FROM `aurora_visual_enchants` WHERE `id` = {}", enchantId);

        if (!result)
            return 0;

        return result->Fetch()[0].Get<uint32>();
    }

    bool PlayerOwns(uint32 guid, uint32 enchantId)
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT 1 FROM `aurora_player_enchants` "
            "WHERE `player_guid` = {} AND `enchant_id` = {} LIMIT 1", guid, enchantId);

        return result != nullptr;
    }

    void DeactivateAll(uint32 guid)
    {
        CharacterDatabase.Execute(
            "UPDATE `aurora_player_enchants` SET `is_active` = 0 WHERE `player_guid` = {}", guid);
    }

    void Activate(uint32 guid, uint32 enchantId)
    {
        CharacterDatabase.Execute(
            "UPDATE `aurora_player_enchants` SET `is_active` = 1 "
            "WHERE `player_guid` = {} AND `enchant_id` = {}", guid, enchantId);
    }

    uint32 GetActiveEnchant(uint32 guid)
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT `enchant_id` FROM `aurora_player_enchants` "
            "WHERE `player_guid` = {} AND `is_active` = 1 LIMIT 1", guid);

        if (!result)
            return 0;

        return result->Fetch()[0].Get<uint32>();
    }

    // ------------------------------------------------------------------------
    // Command implementations
    // ------------------------------------------------------------------------
    bool ShowVisuals(Player* player)
    {
        uint32 const guid = player->GetGUID().GetCounter();
        ChatHandler handler(player->GetSession());

        QueryResult result = CharacterDatabase.Query(
            "SELECT e.`enchant_id`, v.`name_ru`, v.`name_en`, e.`is_active` "
            "FROM `aurora_player_enchants` e "
            "JOIN `aurora_visual_enchants` v ON v.`id` = e.`enchant_id` "
            "WHERE e.`player_guid` = {} ORDER BY e.`enchant_id`", guid);

        if (!result)
        {
            handler.SendSysMessage("|cffff0000[Aurora]|r Визуальных энчантов нет.");
            return true;
        }

        bool const russian =
            player->GetSession()->GetSessionDbcLocale() == LOCALE_ruRU;

        handler.SendSysMessage("|cffFFD100=== ВИЗУАЛЬНЫЕ ЭНЧАНТЫ ===|r");

        do
        {
            Field* fields = result->Fetch();
            uint32 id       = fields[0].Get<uint32>();
            std::string name = russian ? fields[1].Get<std::string>()
                                       : fields[2].Get<std::string>();
            bool active     = fields[3].Get<bool>();

            handler.PSendSysMessage("%s#%u %s",
                                    active ? "|cff00ff00[вкл]|r " : "",
                                    id, name.c_str());
        } while (result->NextRow());

        handler.SendSysMessage("|cff888888Использовать: .aurora visuals use <id>|r");
        return true;
    }

    bool UseVisual(Player* player, uint32 enchantId)
    {
        uint32 const guid = player->GetGUID().GetCounter();
        ChatHandler handler(player->GetSession());

        if (!PlayerOwns(guid, enchantId))
        {
            handler.SendSysMessage("|cffff0000[Aurora]|r Энчант вам не принадлежит.");
            return true;
        }

        uint32 const spellId = GetEnchantSpell(enchantId);
        if (spellId == 0)
        {
            handler.SendSysMessage("|cffff0000[Aurora]|r Энчант не найден в справочнике.");
            return true;
        }

        // Remove whatever was active before switching, so two visuals never
        // fight over the same weapon slot.
        uint32 previous = GetActiveEnchant(guid);
        if (previous > 0 && previous != enchantId)
        {
            uint32 previousSpell = GetEnchantSpell(previous);
            if (previousSpell > 0)
                player->RemoveAurasDueToSpell(previousSpell);
        }

        DeactivateAll(guid);
        Activate(guid, enchantId);
        ApplyVisual(player, spellId);

        handler.PSendSysMessage("|cff00ff00[Aurora]|r Визуальный энчант #%u активирован.", enchantId);
        return true;
    }

    bool DisableVisual(Player* player)
    {
        uint32 const guid = player->GetGUID().GetCounter();
        ChatHandler handler(player->GetSession());

        uint32 const active = GetActiveEnchant(guid);
        if (active == 0)
        {
            handler.SendSysMessage("|cffff0000[Aurora]|r Активного энчанта нет.");
            return true;
        }

        uint32 const spellId = GetEnchantSpell(active);
        if (spellId > 0)
            player->RemoveAurasDueToSpell(spellId);

        DeactivateAll(guid);
        handler.SendSysMessage("|cff00ff00[Aurora]|r Визуальный энчант отключён.");
        return true;
    }
}

// ============================================================================
// Scripts
// ============================================================================
class aurora_visual_enchants : public PlayerScript
{
public:
    aurora_visual_enchants() : PlayerScript("aurora_visual_enchants") { }

    void OnPlayerLogin(Player* player) override
    {
        if (!player)
            return;

        uint32 const enchantId = GetActiveEnchant(player->GetGUID().GetCounter());
        if (enchantId == 0)
            return;

        uint32 const spellId = GetEnchantSpell(enchantId);
        if (spellId > 0)
            ApplyVisual(player, spellId);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (!player)
            return;

        uint32 const enchantId = GetActiveEnchant(player->GetGUID().GetCounter());
        if (enchantId == 0)
            return;

        uint32 const spellId = GetEnchantSpell(enchantId);
        if (spellId > 0)
            player->RemoveAurasDueToSpell(spellId);
    }
};

class aurora_visual_commandscript : public CommandScript
{
public:
    aurora_visual_commandscript() : CommandScript("aurora_visual_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable visualCommands =
        {
            { "list", HandleListCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "use",  HandleUseCommand,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "off",  HandleOffCommand,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        return visualCommands;
    }

private:
    static Player* GetPlayer(ChatHandler* handler)
    {
        return handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    }

    static bool HandleListCommand(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        return player ? ShowVisuals(player) : false;
    }

    static bool HandleUseCommand(ChatHandler* handler, uint32 enchantId)
    {
        Player* player = GetPlayer(handler);
        return player ? UseVisual(player, enchantId) : false;
    }

    static bool HandleOffCommand(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        return player ? DisableVisual(player) : false;
    }
};

void AddSC_aurora_visual_enchants()
{
    new aurora_visual_enchants();
    new aurora_visual_commandscript();
}
