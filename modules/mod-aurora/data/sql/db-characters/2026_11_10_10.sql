-- ============================================================================
-- PROJECT AURORA v7.1.0 - CHARACTER database
-- Shop sales counters.
--
-- `aurora_shop_items` lives in the world database and is content: it must not
-- be mutated at runtime, because one world database serves every realm.
-- Per-offer sales are therefore counted here, one row per offer per day.
-- Load AFTER 2026_11_10_09.sql.
-- ============================================================================

SET NAMES utf8mb4;

CREATE TABLE IF NOT EXISTS `aurora_shop_sales` (
  `shop_id`    INT UNSIGNED NOT NULL COMMENT 'aurora_shop_items.id (world DB)',
  `sold_on`    DATE NOT NULL,
  `sold_count` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`shop_id`, `sold_on`),
  KEY `idx_date` (`sold_on`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Aurora shop sales per offer per day';
