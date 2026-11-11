// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_config.h - single place for tunables, ids and versioning.
// ============================================================================

#ifndef AURORA_CONFIG_H
#define AURORA_CONFIG_H

#define AURORA_PROJECT_ENABLED      1
#define AURORA_CORE_ENABLED         1
#define AURORA_CAMPAIGN_01_ENABLED  1
#define AURORA_SHOP_ENABLED         1
#define AURORA_VOTING_ENABLED       1

#define AURORA_VERSION_MAJOR        7
#define AURORA_VERSION_MINOR        1
#define AURORA_VERSION_PATCH        0
#define AURORA_VERSION_STRING       "7.1.0"

// ----------------------------------------------------------------------------
// Id ranges. Every generated id MUST live inside its range; the runtime
// self-check in aurora_shared_init.cpp refuses to start otherwise.
// ----------------------------------------------------------------------------
#define AURORA_QUEST_ID_START       90001
#define AURORA_QUEST_ID_END         90999
#define AURORA_NPC_ID_START         150001
#define AURORA_NPC_ID_END           150999
#define AURORA_ITEM_ID_START        50001
#define AURORA_ITEM_ID_END          50999
#define AURORA_ACHIEVEMENT_ID_START 10001
#define AURORA_ACHIEVEMENT_ID_END   29999
#define AURORA_TITLE_ID_START       5001
#define AURORA_TITLE_ID_END         5999
#define AURORA_MAP_ID_START         950
#define AURORA_MAP_ID_END           966
#define AURORA_GUID_START           900000
#define AURORA_GUID_END             999999

// ----------------------------------------------------------------------------
// Tuning
// ----------------------------------------------------------------------------
#define AURORA_MIN_PLAYTIME_SEC     3600    // Activity-First gate: 1 hour
#define AURORA_CANDIDATE_LIMIT      50
#define AURORA_VOTE_COOLDOWN_HOURS  20      // < 24h so timezones do not block a day
#define AURORA_SCORE_TICK_MS        3600000 // hourly recalculation
#define AURORA_VOTE_EXPIRE_DAYS     7
#define AURORA_SHOP_RESCAN_MS       300000  // 5 min
#define AURORA_ACTIVITY_FLUSH_MS    60000   // counters flushed once per minute
#define AURORA_MAX_SHOP_ENTRIES     500
#define AURORA_MAX_ACTIVE_EVENTS    32

// Campaign #1 encounter knobs
#define AURORA_C01_ALTARS_REQUIRED  3
#define AURORA_C01_CAPTURE_TIME_MS  10000
#define AURORA_C01_ENRAGE_MS        600000  // 10 minutes after shield break

#endif // AURORA_CONFIG_H
