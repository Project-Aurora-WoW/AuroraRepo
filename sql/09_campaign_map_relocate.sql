-- Move campaign #1 from map 950 (never created) to map 33 (Shadowfang).
-- Map 950 has no entry in Map.dbc and no geometry, so every spawn on it was
-- skipped and the world could not finish loading.
--
-- Map 33 has real terrain and 216 creatures / 173 gameobjects already, so the
-- campaign actors are placed next to that existing cluster instead of in void.
-- Positions are remapped proportionally into the map-33 bounding box.

UPDATE `creature`
SET `map` = 33,
    `position_x` = `position_x` * 0.15 - 190,
    `position_y` = `position_y` * 0.15 + 2210,
    `position_z` = 80
WHERE `map` = 950;

UPDATE `gameobject`
SET `map` = 33,
    `position_x` = `position_x` * 0.15 - 190,
    `position_y` = `position_y` * 0.15 + 2210,
    `position_z` = 80
WHERE `map` = 950;

UPDATE `aurora_campaign_locations`
SET `hub_map_id` = 33,
    `hub_x` = -190,
    `hub_y` = 2210,
    `hub_z` = 80
WHERE `hub_map_id` = 950;

SELECT 'creatures moved to 33' AS step, COUNT(*) AS n
FROM `creature` WHERE `map` = 33 AND `guid` BETWEEN 900001 AND 900999
UNION ALL
SELECT 'gameobjects moved to 33', COUNT(*)
FROM `gameobject` WHERE `map` = 33 AND `guid` BETWEEN 900101 AND 900999;
