-- ============================================================================
-- PROJECT AURORA v7.1.0 - Schema, CHARACTER database
-- Target: AzerothCore master (3.3.5a), MySQL 8.0+
-- Player progress tables only. Apply to the CHARACTER database.
-- ============================================================================

-- SECTION 2. CHARACTER DATABASE
-- ============================================================================

-- ----------------------------------------------------------------------------
-- 2.1. Campaign progress
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_player_progress` (
  `player_guid`         INT UNSIGNED NOT NULL,
  `campaign_id`         INT UNSIGNED NOT NULL,
  `completed_at`        TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `completion_time_sec` INT UNSIGNED NOT NULL DEFAULT 0,
  `difficulty`          TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `group_size`          TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `no_death`            TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`player_guid`, `campaign_id`),
  KEY `idx_campaign` (`campaign_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora campaign completion per character';

-- ----------------------------------------------------------------------------
-- 2.2. Player activity ledger.
-- Split in two tables:
--   * aurora_player_activity       - lifetime totals (one row per player)
--   * aurora_player_activity_period - per-period totals for streak checks
-- Counters are updated only through the C++ bridge; no triggers, no procedures
-- scanning the stock character tables (they would miss most Aurora events).
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_player_activity` (
  `player_guid`          INT UNSIGNED NOT NULL,
  `total_playtime_sec`   BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `sessions_count`       INT UNSIGNED NOT NULL DEFAULT 0,
  `creatures_killed`     INT UNSIGNED NOT NULL DEFAULT 0,
  `players_killed`       INT UNSIGNED NOT NULL DEFAULT 0,
  `deaths_count`         INT UNSIGNED NOT NULL DEFAULT 0,
  `quests_completed`     INT UNSIGNED NOT NULL DEFAULT 0,
  `achievements_earned`  INT UNSIGNED NOT NULL DEFAULT 0,
  `items_looted`         INT UNSIGNED NOT NULL DEFAULT 0,
  `items_crafted`        INT UNSIGNED NOT NULL DEFAULT 0,
  `mails_sent`           INT UNSIGNED NOT NULL DEFAULT 0,
  `trades_completed`     INT UNSIGNED NOT NULL DEFAULT 0,
  `auction_bids`         INT UNSIGNED NOT NULL DEFAULT 0,
  `auction_sales`        INT UNSIGNED NOT NULL DEFAULT 0,
  `honor_earned`         INT UNSIGNED NOT NULL DEFAULT 0,
  `arena_games_played`   INT UNSIGNED NOT NULL DEFAULT 0,
  `arena_wins`           INT UNSIGNED NOT NULL DEFAULT 0,
  `battlegrounds_played` INT UNSIGNED NOT NULL DEFAULT 0,
  `battleground_wins`    INT UNSIGNED NOT NULL DEFAULT 0,
  `dungeons_entered`     INT UNSIGNED NOT NULL DEFAULT 0,
  `raids_entered`        INT UNSIGNED NOT NULL DEFAULT 0,
  `bosses_killed`        INT UNSIGNED NOT NULL DEFAULT 0,
  `aurora_points`        INT UNSIGNED NOT NULL DEFAULT 0,
  `shards`               INT UNSIGNED NOT NULL DEFAULT 0,
  `pve_score`            DECIMAL(12,2) NOT NULL DEFAULT 0,
  `pvp_score`            DECIMAL(12,2) NOT NULL DEFAULT 0,
  `social_score`         DECIMAL(12,2) NOT NULL DEFAULT 0,
  `total_score`          DECIMAL(12,2) NOT NULL DEFAULT 0,
  `activity_period`      DATE NOT NULL DEFAULT '2000-01-01',
  `last_update`          TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`player_guid`),
  KEY `idx_scores` (`pve_score`, `pvp_score`, `social_score`, `total_score`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Lifetime activity per character';

CREATE TABLE IF NOT EXISTS `aurora_player_activity_period` (
  `player_guid`        INT UNSIGNED NOT NULL,
  `period_start`       DATE NOT NULL COMMENT 'Monday of ISO week',
  `playtime_sec`       BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `creatures_killed`   INT UNSIGNED NOT NULL DEFAULT 0,
  `players_killed`     INT UNSIGNED NOT NULL DEFAULT 0,
  `quests_completed`   INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`player_guid`, `period_start`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Weekly rollup used for streak and vote eligibility';

-- ----------------------------------------------------------------------------
-- 2.3. Online rewards + streak state
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_online_progress` (
  `player_guid`       INT UNSIGNED NOT NULL,
  `session_minutes`   INT UNSIGNED NOT NULL DEFAULT 0,
  `daily_minutes`     INT UNSIGNED NOT NULL DEFAULT 0,
  `weekly_minutes`    INT UNSIGNED NOT NULL DEFAULT 0,
  `monthly_minutes`   INT UNSIGNED NOT NULL DEFAULT 0,
  `reset_day`         DATE NOT NULL DEFAULT '2000-01-01',
  `reset_week`        DATE NOT NULL DEFAULT '2000-01-01',
  `reset_month`       DATE NOT NULL DEFAULT '2000-01-01',
  PRIMARY KEY (`player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_online_claims` (
  `player_guid`       INT UNSIGNED NOT NULL,
  `reward_id`         INT UNSIGNED NOT NULL,
  `claim_period`      VARCHAR(16) NOT NULL COMMENT 'YYYY-MM-DD | YYYY-Www | YYYY-MM | once',
  `claimed_at`        TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`player_guid`, `reward_id`, `claim_period`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_daily_streak` (
  `player_guid`        INT UNSIGNED NOT NULL,
  `current_streak`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `max_streak`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `last_login_date`    DATE DEFAULT NULL,
  `last_claim_date`    DATE DEFAULT NULL,
  PRIMARY KEY (`player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ----------------------------------------------------------------------------
-- 2.4. Visual enchants ownership
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_player_enchants` (
  `player_guid`   INT UNSIGNED NOT NULL,
  `enchant_id`    INT UNSIGNED NOT NULL,
  `applied_at`    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `is_active`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`player_guid`, `enchant_id`),
  KEY `idx_active` (`player_guid`, `is_active`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ----------------------------------------------------------------------------
-- 2.5. Voting: candidates snapshot + votes + winners
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_vote_candidates` (
  `period`        VARCHAR(16) NOT NULL COMMENT 'YYYY-MM',
  `category`      ENUM('pve','pvp','helper','monthly') NOT NULL,
  `rank`          TINYINT UNSIGNED NOT NULL COMMENT '1..50',
  `player_guid`   INT UNSIGNED NOT NULL,
  `score`         DECIMAL(12,2) NOT NULL,
  PRIMARY KEY (`period`, `category`, `rank`),
  UNIQUE KEY `uk_candidate` (`period`, `category`, `player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_votes` (
  `voter_guid`    INT UNSIGNED NOT NULL,
  `candidate_guid` INT UNSIGNED NOT NULL,
  `category`      ENUM('pve','pvp','helper','monthly') NOT NULL,
  `vote_date`     DATE NOT NULL,
  PRIMARY KEY (`voter_guid`, `category`, `vote_date`),
  KEY `idx_candidate` (`candidate_guid`, `category`, `vote_date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='One vote per voter per category per day';

CREATE TABLE IF NOT EXISTS `aurora_vote_winners` (
  `period`        VARCHAR(16) NOT NULL,
  `category`      ENUM('pve','pvp','helper','monthly') NOT NULL,
  `rank`          TINYINT UNSIGNED NOT NULL,
  `player_guid`   INT UNSIGNED NOT NULL,
  `votes_count`   INT UNSIGNED NOT NULL,
  `reward_claimed` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`period`, `category`, `rank`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ----------------------------------------------------------------------------
-- 2.6. Event participation
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_active_events` (
  `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `event_id`      INT UNSIGNED NOT NULL,
  `started_at`    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `ends_at`       TIMESTAMP NOT NULL,
  `status`        ENUM('active','completed','cancelled') NOT NULL DEFAULT 'active',
  PRIMARY KEY (`id`),
  KEY `idx_status` (`status`, `ends_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_event_progress` (
  `active_event_id`   INT UNSIGNED NOT NULL,
  `player_guid`       INT UNSIGNED NOT NULL,
  `mobs_killed`       INT UNSIGNED NOT NULL DEFAULT 0,
  `quests_completed`  INT UNSIGNED NOT NULL DEFAULT 0,
  `players_helped`    INT UNSIGNED NOT NULL DEFAULT 0,
  `bosses_killed`     INT UNSIGNED NOT NULL DEFAULT 0,
  `participation_sec` INT UNSIGNED NOT NULL DEFAULT 0,
  `points_earned`     INT UNSIGNED NOT NULL DEFAULT 0,
  `last_update`       TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`active_event_id`, `player_guid`),
  CONSTRAINT `fk_evprog_active` FOREIGN KEY (`active_event_id`)
    REFERENCES `aurora_active_events` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_event_claims` (
  `active_event_id`  INT UNSIGNED NOT NULL,
  `player_guid`      INT UNSIGNED NOT NULL,
  `reward_id`        INT UNSIGNED NOT NULL,
  `claimed_at`       TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`active_event_id`, `player_guid`, `reward_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

SET FOREIGN_KEY_CHECKS = 1;
