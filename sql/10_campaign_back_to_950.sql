-- Restore campaign #1 spawns onto map 950, which now exists in Map.dbc
-- and has cloned geometry (maps/950*.map, vmaps/950*).
-- Coordinates were remapped when the campaign was temporarily moved to map 33;
-- this reverses that mapping back to the original campaign layout.
--   forward:  x' = x*0.15 - 190,  y' = y*0.15 + 2210,  z' = 80
--   inverse:  x  = (x' + 190)/0.15,  y = (y' - 2210)/0.15

UPDATE `creature`
SET `map` = 950,
    `position_x` = (`position_x` + 190) / 0.15,
    `position_y` = (`position_y` - 2210) / 0.15,
    `position_z` = 50
WHERE `guid` BETWEEN 900001 AND 900999 AND `map` = 33;

UPDATE `gameobject`
SET `map` = 950,
    `position_x` = (`position_x` + 190) / 0.15,
    `position_y` = (`position_y` - 2210) / 0.15,
    `position_z` = 50
WHERE `guid` BETWEEN 900101 AND 900999 AND `map` = 33;

UPDATE `aurora_campaign_locations`
SET `hub_map_id` = 950,
    `hub_x` = 0,
    `hub_y` = 0,
    `hub_z` = 60
WHERE `hub_map_id` = 33;

SELECT 'creatures on 950' AS step, COUNT(*) AS n
FROM `creature` WHERE `map` = 950
UNION ALL
SELECT 'gameobjects on 950', COUNT(*)
FROM `gameobject` WHERE `map` = 950;
