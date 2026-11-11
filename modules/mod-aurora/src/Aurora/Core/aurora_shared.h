// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_shared.h - enums, ids and small helpers shared by every Aurora module.
// AzerothCore master (3.3.5a).
// ============================================================================

#ifndef AURORA_SHARED_H
#define AURORA_SHARED_H

#include "aurora_config.h"
#include "Define.h"
#include <string>

namespace Aurora
{
    // ------------------------------------------------------------------------
    // Maps
    // ------------------------------------------------------------------------
    enum Maps : uint32
    {
        MAP_NIGHT_OF_THE_FALLEN = 950,
        MAP_DAWN_OF_ASH         = 951,
        MAP_AURORA_HUB_TANARIS  = 1
    };

    // ------------------------------------------------------------------------
    // Creatures
    // ------------------------------------------------------------------------
    enum Creatures : uint32
    {
        NPC_MALAKOR_FALLEN_KING  = 150001,
        NPC_LADY_VESPERA         = 150002,
        NPC_ARCHMAGE_VALDOR      = 150010,
        NPC_TWILIGHT_WARDEN      = 150020,
        NPC_SHADOW_MINION        = 150030,
        NPC_COMMANDER_LYONDREL   = 150100,
        NPC_WARGOK_BLOODFANG     = 150101,
        NPC_WANDERER_EONIS       = 150102,
        NPC_AURORA_SHOPKEEPER    = 150200
    };

    // ------------------------------------------------------------------------
    // GameObjects
    // ------------------------------------------------------------------------
    enum GameObjects : uint32
    {
        GO_SHADOW_ALTAR      = 150002,
        GO_AURORA_PORTAL     = 150100
    };

    // ------------------------------------------------------------------------
    // Items
    // ------------------------------------------------------------------------
    enum Items : uint32
    {
        ITEM_SHARD_OF_FALLEN_STAR   = 50001,
        ITEM_AURORA_POINTS          = 50100,
        ITEM_BEACON_OF_LIGHT        = 50010,
        ITEM_DARK_TOTEM             = 50011,
        ITEM_MEDALLION_OF_NIGHT     = 50020,
        ITEM_BLOOD_TOTEM_OF_NIGHT   = 50021,
        ITEM_WANDERER_CLOAK         = 50022,
        ITEM_HEART_OF_NIGHT         = 50023,
        ITEM_ALLIANCE_FALLEN_CHEST  = 50030,
        ITEM_HORDE_FALLEN_CHEST     = 50031,
        ITEM_AURORA_CHEST           = 50032,
        ITEM_STAFF_OF_EONIS         = 50040,
        ITEM_DAWN_WARHORSE          = 50050,
        ITEM_TWILIGHT_PACK_WOLF     = 50051,
        ITEM_SHADOW_WHELPLING       = 50060,
        ITEM_TWILIGHT_TORCH         = 50061,
        ITEM_BLADE_OF_AURORA        = 50200,
        ITEM_PHANTOM_OF_AURORA      = 50201,
        ITEM_PRISMATIC_PHANTOM      = 50202,
        ITEM_MASK_OF_EONIS          = 50203,
        ITEM_BANNER_OF_NIGHT        = 50204,
        ITEM_SHADOW_GONG_GLOVES     = 50205
    };

    // ------------------------------------------------------------------------
    // Quests
    // ------------------------------------------------------------------------
    enum Quests : uint32
    {
        QUEST_ALLIANCE_1 = 90001, QUEST_ALLIANCE_2 = 90002, QUEST_ALLIANCE_3 = 90003,
        QUEST_ALLIANCE_4 = 90004, QUEST_ALLIANCE_5 = 90005, QUEST_ALLIANCE_6 = 90006,

        QUEST_HORDE_1    = 90011, QUEST_HORDE_2    = 90012, QUEST_HORDE_3    = 90013,
        QUEST_HORDE_4    = 90014, QUEST_HORDE_5    = 90015, QUEST_HORDE_6    = 90016,

        QUEST_NEUTRAL_1  = 90021, QUEST_NEUTRAL_2  = 90022, QUEST_NEUTRAL_3  = 90023,
        QUEST_NEUTRAL_4  = 90024, QUEST_NEUTRAL_5  = 90025, QUEST_NEUTRAL_6  = 90026
    };

    // ------------------------------------------------------------------------
    // Achievements (ids must exist in the patched client Achievement.dbc)
    // ------------------------------------------------------------------------
    enum Achievements : uint32
    {
        ACH_FIRST_STEP             = 10001,
        ACH_MALAKOR_10             = 10002,
        ACH_MALAKOR_25             = 10003,
        ACH_NO_DEATHS              = 10004,
        ACH_SHIELD_BROKER          = 10010,
        ACH_SHADOW_HUNTER          = 10011,
        ACH_SHADOW_GONG            = 10012,
        ACH_FASTER_THAN_DARK       = 10013,
        ACH_SOLO_HERO              = 10014,
        ACH_ALTAR_CAPTURER         = 10020,
        ACH_FALLEN_HERO            = 10021,
        ACH_LONE_WOLF              = 10022,
        ACH_LIGHT_DEFENDER         = 10023,
        ACH_DOMINION               = 10024,
        ACH_PATH_OF_LIGHT          = 10030,
        ACH_PATH_OF_BLOOD          = 10031,
        ACH_PATH_OF_BALANCE        = 10032,
        ACH_TRIPLE_AGENT           = 10033,
        ACH_SHARD_COLLECTOR        = 10040,
        ACH_LEGENDARY_HUNTER       = 10041,
        ACH_NIGHT_COLLECTOR        = 10042,
        ACH_FULL_EQUIP             = 10043,
        ACH_HERO_OF_FALLEN_NIGHT   = 10099,
        ACH_AURORA_SEEKER          = 20001,
        ACH_AURORA_KEEPER          = 20002,
        ACH_AURORA_VOICE           = 20003,
        ACH_AURORA_COLLECTOR       = 20004,
        ACH_AURORA_LEGEND          = 20005
    };

    // ------------------------------------------------------------------------
    // Titles (CharTitles.dbc ids)
    // ------------------------------------------------------------------------
    enum Titles : uint32
    {
        TITLE_FEARLESS             = 5001,
        TITLE_FASTER_THAN_SHADOW   = 5002,
        TITLE_LONE_WOLF            = 5003,
        TITLE_TRIPLE_AGENT         = 5004,
        TITLE_HERO_OF_FALLEN_NIGHT = 5005,
        TITLE_AURORA_SEEKER        = 5006,
        TITLE_AURORA_KEEPER        = 5007,
        TITLE_AURORA_VOICE         = 5008,
        TITLE_LOYAL_KEEPER         = 5009,
        TITLE_PVE_HERO             = 5010,
        TITLE_PVP_HERO             = 5011,
        TITLE_SOUL_OF_AURORA       = 5012,
        TITLE_REMEMBRANCER         = 5013,
        TITLE_PVP_RUNNER_UP        = 5014,
        TITLE_PVP_SECOND_RUNNER    = 5015,
        TITLE_HELPER_SOUL          = 5016
    };

    // ------------------------------------------------------------------------
    // Spells used by campaign #1
    // ------------------------------------------------------------------------
    enum Spells : uint32
    {
        SPELL_DARK_SHIELD       = 500011,
        SPELL_SHADOW_BOLT       = 500010,
        SPELL_SUMMON_MINION     = 500012,
        SPELL_VULNERABLE        = 500030,
        SPELL_CAPTURE_CHANNEL   = 500020,
        SPELL_DARKNESS_AURA     = 500001,
        SPELL_ENRAGE_STOCK      = 8599
    };

    // ------------------------------------------------------------------------
    // Instance data keys (must be unique inside one InstanceScript)
    // ------------------------------------------------------------------------
    enum InstanceData : uint32
    {
        DATA_FALLEN_KING              = 1,
        DATA_LADY_VESPERA             = 2,
        DATA_ALTARS_CONTROLLED        = 3,
        DATA_SHIELD_BROKEN_TIME       = 4,
        DATA_CAMPAIGN_START_TIME      = 5,
        DATA_RAID_DEATHS              = 6,
        DATA_ALTARS_CAPTURED_TOTAL    = 7,
        DATA_CAPTURES_SOLO            = 8,
        DATA_CAPTURES_DEFENDED        = 9,
        DATA_PVP_KILLS                = 10,
        DATA_MAX_ALTARS_IN_RUN        = 11
    };

    // ------------------------------------------------------------------------
    // Vote categories - keep in sync with the ENUM in the DB
    // ------------------------------------------------------------------------
    enum VoteCategory : uint8
    {
        VOTE_PVE     = 0,
        VOTE_PVP     = 1,
        VOTE_HELPER  = 2,
        VOTE_MONTHLY = 3,
        VOTE_MAX     = 4
    };

    enum class ActivityType : uint8
    {
        CreatureKill = 0,
        PlayerKill,
        QuestComplete,
        AchievementEarn,
        ItemLoot,
        ItemCraft,
        MailSend,
        TradeComplete,
        DungeonEnter,
        RaidEnter,
        BossKill,
        ArenaGame,
        BattlegroundGame,
        HonorEarn,
        AuctionBid,
        AuctionSale,
        PlaytimeMinute,
        Max
    };

    char const* GetVersion();
    char const* GetInternalName();
    bool IsAuroraMap(uint32 mapId);
    bool IsAuroraItem(uint32 entry);
    bool IsAuroraNpc(uint32 entry);
    bool IsAuroraQuest(uint32 id);
    bool IsAuroraAchievement(uint32 id);
    bool IsAuroraMapRange(uint32 mapId);

    std::string const& GetCampaignName(uint32 campaignId, std::string const& locale);

    // ------------------------------------------------------------------------
    // Runtime settings, read from worldserver.conf once per process.
    // See aurora_settings.cpp for the keys and their defaults.
    // ------------------------------------------------------------------------
    void LoadSettings();
    bool IsProjectEnabled();
    bool IsCampaignEnabled(uint32 campaignId);

    // ------------------------------------------------------------------------
    // Campaign and quest chain registry, see aurora_campaigns.cpp
    // ------------------------------------------------------------------------
    struct QuestChain
    {
        uint8       faction;   // 0 = Alliance, 1 = Horde, 2 = neutral
        uint32      first;
        uint32      last;
        char const* labelRu;
        char const* labelEn;
    };

    struct CampaignEntry
    {
        uint32                   id;
        char const*              internalName;
        uint32                   mapId;
        bool                     scripted;
        std::vector<QuestChain>  chains;
    };

    CampaignEntry const* FindCampaign(uint32 campaignId);
    bool IsCampaignPlayable(uint32 campaignId);
    std::vector<QuestChain> const* GetCampaignChains(uint32 campaignId);
    uint32 CountCampaignQuests(uint32 campaignId);
    void VerifyCampaignRegistry();
    bool IsShopEnabled();
    bool IsVotingEnabled();
    bool IsEventsEnabled();
    bool IsStreakEnabled();
    std::string const& GetDefaultLocale();
}

#endif // AURORA_SHARED_H
