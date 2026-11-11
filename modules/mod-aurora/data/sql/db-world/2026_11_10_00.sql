-- ============================================================================
-- PROJECT AURORA v7.1.0 - Schema
-- Target: AzerothCore master (3.3.5a), MySQL 8.0+
-- Apply to: WORLD database (creature/gameobject/item/quest/achievement data)
--           CHARACTER database (player progress tables)
-- Custom tables use the `aurora_` prefix only.
-- Stock AC tables are never altered.
-- ============================================================================

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ============================================================================
-- SECTION 1. WORLD DATABASE
-- ============================================================================

-- ----------------------------------------------------------------------------
-- 1.1. Reference dictionaries (bilingual text lives here, like AC modules do)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_campaigns` (
  `id`                 INT UNSIGNED NOT NULL,
  `internal_name`      VARCHAR(64)  NOT NULL,
  `display_name_ru`    VARCHAR(128) NOT NULL,
  `display_name_en`    VARCHAR(128) NOT NULL,
  `map_id`             INT UNSIGNED NOT NULL,
  `min_level`          TINYINT UNSIGNED NOT NULL DEFAULT 80,
  `max_level`          TINYINT UNSIGNED NOT NULL DEFAULT 80,
  `type`               ENUM('dungeon','raid','pvp','world','solo') NOT NULL,
  `status`             ENUM('planned','development','testing','live','archived') NOT NULL DEFAULT 'planned',
  `release_date`       DATE DEFAULT NULL,
  `created_at`         TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_internal_name` (`internal_name`),
  UNIQUE KEY `uk_map` (`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora campaigns registry';

CREATE TABLE IF NOT EXISTS `aurora_campaign_locations` (
  `campaign_id`   INT UNSIGNED NOT NULL,
  `hub_zone`      VARCHAR(64)  NOT NULL,
  `hub_map_id`    INT UNSIGNED NOT NULL,
  `hub_x`         FLOAT NOT NULL,
  `hub_y`         FLOAT NOT NULL,
  `hub_z`         FLOAT NOT NULL,
  `hub_o`         FLOAT NOT NULL DEFAULT 0,
  `portal_entry`  INT UNSIGNED NOT NULL,
  `phase_mask`    INT UNSIGNED NOT NULL,
  PRIMARY KEY (`campaign_id`),
  KEY `idx_portal` (`portal_entry`),
  CONSTRAINT `fk_loc_campaign` FOREIGN KEY (`campaign_id`)
    REFERENCES `aurora_campaigns` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Hub coordinates + portal + phase per campaign';

-- ----------------------------------------------------------------------------
-- 1.2. Achievements
-- AC 3.3.5a keeps achievements in client DBC (Achievement.dbc / Achievement_Criteria.dbc).
-- Achievements must be patched into the client MPQ, NOT into a world table.
-- This table only stores the *server-side* reward rule for a DBC achievement ID.
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_achievement_rewards` (
  `achievement_id`  INT UNSIGNED NOT NULL COMMENT 'Achievement.dbc ID',
  `title_id`        INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'CharTitles.dbc ID, 0 = none',
  `item_id`         INT UNSIGNED NOT NULL DEFAULT 0,
  `item_amount`     INT UNSIGNED NOT NULL DEFAULT 1,
  `aurora_points`   INT UNSIGNED NOT NULL DEFAULT 0,
  `mail_subject`    VARCHAR(128) NOT NULL DEFAULT 'Aurora',
  `mail_text`       TEXT,
  `send_mail`       TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`achievement_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Server-side rewards for Aurora achievements';

CREATE TABLE IF NOT EXISTS `aurora_achievement_criteria` (
  `achievement_id`  INT UNSIGNED NOT NULL,
  `criteria_index`  TINYINT UNSIGNED NOT NULL COMMENT 'Position inside Achievement_Criteria.dbc (0-based)',
  `tracker_type`    ENUM('none','count','time','altars','raid_deaths','no_damage','chain','meta') NOT NULL DEFAULT 'none',
  `target_value`    INT UNSIGNED NOT NULL DEFAULT 0,
  `map_id`          INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = anywhere',
  `note`            VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`achievement_id`, `criteria_index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='How Aurora script tracks each criteria';

-- ----------------------------------------------------------------------------
-- 1.3. Titles (server-side custom titles)
-- AC ships custom titles in `char_titles`? No. Stock AC uses DBC only.
-- Aurora adds a small lookup so the server can apply a title by internal key
-- without hard-coding DBC ids in several places.
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_titles` (
  `internal_name`  VARCHAR(64) NOT NULL,
  `title_dbc_id`   INT UNSIGNED NOT NULL COMMENT 'Must exist in CharTitles.dbc (client patch)',
  `name_ru`        VARCHAR(128) NOT NULL,
  `name_en`        VARCHAR(128) NOT NULL,
  `achievement_id` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`internal_name`),
  UNIQUE KEY `uk_dbc` (`title_dbc_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora title registry';

-- ----------------------------------------------------------------------------
-- 1.4. Vendors / shop catalogue (world data)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_shop_items` (
  `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `item_entry`   INT UNSIGNED NOT NULL,
  `item_amount`  INT UNSIGNED NOT NULL DEFAULT 1,
  `cost_shards`  INT UNSIGNED NOT NULL DEFAULT 0,
  `cost_aurora`  INT UNSIGNED NOT NULL DEFAULT 0,
  `cost_gold`    BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'copper',
  `category`     ENUM('equipment','mounts','pets','consumables','enchants','daily') NOT NULL DEFAULT 'equipment',
  `name_ru`      VARCHAR(255) NOT NULL,
  `name_en`      VARCHAR(255) NOT NULL,
  `description_ru` TEXT,
  `description_en` TEXT,
  `is_daily`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `discount`     TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'percent 0..90',
  `max_per_player` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = unlimited',
  `enabled`      TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`),
  KEY `idx_category` (`category`, `enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora shop catalogue';

-- ----------------------------------------------------------------------------
-- 1.5. Events definitions (scheduling is done by the world server, not by MySQL EVENT)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_events` (
  `id`               INT UNSIGNED NOT NULL,
  `internal_name`    VARCHAR(64)  NOT NULL,
  `name_ru`          VARCHAR(128) NOT NULL,
  `name_en`          VARCHAR(128) NOT NULL,
  `description_ru`   TEXT,
  `description_en`   TEXT,
  `type`             ENUM('daily','weekly','monthly','seasonal','dynamic') NOT NULL,
  `duration_hours`   INT UNSIGNED NOT NULL,
  `schedule_type`    ENUM('time','calendar','activity') NOT NULL,
  `schedule_value`   VARCHAR(64) NOT NULL COMMENT 'HH:MM | SUN:HH:MM | D1-D2 | SOLSTICE | online:300;activity:50000',
  `map_id`           INT UNSIGNED NOT NULL DEFAULT 0,
  `min_activity_score` DECIMAL(10,2) NOT NULL DEFAULT 0,
  `world_state_id`   INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '30000 + event id if 0',
  `enabled`          TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_internal_name` (`internal_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora event definitions';

CREATE TABLE IF NOT EXISTS `aurora_event_rewards` (
  `id`               INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `event_id`         INT UNSIGNED NOT NULL,
  `tier`             ENUM('participation','bronze','silver','gold','legendary') NOT NULL,
  `requirement_type` ENUM('kills','quests','helps','time','bosses','points','world_complete') NOT NULL,
  `requirement_value` INT UNSIGNED NOT NULL,
  `item_entry`       INT UNSIGNED NOT NULL DEFAULT 0,
  `item_amount`      INT UNSIGNED NOT NULL DEFAULT 1,
  `aurora_points`    INT UNSIGNED NOT NULL DEFAULT 0,
  `shards`           INT UNSIGNED NOT NULL DEFAULT 0,
  `title_dbc_id`     INT UNSIGNED NOT NULL DEFAULT 0,
  `visual_enchant_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `name_ru`          VARCHAR(128) NOT NULL,
  `name_en`          VARCHAR(128) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_event` (`event_id`, `tier`),
  CONSTRAINT `fk_evreward_event` FOREIGN KEY (`event_id`)
    REFERENCES `aurora_events` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora event reward tiers';

-- ----------------------------------------------------------------------------
-- 1.6. Online rewards + daily streak definitions
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_online_rewards` (
  `id`             INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `minutes_required` INT UNSIGNED NOT NULL,
  `item_entry`     INT UNSIGNED NOT NULL DEFAULT 0,
  `item_amount`    INT UNSIGNED NOT NULL DEFAULT 1,
  `aurora_points`  INT UNSIGNED NOT NULL DEFAULT 0,
  `shards`         INT UNSIGNED NOT NULL DEFAULT 0,
  `title_dbc_id`   INT UNSIGNED NOT NULL DEFAULT 0,
  `reset_period`   ENUM('daily','weekly','monthly','once') NOT NULL DEFAULT 'daily',
  `mail_subject_ru` VARCHAR(255) NOT NULL,
  `mail_subject_en` VARCHAR(255) NOT NULL,
  `mail_text_ru`   TEXT,
  `mail_text_en`   TEXT,
  `enabled`        TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_threshold_period` (`minutes_required`, `reset_period`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_streak_rewards` (
  `day`            INT UNSIGNED NOT NULL COMMENT '1..7',
  `item_entry`     INT UNSIGNED NOT NULL DEFAULT 0,
  `item_amount`    INT UNSIGNED NOT NULL DEFAULT 1,
  `aurora_points`  INT UNSIGNED NOT NULL DEFAULT 0,
  `shards`         INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`day`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ----------------------------------------------------------------------------
-- 1.7. Visual enchantments
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_visual_enchants` (
  `id`               INT UNSIGNED NOT NULL,
  `name_ru`          VARCHAR(128) NOT NULL,
  `name_en`          VARCHAR(128) NOT NULL,
  `description_ru`   TEXT,
  `description_en`   TEXT,
  `spell_visual_id`  INT UNSIGNED NOT NULL COMMENT 'SpellVisual.dbc id',
  `item_subclass_mask` INT UNSIGNED NOT NULL DEFAULT 0,
  `rarity`           ENUM('common','rare','epic','legendary') NOT NULL DEFAULT 'rare',
  `source`           ENUM('online','vote','achievement','shop','quest') NOT NULL DEFAULT 'achievement',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ----------------------------------------------------------------------------
-- 1.8. Voting reward matrix
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_vote_rewards` (
  `category`         ENUM('pve','pvp','helper','monthly') NOT NULL,
  `rank`             TINYINT UNSIGNED NOT NULL,
  `title_dbc_id`     INT UNSIGNED NOT NULL DEFAULT 0,
  `item_entry`       INT UNSIGNED NOT NULL DEFAULT 0,
  `item_amount`      INT UNSIGNED NOT NULL DEFAULT 1,
  `aurora_points`    INT UNSIGNED NOT NULL DEFAULT 0,
  `visual_enchant_id` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`category`, `rank`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ----------------------------------------------------------------------------
-- 1.9. Activity scoring weights (percent weights, integers to avoid decimal drift)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_activity_weights` (
  `activity`          ENUM('creature_kill','player_kill','quest_complete','achievement_earn',
                           'item_loot','item_craft','mail_send','trade_complete',
                           'dungeon_enter','raid_enter','boss_kill',
                           'arena_game','bg_game','honor_earn',
                           'auction_bid','auction_sale','playtime_minute') NOT NULL,
  `base_points`       INT UNSIGNED NOT NULL DEFAULT 1,
  `pve_percent`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `pvp_percent`       TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `social_percent`    TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `description_ru`    VARCHAR(255) DEFAULT NULL,
  `description_en`    VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`activity`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ----------------------------------------------------------------------------
-- 1.10. Lore chronicles (server-driven, client displays in addon tab)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_lore_chronicles` (
  `id`                    INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `campaign_id`           INT UNSIGNED NOT NULL,
  `chapter`               INT UNSIGNED NOT NULL,
  `title_ru`              VARCHAR(255) NOT NULL,
  `title_en`              VARCHAR(255) NOT NULL,
  `text_ru`               TEXT NOT NULL,
  `text_en`               TEXT NOT NULL,
  `unlock_achievement_id` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_campaign_chapter` (`campaign_id`, `chapter`),
  CONSTRAINT `fk_lore_campaign` FOREIGN KEY (`campaign_id`)
    REFERENCES `aurora_campaigns` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ----------------------------------------------------------------------------
-- 1.11. Gossip menu item texts (bilingual; edited in DB, no server rebuild)
-- One row per gossip item per NPC. `action` is the C++-side action identifier
-- ('chain' / 'leave'); `sort_order` controls the display order per NPC.
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_gossip_texts` (
  `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `creature_entry` INT UNSIGNED NOT NULL COMMENT 'creature_template.entry this item belongs to',
  `action`        VARCHAR(32) NOT NULL COMMENT 'C++ action identifier: chain, leave, ...',
  `text_ru`       VARCHAR(255) NOT NULL,
  `text_en`       VARCHAR(255) NOT NULL,
  `icon`          TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'GOSSIP_ICON_* value (uint8)',
  `sort_order`    INT UNSIGNED NOT NULL DEFAULT 0,
  `enabled`       TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`),
  KEY `idx_creature_sort` (`creature_entry`, `sort_order`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora gossip menu item texts';

-- ============================================================================

SET FOREIGN_KEY_CHECKS = 1;
