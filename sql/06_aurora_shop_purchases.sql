// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_shop_purchases.sql is referenced from aurora_commands.cpp.
// Schema fragment for the CHARACTER database - keep in sync with
// sql/01_aurora_schema.sql (Section 2.7).
// ============================================================================

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
