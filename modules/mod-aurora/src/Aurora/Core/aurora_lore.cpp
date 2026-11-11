// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_lore.cpp - chronicle browser and title registry.
//
// Two data-driven surfaces live here, both read straight from the world DB:
//
//   aurora_lore_chronicles  -> .aurora lore
//       A chapter unlocks when the player has the achievement named in
//       `unlock_achievement_id` (0 = always readable). Clients only get the
//       text, never the whole table.
//
//   aurora_titles           -> .aurora titles
//       Maps internal names to CharTitles.dbc ids so the C++ never hard-codes
//       them. `achievement_id` > 0 means "this title is owned when the player
//       has that achievement", which is what the list command reports.
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "CommandScript.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Log.h"
#include "StringFormat.h"

#include "aurora_shared.h"

using namespace Aurora;
using namespace Acore::ChatCommands;

namespace
{
    bool IsRussianLocale(Player const* player)
    {
        if (!player || !player->GetSession())
            return true;

        return player->GetSession()->GetSessionDbcLocale() == LOCALE_ruRU;
    }

    // ------------------------------------------------------------------------
    // .aurora lore        - list chapters the player may read
    // .aurora lore <n>    - print one chapter
    // ------------------------------------------------------------------------
    bool ShowLoreIndex(Player* player)
    {
        ChatHandler handler(player->GetSession());
        bool const ru = IsRussianLocale(player);

        QueryResult result = WorldDatabase.Query(
            "SELECT `chapter`, `title_ru`, `title_en`, `unlock_achievement_id` "
            "FROM `aurora_lore_chronicles` WHERE `campaign_id` = 1 "
            "ORDER BY `chapter` ASC");

        if (!result)
        {
            handler.SendSysMessage(ru ? "|cffff8800Хроники кампании не загружены.|r"
                                      : "|cffff8800No campaign chronicles loaded.|r");
            return true;
        }

        handler.PSendSysMessage("|cffFFD100=== %s ===|r",
            ru ? "ХРОНИКИ: НОЧЬ ПАДШИХ" : "CHRONICLES: NIGHT OF THE FALLEN");

        do
        {
            Field* fields     = result->Fetch();
            uint32 chapter    = fields[0].Get<uint32>();
            std::string title = ru ? fields[1].Get<std::string>()
                                   : fields[2].Get<std::string>();
            uint32 unlockAch  = fields[3].Get<uint32>();

            bool unlocked = (unlockAch == 0) || player->HasAchieved(unlockAch);

            if (unlocked)
                handler.PSendSysMessage("|cff00ff00[%u]|r %s", chapter, title.c_str());
            else
                handler.PSendSysMessage("|cff888888[%u] %s - закрыто|r",
                                        chapter, title.c_str());
        } while (result->NextRow());

        handler.SendSysMessage(ru ? "|cff888888Читать: .aurora lore <глава>|r"
                                  : "|cff888888Read: .aurora lore <chapter>|r");
        return true;
    }

    bool ShowLoreChapter(Player* player, uint32 chapter)
    {
        ChatHandler handler(player->GetSession());
        bool const ru = IsRussianLocale(player);

        QueryResult result = WorldDatabase.Query(
            "SELECT `title_ru`, `title_en`, `text_ru`, `text_en`, `unlock_achievement_id` "
            "FROM `aurora_lore_chronicles` WHERE `campaign_id` = 1 AND `chapter` = {}",
            chapter);

        if (!result)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                ru ? "Такой главы нет." : "No such chapter.");
            return true;
        }

        Field* fields      = result->Fetch();
        std::string title  = ru ? fields[0].Get<std::string>() : fields[1].Get<std::string>();
        std::string text   = ru ? fields[2].Get<std::string>() : fields[3].Get<std::string>();
        uint32 unlockAch   = fields[4].Get<uint32>();

        if (unlockAch > 0 && !player->HasAchieved(unlockAch))
        {
            handler.PSendSysMessage("|cffff8800[Aurora]|r %s",
                ru ? "Глава ещё закрыта: нужно достижение." 
                   : "This chapter is still locked: an achievement is required.");
            return true;
        }

        handler.PSendSysMessage("|cffFFD100%s|r", title.c_str());
        handler.SendSysMessage(text.c_str());
        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora titles - which registry titles the player owns
    // ------------------------------------------------------------------------
    bool ShowTitles(Player* player)
    {
        ChatHandler handler(player->GetSession());
        bool const ru = IsRussianLocale(player);

        QueryResult result = WorldDatabase.Query(
            "SELECT `internal_name`, `title_dbc_id`, `name_ru`, `name_en`, `achievement_id` "
            "FROM `aurora_titles` ORDER BY `title_dbc_id` ASC");

        if (!result)
        {
            handler.SendSysMessage(ru ? "|cffff8800Реестр титулов пуст.|r"
                                      : "|cffff8800Title registry is empty.|r");
            return true;
        }

        handler.PSendSysMessage("|cffFFD100=== %s ===|r",
            ru ? "ТИТУЛЫ АВРОРЫ" : "AURORA TITLES");

        uint32 owned = 0;
        uint32 total = 0;

        do
        {
            Field* fields       = result->Fetch();
            uint32 dbcId        = fields[1].Get<uint32>();
            std::string name    = ru ? fields[2].Get<std::string>() : fields[3].Get<std::string>();
            uint32 achievement  = fields[4].Get<uint32>();

            ++total;

            // Title ownership is checked against the DBC knowledge mask, so a
            // title that the client patch never added is reported as missing
            // instead of pretending the player owns it.
            CharTitlesEntry const* title = sCharTitlesStore.LookupEntry(dbcId);
            bool knownByClient = title != nullptr;

            bool has = false;
            if (knownByClient)
                has = player->HasTitle(title);
            else if (achievement > 0)
                has = player->HasAchieved(achievement);

            if (has)
                ++owned;

            if (!knownByClient)
                handler.PSendSysMessage("|cff888888[%u] %s - нет в клиенте|r",
                                        dbcId, name.c_str());
            else if (has)
                handler.PSendSysMessage("|cff00ff00[%u]%r %s", dbcId, name.c_str());
            else
                handler.PSendSysMessage("|cff888888[%u] %s|r", dbcId, name.c_str());
        } while (result->NextRow());

        handler.PSendSysMessage("%s: %u/%u",
            ru ? "Получено" : "Owned", owned, total);

        return true;
    }
}

class aurora_lore_commandscript : public CommandScript
{
public:
    aurora_lore_commandscript() : CommandScript("aurora_lore_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable loreCommands =
        {
            { "",    HandleLoreIndexCommand,   rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "list", HandleLoreIndexCommand,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "read", HandleLoreChapterCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable titleCommands =
        {
            { "list", HandleTitleListCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "lore",   loreCommands  },
            { "titles", titleCommands },
        };

        return commandTable;
    }

private:
    static Player* GetPlayer(ChatHandler* handler)
    {
        return handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    }

    static bool HandleLoreIndexCommand(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        return player ? ShowLoreIndex(player) : false;
    }

    static bool HandleLoreChapterCommand(ChatHandler* handler, uint32 chapter)
    {
        Player* player = GetPlayer(handler);
        return player ? ShowLoreChapter(player, chapter) : false;
    }

    static bool HandleTitleListCommand(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        return player ? ShowTitles(player) : false;
    }
};

void AddSC_aurora_lore()
{
    new aurora_lore_commandscript();
}
