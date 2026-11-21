-- ============================================================================
-- PROJECT AURORA v7.1.0 - WORLD database
-- Item set bonuses for the campaign armour sets (server side).
--
-- Set bonuses in 3.3.5a normally live in ItemSet.dbc, which is a client file.
-- The server applies them here instead: `aurora_item_sets` names the set,
-- `aurora_item_set_bonuses` maps a piece count to a passive spell, and the
-- membership itself comes from `item_template`.`setID` (written by
-- 2026_11_10_12.sql). One source of truth: change a piece's setID and the
-- bonus follows, with no second list to keep in sync.
--
-- Spells 600100-600105 are the passive auras from 2026_11_10_12.sql.
-- Apply AFTER 2026_11_10_12.sql.
-- ============================================================================

SET NAMES utf8mb4;

-- ----------------------------------------------------------------------------
-- Set definitions. The id matches `item_template`.`setID`.
--   5300 Р РЋРЎвЂљРЎР‚Р В°Р В¶ Р В Р В°РЎРѓРЎРѓР Р†Р ВµРЎвЂљР В°  (plate)   2 pieces +40 Str, 5 pieces +2% crit
--   5301 Р СћР ВµР Р…Р ВµР Р†Р С•Р в„– Р С›РЎвЂ¦Р С•РЎвЂљР Р…Р С‘Р С” (leather) 2 pieces +40 Agi, 5 pieces +2% crit
--   5302 Р вЂ™Р ВµРЎРѓРЎвЂљР В°Р В»Р С”Р В°        (cloth)   2 pieces +40 Int, 5 pieces +2% crit
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_item_sets` (
  `id`      INT UNSIGNED NOT NULL,
  `name_ru` VARCHAR(100) NOT NULL DEFAULT '',
  `name_en` VARCHAR(100) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DELETE FROM `aurora_item_sets`;
INSERT INTO `aurora_item_sets` (`id`, `name_ru`, `name_en`) VALUES
  (5300, CONVERT(0xD0A1D182D180D0B0D0B620D0A0D0B0D181D181D0B2D0B5D182D0B0 USING utf8mb4),  'Dawn Warden'),
  (5301, CONVERT(0xD0A2D0B5D0BDD0B5D0B2D0BED0B920D09ED185D0BED182D0BDD0B8D0BA USING utf8mb4), 'Shadow Hunter'),
  (5302, CONVERT(0xD092D0B5D181D182D0B0D0BBD0BAD0B0 USING utf8mb4),        'Vestal');

-- ----------------------------------------------------------------------------
-- Bonuses. `pieces` is the number of worn pieces that unlocks `spell_id`.
-- Exactly one row per (set, piece count): the loader keeps the first three
-- rows of each set in ascending order.
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_item_set_bonuses` (
  `set_id`   INT UNSIGNED NOT NULL,
  `pieces`   TINYINT UNSIGNED NOT NULL,
  `spell_id` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`set_id`, `pieces`),
  KEY `idx_pieces` (`pieces`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DELETE FROM `aurora_item_set_bonuses`;
INSERT INTO `aurora_item_set_bonuses` (`set_id`, `pieces`, `spell_id`) VALUES
  (5300, 2, 600100),
  (5300, 5, 600101),
  (5301, 2, 600102),
  (5301, 5, 600103),
  (5302, 2, 600104),
  (5302, 5, 600105);

-- ----------------------------------------------------------------------------
-- Membership. `setID` on `item_template` is written for the client tooltip,
-- but the core of this schema does not read it, so the running module keeps
-- its own list here. Change a piece in this table and the bonus follows.
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_item_set_items` (
  `set_id`     INT UNSIGNED NOT NULL,
  `item_entry` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`set_id`, `item_entry`),
  KEY `idx_item_entry` (`item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DELETE FROM `aurora_item_set_items`;
INSERT INTO `aurora_item_set_items` (`set_id`, `item_entry`) VALUES
  (5300, 50300), (5300, 50301), (5300, 50302), (5300, 50303), (5300, 50304),
  (5301, 50310), (5301, 50311), (5301, 50312), (5301, 50313), (5301, 50314),
  (5302, 50320), (5302, 50321), (5302, 50322), (5302, 50323), (5302, 50324);

-- ----------------------------------------------------------------------------
-- Sanity check: every set must own at least one item, otherwise its bonuses
-- can never fire and the definition is dead weight.
-- ----------------------------------------------------------------------------
SELECT s.`id`, s.`name_ru`, COUNT(i.`item_entry`) AS items,
       (SELECT COUNT(*) FROM `aurora_item_set_bonuses` b WHERE b.`set_id` = s.`id`) AS bonuses
  FROM `aurora_item_sets` s
  LEFT JOIN `aurora_item_set_items` i ON i.`set_id` = s.`id`
 GROUP BY s.`id`, s.`name_ru`;
