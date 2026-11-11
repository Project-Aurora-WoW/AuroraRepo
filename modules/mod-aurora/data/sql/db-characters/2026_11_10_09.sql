-- ============================================================================
-- PROJECT AURORA v7.1.0 - CHARACTER database
-- Daily commission state for aurora_daily_quests.cpp.
-- Load AFTER 2026_11_10_08.sql.
-- ============================================================================

SET NAMES utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_daily_quests` (
  `player_guid`   INT UNSIGNED NOT NULL,
  `quest_date`    DATE NOT NULL,
  `slot`          TINYINT UNSIGNED NOT NULL COMMENT '0..2',
  `quest_id`      INT UNSIGNED NOT NULL,
  `progress`      INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'counter value seen when the commission was taken',
  `completed_at`  TIMESTAMP NULL DEFAULT NULL,
  PRIMARY KEY (`player_guid`, `quest_date`, `slot`),
  KEY `idx_pending` (`player_guid`, `quest_date`, `completed_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora daily commissions per player per day';
