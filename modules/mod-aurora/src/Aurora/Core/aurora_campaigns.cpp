// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_campaigns.cpp - campaign and quest chain registry.
//
// Two sources are involved and both are needed:
//   * `aurora_campaigns` in the world DB describes the campaign roster
//     (id, name, map, type, status). Gameplay reads it at runtime.
//   * this file carries the compile-time links between a campaign id and the
//     code that actually implements it: instance script, quest chains and
//     the supported map. A campaign row in the DB whose id has no entry here
//     is playable only as data (no instance, no quests).
//
// Quest chains are declared explicitly for the same reason: `RewardNextQuest`
// in `quest_template` can break silently, while the chain list also drives
// `.aurora campaign`, the achievement checks and the gossip givers.
// ============================================================================

#include "DatabaseEnv.h"
#include "Log.h"
#include "StringFormat.h"

#include "aurora_shared.h"

namespace Aurora
{
    namespace
    {
        // One ordered quest chain belonging to a campaign.
        struct QuestChain
        {
            uint8       faction;        // 0 = Alliance, 1 = Horde, 2 = neutral
            uint32      first;
            uint32      last;
            char const* labelRu;
            char const* labelEn;
        };

        struct CampaignEntry
        {
            uint32              id;
            char const*         internalName;
            uint32              mapId;
            bool                scripted;      // instance script exists in this module
            std::vector<QuestChain> chains;
        };

        std::vector<CampaignEntry> const& Registry()
        {
            static std::vector<CampaignEntry> const s_registry =
            {
                { 1, "night_of_the_fallen", MAP_NIGHT_OF_THE_FALLEN, true,
                  {
                      { 0, QUEST_ALLIANCE_1, QUEST_ALLIANCE_6, "Путь Света",     "Path of Light" },
                      { 1, QUEST_HORDE_1,    QUEST_HORDE_6,    "Путь Крови",     "Path of Blood" },
                      { 2, QUEST_NEUTRAL_1,  QUEST_NEUTRAL_6,  "Путь Равновесия", "Path of Balance" }
                  }
                }
                // Campaigns 2..17 exist only as `aurora_campaigns` rows: no map,
                // no instance script and no quest data yet. They are deliberately
                // absent so IsCampaignPlayable() answers false for them.
            };

            return s_registry;
        }
    }

    CampaignEntry const* FindCampaign(uint32 campaignId)
    {
        for (CampaignEntry const& entry : Registry())
            if (entry.id == campaignId)
                return &entry;

        return nullptr;
    }

    bool IsCampaignPlayable(uint32 campaignId)
    {
        CampaignEntry const* entry = FindCampaign(campaignId);
        if (!entry || !entry->scripted)
            return false;

        return IsCampaignEnabled(campaignId);
    }

    std::vector<QuestChain> const* GetCampaignChains(uint32 campaignId)
    {
        CampaignEntry const* entry = FindCampaign(campaignId);
        return entry ? &entry->chains : nullptr;
    }

    uint32 CountCampaignQuests(uint32 campaignId)
    {
        std::vector<QuestChain> const* chains = GetCampaignChains(campaignId);
        if (!chains)
            return 0;

        uint32 total = 0;
        for (QuestChain const& chain : *chains)
            total += chain.last - chain.first + 1;

        return total;
    }

    // Startup check: every campaign row marked 'live' in the DB must be
    // backed by code, otherwise players see an entry that does nothing.
    void VerifyCampaignRegistry()
    {
        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `internal_name`, `status` FROM `aurora_campaigns` "
            "WHERE `status` = 'live'");

        if (!result)
        {
            LOG_ERROR("module", "[Aurora] No live campaign rows - registry check skipped.");
            return;
        }

        do
        {
            Field* fields = result->Fetch();
            uint32 const id = fields[0].Get<uint32>();
            std::string const name = fields[1].Get<std::string>();

            CampaignEntry const* entry = FindCampaign(id);
            if (!entry)
            {
                LOG_ERROR("module", "[Aurora] Campaign #{} '{}' is live in the DB but has "
                                    "no code registry entry - it will not be playable.",
                          id, name);
                continue;
            }

            LOG_INFO("module", "[Aurora] Campaign #{} '{}': map {}, {} quests, script {}.",
                     id, name, entry->mapId, CountCampaignQuests(id),
                     entry->scripted ? "present" : "missing");
        } while (result->NextRow());
    }
}
