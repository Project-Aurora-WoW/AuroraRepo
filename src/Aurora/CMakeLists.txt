# ============================================================================
# PROJECT AURORA v7.1.0 - CMake
# Drop this folder into: src/server/scripts/Aurora/
#
# IMPORTANT: the stock AzerothCore scripts/CMakeLists.txt already builds a
# single `scripts` target from a GLOB over the whole scripts/ directory.
# Creating a second `scripts` library here would collide with it, so this
# file only declares the sources and exports them as a list variable that
# the parent CMakeLists can append to. If your tree does not glob, use the
# block at the bottom (commented out) instead.
# ============================================================================

set(AURORA_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/aurora_shared.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/aurora_bootstrap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/aurora_commands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/aurora_activity_bridge.cpp
)

set(AURORA_CAMPAIGN01_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Campaigns/Campaign01_NightOfTheFallen/campaign01.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Campaigns/Campaign01_NightOfTheFallen/night_of_the_fallen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Campaigns/Campaign01_NightOfTheFallen/boss_malakor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Campaigns/Campaign01_NightOfTheFallen/boss_vespera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Campaigns/Campaign01_NightOfTheFallen/npc_twilight_warden.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Campaigns/Campaign01_NightOfTheFallen/go_shadow_altar.cpp
)

set(AURORA_SOURCES ${AURORA_CORE_SOURCES} ${AURORA_CAMPAIGN01_SOURCES})

# Headers are listed for IDE navigation only.
set(AURORA_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/aurora_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/aurora_shared.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Campaigns/Campaign01_NightOfTheFallen/campaign01.h
)

# Export to the parent scope so scripts/CMakeLists.txt can do:
#   include(${CMAKE_CURRENT_SOURCE_DIR}/Aurora/CMakeLists.txt)
#   target_sources(scripts PRIVATE ${AURORA_SOURCES})
set(AURORA_SOURCES ${AURORA_SOURCES} PARENT_SCOPE)

message(STATUS "[AURORA] ${AURORA_SOURCES}")

# ============================================================================
# FALLBACK - only if your scripts/CMakeLists.txt does NOT glob sources and
# does not define a `scripts` target of its own. Uncomment and remove the
# PARENT_SCOPE export above.
# ============================================================================
# add_library(aurora_scripts STATIC ${AURORA_SOURCES})
# target_include_directories(aurora_scripts PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/Core
#     ${CMAKE_CURRENT_SOURCE_DIR}/Campaigns/Campaign01_NightOfTheFallen
# )
