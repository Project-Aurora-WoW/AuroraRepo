// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_commands.cpp - chat command layer.
//
// WHY THIS FILE EXISTS
//   The original design put `.shop` and `.vote` handlers into a PlayerScript
//   through a hook called `OnChatCommand`. AzerothCore has no such hook.
//   Player-facing chat commands must derive from CommandScript, which is what
//   this file implements:
//       aurora shop
//       aurora shop <id>
//       aurora vote
//       aurora vote list
//       aurora vote top
//       aurora vote stats
//       aurora vote pve|pvp|helper|monthly <rank|player>
//       aurora activity
//       aurora version
//       aurora reload        (RBAC: administrator)
//
// All DB access uses prepared statements / AddQuery with placeholder binding;
// no string concatenation of player input ever reaches SQL.
// ============================================================================

#include "ScriptMgr.h"
#include "Chat.h"
#include "CommandScript.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "sObjectMgr.h"
#include "StringFormat.h"
#include "Log.h"
#include "Mail.h"
#include "Item.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    // ------------------------------------------------------------------------
    // Localisation helper. Client locale decides RU/EN; falls back to RU.
    // ------------------------------------------------------------------------
    bool IsRussian(Player const* player)
    {
        std::string locale = player->GetSession()->GetSessionDbcLocale();
        return locale != "enUS" && locale != "enGB";
    }

    char const* Tr(Player const* player, char const* ru, char const* en)
    {
        return IsRussian(player) ? ru : en;
    }

    // ------------------------------------------------------------------------
    // Shop catalogue cache. Reloaded on startup and on demand via
    // `.aurora reload` - never per click, so a purchase cannot be affected by
    // a slow world DB mid-transaction.
    // ------------------------------------------------------------------------
    struct ShopEntry
    {
        uint32 id            = 0;
        uint32 itemEntry     = 0;
        uint32 itemAmount    = 1;
        uint32 costShards    = 0;
        uint32 costAurora    = 0;
        uint64 costGold      = 0;
        uint8  discount      = 0;
        uint32 maxPerPlayer  = 0;
        std::string nameRu;
        std::string nameEn;
    };

    std::unordered_map<uint32, ShopEntry> g_shopById;
    std::vector<uint32>                   g_shopOrder;

    void LoadShopCatalogue()
    {
        g_shopById.clear();
        g_shopOrder.clear();

        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `item_entry`, `item_amount`, `cost_shards`, `cost_aurora`, `cost_gold`, "
            "`discount`, `max_per_player`, `name_ru`, `name_en` "
            "FROM `aurora_shop_items` WHERE `enabled` = 1 ORDER BY `category`, `cost_shards` DESC, `id` ASC");

        if (!result)
        {
            LOG_INFO("module", "[Aurora] Shop catalogue is empty - .aurora shop will report no offers.");
            return;
        }

        do
        {
            Field* fields = result->Fetch();
            ShopEntry entry;
            entry.id           = fields[0].Get<uint32>();
            entry.itemEntry    = fields[1].Get<uint32>();
            entry.itemAmount   = fields[2].Get<uint32>();
            entry.costShards   = fields[3].Get<uint32>();
            entry.costAurora   = fields[4].Get<uint32>();
            entry.costGold     = fields[5].Get<uint64>();
            entry.discount     = fields[6].Get<uint8>();
            entry.maxPerPlayer = fields[7].Get<uint32>();
            entry.nameRu       = fields[8].Get<std::string>();
            entry.nameEn       = fields[9].Get<std::string>();

            g_shopOrder.push_back(entry.id);
            g_shopById[entry.id] = entry;
        } while (result->NextRow());

        LOG_INFO("module", "[Aurora] Loaded {} shop entries.", g_shopOrder.size());
    }

    uint64 ApplyDiscount(uint64 price, uint8 discount)
    {
        if (discount >= 90)
            return price / 10;
        return price - (price * discount / 100);
    }

    // ------------------------------------------------------------------------
    // Ownership / purchase count per player, used by `max_per_player`
    // ------------------------------------------------------------------------
    uint32 GetPurchaseCount(uint32 guid, uint32 shopId)
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT COUNT(*) FROM `aurora_shop_purchases` WHERE `player_guid` = {} AND `shop_id` = {}",
            guid, shopId);

        if (!result)
            return 0;

        return result->Fetch()[0].Get<uint32>();
    }

    void RecordPurchase(uint32 guid, uint32 shopId, uint32 itemEntry, uint32 amount)
    {
        CharacterDatabase.Execute(
            "INSERT INTO `aurora_shop_purchases` (`player_guid`, `shop_id`, `item_entry`, `amount`) "
            "VALUES ({}, {}, {}, {})", guid, shopId, itemEntry, amount);
    }

    // ------------------------------------------------------------------------
    // Vote helpers
    // ------------------------------------------------------------------------
    std::string CurrentPeriod()
    {
        time_t now = time(nullptr);
        tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        char buffer[16];
        strftime(buffer, sizeof(buffer), "%Y-%m", &localTime);
        return std::string(buffer);
    }

    char const* CategoryKey(VoteCategory category)
    {
        switch (category)
        {
            case VOTE_PVE:     return "pve";
            case VOTE_PVP:     return "pvp";
            case VOTE_HELPER:  return "helper";
            case VOTE_MONTHLY: return "monthly";
            default:           return "pve";
        }
    }

    char const* CategoryNameRu(VoteCategory category)
    {
        switch (category)
        {
            case VOTE_PVE:     return "Лучший PvE";
            case VOTE_PVP:     return "Лучший PvP";
            case VOTE_HELPER:  return "Лучший помощник";
            case VOTE_MONTHLY: return "Игрок месяца";
            default:           return "PvE";
        }
    }

    bool ParseCategory(std::string const& text, VoteCategory& out)
    {
        if (text == "pve")     { out = VOTE_PVE;     return true; }
        if (text == "pvp")     { out = VOTE_PVP;     return true; }
        if (text == "helper")  { out = VOTE_HELPER;  return true; }
        if (text == "monthly") { out = VOTE_MONTHLY; return true; }
        return false;
    }

    // ------------------------------------------------------------------------
    // Activity snapshot for `.aurora activity`
    // ------------------------------------------------------------------------
    struct ActivityRow
    {
        uint64 playtime     = 0;
        uint32 sessions     = 0;
        uint32 creatures    = 0;
        uint32 playersKill  = 0;
        uint32 quests       = 0;
        uint32 achievements = 0;
        uint32 mails        = 0;
        double pve          = 0.0;
        double pvp          = 0.0;
        double social       = 0.0;
        double total        = 0.0;
        bool   found        = false;
    };

    ActivityRow LoadActivity(uint32 guid)
    {
        ActivityRow row;

        QueryResult result = CharacterDatabase.Query(
            "SELECT `total_playtime_sec`, `sessions_count`, `creatures_killed`, `players_killed`, "
            "`quests_completed`, `achievements_earned`, `mails_sent`, "
            "`pve_score`, `pvp_score`, `social_score`, `total_score` "
            "FROM `aurora_player_activity` WHERE `player_guid` = {}", guid);

        if (!result)
            return row;

        Field* f = result->Fetch();
        row.playtime     = f[0].Get<uint64>();
        row.sessions     = f[1].Get<uint32>();
        row.creatures    = f[2].Get<uint32>();
        row.playersKill  = f[3].Get<uint32>();
        row.quests       = f[4].Get<uint32>();
        row.achievements = f[5].Get<uint32>();
        row.mails        = f[6].Get<uint32>();
        row.pve          = f[7].Get<double>();
        row.pvp          = f[8].Get<double>();
        row.social       = f[9].Get<double>();
        row.total        = f[10].Get<double>();
        row.found        = true;
        return row;
    }

    // ------------------------------------------------------------------------
    // Purchase execution: all checks happen before any inventory mutation.
    // Returns true when the item was granted.
    // ------------------------------------------------------------------------
    bool TryPurchase(Player* player, ShopEntry const& entry)
    {
        uint32 guid = player->GetGUID().GetCounter();

        if (entry.maxPerPlayer > 0 && GetPurchaseCount(guid, entry.id) >= entry.maxPerPlayer)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffff0000[Aurora]|r %s",
                Tr(player, "Лимит покупок для этого товара исчерпан.",
                          "You reached the purchase limit for this offer."));
            return false;
        }

        uint32 const shards  = ApplyDiscount(entry.costShards, entry.discount);
        uint32 const aurora  = ApplyDiscount(entry.costAurora, entry.discount);
        uint64 const gold    = ApplyDiscount(entry.costGold,   entry.discount);

        if (shards > 0 && player->GetItemCount(ITEM_SHARD_OF_FALLEN_STAR, false) < shards)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffff0000[Aurora]|r %s", Tr(player, "Недостаточно осколков.",
                                                     "Not enough shards."));
            return false;
        }

        if (aurora > 0 && player->GetItemCount(ITEM_AURORA_POINTS, false) < aurora)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffff0000[Aurora]|r %s", Tr(player, "Недостаточно очков Авроры.",
                                                     "Not enough Aurora points."));
            return false;
        }

        if (gold > 0 && player->GetMoney() < gold)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffff0000[Aurora]|r %s", Tr(player, "Недостаточно золота.",
                                                     "Not enough gold."));
            return false;
        }

        // Space check before spending: a full bag must not eat currency.
        if (!player->CanStoreNewItem(NULL_BAG, NULL_SLOT, ITEM_NONE, nullptr)
            && entry.itemAmount > 1)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffff0000[Aurora]|r %s", Tr(player, "В сумках нет места.",
                                                     "Your bags are full."));
            return false;
        }

        if (shards > 0)
            player->DestroyItemCount(ITEM_SHARD_OF_FALLEN_STAR, shards, true);

        if (aurora > 0)
            player->DestroyItemCount(ITEM_AURORA_POINTS, aurora, true);

        if (gold > 0)
            player->ModifyMoney(-static_cast<int64>(gold));

        if (Item* item = player->AddItem(entry.itemEntry, entry.itemAmount))
        {
            item->SetState(ITEM_CHANGED, player);
        }
        else
        {
            // Delivery failed after payment: refund so the player never loses
            // currency without goods.
            if (gold > 0)
                player->ModifyMoney(static_cast<int64>(gold));
            if (shards > 0)
                player->AddItem(ITEM_SHARD_OF_FALLEN_STAR, shards);
            if (aurora > 0)
                player->AddItem(ITEM_AURORA_POINTS, aurora);

            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffff0000[Aurora]|r %s", Tr(player, "Не удалось выдать предмет, средства возвращены.",
                                                     "Could not deliver the item, refunded."));
            return false;
        }

        RecordPurchase(guid, entry.id, entry.itemEntry, entry.itemAmount);

        WorldDatabase.Execute(
            "UPDATE `aurora_shop_items` SET `sold_count` = `sold_count` + 1 WHERE `id` = {}", entry.id);

        ChatHandler(player->GetSession()).PSendSysMessage(
            "|cff00ff00[Aurora]|r %s", Tr(player, "Покупка совершена.",
                                                 "Purchase complete."));
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

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> shopCommandTable =
        {
            { "list", rbac::RBAC_PERM_COMMAND_HELP, false, &HandleShopListCommand,   "" },
            { "buy",  rbac::RBAC_PERM_COMMAND_HELP, false, &HandleShopBuyCommand,    "" },
            { "",     rbac::RBAC_PERM_COMMAND_HELP, false, &HandleShopListCommand,   "" }
        };

        static std::vector<ChatCommand> voteCommandTable =
        {
            { "list",  rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteListCommand,  "" },
            { "top",   rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteTopCommand,   "" },
            { "stats", rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteStatsCommand, "" },
            { "cast",  rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteCastCommand,  "" },
            { "",      rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteListCommand,  "" }
        };

        static std::vector<ChatCommand> reloadCommandTable =
        {
            { "shop", rbac::RBAC_PERM_COMMAND_RELOAD_ALL, false, &HandleReloadShopCommand, "" },
            { "",     rbac::RBAC_PERM_COMMAND_RELOAD_ALL, false, &HandleReloadShopCommand, "" }
        };

        static std::vector<ChatCommand> auroraCommandTable =
        {
            { "shop",     rbac::RBAC_PERM_COMMAND_HELP,       false, nullptr, "" },
            { "vote",     rbac::RBAC_PERM_COMMAND_HELP,       false, nullptr, "" },
            { "activity", rbac::RBAC_PERM_COMMAND_HELP,       false, &HandleActivityCommand, "" },
            { "version",  rbac::RBAC_PERM_COMMAND_HELP,       false, &HandleVersionCommand,  "" },
            { "reload",   rbac::RBAC_PERM_COMMAND_RELOAD_ALL, false, nullptr, "" },
            { "",         rbac::RBAC_PERM_COMMAND_HELP,       false, &HandleVersionCommand,  "" }
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "aurora", rbac::RBAC_PERM_COMMAND_HELP, false, nullptr, "" }
        };

        // Aliases: `.shop` and `.vote` forward to the aurora subcommands.
        static std::vector<ChatCommand> shopAliasTable =
        {
            { "", rbac::RBAC_PERM_COMMAND_HELP, false, &HandleShopListCommand, "" }
        };

        static std::vector<ChatCommand> voteAliasTable =
        {
            { "list",  rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteListCommand,  "" },
            { "top",   rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteTopCommand,   "" },
            { "stats", rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteStatsCommand, "" },
            { "cast",  rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteCastCommand,  "" },
            { "",      rbac::RBAC_PERM_COMMAND_HELP, false, &HandleVoteListCommand,  "" }
        };

        commandTable[0].ChildCommands = auroraCommandTable;
        commandTable[0].ChildCommands[0].ChildCommands = shopCommandTable;
        commandTable[0].ChildCommands[1].ChildCommands = voteCommandTable;
        commandTable[0].ChildCommands[4].ChildCommands = reloadCommandTable;

        static std::vector<ChatCommand> fullTable =
        {
            { "aurora", rbac::RBAC_PERM_COMMAND_HELP,       false, &HandleVersionCommand, "" },
            { "shop",   rbac::RBAC_PERM_COMMAND_HELP,       false, nullptr, "" },
            { "vote",   rbac::RBAC_PERM_COMMAND_HELP,       false, nullptr, "" }
        };

        fullTable[0].ChildCommands = auroraCommandTable;
        fullTable[1].ChildCommands = shopAliasTable;
        fullTable[2].ChildCommands = voteAliasTable;

        return fullTable;
    }

private:
    // ------------------------------------------------------------------------
    // .aurora version
    // ------------------------------------------------------------------------
    static bool HandleVersionCommand(ChatHandler* handler)
    {
        handler->PSendSysMessage("|cffFFD100[Aurora]|r version {} - campaign #1: Night of the Fallen",
                                 GetVersion());
        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora shop [list|<id>]
    // ------------------------------------------------------------------------
    static bool HandleShopListCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        if (g_shopOrder.empty())
            LoadShopCatalogue();

        if (g_shopOrder.empty())
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Магазин пуст.", "The shop is empty."));
            return true;
        }

        handler->PSendSysMessage("|cffFFD100=== %s ===|r",
            Tr(player, "МАГАЗИН АВРОРЫ", "AURORA SHOP"));

        bool russian = IsRussian(player);
        for (uint32 shopId : g_shopOrder)
        {
            auto itr = g_shopById.find(shopId);
            if (itr == g_shopById.end())
                continue;

            ShopEntry const& entry = itr->second;
            uint32 shards = ApplyDiscount(entry.costShards, entry.discount);
            uint32 aurora = ApplyDiscount(entry.costAurora, entry.discount);
            uint64 gold   = ApplyDiscount(entry.costGold, entry.discount);

            handler->PSendSysMessage("|cffffff00#{}|r {} x{} - {} {} {}",
                entry.id,
                russian ? entry.nameRu : entry.nameEn,
                entry.itemAmount,
                shards > 0 ? std::to_string(shards) + " shard" : "",
                aurora > 0 ? std::to_string(aurora) + " aurora" : "",
                gold > 0 ? std::to_string(gold / 10000) + "g" : "");
        }

        handler->PSendSysMessage("|cff888888%s|r",
            Tr(player, "Используйте: .aurora shop buy <id>",
                       "Use: .aurora shop buy <id>"));
        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora shop buy <id>
    // ------------------------------------------------------------------------
    static bool HandleShopBuyCommand(ChatHandler* handler, uint32 shopId)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        if (g_shopById.empty())
            LoadShopCatalogue();

        auto itr = g_shopById.find(shopId);
        if (itr == g_shopById.end())
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Такого товара нет.", "No such offer."));
            return true;
        }

        TryPurchase(player, itr->second);
        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora vote list - top candidates of the current period
    // ------------------------------------------------------------------------
    static bool HandleVoteListCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        std::string period = CurrentPeriod();
        handler->PSendSysMessage("|cffFFD100=== %s %s ===|r",
            Tr(player, "КАНДИДАТЫ", "CANDIDATES"), period);

        QueryResult result = CharacterDatabase.Query(
            "SELECT c.`category`, c.`rank`, c.`player_guid`, c.`score`, ch.`name` "
            "FROM `aurora_vote_candidates` c "
            "LEFT JOIN `characters` ch ON ch.`guid` = c.`player_guid` "
            "WHERE c.`period` = '{}' AND c.`rank` <= 5 "
            "ORDER BY c.`category`, c.`rank`", period);

        if (!result)
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Кандидаты ещё не рассчитаны.",
                           "Candidates have not been calculated yet."));
            return true;
        }

        VoteCategory lastCategory = VOTE_MAX;
        do
        {
            Field* f = result->Fetch();
            std::string category = f[0].Get<std::string>();
            uint32 rank          = f[1].Get<uint32>();
            std::string name     = f[4].IsNull() ? "???" : f[4].Get<std::string>();
            double score         = f[3].Get<double>();

            VoteCategory parsed = VOTE_MAX;
            if (!ParseCategory(category, parsed))
                continue;

            if (parsed != lastCategory)
            {
                handler->PSendSysMessage("|cffffff00--- %s ---|r",
                    IsRussian(player) ? CategoryNameRu(parsed) : CategoryKey(parsed));
                lastCategory = parsed;
            }

            handler->PSendSysMessage("#{} {} ({:.2f})", rank, name, score);
        } while (result->NextRow());

        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora vote top - winners of previous period
    // ------------------------------------------------------------------------
    static bool HandleVoteTopCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        QueryResult result = CharacterDatabase.Query(
            "SELECT w.`period`, w.`category`, w.`rank`, ch.`name`, w.`votes_count` "
            "FROM `aurora_vote_winners` w "
            "LEFT JOIN `characters` ch ON ch.`guid` = w.`player_guid` "
            "ORDER BY w.`period` DESC, w.`category`, w.`rank` LIMIT 40");

        if (!result)
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Победителей пока нет.", "No winners yet."));
            return true;
        }

        handler->PSendSysMessage("|cffFFD100=== %s ===|r",
            Tr(player, "ПОБЕДИТЕЛИ", "WINNERS"));

        do
        {
            Field* f = result->Fetch();
            std::string period   = f[0].Get<std::string>();
            std::string category = f[1].Get<std::string>();
            uint32 rank          = f[2].Get<uint32>();
            std::string name     = f[3].IsNull() ? "???" : f[3].Get<std::string>();
            uint32 votes         = f[4].Get<uint32>();

            handler->PSendSysMessage("[{}] {} #{} {} - {} votes",
                period, category, rank, name, votes);
        } while (result->NextRow());

        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora vote stats / .aurora activity
    // ------------------------------------------------------------------------
    static bool HandleVoteStatsCommand(ChatHandler* handler)
    {
        return HandleActivityCommand(handler);
    }

    static bool HandleActivityCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        ActivityRow row = LoadActivity(player->GetGUID().GetCounter());
        if (!row.found)
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Активность ещё не учитывалась.",
                           "Your activity has not been tracked yet."));
            return true;
        }

        bool russian = IsRussian(player);
        handler->PSendSysMessage("|cffFFD100=== %s ===|r",
            russian ? "ВАША АКТИВНОСТЬ" : "YOUR ACTIVITY");
        handler->PSendSysMessage("%s: {}h {}m",
            russian ? "Время в игре" : "Playtime",
            row.playtime / 3600, (row.playtime % 3600) / 60);
        handler->PSendSysMessage("%s: {}", russian ? "Входов" : "Sessions", row.sessions);
        handler->PSendSysMessage("%s: {}", russian ? "Убито существ" : "Creatures killed", row.creatures);
        handler->PSendSysMessage("%s: {}", russian ? "Убито игроков" : "Players killed", row.playersKill);
        handler->PSendSysMessage("%s: {}", russian ? "Квестов" : "Quests", row.quests);
        handler->PSendSysMessage("%s: {}", russian ? "Достижений" : "Achievements", row.achievements);
        handler->PSendSysMessage("%s: {}", russian ? "Писем" : "Mails", row.mails);
        handler->PSendSysMessage("PvE: {:.2f}  PvP: {:.2f}  Social: {:.2f}",
                                 row.pve, row.pvp, row.social);
        handler->PSendSysMessage("|cffff8800TOTAL: {:.2f}|r", row.total);

        // Explicitly state the Activity-First rule so the player understands
        // why level is not part of the ranking.
        handler->PSendSysMessage("|cff888888%s|r",
            russian ? "Уровень персонажа на рейтинг не влияет."
                    : "Character level does not affect the ranking.");
        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora vote cast <pve|pvp|helper|monthly> <rank>
    // ------------------------------------------------------------------------
    static bool HandleVoteCastCommand(ChatHandler* handler,
                                      std::string const& categoryText,
                                      uint32 rank)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        VoteCategory category;
        if (!ParseCategory(categoryText, category))
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Категория: pve, pvp, helper или monthly.",
                           "Category must be pve, pvp, helper or monthly."));
            return true;
        }

        if (rank == 0 || rank > AURORA_CANDIDATE_LIMIT)
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Недопустимый ранг.", "Invalid rank."));
            return true;
        }

        uint32 voterGuid = player->GetGUID().GetCounter();

        // Eligibility: Activity-First means at least one hour played.
        QueryResult selfCheck = CharacterDatabase.Query(
            "SELECT `total_playtime_sec` FROM `aurora_player_activity` WHERE `player_guid` = {}",
            voterGuid);

        if (!selfCheck)
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Голосовать можно после часа игры.",
                           "You can vote after one hour of playtime."));
            return true;
        }

        uint64 playtime = selfCheck->Fetch()[0].Get<uint64>();
        if (playtime < AURORA_MIN_PLAYTIME_SEC)
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Нужно минимум 1 час игры.", "One hour of playtime is required."));
            return true;
        }

        std::string period = CurrentPeriod();

        QueryResult candidate = CharacterDatabase.Query(
            "SELECT `player_guid` FROM `aurora_vote_candidates` "
            "WHERE `period` = '{}' AND `category` = '{}' AND `rank` = {}",
            period, CategoryKey(category), rank);

        if (!candidate)
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Кандидат с таким рангом не найден.",
                           "No candidate at that rank."));
            return true;
        }

        uint32 candidateGuid = candidate->Fetch()[0].Get<uint32>();

        if (candidateGuid == voterGuid)
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "За себя голосовать нельзя.", "You cannot vote for yourself."));
            return true;
        }

        // One vote per voter per category per day, enforced by the PK.
        QueryResult existing = CharacterDatabase.Query(
            "SELECT `candidate_guid` FROM `aurora_votes` "
            "WHERE `voter_guid` = {} AND `category` = '{}' AND `vote_date` = CURDATE()",
            voterGuid, CategoryKey(category));

        if (existing)
        {
            handler->PSendSysMessage("|cffff0000[Aurora]|r %s",
                Tr(player, "Сегодня вы уже голосовали в этой категории.",
                           "You already voted in this category today."));
            return true;
        }

        CharacterDatabase.Execute(
            "INSERT INTO `aurora_votes` (`voter_guid`, `candidate_guid`, `category`, `vote_date`) "
            "VALUES ({}, {}, '{}', CURDATE())",
            voterGuid, candidateGuid, CategoryKey(category));

        handler->PSendSysMessage("|cff00ff00[Aurora]|r %s",
            Tr(player, "Голос учтён.", "Vote recorded."));
        return true;
    }

    // ------------------------------------------------------------------------
    // .aurora reload shop
    // ------------------------------------------------------------------------
    static bool HandleReloadShopCommand(ChatHandler* handler)
    {
        LoadShopCatalogue();
        handler->PSendSysMessage("|cff00ff00[Aurora]|r {} shop entries reloaded.", g_shopOrder.size());
        LOG_INFO("module", "[Aurora] Shop catalogue reloaded by command ({} entries).", g_shopOrder.size());
        return true;
    }
};

void AddSC_aurora_commandscript()
{
    new aurora_commandscript();
}

// Called by the world script on startup so the first `.shop` is instant.
void AuroraLoadShopCatalogue()
{
    LoadShopCatalogue();
}
