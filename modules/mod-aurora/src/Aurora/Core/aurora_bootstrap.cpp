// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_bootstrap.cpp - world startup/shutdown glue for the command layer.
//
// Responsibilities
//   * verify the Aurora tables exist before any command runs;
//   * load the shop catalogue so `.shop` answers immediately;
//   * log the campaign status so operators see #1 as live;
//   * fail loudly (but do not crash) when the SQL from ./sql is missing.
// ============================================================================

#include "ScriptMgr.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "StringFormat.h"

#include "aurora_shared.h"

using namespace Aurora;

// Defined in aurora_commands.cpp
void AuroraLoadShopCatalogue();

// Defined in npc_campaign01_questgivers.cpp
void AuroraLoadGossipTexts();

namespace
{
    struct TableCheck
    {
        char const* db;
        char const* table;
    };

    uint32 CountRows(char const* query)
    {
        QueryResult result = WorldDatabase.Query(query);
        if (!result)
            return 0;

        return result->Fetch()[0].Get<uint32>();
    }
}

class aurora_bootstrap : public WorldScript
{
public:
    aurora_bootstrap() : WorldScript("aurora_bootstrap") { }

    void OnStartup() override
    {
        LoadSettings();

        LOG_INFO("module", " ");
        LOG_INFO("module", "+--------------------------------------------------+");
        LOG_INFO("module", "|  PROJECT AURORA - bootstrap                |");
        LOG_INFO("module", "|  Campaign #1: Night of the Fallen (map 950)      |");
        LOG_INFO("module", "+--------------------------------------------------+");

        if (!IsProjectEnabled())
        {
            LOG_INFO("module", "[Aurora] Disabled by configuration (Aurora.Enable = 0). "
                               "Tables are not touched.");
            return;
        }

        bool ok = true;

        uint32 campaigns = CountRows("SELECT COUNT(*) FROM `aurora_campaigns`");
        if (!campaigns)
        {
            LOG_ERROR("module", "[Aurora] Table `aurora_campaigns` is missing or empty. "
                                "Apply sql/01_aurora_schema.sql and sql/02_aurora_world_data.sql.");
            ok = false;
        }
        else
        {
            LOG_INFO("module", "[Aurora] Campaigns registered: {}", campaigns);
        }

        uint32 events = CountRows("SELECT COUNT(*) FROM `aurora_events` WHERE `enabled` = 1");
        if (!events)
        {
            LOG_ERROR("module", "[Aurora] No enabled rows in `aurora_events`.");
            ok = false;
        }
        else
        {
            LOG_INFO("module", "[Aurora] Enabled events: {}", events);
        }

        uint32 shop = CountRows("SELECT COUNT(*) FROM `aurora_shop_items` WHERE `enabled` = 1");
        LOG_INFO("module", "[Aurora] Shop offers: {}", shop);

        uint32 quests = CountRows("SELECT COUNT(*) FROM `quest_template` WHERE `ID` BETWEEN 90001 AND 90999");
        LOG_INFO("module", "[Aurora] Quest templates: {}", quests);

        uint32 achs = CountRows("SELECT COUNT(*) FROM `aurora_achievement_criteria`");
        LOG_INFO("module", "[Aurora] Achievement rules: {} (client DBC patch required)", achs);

        if (!ok)
        {
            LOG_ERROR("module", "[Aurora] Bootstrap finished with errors - commands will still load, "
                                "but most features will no-op.");
            return;
        }

        AuroraLoadShopCatalogue();
        AuroraLoadGossipTexts();

        QueryResult live = WorldDatabase.Query(
            "SELECT `internal_name` FROM `aurora_campaigns` WHERE `status` = 'live'");

        if (live)
        {
            do
            {
                LOG_INFO("module", "[Aurora] Live campaign: {}",
                         live->Fetch()[0].Get<std::string>());
            } while (live->NextRow());
        }

        LOG_INFO("module", "[Aurora] Bootstrap complete.");
    }
};

void AddSC_aurora_bootstrap()
{
    new aurora_bootstrap();
}
