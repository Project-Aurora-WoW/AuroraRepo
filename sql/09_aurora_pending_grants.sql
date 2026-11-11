-- ============================================================================
-- PROJECT AURORA v7.1.0
-- 09_aurora_pending_grants.sql
-- Target: CHARACTER database - load AFTER 05_character_extra_tables.sql
--
-- `aurora_pending_grants` is written by aurora_event_rewards.cpp (rewards for
-- players who were offline when an event ended) and by aurora_commands.cpp
-- (shop purchases that fail to deliver). Rows are applied on the next login by
-- ApplyPendingGrants() in aurora_activity_bridge.cpp and marked with
-- `applied_at`.
--
-- Until this file is applied, every INSERT into the table fails with
-- "Table 'character.aurora_pending_grants' doesn't exist" and offline rewards
-- and failed-delivery refunds are silently lost.
-- ============================================================================

SET NAMES utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_pending_grants` (
  `id`                INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `player_guid`       INT UNSIGNED NOT NULL,
  `title_dbc_id`      INT UNSIGNED NOT NULL DEFAULT 0,
  `visual_enchant_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `item_entry`        INT UNSIGNED NOT NULL DEFAULT 0,
  `item_amount`       INT UNSIGNED NOT NULL DEFAULT 0,
  `aurora_points`     INT UNSIGNED NOT NULL DEFAULT 0,
  `reason`            VARCHAR(64) NOT NULL DEFAULT '' COMMENT 'event / shop_refund / vote',
  `granted_at`        TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `applied_at`        TIMESTAMP NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_pending` (`player_guid`, `applied_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora grants applied on next login';
