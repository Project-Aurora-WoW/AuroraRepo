-- Move campaign #1 from map 950 to the new map 951 (Night of the Fallen - Depths).
-- Map 951 is a fresh clone: Map.dbc + MapDifficulty.dbc rows and cloned
-- maps/vmaps tiles now exist, so the core accepts spawns there.

UPDATE `creature`
SET `map` = 951
WHERE `map` = 950 AND `guid` BETWEEN 900001 AND 900999;

UPDATE `gameobject`
SET `map` = 951
WHERE `map` = 950 AND `guid` BETWEEN 900101 AND 900999;

UPDATE `aurora_campaign_locations`
SET `hub_map_id` = 951
WHERE `hub_map_id` = 950;

INSERT INTO `creature_queststarter` (`id`, `quest`) VALUES
  (150100, 90001)
ON DUPLICATE KEY UPDATE `quest` = VALUES(`quest`);

SELECT 'creatures on 951' AS step, COUNT(*) AS n FROM `creature` WHERE `map` = 951
UNION ALL
SELECT 'gameobjects on 951', COUNT(*) FROM `gameobject` WHERE `map` = 951
UNION ALL
SELECT 'creatures left on 950', COUNT(*) FROM `creature` WHERE `map` = 950;
