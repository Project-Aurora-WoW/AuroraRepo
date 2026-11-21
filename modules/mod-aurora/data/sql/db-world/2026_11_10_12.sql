-- ============================================================================
-- PROJECT AURORA v7.1.0 - WORLD database
-- Passive set-bonus spells for the campaign armour sets.
--
-- Written against the real `spell_dbc` schema of the world DB used here
-- (234 columns, names with `_1` suffixes: `EffectAura_1`, `Reagent_1`,
-- `Name_Lang_enUS`, `Name_Lang_Mask`). Every column not listed below takes
-- its column default, which is 0 for all effect fields and NULL for the
-- localised strings - exactly what a hidden passive aura needs.
--
-- All mask/level columns in this schema are UNSIGNED, so no -1 is used.
-- Attributes 256 = SPELL_ATTR0_PASSIVE: the aura is permanent while the
-- server keeps it and never enters the spellbook.
-- Apply BEFORE 2026_11_10_13.sql.
-- ============================================================================

SET NAMES utf8mb4;

-- ----------------------------------------------------------------------------
-- 5300 Страж Рассвета   (plate)   2 pieces +40 Str, 5 pieces +2% crit
-- 5301 Теневой Охотник  (leather) 2 pieces +40 Agi, 5 pieces +2% crit
-- 5302 Весталка         (cloth)   2 pieces +40 Int, 5 pieces +2% crit
--
-- Effect_1 = 6 (SPELL_EFFECT_APPLY_AURA)
-- EffectAura_1 = 29 (MOD_STAT, MiscValue 0/1/3 = Str/Agi/Int) or 52 (MOD_CRIT_PCT)
-- EffectBasePoints_1 for MOD_STAT is the stat amount minus one: 39 -> +40.
-- For MOD_CRIT_PCT the stored value is the percentage itself: 2 -> +2%.
-- ----------------------------------------------------------------------------
REPLACE INTO `spell_dbc`
(`ID`,`Attributes`,`CastingTimeIndex`,`BaseLevel`,
 `Effect_1`,`EffectAura_1`,`EffectBasePoints_1`,`EffectMiscValue_1`,
 `Name_Lang_enUS`,`Name_Lang_ruRU`,`Name_Lang_Mask`,
 `Description_Lang_enUS`,`Description_Lang_ruRU`,`Description_Lang_Mask`,
 `SchoolMask`)
VALUES
-- 600100 Страж Рассвета (2): +40 Strength
(600100,256,1,80, 6,29,39,0,
 'Dawn Warden (2)',CONVERT(0xD0A1D182D180D0B0D0B620D0A0D0B0D181D181D0B2D0B5D182D0B020283229 USING utf8mb4),0,
 'Set bonus: +40 Strength.',CONVERT(0xD09AD0BED0BCD0BFD0BBD0B5D0BAD1823A202B343020D0BA20D181D0B8D0BBD0B52E USING utf8mb4),0,
 1),
-- 600101 Страж Рассвета (5): +2% crit
(600101,256,1,80, 6,52,2,0,
 'Dawn Warden (5)',CONVERT(0xD0A1D182D180D0B0D0B620D0A0D0B0D181D181D0B2D0B5D182D0B020283529 USING utf8mb4),0,
 'Set bonus: +2% critical strike.',CONVERT(0xD09AD0BED0BCD0BFD0BBD0B5D0BAD1823A202B322520D0BA20D0BAD180D0B8D182D0B8D187D0B5D181D0BAD0BED0BCD18320D183D0B4D0B0D180D1832E USING utf8mb4),0,
 1),
-- 600102 Теневой Охотник (2): +40 Agility
(600102,256,1,80, 6,29,39,1,
 'Shadow Hunter (2)',CONVERT(0xD0A2D0B5D0BDD0B5D0B2D0BED0B920D09ED185D0BED182D0BDD0B8D0BA20283229 USING utf8mb4),0,
 'Set bonus: +40 Agility.',CONVERT(0xD09AD0BED0BCD0BFD0BBD0B5D0BAD1823A202B343020D0BA20D0BBD0BED0B2D0BAD0BED181D182D0B82E USING utf8mb4),0,
 1),
-- 600103 Теневой Охотник (5): +2% crit
(600103,256,1,80, 6,52,2,0,
 'Shadow Hunter (5)',CONVERT(0xD0A2D0B5D0BDD0B5D0B2D0BED0B920D09ED185D0BED182D0BDD0B8D0BA20283529 USING utf8mb4),0,
 'Set bonus: +2% critical strike.',CONVERT(0xD09AD0BED0BCD0BFD0BBD0B5D0BAD1823A202B322520D0BA20D0BAD180D0B8D182D0B8D187D0B5D181D0BAD0BED0BCD18320D183D0B4D0B0D180D1832E USING utf8mb4),0,
 1),
-- 600104 Весталка (2): +40 Intellect
(600104,256,1,80, 6,29,39,3,
 'Vestal (2)',CONVERT(0xD092D0B5D181D182D0B0D0BBD0BAD0B020283229 USING utf8mb4),0,
 'Set bonus: +40 Intellect.',CONVERT(0xD09AD0BED0BCD0BFD0BBD0B5D0BAD1823A202B343020D0BA20D0B8D0BDD182D0B5D0BBD0BBD0B5D0BAD182D1832E USING utf8mb4),0,
 1),
-- 600105 Весталка (5): +2% crit
(600105,256,1,80, 6,52,2,0,
 'Vestal (5)',CONVERT(0xD092D0B5D181D182D0B0D0BBD0BAD0B020283529 USING utf8mb4),0,
 'Set bonus: +2% critical strike.',CONVERT(0xD09AD0BED0BCD0BFD0BBD0B5D0BAD1823A202B322520D0BA20D0BAD180D0B8D182D0B8D187D0B5D181D0BAD0BED0BCD18320D183D0B4D0B0D180D1832E USING utf8mb4),0,
 1);

-- ----------------------------------------------------------------------------
-- Set membership for the client tooltip. ItemSet.dbc is a client file and
-- `setID` is absent from this world schema, so the column is created here
-- before being filled. The running module reads membership from
-- `aurora_item_set_items` (2026_11_10_13.sql), not from this column.
-- ----------------------------------------------------------------------------
SET @aurora_sql = (
  SELECT IF(
    EXISTS(SELECT 1 FROM `information_schema`.`COLUMNS`
            WHERE `TABLE_SCHEMA` = DATABASE()
              AND `TABLE_NAME`   = 'item_template'
              AND `COLUMN_NAME`  = 'setID'),
    'DO 0',
    'ALTER TABLE `item_template` ADD COLUMN `setID` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `entry`'
  )
);
PREPARE aurora_stmt FROM @aurora_sql;
EXECUTE aurora_stmt;
DEALLOCATE PREPARE aurora_stmt;

SET @aurora_sql = (
  SELECT IF(
    EXISTS(SELECT 1 FROM `information_schema`.`STATISTICS`
            WHERE `TABLE_SCHEMA` = DATABASE()
              AND `TABLE_NAME`   = 'item_template'
              AND `INDEX_NAME`   = 'idx_setID'),
    'DO 0',
    'ALTER TABLE `item_template` ADD INDEX `idx_setID` (`setID`)'
  )
);
PREPARE aurora_stmt FROM @aurora_sql;
EXECUTE aurora_stmt;
DEALLOCATE PREPARE aurora_stmt;

UPDATE `item_template` SET `setID`=5300 WHERE `entry` IN (50300,50301,50302,50303,50304);
UPDATE `item_template` SET `setID`=5301 WHERE `entry` IN (50310,50311,50312,50313,50314);
UPDATE `item_template` SET `setID`=5302 WHERE `entry` IN (50320,50321,50322,50323,50324);

-- ----------------------------------------------------------------------------
-- Verification: six spells present, fifteen pieces linked.
-- ----------------------------------------------------------------------------
SELECT COUNT(*) AS spells FROM `spell_dbc` WHERE `ID` BETWEEN 600100 AND 600105;
SELECT `setID`, COUNT(*) AS pieces
  FROM `item_template` WHERE `setID` BETWEEN 5300 AND 5302 GROUP BY `setID`;
