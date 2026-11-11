-- Map 950 is now a dungeon-type map (cloned from 33), so it only accepts
-- spawn masks matching raid/dungeon modes. Spawns were left with mask 1
-- (open world), which the core rejects as "wrong spawn mask".
-- Sets the mask to 3 (normal + heroic) for the campaign actors on map 950.

UPDATE `creature`
SET `spawnMask` = 3
WHERE `map` = 950 AND `guid` BETWEEN 900001 AND 900999;

UPDATE `gameobject`
SET `spawnMask` = 3
WHERE `map` = 950 AND `guid` BETWEEN 900101 AND 900999;

SELECT 'creatures fixed' AS step, COUNT(*) AS n
FROM `creature` WHERE `map` = 950 AND `spawnMask` = 3
UNION ALL
SELECT 'gameobjects fixed', COUNT(*)
FROM `gameobject` WHERE `map` = 950 AND `spawnMask` = 3;
