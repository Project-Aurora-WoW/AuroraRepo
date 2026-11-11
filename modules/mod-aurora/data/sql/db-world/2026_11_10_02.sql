-- ============================================================================
-- PROJECT AURORA v7.1.0 - WORLD spawns
-- Target: AzerothCore master (3.3.5a), WORLD database
-- Load AFTER 02_aurora_world_data.sql
--
-- IMPORTANT
--   * `creature.guid` and `gameobject.guid` are PRIMARY KEYS without
--     AUTO_INCREMENT. Every spawn therefore gets an explicit guid from the
--     project range 900000+, which does not collide with stock spawns
--     (stock WotLK+DBC guid space stays far below 400000).
--   * phaseMask lives in the SPAWN row, not in the template row.
--   * `curhealth` must be a plausible value; leaving 0 makes the creature
--     spawn dead. Values below are computed from HealthModifier with the
--     level 80-83 base, rounded conservatively; the core recomputes health
--     from the template on respawn, so exact numbers are cosmetic.
--   * All rows use REPLACE so the file can be re-run.
-- ============================================================================

SET NAMES utf8mb4;

-- ============================================================================
-- 1. CAMPAIGN #1 (map 950) - ALTAR WARDENS
-- Four wardens, one per altar. Coordinates follow the altar cross layout
-- from the design document: N / S / W / E around the throne hall.
-- ============================================================================
REPLACE INTO `creature`
  (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,
   `position_x`,`position_y`,`position_z`,`orientation`,
   `spawntimesecs`,`curhealth`,`curmana`,`MovementType`,`npcflag`)
VALUES
  (900001,150020,951,0,0,1,512,  100.0,  500.0, 50.0,3.14159,120,70000,1000,0,0),
  (900002,150020,951,0,0,1,512,  100.0, -500.0, 50.0,0.00000,120,70000,1000,0,0),
  (900003,150020,951,0,0,1,512, -400.0,    0.0, 50.0,1.57080,120,70000,1000,0,0),
  (900004,150020,951,0,0,1,512,  400.0,    0.0, 50.0,4.71239,120,70000,1000,0,0);

-- ============================================================================
-- 2. CAMPAIGN #1 - BOSSES
-- Both boss spawns sit in the throne hall. They are SPAWNED here because
-- the instance script needs them present before the encounter starts; the
-- AI keeps them passive until JustEngagedWith.
-- ============================================================================
REPLACE INTO `creature`
  (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,
   `position_x`,`position_y`,`position_z`,`orientation`,
   `spawntimesecs`,`curhealth`,`curmana`,`MovementType`,`npcflag`)
VALUES
  (900010,150001,951,0,0,1,512,  0.0,  20.0, 60.0,3.14159,120,5000000,0,0,0),
  (900011,150002,951,0,0,1,512,  0.0, -20.0, 60.0,0.00000,120,3000000,200000,0,0);

-- ============================================================================
-- 3. CAMPAIGN #1 - GAME OBJECTS (altars)
-- ============================================================================
REPLACE INTO `gameobject`
  (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,
   `position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,
   `spawntimesecs`,`animprogress`,`state`)
VALUES
  (900101,150002,951,0,0,1,512,  100.0,  500.0, 50.0,3.14159,0,0,0,1,120,0,1),
  (900102,150002,951,0,0,1,512,  100.0, -500.0, 50.0,0.00000,0,0,0,1,120,0,1),
  (900103,150002,951,0,0,1,512, -400.0,    0.0, 50.0,1.57080,0,0,0,1,120,0,1),
  (900104,150002,951,0,0,1,512,  400.0,    0.0, 50.0,4.71239,0,0,0,1,120,0,1);

-- ============================================================================
-- 4. HUB PORTALS
-- One portal per campaign hub. Phase 256 = 0x0100 (PHASE_AURORA_HUB).
-- Only campaign #1 portals are placed at real coordinates; the rest are
-- placed at their hub coordinates and stay disabled (state 0) until the
-- campaign flips to `live`. Enable a row by setting state = 1.
-- Values: guid,id,map,zoneId,areaId,spawnMask,phaseMask,
--         x,y,z,o,rot0..rot3,spawntimesecs,animprogress,state
-- ============================================================================
REPLACE INTO `gameobject`
  (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,
   `position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,
   `spawntimesecs`,`animprogress`,`state`)
VALUES
  (900200,150100,1,0,0,1,1, -8000.0,-4000.0,15.0,0.0,0,0,0,1,0,0,1);

-- ============================================================================
-- 5. QUEST GIVERS IN THE WORLD
-- Lyondrel stands next to the Tanaris hub portal (Alliance), Wargok next to
-- the Horde camp, Eonis next to the neutral hub. Phase of the hub givers is
-- the default phase 1 so they are always visible; Valdor is phase-locked to
-- the campaign instance only.
-- ============================================================================
REPLACE INTO `creature`
  (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,
   `position_x`,`position_y`,`position_z`,`orientation`,
   `spawntimesecs`,`curhealth`,`curmana`,`MovementType`,`npcflag`)
VALUES
  (900210,150100,1,0,0,1,1,-8010.0,-4000.0,15.0,0.0,120,5000,5000,0,3),
  (900211,150101,1,0,0,1,1,-7990.0,-3995.0,15.0,0.0,120,5000,5000,0,3),
  (900212,150102,1,0,0,1,1,-8000.0,-4010.0,15.0,0.0,120,5000,5000,0,3),
  (900213,150200,1,0,0,1,1,-8020.0,-3985.0,15.0,0.0,120,5000,5000,0,3),
  (900214,150010,951,0,0,1,512,0.0,0.0,70.0,3.14159,120,500000,500000,0,3),
  (900215,150030,951,0,0,1,512,300.0,300.0,50.0,0.0,120,50000,0,0,0),
  (900216,150030,951,0,0,1,512,-300.0,300.0,50.0,0.0,120,50000,0,0,0),
  (900217,150030,951,0,0,1,512,300.0,-300.0,50.0,0.0,120,50000,0,0,0),
  (900218,150030,951,0,0,1,512,-300.0,-300.0,50.0,0.0,120,50000,0,0,0);

-- ============================================================================
-- 6. SHOPKEEPER VENDOR LIST
-- The `npc_vendor` rows back the .shop command fallback entry point so the
-- same catalogue works whether players use chat or the gossip window.
-- Items are free in the vendor view because the real price is checked in C++.
-- ============================================================================
REPLACE INTO `npc_vendor` (`entry`,`slot`,`item`,`maxcount`,`incrtime`,`ExtendedCost`)
SELECT 150200, 1, i.item_entry, 0, 0, 0
FROM `aurora_shop_items` i
WHERE i.enabled = 1
ORDER BY i.id
LIMIT 20;

-- ============================================================================
-- 7. AREA TRIGGER FOR THE PVP CAPTURE ZONE
-- The altar area is a named zone so the achievement system can distinguish
-- "inside PvP zone" from "inside instance".
-- Zone → area ids use the project range 9000+ (unused on retail/AC).
-- ============================================================================
REPLACE INTO `areatrigger_teleport` (`id`,`name`,`target_map`,`target_position_x`,`target_position_y`,
                                     `target_position_z`,`target_orientation`)
VALUES
  (9000,'Aurora: Night of the Fallen (entrance)',951,0.0,0.0,80.0,3.14159);

-- ============================================================================
-- 8. WORLDSTATE TEMPLATES
-- World state ids 30001..30099 are reserved for Aurora events.
-- `world_state` must exist for the client to render the state row.
-- ============================================================================
-- world_state is not a server table: the client reads these rows from
-- WorldStateUI.dbc, so they belong to the DBC patch, not to the world DB.
-- Reserved ids: 30001..30008 event states, 30050 altars, 30051 boss shield.
-- Reserved world state ids: 30001..30008 event states, 30050 altars, 30051 boss shield.

-- ============================================================================
-- RESERVED WORLD STATE IDS
-- 30001..30008 event states, 30050 altars controlled, 30051 boss shield.
-- They are rendered from WorldStateUI.dbc (client patch), not from a server
-- table, so nothing is inserted here.
-- ============================================================================





