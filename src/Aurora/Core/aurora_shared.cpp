// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_shared.cpp - non-inline helpers from aurora_shared.h
// ============================================================================

#include "aurora_shared.h"
#include "Log.h"

namespace Aurora
{
    char const* GetVersion()
    {
        return AURORA_VERSION_STRING;
    }

    char const* GetInternalName()
    {
        return "aurora";
    }

    bool IsAuroraMap(uint32 mapId)
    {
        return mapId >= AURORA_MAP_ID_START && mapId <= AURORA_MAP_ID_END;
    }

    bool IsAuroraMapRange(uint32 mapId)
    {
        return IsAuroraMap(mapId);
    }

    bool IsAuroraItem(uint32 entry)
    {
        return entry >= AURORA_ITEM_ID_START && entry <= AURORA_ITEM_ID_END;
    }

    bool IsAuroraNpc(uint32 entry)
    {
        return entry >= AURORA_NPC_ID_START && entry <= AURORA_NPC_ID_END;
    }

    bool IsAuroraQuest(uint32 id)
    {
        return id >= AURORA_QUEST_ID_START && id <= AURORA_QUEST_ID_END;
    }

    bool IsAuroraAchievement(uint32 id)
    {
        return (id >= AURORA_ACHIEVEMENT_ID_START && id <= AURORA_ACHIEVEMENT_ID_END)
            || (id >= 20001 && id <= 20099);
    }

    std::string const& GetCampaignName(uint32 campaignId, std::string const& locale)
    {
        // Static storage keeps the reference valid; the table is small and
        // immutable for the lifetime of the process.
        static std::string s_empty;
        static bool s_loaded = false;
        static std::unordered_map<uint32, std::pair<std::string, std::string>> s_names;

        if (!s_loaded)
        {
            s_loaded = true;
            QueryResult result = WorldDatabase.Query(
                "SELECT `id`, `display_name_ru`, `display_name_en` FROM `aurora_campaigns`");

            if (result)
            {
                do
                {
                    Field* fields = result->Fetch();
                    s_names[fields[0].Get<uint32>()] =
                        std::make_pair(fields[1].Get<std::string>(), fields[2].Get<std::string>());
                } while (result->NextRow());
            }
        }

        auto itr = s_names.find(campaignId);
        if (itr == s_names.end())
            return s_empty;

        return (locale == "ruRU") ? itr->second.first : itr->second.second;
    }
}
