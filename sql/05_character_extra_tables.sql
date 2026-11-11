-- ============================================================================
-- PROJECT AURORA v7.1.0 - missing tables referenced from C++
-- Target: CHARACTER database (MySQL 8.0+)
--
-- Apply AFTER 01_aurora_schema.sql. Idempotent: CREATE TABLE IF NOT EXISTS.
--
-- These three tables are written by the server module but were absent from
-- the original schema, which would make the corresponding features fail at
-- runtime:
--   * aurora_shop_purchases   - .aurora shop buy history / max_per_player
--   * aurora_vote_settlement  - guard against double weekly settlement
--   * aurora_pending_grants   - titles/visual enchants owed to a player who
--                               was offline when the vote was settled
--   * aurora_server_activity  - per-day total activity, used by the
--                               "activity" schedule type of dynamic events
-- ============================================================================

SET NAMES utf8mb4;

-- ----------------------------------------------------------------------------
-- 1. Shop purchases
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_shop_purchases` (
  `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `player_guid`  INT UNSIGNED NOT NULL,
  `shop_id`      INT UNSIGNED NOT NULL,
  `item_entry`   INT UNSIGNED NOT NULL,
  `amount`       INT UNSIGNED NOT NULL DEFAULT 1,
  `purchased_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_player_shop` (`player_guid`, `shop_id`),
  KEY `idx_time` (`purchased_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora shop purchase log';

-- ----------------------------------------------------------------------------
-- 2. Vote settlement guard - one row per settled period
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_vote_settlement` (
  `id`             INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `settled_period` VARCHAR(16) NOT NULL COMMENT 'YYYY-MM',
  `settled_at`     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_period` (`settled_period`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Guard against double weekly payout';

-- ----------------------------------------------------------------------------
-- 3. Pending grants - titles and visual enchants for offline winners
--    A mail item can carry items, money and text, but not a title id.
--    The grant is written here at settlement and applied on next login.
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_pending_grants` (
  `id`                INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `player_guid`       INT UNSIGNED NOT NULL,
  `title_dbc_id`      INT UNSIGNED NOT NULL DEFAULT 0,
  `visual_enchant_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `created_at`        TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `applied_at`        TIMESTAMP NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_unapplied` (`player_guid`, `applied_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Titles owed to a player, applied on login';

-- ----------------------------------------------------------------------------
-- 4. Server activity per day
--    Read by aurora_event_manager.cpp for schedule_type = 'activity'.
--    Written from C++ every hour (aurora_voting_engine or a dedicated tick).
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_server_activity` (
  `date`                  DATE NOT NULL,
  `peak_online`           INT UNSIGNED NOT NULL DEFAULT 0,
  `total_activity_score`  DECIMAL(15,2) NOT NULL DEFAULT 0,
  `eclipse_triggered`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `eclipse_time`          TIMESTAMP NULL DEFAULT NULL,
  PRIMARY KEY (`date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Daily server activity, drives dynamic events';

-- ----------------------------------------------------------------------------
-- 5. Per-player campaign counters
--    Lifetime totals that do not fit the activity ledger, e.g. how many
--    altars a character has captured across all runs (achievement 10020).
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_campaign_counters` (
  `player_guid` INT UNSIGNED NOT NULL,
  `counter`     VARCHAR(32) NOT NULL,
  `value`       INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`player_guid`, `counter`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora campaign lifetime counters';
