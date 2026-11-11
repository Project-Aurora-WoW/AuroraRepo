# ============================================================================
# PROJECT AURORA v7.1.0 - CMake for the mod-aurora module
#
# Placement: <AzerothCore>/modules/mod-aurora/
#
# AzerothCore's modules/CMakeLists.txt includes every subdirectory that has a
# CMakeLists.txt, so this file is picked up automatically - no edit of the
# core tree is required and `git pull` upstream never touches it.
#
# The sources are appended to the existing `scripts` target, which the core
# already links into worldserver. No second library is created, so nothing
# collides with the core's own `scripts` target.
# ============================================================================

set(AURORA_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

set(AURORA_CORE_SOURCES
    ${AURORA_ROOT_DIR}/src/Aurora/aurora_script_loader.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_shared.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_bootstrap.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_commands.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_activity_bridge.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_event_manager.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_event_rewards.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_online_rewards.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_daily_streak.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_voting_engine.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_visual_enchants.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_lore.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_settings.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_daily_quests.cpp
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_campaigns.cpp
)

set(AURORA_CAMPAIGN01_DIR ${AURORA_ROOT_DIR}/src/Aurora/Campaigns/Campaign01_NightOfTheFallen)

set(AURORA_CAMPAIGN01_SOURCES
    ${AURORA_CAMPAIGN01_DIR}/campaign01.cpp
    ${AURORA_CAMPAIGN01_DIR}/night_of_the_fallen.cpp
    ${AURORA_CAMPAIGN01_DIR}/boss_malakor.cpp
    ${AURORA_CAMPAIGN01_DIR}/boss_vespera.cpp
    ${AURORA_CAMPAIGN01_DIR}/npc_twilight_warden.cpp
    ${AURORA_CAMPAIGN01_DIR}/go_shadow_altar.cpp
    ${AURORA_CAMPAIGN01_DIR}/npc_campaign01_questgivers.cpp
    ${AURORA_CAMPAIGN01_DIR}/go_aurora_portal.cpp
    ${AURORA_CAMPAIGN01_DIR}/aurora_campaign01_achievements.cpp
    ${AURORA_CAMPAIGN01_DIR}/aurora_campaign01_progress.cpp
)

set(AURORA_SOURCES ${AURORA_CORE_SOURCES} ${AURORA_CAMPAIGN01_SOURCES})

target_sources(scripts PRIVATE ${AURORA_SOURCES})

target_include_directories(scripts PRIVATE
    ${AURORA_ROOT_DIR}/src/Aurora/Core
    ${AURORA_CAMPAIGN01_DIR}
)

# Headers are listed for IDE navigation only.
target_sources(scripts PRIVATE
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_config.h
    ${AURORA_ROOT_DIR}/src/Aurora/Core/aurora_shared.h
    ${AURORA_CAMPAIGN01_DIR}/campaign01.h
)

message(STATUS "[AURORA] module mod-aurora: ${AURORA_SOURCES}")
