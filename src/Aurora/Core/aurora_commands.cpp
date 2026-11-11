// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_commands.cpp - chat command layer.
//
// User-facing commands (all through CommandScript / ChatCommandTable):
//     .aurora version
//     .aurora shop            - catalogue
//     .aurora shop buy <id>   - purchase
//     .aurora vote list       - current candidates
//     .aurora vote top        - past winners
//     .aurora vote cast <cat> <rank>
//     .aurora activity        - personal activity snapshot
//
// Every DB write uses parameterised statements; no player input is ever
// concatenated into SQL text.
// ============================================================================

#include "ScriptMgr.h"
#include "Chat.h"
#include "CommandScript.h"
#include "Player.h"
#include "Item.h"
#include "Language.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "StringFormat.h"

#include "aurora_shared.h"

using namespace Aurora;
using namespace Acore::ChatCommands;

namespace
{
    // ------------------------------------------------------------------------
    // Shared helpers
    // ------------------------------------------------------------------------
    bool IsRussian(Player const* player)
    {
        if (!player || !player->GetSession())
            return true;

        return player->GetSession()->GetSessionDbcLocale() == LOCALE_ruRU;
    }

    char const* Tr(Player const* player, char const* ru, char const* en)
    {
        return IsRussian(player) ? ru : en;
    }

    std::string CurrentPeriod()
    {
        time_t now = time(nullptr);
        tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        char buffer[16];
        std::strftime(buffer, sizeof(buffer), "%Y-%m", &localTime);
        return buffer;
    }

    char const* CategoryKey(uint8 category)
    {
        switch (category)
        {
            case 0: return "pve";
            case 1: return "pvp";
            case 2: return "helper";
            case 3: return "monthly";
            default: return "pve";
        }
    }

    bool ParseCategory(std::string const& text, std::string& out)
    {
        if (text == "pve" || text == "pvp" || text == "helper" || text == "monthly")
        {
            out = text;
            return true;
        }
        return false;
    }

    // ------------------------------------------------------------------------
    // Shop catalogue - loaded once, reloaded by .aurora reload shop
    // ------------------------------------------------------------------------
    struct ShopEntry
    {
        uint32      id           = 0;
        uint32      itemEntry    = 0;
        uint32      itemAmount   = 1;
        uint32      costShards   = 0;
        uint32      costAurora   = 0;
        uint32      costGold     = 0;
        std::string nameRu;
        std::string nameEn;
    };

    std::unordered_map<uint32, ShopEntry> g_shopById;
    std::vector<uint32>                   g_shopOrder;
    bool                                  g_shopLoaded = false;

    void LoadShopCatalogue()
    {
        g_shopById.clear();
        g_shopOrder.clear();

        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `item_entry`, `item_amount`, `cost_shards`, `cost_aurora`, "
            "`cost_gold`, `name_ru`, `name_en` "
            "FROM `aurora_shop_items` WHERE `enabled` = 1 ORDER BY `id` ASC");

        if (!result)
        {
            LOG_ERROR("module", "[Aurora] Shop catalogue is empty or unreadable.");
            g_shopLoaded = true;
            return;
        }

        do
        {
            Field* fields = result->Fetch();

            ShopEntry entry;
            entry.id         = fields[0].Get<uint32>();
            entry.itemEntry  = fields[1].Get<uint32>();
            entry.itemAmount = fields[2].Get<uint32>();
            entry.costShards = fields[3].Get<uint32>();
            entry.costAurora = fields[4].Get<uint32>();
            entry.costGold   = fields[5].Get<uint32>();
            entry.nameRu     = fields[6].Get<std::string>();
            entry.nameEn     = fields[7].Get<std::string>();

            g_shopOrder.push_back(entry.id);
            g_shopById[entry.id] = entry;
        } while (result->NextRow());

        g_shopLoaded = true;
        LOG_INFO("module", "[Aurora] Shop catalogue loaded: {} entries.", g_shopOrder.size());
    }

    void EnsureShopLoaded()
    {
        if (!g_shopLoaded)
            LoadShopCatalogue();
    }

    // ------------------------------------------------------------------------
    // .aurora shop
    // ------------------------------------------------------------------------
    bool ShowShopList(Player* player)
    {
        EnsureShopLoaded();
        ChatHandler handler(player->GetSession());

        if (g_shopOrder.empty())
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Магазин пуст.", "The shop is empty."));
            return true;
        }

        handler.PSendSysMessage("|cffFFD100=== %s ===|r",
            Tr(player, "МАГАЗИН АВРОРЫ", "AURORA SHOP"));

        bool const russian = IsRussian(player);

        for (uint32 shopId : g_shopOrder)
        {
            auto itr = g_shopById.find(shopId);
            if (itr == g_shopById.end())
                continue;

            ShopEntry const& entry = itr->second;

            std::string price;
            if (entry.costShards > 0)
                price += std::to_string(entry.costShards) + " osc";
            if (entry.costAurora > 0)
                price += (price.empty() ? "" : " + ") + std::to_string(entry.costAurora) + " aur";
            if (entry.costGold > 0)
                price += (price.empty() ? "" : " + ") + std::to_string(entry.costGold / 10000) + "g";

            handler.PSendSysMessage("|cffffff00#{}|r {} x{} - |cff88ff88{}|r",
                entry.id,
                russian ? entry.nameRu.c_str() : entry.nameEn.c_str(),
                entry.itemAmount,
                price.c_str());
        }

        handler.PSendSysMessage("|cff888888%s|r",
            Tr(player, "Купить: .aurora shop buy <id>", "Buy: .aurora shop buy <id>"));

        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora shop buy <id>
    // ------------------------------------------------------------------------
    bool TryPurchase(Player* player, uint32 shopId)
    {
        EnsureShopLoaded();
        ChatHandler handler(player->GetSession());

        auto itr = g_shopById.find(shopId);
        if (itr == g_shopById.end())
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Такого товара нет.", "No such offer."));
            return true;
        }

        ShopEntry const& entry = itr->second;
        uint32 const guid = player->GetGUID().GetCounter();

        if (entry.costShards > 0 && player->GetItemCount(ITEM_SHARD_OF_FALLEN_STAR, true) < entry.costShards)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Недостаточно осколков.", "Not enough shards."));
            return true;
        }

        if (entry.costAurora > 0 && player->GetItemCount(ITEM_AURORA_POINTS, true) < entry.costAurora)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Недостаточно очков Авроры.", "Not enough Aurora points."));
            return true;
        }

        if (entry.costGold > 0 && player->GetMoney() < entry.costGold)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Недостаточно золота.", "Not enough gold."));
            return true;
        }

        // Charge first, then deliver. If delivery fails the currency is
        // refunded, so the player never pays for nothing.
        if (entry.costShards > 0)
            player->DestroyItemCount(ITEM_SHARD_OF_FALLEN_STAR, entry.costShards, true);

        if (entry.costAurora > 0)
            player->DestroyItemCount(ITEM_AURORA_POINTS, entry.costAurora, true);

        if (entry.costGold > 0)
            player->ModifyMoney(-static_cast<int32>(entry.costGold));

        player->AddItem(entry.itemEntry, entry.itemAmount);

        CharacterDatabase.Execute(
            "INSERT INTO `aurora_shop_purchases` "
            "(`player_guid`, `shop_id`, `item_entry`, `amount`) VALUES ({}, {}, {}, {})",
            guid, entry.id, entry.itemEntry, entry.itemAmount);

        WorldDatabase.Execute(
            "UPDATE `aurora_shop_items` SET `sold_count` = `sold_count` + 1 WHERE `id` = {}",
            entry.id);

        handler.PSendSysMessage("|cff00ff00[Aurora]|r %s",
            Tr(player, "Покупка совершена.", "Purchase complete."));

        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora activity
    // ------------------------------------------------------------------------
    bool ShowActivity(Player* player)
    {
        uint32 const guid = player->GetGUID().GetCounter();
        ChatHandler handler(player->GetSession());

        QueryResult result = CharacterDatabase.Query(
            "SELECT `total_playtime_sec`, `sessions_count`, `creatures_killed`, "
            "`players_killed`, `quests_completed`, `achievements_earned`, "
            "`pve_score`, `pvp_score`, `social_score`, `total_score` "
            "FROM `aurora_player_activity` WHERE `player_guid` = {}", guid);

        if (!result)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Активность ещё не учитывалась.",
                           "Your activity has not been tracked yet."));
            return true;
        }

        Field* fields = result->Fetch();
        uint64 playtime = fields[0].Get<uint64>();

        bool const russian = IsRussian(player);

        handler.PSendSysMessage("|cffFFD100=== %s ===|r",
            russian ? "ВАША АКТИВНОСТЬ" : "YOUR ACTIVITY");

        handler.PSendSysMessage("%s: {}h {}m",
            russian ? "Время в игре" : "Playtime",
            playtime / 3600, (playtime % 3600) / 60);

        handler.PSendSysMessage("%s: {}", russian ? "Входов" : "Sessions", fields[1].Get<uint32>());
        handler.PSendSysMessage("%s: {}", russian ? "Убито существ" : "Creatures killed", fields[2].Get<uint32>());
        handler.PSendSysMessage("%s: {}", russian ? "Убито игроков" : "Players killed", fields[3].Get<uint32>());
        handler.PSendSysMessage("%s: {}", russian ? "Квестов" : "Quests", fields[4].Get<uint32>());
        handler.PSendSysMessage("%s: {}", russian ? "Достижений" : "Achievements", fields[5].Get<uint32>());

        handler.PSendSysMessage("PvE: {:.2f}  PvP: {:.2f}  Social: {:.2f}",
                                fields[6].Get<double>(),
                                fields[7].Get<double>(),
                                fields[8].Get<double>());

        handler.PSendSysMessage("|cffff8800TOTAL: {:.2f}|r", fields[9].Get<double>());

        handler.PSendSysMessage("|cff888888%s|r",
            russian ? "Уровень персонажа на рейтинг не влияет."
                    : "Character level does not affect the ranking.");

        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora vote list
    // ------------------------------------------------------------------------
    bool ShowVoteList(Player* player)
    {
        std::string const period = CurrentPeriod();
        ChatHandler handler(player->GetSession());

        handler.PSendSysMessage("|cffFFD100=== %s %s ===|r",
            Tr(player, "КАНДИДАТЫ", "CANDIDATES"), period.c_str());

        QueryResult result = CharacterDatabase.Query(
            "SELECT c.`category`, c.`rank`, c.`score`, ch.`name` "
            "FROM `aurora_vote_candidates` c "
            "LEFT JOIN `characters` ch ON ch.`guid` = c.`player_guid` "
            "WHERE c.`period` = '{}' AND c.`rank` <= 5 "
            "ORDER BY c.`category`, c.`rank`", period);

        if (!result)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Кандидаты ещё не рассчитаны.",
                           "Candidates have not been calculated yet."));
            return true;
        }

        std::string lastCategory;

        do
        {
            Field* fields = result->Fetch();
            std::string category = fields[0].Get<std::string>();
            uint32 rank          = fields[1].Get<uint32>();
            double score         = fields[2].Get<double>();
            std::string name     = fields[3].IsNull() ? "???" : fields[3].Get<std::string>();

            if (category != lastCategory)
            {
                handler.PSendSysMessage("|cffffff00--- %s ---|r", category.c_str());
                lastCategory = category;
            }

            handler.PSendSysMessage("#{} {} ({:.2f})", rank, name.c_str(), score);
        } while (result->NextRow());

        handler.PSendSysMessage("|cff888888%s|r",
            Tr(player, "Голос: .aurora vote cast <pve|pvp|helper|monthly> <ранг>",
                       "Vote: .aurora vote cast <pve|pvp|helper|monthly> <rank>"));

        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora vote top
    // ------------------------------------------------------------------------
    bool ShowVoteTop(Player* player)
    {
        ChatHandler handler(player->GetSession());

        QueryResult result = CharacterDatabase.Query(
            "SELECT w.`period`, w.`category`, w.`rank`, w.`votes_count`, ch.`name` "
            "FROM `aurora_vote_winners` w "
            "LEFT JOIN `characters` ch ON ch.`guid` = w.`player_guid` "
            "ORDER BY w.`period` DESC, w.`category`, w.`rank` LIMIT 40");

        if (!result)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Победителей пока нет.", "No winners yet."));
            return true;
        }

        handler.PSendSysMessage("|cffFFD100=== %s ===|r",
            Tr(player, "ПОБЕДИТЕЛИ", "WINNERS"));

        do
        {
            Field* fields = result->Fetch();
            std::string period   = fields[0].Get<std::string>();
            std::string category = fields[1].Get<std::string>();
            uint32 rank          = fields[2].Get<uint32>();
            uint32 votes         = fields[3].Get<uint32>();
            std::string name     = fields[4].IsNull() ? "???" : fields[4].Get<std::string>();

            handler.PSendSysMessage("[{}] {} #{} {} - {} votes",
                                    period.c_str(), category.c_str(), rank,
                                    name.c_str(), votes);
        } while (result->NextRow());

        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora vote cast <category> <rank>
    // ------------------------------------------------------------------------
    bool CastVote(Player* player, std::string const& categoryText, uint32 rank)
    {
        ChatHandler handler(player->GetSession());

        std::string category;
        if (!ParseCategory(categoryText, category))
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Категория: pve, pvp, helper или monthly.",
                           "Category must be pve, pvp, helper or monthly."));
            return true;
        }

        if (rank == 0 || rank > AURORA_CANDIDATE_LIMIT)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Недопустимый ранг.", "Invalid rank."));
            return true;
        }

        uint32 const voterGuid = player->GetGUID().GetCounter();

        // Activity-First: one hour of playtime before a vote counts.
        QueryResult selfCheck = CharacterDatabase.Query(
            "SELECT `total_playtime_sec` FROM `aurora_player_activity` WHERE `player_guid` = {}",
            voterGuid);

        if (!selfCheck || selfCheck->Fetch()[0].Get<uint64>() < AURORA_MIN_PLAYTIME_SEC)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Нужно минимум 1 час игры.",
                           "At least one hour of playtime is required."));
            return true;
        }

        std::string const period = CurrentPeriod();

        QueryResult candidate = CharacterDatabase.Query(
            "SELECT `player_guid` FROM `aurora_vote_candidates` "
            "WHERE `period` = '{}' AND `category` = '{}' AND `rank` = {}",
            period, category, rank);

        if (!candidate)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Кандидат с таким рангом не найден.",
                           "No candidate at that rank."));
            return true;
        }

        uint32 const candidateGuid = candidate->Fetch()[0].Get<uint32>();

        if (candidateGuid == voterGuid)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "За себя голосовать нельзя.",
                           "You cannot vote for yourself."));
            return true;
        }

        QueryResult existing = CharacterDatabase.Query(
            "SELECT 1 FROM `aurora_votes` "
            "WHERE `voter_guid` = {} AND `category` = '{}' AND `vote_date` = CURDATE() LIMIT 1",
            voterGuid, category);
        if (existing)
        {
            handler.PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Сегодня вы уже голосовали в этой категории.",
                           "You already voted in this category today."));
            return true;
        }

        CharacterDatabase.Execute(
            "INSERT INTO `aurora_votes` (`voter_guid`, `candidate_guid`, `category`, `vote_date`) "
            "VALUES ({}, {}, '{}', CURDATE())",
            voterGuid, candidateGuid, category);

        handler.PSendSysMessage("|cff00ff00[Aurora]|r %s",
            Tr(player, "Голос учтён.", "Vote recorded."));

        return true;
    }
    // ------------------------------------------------------------------------
    // .aurora campaign - progress through campaign #1
    // ------------------------------------------------------------------------
    bool ShowCampaign(Player* player)
    {
        uint32 const guid = player->GetGUID().GetCounter();
        ChatHandler handler(player->GetSession());
        bool const ru = IsRussian(player);

        handler.PSendSysMessage("|cffFFD100=== %s ===|r",
            ru ? "КАМПАНИЯ #1: НОЧЬ ПАДШИХ" : "CAMPAIGN #1: NIGHT OF THE FALLEN");

        struct Chain
        {
            char const* label;
            uint32 first;
            uint32 last;
        };

        Chain const chains[] =
        {
            { ru ? "Альянс" : "Alliance", QUEST_ALLIANCE_1, QUEST_ALLIANCE_6 },
            { ru ? "Орда"   : "Horde",    QUEST_HORDE_1,    QUEST_HORDE_6    },
            { ru ? "Равновесие" : "Balance", QUEST_NEUTRAL_1, QUEST_NEUTRAL_6 },
        };

        for (Chain const& chain : chains)
        {
            uint32 done = 0;
            uint32 total = 0;
            for (uint32 q = chain.first; q <= chain.last; ++q, ++total)
                if (player->GetQuestRewardStatus(q))
                    ++done;

            handler.PSendSysMessage("%s: %u/%u", chain.label, done, total);
        }

        QueryResult captured = CharacterDatabase.Query(
            "SELECT `value` FROM `aurora_campaign_counters` "
            "WHERE `player_guid` = {} AND `counter` = 'altar_captures'", guid);

        uint32 const total = captured ? captured->Fetch()[0].Get<uint32>() : 0;
        handler.PSendSysMessage("%s: %u",
            ru ? "Захвачено алтарей всего" : "Altars captured total", total);

        QueryResult progress = CharacterDatabase.Query(
            "SELECT `completed_at`, `no_death` FROM `aurora_player_progress` "
            "WHERE `player_guid` = {} AND `campaign_id` = 1", guid);

        if (progress)
        {
            Field* fields = progress->Fetch();
            handler.PSendSysMessage("|cff00ff00%s|r %s",
                ru ? "Кампания пройдена:" : "Campaign completed:",
                fields[0].Get<std::string>().c_str());
        }
        else
        {
            handler.SendSysMessage(ru ? "|cffff8800Кампания ещё не завершена.|r"
                                      : "|cffff8800Campaign not finished yet.|r");
        }

        return true;
    }
}

// ============================================================================
// Command table
// ============================================================================
class aurora_commandscript : public CommandScript
{
public:
    aurora_commandscript() : CommandScript("aurora_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable shopCommands =
        {
            { "list", HandleShopListCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "buy",  HandleShopBuyCommand,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable voteCommands =
        {
            { "list", HandleVoteListCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "top",  HandleVoteTopCommand,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "cast", HandleVoteCastCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };

        static ChatCommandTable reloadCommands =
        {
            { "shop", HandleReloadShopCommand, rbac::RBAC_PERM_COMMAND_RELOAD_ALL, Console::Yes },
        };

        static ChatCommandTable auroraCommands =
        {
            { "shop",     shopCommands },
            { "vote",     voteCommands },
            { "reload",   reloadCommands },
            { "campaign", HandleCampaignCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "activity", HandleActivityCommand, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "version",  HandleVersionCommand,  rbac::RBAC_PERM_COMMAND_HELP, Console::Yes },
        };

        static ChatCommandTable commandTable =
        {
            { "aurora", auroraCommands },
        };

        return commandTable;
    }

private:
    static Player* GetPlayer(ChatHandler* handler)
    {
        return handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    }

    static bool HandleCampaignCommand(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        return player ? ShowCampaign(player) : false;
    }

    static bool HandleVersionCommand(ChatHandler* handler)
    {
        handler->PSendSysMessage("|cffFFD100[Aurora]|r {} - campaign #1: Night of the Fallen",
                                 Aurora::GetVersion());
        return true;
    }

    static bool HandleShopListCommand(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        return player ? ShowShopList(player) : false;
    }

    static bool HandleShopBuyCommand(ChatHandler* handler, uint32 shopId)
    {
        Player* player = GetPlayer(handler);
        return player ? TryPurchase(player, shopId) : false;
    }

    static bool HandleActivityCommand(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        return player ? ShowActivity(player) : false;
    }

    static bool HandleVoteListCommand(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        return player ? ShowVoteList(player) : false;
    }

    static bool HandleVoteTopCommand(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        return player ? ShowVoteTop(player) : false;
    }

    static bool HandleVoteCastCommand(ChatHandler* handler, std::string const& category, uint32 rank)
    {
        Player* player = GetPlayer(handler);
        return player ? CastVote(player, category, rank) : false;
    }

    static bool HandleReloadShopCommand(ChatHandler* handler)
    {
        LoadShopCatalogue();
        handler->PSendSysMessage("|cff00ff00[Aurora]|r {} shop entries reloaded.",
                                 g_shopOrder.size());
        return true;
    }
};

void AddSC_aurora_commandscript()
{
    new aurora_commandscript();
}

// Called by aurora_bootstrap so `.aurora shop` answers on the first click.
void AuroraLoadShopCatalogue()
{
    LoadShopCatalogue();
}
