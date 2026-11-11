/*
 * PROJECT AURORA v7.1.0 - script loader for the Aurora module.
 *
 * AzerothCore generates a call to Add<Directory>Scripts() for every
 * directory under src/server/scripts. This file provides AddAuroraScripts().
 */

void AddSC_aurora_bootstrap();
void AddSC_aurora_commandscript();
void AddSC_aurora_activity_bridge();
void AddSC_aurora_event_manager();
void AddSC_aurora_online_rewards();
void AddSC_aurora_daily_streak();
void AddSC_aurora_voting_engine();
void AddSC_aurora_visual_enchants();
void AddSC_aurora_lore();
void AddSC_aurora_daily_quests();
void AddSC_night_of_the_fallen();
void AddSC_boss_malakor();
void AddSC_boss_vespera();
void AddSC_npc_twilight_warden();
void AddSC_go_shadow_altar();
void AddSC_go_aurora_portal();
void AddSC_npc_campaign01_questgivers();
void AddSC_aurora_campaign01_achievements();
void AddSC_npc_valdor_questgiver();
void AddSC_npc_aurora_shopkeeper();
void AddSC_npc_shadow_minion();

void AddAuroraScripts()
{
    // Core systems
    AddSC_aurora_bootstrap();
    AddSC_aurora_commandscript();
    AddSC_aurora_activity_bridge();
    AddSC_aurora_event_manager();
    AddSC_aurora_online_rewards();
    AddSC_aurora_daily_streak();
    AddSC_aurora_voting_engine();
    AddSC_aurora_visual_enchants();
    AddSC_aurora_lore();

    // Campaign #1 - Night of the Fallen
    AddSC_night_of_the_fallen();
    AddSC_boss_malakor();
    AddSC_boss_vespera();
    AddSC_npc_twilight_warden();
    AddSC_go_shadow_altar();
    AddSC_go_aurora_portal();
    AddSC_npc_campaign01_questgivers();
    AddSC_npc_valdor_questgiver();
    AddSC_npc_aurora_shopkeeper();
    AddSC_npc_shadow_minion();
    AddSC_aurora_campaign01_achievements();
}

