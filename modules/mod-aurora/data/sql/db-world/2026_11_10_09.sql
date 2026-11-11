-- ============================================================================
-- PROJECT AURORA v7.1.0 - WORLD database
-- Daily quest pool for aurora_daily_quests.cpp.
-- Load AFTER 2026_11_10_08.sql.
-- ============================================================================

SET NAMES utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_daily_quest_pool` (
  `id`            INT UNSIGNED NOT NULL,
  `activity`      ENUM('creature_kill','player_kill','quest_complete','boss_kill',
                       'playtime_minute','item_loot') NOT NULL,
  `target`        INT UNSIGNED NOT NULL DEFAULT 1,
  `item_entry`    INT UNSIGNED NOT NULL DEFAULT 0,
  `item_amount`   INT UNSIGNED NOT NULL DEFAULT 1,
  `aurora_points` INT UNSIGNED NOT NULL DEFAULT 0,
  `shards`        INT UNSIGNED NOT NULL DEFAULT 0,
  `title_ru`      VARCHAR(128) NOT NULL,
  `title_en`      VARCHAR(128) NOT NULL,
  `enabled`       TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`),
  KEY `idx_enabled` (`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora daily commission pool';

REPLACE INTO `aurora_daily_quest_pool`
  (`id`,`activity`,`target`,`item_entry`,`item_amount`,`aurora_points`,`shards`,
   `title_ru`,`title_en`,`enabled`)
VALUES
  (1,'creature_kill',40,0,1,5,3,'Очистить окрестности','Clear the outskirts',1),
  (2,'creature_kill',80,0,1,10,5,'Зачистка логова','Lair sweep',1),
  (3,'quest_complete',3,0,1,10,4,'Помочь жителям','Help the locals',1),
  (4,'playtime_minute',60,0,1,0,5,'Час в мире Авроры','An hour in Aurora',1),
  (5,'playtime_minute',120,0,1,15,8,'Два часа верности','Two hours of loyalty',1),
  (6,'player_kill',5,0,1,12,6,'Дуэльный долг','Duelling debt',1),
  (7,'boss_kill',1,0,1,20,10,'Повергнуть босса','Fell a boss',1),
  (8,'item_loot',30,0,1,8,4,'Найти осколки','Find shards',1),
  (9,'creature_kill',15,0,1,4,2,'Лёгкая прогулка','Light stroll',1),
  (10,'quest_complete',1,0,1,6,2,'Первое поручение','First errand',1);
