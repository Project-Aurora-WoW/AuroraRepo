-- ============================================================================
-- PROJECT AURORA v7.1.0 - WORLD data
-- Target: AzerothCore master (3.3.5a), WORLD database
-- Load AFTER 01_aurora_schema.sql
-- Idempotent: pure REPLACE INTO (safe to re-run, never duplicates rows).
-- ============================================================================

SET NAMES utf8mb4;

-- ============================================================================
-- 1. CAMPAIGNS
-- ============================================================================
INSERT IGNORE INTO `aurora_campaigns`
  (`id`,`internal_name`,`display_name_ru`,`display_name_en`,`map_id`,`min_level`,`max_level`,`type`,`status`,`release_date`)
VALUES
  (1,'night_of_the_fallen','Ночь Падших','Night of the Fallen',950,80,80,'raid','live','2026-12-01'),
  (2,'dawn_of_ash','Рассвет Пепла','Dawn of Ash',951,80,80,'raid','planned',NULL),
  (3,'storm_of_the_forgotten','Шторм Забытых','Storm of the Forgotten',952,80,80,'world','planned',NULL),
  (4,'chronicles_of_the_abyss','Хроники Бездны','Chronicles of the Abyss',953,80,80,'dungeon','planned',NULL),
  (5,'golden_sunset','Золотой Закат','Golden Sunset',954,80,80,'pvp','planned',NULL),
  (6,'shadows_of_aurora','Тени Авроры','Shadows of Aurora',955,80,80,'dungeon','planned',NULL),
  (7,'iron_legion','Железный Легион','Iron Legion',956,80,80,'world','planned',NULL),
  (8,'songs_of_the_ancients','Песни Древних','Songs of the Ancients',957,80,80,'world','planned',NULL),
  (9,'frozen_throne_v2','Ледяной Трон 2.0','Frozen Throne 2.0',958,80,80,'raid','planned',NULL),
  (10,'war_of_factions','Война Фракций','War of Factions',959,80,80,'pvp','planned',NULL),
  (11,'kings_curse','Проклятие Короля','King''s Curse',960,80,80,'solo','planned',NULL),
  (12,'star_path','Звёздный Путь','Star Path',961,80,80,'dungeon','planned',NULL),
  (13,'ash_of_empire','Пепел Империи','Ash of Empire',962,80,80,'raid','planned',NULL),
  (14,'last_bastion','Последний Бастион','Last Bastion',963,80,80,'world','planned',NULL),
  (15,'voices_of_the_void','Голоса Пустоты','Voices of the Void',964,80,80,'dungeon','planned',NULL),
  (16,'aurora_finale','Аврора: Финал','Aurora: Finale',965,80,80,'raid','planned',NULL),
  (17,'aurora_legacy','Наследие Авроры','Legacy of Aurora',966,80,80,'world','planned',NULL);

-- ============================================================================
-- 2. LOCATIONS (hub coordinates + portal entry + phase)
-- Phase masks follow the core enum: PHASE_CAMPAIGN_01 = 0x0200 ... 0x2000000
-- ============================================================================
INSERT IGNORE INTO `aurora_campaign_locations`
  (`campaign_id`,`hub_zone`,`hub_map_id`,`hub_x`,`hub_y`,`hub_z`,`hub_o`,`portal_entry`,`phase_mask`)
VALUES
  (1,'Танарис',1,-8000,-4000,15,0,150100,512),
  (2,'Пылевые топи',357,-5000,2000,20,0,150101,1024),
  (3,'Пылевые топи',357,-4000,3000,25,0,150102,2048),
  (4,'Тёмный Берег',4,2000,1500,30,0,150103,4096),
  (5,'Прибамбасск',215,-5000,-3000,10,0,150104,8192),
  (6,'Сумеречный лес',40,3000,2000,15,0,150105,16384),
  (7,'Стальгорн',15,-6000,3000,5,0,150106,32768),
  (8,'Ун''Горо',49,-3000,-2000,20,0,150107,65536),
  (9,'Зимние Ключи',83,-4000,-1000,30,0,150108,131072),
  (10,'Альтерак',959,0,0,50,0,150109,262144),
  (11,'Лордерон',960,1000,2000,10,0,150110,524288),
  (12,'Азшара',16,5000,-3000,25,0,150111,1048576),
  (13,'Силитус',1,2000,-4000,15,0,150112,2097152),
  (14,'Фералас',69,-3000,3000,20,0,150113,4194304),
  (15,'Фералас',69,-2000,2000,15,0,150114,8388608),
  (16,'Даларан',571,5000,500,100,0,150115,16777216),
  (17,'Обсерватория',966,0,0,100,0,150116,33554432);

-- ============================================================================
-- 3. CUSTOM CURRENCIES (items)
-- AC 3.3.5a class 15 subclass 0 = Miscellaneous / Junk.
-- Both are BIND_PICKUP-free (flag 0) so they can be mailed as rewards.
-- ============================================================================
REPLACE INTO `item_template`
  (`entry`,`class`,`subclass`,`name`,`displayid`,`Quality`,`Flags`,`BuyCount`,`BuyPrice`,`SellPrice`,
   `InventoryType`,`ItemLevel`,`RequiredLevel`,`Stackable`,`bonding`,`description`)
VALUES
  (50001,15,0,'Осколок падшей звезды',50565,1,0,1,0,0,0,1,80,9999,0,'Валюта кампании «Ночь Падших»'),
  (50100,15,0,'Очки Авроры',50565,1,0,1,0,0,0,1,80,99999,0,'Мета-валюта проекта «Аврора»');

-- ============================================================================
-- 4. CAMPAIGN #1 ITEMS
-- ============================================================================
REPLACE INTO `item_template`
  (`entry`,`class`,`subclass`,`name`,`displayid`,`Quality`,`Flags`,`BuyCount`,`BuyPrice`,`SellPrice`,
   `InventoryType`,`ItemLevel`,`RequiredLevel`,`Stackable`,`bonding`,`description`)
VALUES
  -- keys
  (50010,15,0,'Маяк Света',5279,2,0,1,0,0,0,1,80,1,1,'Ключ Альянса для входа в «Ночь Падших»'),
  (50011,15,0,'Тёмный тотем',5279,2,0,1,0,0,0,1,80,1,1,'Ключ Орды для входа в «Ночь Падших»'),

  -- quest rewards
  (50020,4,1,'Медальон Ночи',3969,3,0,1,50000,10000,2,80,80,1,1,'Награда цепочки Альянса'),
  (50021,4,1,'Тотем Крови Ночи',3969,3,0,1,50000,10000,2,80,80,1,1,'Награда цепочки Орды'),
  (50022,4,1,'Плащ Странника',37871,2,0,1,10000,2000,16,80,80,1,1,'Нейтральная награда Эониса'),
  (50023,4,2,'Сердце Ночи',35186,4,0,1,100000,20000,0,80,80,200,1,'Реагент для легендарного предмета'),

  -- chests (unopened containers, flag 64)
  (50030,15,0,'Сундук Альянса Падших',13014,3,64,1,0,0,0,200,80,1,1,'Эпическая экипировка ilvl 200'),
  (50031,15,0,'Сундук Орды Падших',13014,3,64,1,0,0,0,200,80,1,1,'Эпическая экипировка ilvl 200'),
  (50032,15,0,'Сундук Авроры',13014,4,64,1,0,0,0,213,80,1,1,'Эпическая экипировка ilvl 213'),

  -- weapons
  (50040,2,10,'Посох Эониса',42847,4,0,1,500000,100000,17,213,80,1,1,'Дар Хранителей Равновесия'),
  (50200,2,7,'Клинок Авроры',45122,5,0,1,1000000,200000,17,284,80,1,1,'Легендарный меч проекта «Аврора»'),

  -- mounts (class 15 subclass 5)
  (50050,15,5,'Боевой конь Рассвета',14337,4,64,1,1000000,0,0,80,80,1,1,'Маунт Альянса'),
  (50051,15,5,'Волк Сумеречной Стаи',14334,4,64,1,1000000,0,0,80,80,1,1,'Маунт Орды'),
  (50201,15,5,'Фантом Авроры',29268,4,64,1,2000000,0,0,80,80,1,1,'Уникальный маунт проекта'),
  (50202,15,5,'Призматический Фантом',29268,4,64,1,2000000,0,0,80,80,1,1,'Уникальный маунт проекта'),

  -- pets / toys / transmog
  (50060,15,0,'Теневой детёныш',32843,3,64,1,200000,0,0,80,80,1,1,'Компаньон'),
  (50061,15,0,'Факел Сумерек',4091,2,64,1,50000,0,0,80,80,1,1,'Игрушка'),
  (50203,4,1,'Маска Эониса',33429,3,0,1,100000,20000,1,80,80,1,1,'Трансмогрификация'),
  (50204,15,0,'Знамя Ночи',15365,3,64,1,50000,0,0,80,80,1,1,'Игрушка'),
  (50205,4,1,'Перчатки Теневого Гонга',33429,3,0,1,80000,16000,10,80,80,1,1,'Трансмогрификация');

-- Campaign sets. item_template keeps the 16-column shape used everywhere else
-- in this file; stats, armour and weapon damage are applied below by UPDATE so
-- the REPLACE INTO stays compatible with stock AzerothCore item_template.
REPLACE INTO `item_template`
  (`entry`,`class`,`subclass`,`name`,`displayid`,`Quality`,`Flags`,`BuyCount`,`BuyPrice`,`SellPrice`,
   `InventoryType`,`ItemLevel`,`RequiredLevel`,`Stackable`,`bonding`,`description`)
VALUES
  (50300,4,4,'Нагрудник Стража Рассвета',1000,4,0,1,800000,160000,5,200,80,1,1,'Комплект Стража Рассвета'),
  (50301,4,4,'Ноги Стража Рассвета',1000,4,0,1,600000,120000,7,200,80,1,1,'Комплект Стража Рассвета'),
  (50302,4,4,'Плечи Стража Рассвета',1000,4,0,1,400000,80000,3,200,80,1,1,'Комплект Стража Рассвета'),
  (50303,4,4,'Шлем Стража Рассвета',1000,4,0,1,400000,80000,1,200,80,1,1,'Комплект Стража Рассвета'),
  (50304,4,4,'Перчатки Стража Рассвета',1000,4,0,1,300000,60000,10,200,80,1,1,'Комплект Стража Рассвета'),

  (50310,4,2,'Кираса Теневого Охотника',1000,4,0,1,800000,160000,5,200,80,1,1,'Комплект Теневого Охотника'),
  (50311,4,2,'Поножи Теневого Охотника',1000,4,0,1,600000,120000,7,200,80,1,1,'Комплект Теневого Охотника'),
  (50312,4,2,'Наплечники Теневого Охотника',1000,4,0,1,400000,80000,3,200,80,1,1,'Комплект Теневого Охотника'),
  (50313,4,2,'Капюшон Теневого Охотника',1000,4,0,1,400000,80000,1,200,80,1,1,'Комплект Теневого Охотника'),
  (50314,4,2,'Рукавицы Теневого Охотника',1000,4,0,1,300000,60000,10,200,80,1,1,'Комплект Теневого Охотника'),

  (50320,4,1,'Одеяние Весталки',1000,4,0,1,800000,160000,20,200,80,1,1,'Комплект Весталки'),
  (50321,4,1,'Штаны Весталки',1000,4,0,1,600000,120000,7,200,80,1,1,'Комплект Весталки'),
  (50322,4,1,'Оплечье Весталки',1000,4,0,1,400000,80000,3,200,80,1,1,'Комплект Весталки'),
  (50323,4,1,'Венец Весталки',1000,4,0,1,400000,80000,1,200,80,1,1,'Комплект Весталки'),
  (50324,4,1,'Стяжатели Весталки',1000,4,0,1,300000,60000,10,200,80,1,1,'Комплект Весталки'),

  (50330,2,7,'Клинок Стража Рассвета',1000,4,0,1,900000,180000,13,200,80,1,1,'Оружие комплекта Стража'),
  (50331,2,2,'Лук Теневого Охотника',1000,4,0,1,900000,180000,15,200,80,1,1,'Оружие комплекта Охотника'),
  (50332,2,10,'Посох Весталки',1000,4,0,1,900000,180000,17,200,80,1,1,'Оружие комплекта Весталки'),
  (50333,4,1,'Плащ Хранителя Собора',1000,4,0,1,500000,100000,16,200,80,1,1,'Плащ кампании'),
  (50334,4,0,'Печать Равновесия',1000,4,0,1,700000,140000,12,200,80,1,1,'Аксессуар кампании');

-- ============================================================================
-- ITEM ATTRIBUTES ALIGNED WITH Item.dbc
-- With DBC.EnforceItemAttributes = 1 (default) the core checks Class,
-- Subclass, Material, InventoryType, DisplayInfoID and Sheath against the
-- client DBCs and rewrites mismatched values in memory with a log error.
-- The UPDATEs below make the database agree so a load pass stays silent.
-- Generated from a real server load; rerun after changing item visuals.
-- ============================================================================UPDATE `item_template` SET `Material` = 8, `InventoryType` = 5, `displayid` = 64426 WHERE `entry` = 50001;
UPDATE `item_template` SET `Material` = 1, `InventoryType` = 6, `displayid` = 64660 WHERE `entry` = 50010;
UPDATE `item_template` SET `Material` = 7, `InventoryType` = 10, `displayid` = 65002 WHERE `entry` = 50011;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 3, `displayid` = 64622 WHERE `entry` = 50020;
UPDATE `item_template` SET `Material` = 8, `InventoryType` = 10, `displayid` = 64424 WHERE `entry` = 50021;
UPDATE `item_template` SET `Material` = 5, `InventoryType` = 3, `displayid` = 64819 WHERE `entry` = 50022;
UPDATE `item_template` SET `Material` = 4, `InventoryType` = 2, `displayid` = 64213 WHERE `entry` = 50023;
UPDATE `item_template` SET `Material` = 5, `InventoryType` = 9, `displayid` = 65034 WHERE `entry` = 50030;
UPDATE `item_template` SET `Material` = 7, `InventoryType` = 9, `displayid` = 64165 WHERE `entry` = 50032;
UPDATE `item_template` SET `Material` = 2, `displayid` = 64340 WHERE `entry` = 50040;
UPDATE `item_template` SET `Material` = 1, `InventoryType` = 13, `displayid` = 41873 WHERE `entry` = 50050;
UPDATE `item_template` SET `Material` = 2, `InventoryType` = 21, `displayid` = 64954 WHERE `entry` = 50051;
UPDATE `item_template` SET `Material` = -1, `InventoryType` = 1, `displayid` = 64688 WHERE `entry` = 50060;
UPDATE `item_template` SET `Material` = 4, `InventoryType` = 2, `displayid` = 64203 WHERE `entry` = 50061;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 9, `displayid` = 63569 WHERE `entry` = 50100;
UPDATE `item_template` SET `Material` = 5, `InventoryType` = 7, `displayid` = 59257 WHERE `entry` = 50201;
UPDATE `item_template` SET `Material` = 5, `InventoryType` = 1, `displayid` = 65040 WHERE `entry` = 50202;
UPDATE `item_template` SET `Material` = 1, `InventoryType` = 13, `displayid` = 64768 WHERE `entry` = 50203;
UPDATE `item_template` SET `Material` = 2, `InventoryType` = 26, `displayid` = 64700 WHERE `entry` = 50204;
UPDATE `item_template` SET `Material` = 7, `InventoryType` = 16, `displayid` = 28951 WHERE `entry` = 50205;
UPDATE `item_template` SET `Material` = 8, `displayid` = 64559 WHERE `entry` = 50300;
UPDATE `item_template` SET `Material` = 4, `InventoryType` = 0, `displayid` = 35407 WHERE `entry` = 50301;
UPDATE `item_template` SET `Material` = 1, `InventoryType` = 13, `displayid` = 64756 WHERE `entry` = 50302;
UPDATE `item_template` SET `Material` = 2, `InventoryType` = 13, `displayid` = 64755 WHERE `entry` = 50303;
UPDATE `item_template` SET `Material` = 5, `displayid` = 64704 WHERE `entry` = 50304;
UPDATE `item_template` SET `Material` = 4, `InventoryType` = 2, `displayid` = 64197 WHERE `entry` = 50310;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 1, `displayid` = 64950 WHERE `entry` = 50311;
UPDATE `item_template` SET `Material` = 8, `InventoryType` = 5, `displayid` = 64547 WHERE `entry` = 50312;
UPDATE `item_template` SET `Material` = 3, `InventoryType` = 11, `displayid` = 44358 WHERE `entry` = 50313;
UPDATE `item_template` SET `Material` = 7, `InventoryType` = 6, `displayid` = 64953 WHERE `entry` = 50314;
UPDATE `item_template` SET `Material` = 7, `InventoryType` = 0, `displayid` = 34656 WHERE `entry` = 50320;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 6, `displayid` = 63928 WHERE `entry` = 50321;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 9, `displayid` = 63927 WHERE `entry` = 50322;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 8, `displayid` = 63934 WHERE `entry` = 50323;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 3, `displayid` = 64999 WHERE `entry` = 50324;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 9, `displayid` = 63935 WHERE `entry` = 50330;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 9, `displayid` = 63935 WHERE `entry` = 50331;
UPDATE `item_template` SET `Material` = 6, `InventoryType` = 9, `displayid` = 63935 WHERE `entry` = 50332;
UPDATE `item_template` SET `Material` = 8, `InventoryType` = 9, `displayid` = 64421 WHERE `entry` = 50333;

-- Stats. displayid 1000 is the stock "question mark" placeholder: swap each
-- model for a real WotLK displayid once the art is chosen.
UPDATE `item_template` SET `armor`=2426, `stat_type1`=7, `stat_value1`=120, `stat_type2`=3, `stat_value2`=80 WHERE `entry`=50300;
UPDATE `item_template` SET `armor`=2123, `stat_type1`=7, `stat_value1`=95,  `stat_type2`=3, `stat_value2`=60 WHERE `entry`=50301;
UPDATE `item_template` SET `armor`=1592, `stat_type1`=7, `stat_value1`=70,  `stat_type2`=3, `stat_value2`=45 WHERE `entry`=50302;
UPDATE `item_template` SET `armor`=1819, `stat_type1`=7, `stat_value1`=70,  `stat_type2`=3, `stat_value2`=50 WHERE `entry`=50303;
UPDATE `item_template` SET `armor`=1326, `stat_type1`=7, `stat_value1`=60,  `stat_type2`=3, `stat_value2`=40 WHERE `entry`=50304;

UPDATE `item_template` SET `armor`=3044, `stat_type1`=3, `stat_value1`=150, `stat_type2`=7, `stat_value2`=90 WHERE `entry`=50310;
UPDATE `item_template` SET `armor`=2663, `stat_type1`=3, `stat_value1`=120, `stat_type2`=7, `stat_value2`=70 WHERE `entry`=50311;
UPDATE `item_template` SET `armor`=1997, `stat_type1`=3, `stat_value1`=90,  `stat_type2`=7, `stat_value2`=55 WHERE `entry`=50312;
UPDATE `item_template` SET `armor`=2204, `stat_type1`=3, `stat_value1`=90,  `stat_type2`=7, `stat_value2`=60 WHERE `entry`=50313;
UPDATE `item_template` SET `armor`=1663, `stat_type1`=3, `stat_value1`=75,  `stat_type2`=7, `stat_value2`=50 WHERE `entry`=50314;

UPDATE `item_template` SET `armor`=1819, `stat_type1`=5, `stat_value1`=150, `stat_type2`=6, `stat_value2`=100 WHERE `entry`=50320;
UPDATE `item_template` SET `armor`=1592, `stat_type1`=5, `stat_value1`=120, `stat_type2`=6, `stat_value2`=85  WHERE `entry`=50321;
UPDATE `item_template` SET `armor`=1194, `stat_type1`=5, `stat_value1`=90,  `stat_type2`=6, `stat_value2`=60  WHERE `entry`=50322;
UPDATE `item_template` SET `armor`=1326, `stat_type1`=5, `stat_value1`=90,  `stat_type2`=6, `stat_value2`=65  WHERE `entry`=50323;
UPDATE `item_template` SET `armor`=995,  `stat_type1`=5, `stat_value1`=75,  `stat_type2`=6, `stat_value2`=55  WHERE `entry`=50324;

UPDATE `item_template` SET `dmg_min1`=214, `dmg_max1`=398, `dmg_type1`=0, `delay`=2600, `stat_type1`=3, `stat_value1`=90, `stat_type2`=7, `stat_value2`=60 WHERE `entry`=50330;
UPDATE `item_template` SET `dmg_min1`=271, `dmg_max1`=504, `dmg_type1`=0, `delay`=2800, `stat_type1`=3, `stat_value1`=80, `stat_type2`=7, `stat_value2`=55 WHERE `entry`=50331;
UPDATE `item_template` SET `dmg_min1`=186, `dmg_max1`=346, `dmg_type1`=0, `delay`=3100, `stat_type1`=5, `stat_value1`=120,`stat_type2`=6, `stat_value2`=80 WHERE `entry`=50332;
UPDATE `item_template` SET `armor`=1101, `stat_type1`=5, `stat_value1`=70, `stat_type2`=7, `stat_value2`=50 WHERE `entry`=50333;
UPDATE `item_template` SET `stat_type1`=3, `stat_value1`=85, `stat_type2`=7, `stat_value2`=65 WHERE `entry`=50334;

-- NOTE: set bonuses need ItemSet.dbc in the client patch; item_template has no
-- setID column in AzerothCore, so no server-side set mapping is written here.
-- mount spells (spell_1)
UPDATE `item_template` SET `spellid_1`=60050, `spelltrigger_1`=0, `spellcharges_1`=-1 WHERE `entry`=50050;
UPDATE `item_template` SET `spellid_1`=60051, `spelltrigger_1`=0, `spellcharges_1`=-1 WHERE `entry`=50051;
UPDATE `item_template` SET `spellid_1`=70002, `spelltrigger_1`=0, `spellcharges_1`=-1 WHERE `entry`=50201;
UPDATE `item_template` SET `spellid_1`=70003, `spelltrigger_1`=0, `spellcharges_1`=-1 WHERE `entry`=50202;
UPDATE `item_template` SET `spellid_1`=70004, `spelltrigger_1`=0, `spellcharges_1`=-1 WHERE `entry`=50204;

-- ============================================================================
-- 5. NPC / GO TEMPLATES
-- NOTE: phaseMask is NOT a column of creature_template / gameobject_template
-- Supported columns in AC 3.3.5a:
--   creature_template.Modelid1, faction, npcflag, rank, AIName, ScriptName
-- ============================================================================
REPLACE INTO `creature_template`
  (`entry`,`name`,`subname`,`minlevel`,`maxlevel`,`faction`,`npcflag`,`rank`,
   `baseattacktime`,`unit_class`,`unit_flags`,`type`,`HealthModifier`,`ManaModifier`,
   `ArmorModifier`,`DamageModifier`,`AIName`,`ScriptName`,`RegenHealth`)
VALUES
  (150001,'Малакор, Падший Король','Вестник Ноктиса',83,83,14,0,3,2000,1,0,7,100,100,1,1,'','boss_malakor',1),
  (150002,'Леди Веспера','Ткачица Теней',82,82,14,0,3,2000,1,0,7,50,100,1,1,'','boss_vespera',1),
  (150010,'Архимаг Вальдор','Хранитель Врат',80,80,35,3,0,2000,1,0,7,1,1,1,1,'','npc_valdor_questgiver',1),
  (150100,'Командир Лайондрел','Авангард Альянса',80,80,1801,3,0,2000,1,0,7,1,1,1,1,'','npc_lyondrel_questgiver',1),
  (150101,'Варгок Кровавый Клык','Вождь Орды',80,80,1802,3,0,2000,1,0,7,1,1,1,1,'','npc_wargok_questgiver',1),
  (150102,'Странник Эонис','Хранитель Равновесия',80,80,35,3,0,2000,1,0,7,1,1,1,1,'','npc_eonis_questgiver',1),
  (150020,'Страж Сумерек','Защитник Алтаря',81,81,14,0,1,2000,1,0,7,20,50,1,1,'','npc_twilight_warden',1),
  (150030,'Теневой прислужник','',80,80,14,0,0,2000,1,0,7,5,10,1,1,'','npc_shadow_minion',1),
  (150200,'Управляющий магазином Авроры','Торговец',80,80,35,3,0,2000,1,0,7,1,1,1,1,'','npc_aurora_shopkeeper',1);

-- ----------------------------------------------------------------------------
-- NPC GREETINGS AND GOSSIP (project range 7000+)
-- Scripts npc_lyondrel/wargok/eonis_questgiver read gossip_menu_id from
-- creature_template and the text from npc_text.
-- ----------------------------------------------------------------------------
REPLACE INTO `npc_text` (`ID`,`text0_0`,`lang0`,`Probability0`)
VALUES
  (7001,'Командир Лайондрел смотрит на юг, где небо темнеет раньше срока.',1,1),
  (7002,'Варгок Кровавый Клык сплёвывает на песок: ночь придёт, и мы её встретим.',1,1),
  (7003,'Странник Эонис молчит, глядя на весы в руке. Тьма и свет равны лишь в начале.',1,1),
  (7004,'Управляющий магазином Авроры перебирает осколки. Цены проверяет сервер.',1,1);

UPDATE `creature_template` SET `gossip_menu_id`=7001 WHERE `entry`=150100;
UPDATE `creature_template` SET `gossip_menu_id`=7002 WHERE `entry`=150101;
UPDATE `creature_template` SET `gossip_menu_id`=7003 WHERE `entry`=150102;
UPDATE `creature_template` SET `gossip_menu_id`=7004 WHERE `entry`=150200;
UPDATE `creature_template` SET `gossip_menu_id`=7004 WHERE `entry`=150010;

-- models
-- AC master keeps models in creature_template_model, not in creature_template.modelid*.
REPLACE INTO `creature_template_model` (`CreatureID`,`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`,`VerifiedBuild`)
VALUES
  (150001,0,25000,1,1,NULL),
  (150002,0,25001,1,1,NULL),
  (150010,0,25010,1,1,NULL),
  (150100,0,25100,1,1,NULL),
  (150101,0,25101,1,1,NULL),
  (150102,0,25102,1,1,NULL),
  (150020,0,25020,1,1,NULL),
  (150030,0,25030,1,1,NULL),
  (150200,0,25103,1,1,NULL);

-- gameobjects: type 10 = GO type button? AC 3.3.5a: type 10 = GAMEOBJECT_TYPE_GOOBER
-- Altars are goober-type interactable objects with ScriptName hook.
REPLACE INTO `gameobject_template`
  (`entry`,`type`,`displayId`,`name`,`IconName`,`castBarCaption`,`unk1`,`size`,`Data0`,`Data1`,`Data2`,`Data3`,
   `Data4`,`Data5`,`Data6`,`Data7`,`Data8`,`Data9`,`Data10`,`Data11`,`ScriptName`)
VALUES
  (150002,10,7143,'Алтарь Сумерек','','',0,1,3000,0,0,0,0,0,0,0,0,0,0,0,'go_shadow_altar'),
  (150100,10,8038,'Портал: Ночь Падших','','',0,1,0,0,0,0,0,0,0,0,0,0,0,0,'go_aurora_portal');

-- ============================================================================
-- 6. QUESTS
-- AC 3.3.5a quest_template uses RewardItem1..4 / RewardAmount1..4 and
-- RewardNextQuest. We fill those directly instead of a separate reward table.
-- AllowableRaces: 1101 = Alliance masks, 690 = Horde masks, 1023 = all.
-- ============================================================================

-- ---------------------------------------------------------------- ALLIANCE ---
REPLACE INTO `quest_template`
  (`ID`,`QuestType`,`QuestLevel`,`MinLevel`,`AllowableRaces`,`Flags`,`RewardNextQuest`,`RewardXPDifficulty`,`RewardMoney`,`RewardItem1`,`RewardAmount1`,`LogTitle`,`LogDescription`,`QuestDescription`,`RequiredNpcOrGo1`,`RequiredNpcOrGoCount1`,`RequiredItemId1`,`RequiredItemCount1`,`AreaDescription`)
VALUES
  (90001,2,80,80,1101,0,90002,0,0,50010,1,'Зов Ночи','Найди Архимага Вальдора','Командир Лайондрел посылает тебя к Вальдору.',150010,1,0,0,''),
  (90002,2,80,80,1101,0,90003,1,25000,50001,5,'Первые тени','Убей 10 Теневых прислужников','Очисти путь от теней.',150030,10,0,0,''),
  (90003,2,80,80,1101,0,90004,2,50000,50001,15,'Алтари Сумерек','Захвати 3 алтаря','Захвати алтари силой Света.',150002,3,0,0,''),
  (90004,2,80,80,1101,0,90005,5,100000,50030,1,'Падший Король','Убей Малакора','Сразись с Падшим Королём.',150001,1,0,0,''),
  (90005,2,80,80,1101,0,90006,3,150000,50020,1,'Осколок надежды','Принеси 50 осколков','Собери осколки для командира.',0,0,50001,50,''),
  (90006,2,80,80,1101,0,0,0,200000,50050,1,'Наследие ночи','Используй медальон в зоне алтарей','Заверши путь Света.',0,0,0,0,'');



-- ---------------------------------------------------------------- HORDE ------
REPLACE INTO `quest_template`
  (`ID`,`QuestType`,`QuestLevel`,`MinLevel`,`AllowableRaces`,`Flags`,`RewardNextQuest`,`RewardXPDifficulty`,`RewardMoney`,`RewardItem1`,`RewardAmount1`,`LogTitle`,`LogDescription`,`QuestDescription`,`RequiredNpcOrGo1`,`RequiredNpcOrGoCount1`,`RequiredItemId1`,`RequiredItemCount1`,`AreaDescription`)
VALUES
  (90011,2,80,80,80,690,90012,0,0,50011,1,'Зов Ночи','Найди Вальдора','Варгок посылает тебя к Вальдору.',150010,1,0,0,''),
  (90012,2,80,80,80,690,90013,1,25000,50001,5,'Первые тени','Убей 10 прислужников','Очисти путь от теней.',150030,10,0,0,''),
  (90013,2,80,80,80,690,90014,2,50000,50001,15,'Алтари Сумерек','Оскверни 3 алтаря','Оскверни алтари силой Орды.',150002,3,0,0,''),
  (90014,2,80,80,80,690,90015,5,100000,50031,1,'Падший Король','Убей Малакора','Сразись с Падшим Королём.',150001,1,0,0,''),
  (90015,2,80,80,80,690,90016,3,150000,50021,1,'Осколок надежды','Принеси 50 осколков','Принеси 50 осколков Варгоку.',0,0,50001,50,''),
  (90016,2,80,80,80,690,0,0,200000,50051,1,'Наследие ночи','Используй тотем','Заверши путь Орды.',0,0,0,0,'');



-- ---------------------------------------------------------------- NEUTRAL ----
REPLACE INTO `quest_template`
  (`ID`,`QuestType`,`QuestLevel`,`MinLevel`,`AllowableRaces`,`Flags`,`RewardNextQuest`,`RewardXPDifficulty`,`RewardMoney`,`RewardItem1`,`RewardAmount1`,`LogTitle`,`LogDescription`,`QuestDescription`,`RequiredNpcOrGo1`,`RequiredNpcOrGoCount1`,`RequiredItemId1`,`RequiredItemCount1`,`AreaDescription`)
VALUES
  (90021,2,80,80,80,1023,90022,0,10000,50022,1,'Баланс нарушен','Войди в «Ночь Падших»','Баланс нарушен. Войди в подземелье.',0,0,0,0,''),
  (90022,2,80,80,80,1023,90023,1,30000,50001,10,'Жертва ради равновесия','Убей 15 прислужников без PvP-урона','Убей 15 прислужников, не получая урона от игроков.',150030,15,0,0,''),
  (90023,2,80,80,80,1023,90024,2,50000,50001,15,'Три стороны медали','Захвати 3 алтаря лично','Захвати алтари без помощи группы.',150002,3,0,0,''),
  (90024,2,80,80,80,1023,90025,5,120000,50040,1,'Истинный враг','Нанеси не менее 15% урона Малакору','Внеси вклад в победу над Падшим Королём.',150001,1,0,0,''),
  (90025,2,80,80,80,1023,90026,3,180000,50023,1,'Хранитель равновесия','Принеси 100 осколков','Принеси 100 осколков Эонису.',0,0,50001,100,''),
  (90026,2,80,80,80,1023,0,0,250000,50060,1,'Вечная ночь','Заверши путь Странника','Заверши все квесты нейтральной цепочки.',0,0,0,0,'');



-- second reward for 90026 (torch)
UPDATE `quest_template` SET `RewardItem2`=50061, `RewardAmount2`=1 WHERE `ID`=90026;
-- second reward for 90004 / 90014 (20 shards)
UPDATE `quest_template` SET `RewardItem2`=50001, `RewardAmount2`=20 WHERE `ID` IN (90004,90014);

-- faction/reputation rewards
UPDATE quest_template SET `RewardFactionValue1`=100 WHERE `ID`=90021;
UPDATE quest_template SET `RewardFactionValue1`=300 WHERE `ID`=90022;
UPDATE quest_template SET `RewardFactionValue1`=600 WHERE `ID`=90023;
UPDATE quest_template SET `RewardFactionValue1`=1200 WHERE `ID`=90024;
UPDATE quest_template SET `RewardFactionValue1`=900 WHERE `ID`=90025;
UPDATE quest_template SET `RewardFactionValue1`=600 WHERE `ID`=90026;

-- quest givers (quest_template does not store giver: vendored via creature_queststarter)
REPLACE INTO `creature_queststarter` (`id`,`quest`) VALUES
  (150100,90001),(150100,90002),(150100,90003),(150100,90004),(150100,90005),(150100,90006),
  (150101,90011),(150101,90012),(150101,90013),(150101,90014),(150101,90015),(150101,90016),
  (150102,90021),(150102,90022),(150102,90023),(150102,90024),(150102,90025),(150102,90026),
  (150010,90001),(150010,90011);

REPLACE INTO `creature_questender` (`id`,`quest`) VALUES
  (150100,90001),(150100,90002),(150100,90003),(150100,90004),(150100,90005),(150100,90006),
  (150101,90011),(150101,90012),(150101,90013),(150101,90014),(150101,90015),(150101,90016),
  (150102,90021),(150102,90022),(150102,90023),(150102,90024),(150102,90025),(150102,90026);

-- ============================================================================
-- 7. ACHIEVEMENT CRITERIA RULES
-- Achievement.dbc IDs 10001..10099 must still be added to the client MPQ.
-- This table only teaches the server HOW to complete them.
-- ============================================================================
REPLACE INTO `aurora_achievement_criteria`
  (`achievement_id`,`criteria_index`,`tracker_type`,`target_value`,`map_id`,`note`)
VALUES
  (10001,0,'none',0,950,'Вход на карту 950'),
  (10002,0,'count',1,950,'Убийство Малакора (10 игроков)'),
  (10003,0,'count',1,950,'Убийство Малакора (25 игроков)'),
  (10004,0,'raid_deaths',0,950,'Пройти без смертей'),
  (10010,0,'time',300,950,'Щит снят за <=5 минут'),
  (10011,0,'count',50,950,'50 Теневых прислужников'),
  (10012,0,'no_damage',1,950,'Без урона от способностей'),
  (10013,0,'time',1800,950,'Полный проход за 30 минут'),
  (10014,0,'count',1,950,'Соло-убийство Стража Сумерек'),
  (10020,0,'altars',10,950,'10 захватов алтарей'),
  (10021,0,'count',25,950,'25 PvP-убийств'),
  (10022,0,'altars',3,950,'3 алтаря в одиночку'),
  (10023,0,'count',5,950,'Отбить 5 захватов'),
  (10024,0,'altars',4,950,'Все 4 алтаря за один заход'),
  (10030,0,'chain',1,0,'Цепочка Альянса (90006)'),
  (10031,0,'chain',1,0,'Цепочка Орды (90016)'),
  (10032,0,'chain',1,0,'Нейтральная цепочка (90026)'),
  (10033,0,'meta',3,0,'Все 3 цепочки'),
  (10040,0,'count',500,0,'500 осколков за всё время'),
  (10041,0,'count',1,0,'Получить Сердце Ночи'),
  (10042,0,'count',4,0,'Все маунты кампании'),
  (10043,0,'count',5,0,'Все трансмоги'),
  (10099,0,'meta',22,0,'Все 22 достижения кампании #1'),
  (20001,0,'meta',5,0,'5 кампаний'),
  (20002,0,'meta',10,0,'10 кампаний'),
  (20003,0,'meta',17,0,'17 кампаний'),
  (20004,0,'meta',10000,0,'10 000 Очков Авроры'),
  (20005,0,'meta',1,0,'Все мета-награды');

-- ============================================================================
-- 8. SERVER-SIDE REWARDS
-- title_dbc_id left as 0 until the client patch lands; replace when ready.
-- ============================================================================
REPLACE INTO `aurora_achievement_rewards`
  (`achievement_id`,`title_id`,`item_id`,`item_amount`,`aurora_points`,`mail_subject`,`mail_text`,`send_mail`)
VALUES
  (10004,5001,0,1,0,'Достижение: Ночь без жертв','Титул «Неустрашимый» выдан.',1),
  (10012,0,50205,1,0,'Достижение: Теневой гонг','Перчатки Теневого Гонга.',1),
  (10013,5002,0,1,0,'Достижение: Быстрее тьмы','Титул «Быстрее Тени» выдан.',1),
  (10022,5003,0,1,0,'Достижение: Одинокий волк','Титул «Одинокий Волк» выдан.',1),
  (10024,0,50204,1,0,'Достижение: Доминион','Знамя Ночи.',1),
  (10033,5004,50203,1,0,'Достижение: Тройной агент','Маска Эониса и титул.',1),
  (10042,0,50202,1,0,'Достижение: Коллекционер ночи','Призматический Фантом.',1),
  (10099,5005,50200,1,500,'Достижение: Герой Ночи Падших','Клинок Авроры и титул.',1),
  (20001,5006,0,1,250,'Мета: Искатель Авроры','Титул выдан.',1),
  (20002,5007,0,1,500,'Мета: Хранитель Авроры','Титул выдан.',1),
  (20003,5008,50201,1,1000,'Мета: Глас Авроры','Титул и Фантом Авроры.',1);

-- ============================================================================
-- 9. TITLE REGISTRY
-- title_dbc_id values must match the patched CharTitles.dbc
-- ============================================================================
REPLACE INTO `aurora_titles` (`internal_name`,`title_dbc_id`,`name_ru`,`name_en`,`achievement_id`)
VALUES
  ('fearless',5001,'%s Неустрашимый','%s the Fearless',10004),
  ('faster_than_shadow',5002,'%s Быстрее Тени','%s Faster than Shadow',10013),
  ('lone_wolf',5003,'%s Одинокий Волк','%s the Lone Wolf',10022),
  ('triple_agent',5004,'%s Тройной Агент','%s the Triple Agent',10033),
  ('hero_of_fallen_night',5005,'%s Герой Ночи Падших','%s Hero of the Fallen Night',10099),
  ('aurora_seeker',5006,'%s Искатель Авроры','%s Seeker of Aurora',20001),
  ('aurora_keeper',5007,'%s Хранитель Авроры','%s Keeper of Aurora',20002),
  ('aurora_voice',5008,'%s Глас Авроры','%s Voice of Aurora',20003),
  ('loyal_keeper',5009,'%s Верный Хранитель','%s the Loyal Keeper',0),
  ('pve_hero',5010,'%s Герой PvE','%s PvE Hero',0),
  ('pvp_hero',5011,'%s Герой PvP','%s PvP Hero',0),
  ('soul_of_aurora',5012,'%s Душа Авроры','%s Soul of Aurora',0),
  ('remembrancer',5013,'%s Помнящий','%s the Remembrancer',0),
  ('pvp_runner_up',5014,'%s Соратник PvP','%s PvP Companion',0),
  ('pvp_second_runner',5015,'%s Участник PvP','%s PvP Participant',0),
  ('helper_soul',5016,'%s Помощник Авроры','%s Aurora Helper',0);

-- ============================================================================
-- 10. ACTIVITY WEIGHTS
-- pve/pvp/social_percent: integer percent (0..100), total 100.
-- This is the SINGLE source of truth used by sp_RecalculateScores.
-- ============================================================================
REPLACE INTO `aurora_activity_weights`
  (`activity`,`base_points`,`pve_percent`,`pvp_percent`,`social_percent`,`description_ru`,`description_en`)
VALUES
  ('creature_kill',     1,100,0,  0,'Убийство существа','Creature kill'),
  ('player_kill',       5,  0,100,0,'Убийство игрока','Player kill'),
  ('quest_complete',   10, 70,0, 30,'Завершение квеста','Quest completion'),
  ('achievement_earn', 25, 50,0, 50,'Получение достижения','Achievement earned'),
  ('item_loot',         1,100,0,  0,'Поднятие предмета','Item looted'),
  ('item_craft',        3, 80,0, 20,'Создание предмета','Item crafted'),
  ('mail_send',         2,  0,0,100,'Отправка письма','Mail sent'),
  ('trade_complete',    3,  0,0,100,'Завершение обмена','Trade completed'),
  ('dungeon_enter',    15,100,0,  0,'Вход в подземелье','Dungeon entered'),
  ('raid_enter',       30,100,0,  0,'Вход в рейд','Raid entered'),
  ('boss_kill',        50,100,0,  0,'Убийство босса','Boss killed'),
  ('arena_game',       20,  0,100,0,'Игра на арене','Arena game'),
  ('bg_game',          15,  0,100,0,'Игра на ПБ','Battleground game'),
  ('honor_earn',        1,  0,100,0,'Единица чести','Honor point'),
  ('auction_bid',       1,  0,0,100,'Ставка на аукционе','Auction bid'),
  ('auction_sale',      2,  0,0,100,'Продажа на аукционе','Auction sale'),
  ('playtime_minute',   1, 50,0, 50,'Минута в игре','Minute played');

-- ============================================================================
-- 11. ONLINE REWARDS
-- ============================================================================
REPLACE INTO `aurora_online_rewards`
  (`id`,`minutes_required`,`item_entry`,`item_amount`,`aurora_points`,`shards`,`title_dbc_id`,
   `reset_period`,`mail_subject_ru`,`mail_subject_en`,`mail_text_ru`,`mail_text_en`,`enabled`)
VALUES
  (1,60,0,1,0,5,0,'daily',
     'Награда за 1 час','Reward for 1 hour',
     'Благодарим за время в мире Авроры!','Thank you for your time in the world of Aurora!',1),
  (2,180,50030,1,0,0,0,'daily',
     'Сундук Странника','Wanderer Chest',
     'Ваша преданность награждена!','Your dedication is rewarded!',1),
  (3,360,0,1,50,15,0,'daily',
     'Награда за 6 часов','Reward for 6 hours',
     'Вы — истинный герой Авроры!','You are a true hero of Aurora!',1),
  (4,720,50031,1,0,0,0,'daily',
     'Сундук Хранителя','Keeper Chest',
     'Хранители заметили вашу доблесть!','The Keepers noticed your valor!',1),
  (5,1440,50032,1,200,0,0,'daily',
     'Сундук Авроры','Aurora Chest',
     'Легенда гласит о вас!','Legends speak of you!',1),
  (6,3000,0,1,0,0,0,'weekly',
     'Сияние Авроры','Aurora Glow',
     'Ваше оружие сияет светом Авроры!','Your weapon glows with the light of Aurora!',1),
  (7,7200,0,1,0,0,5009,'monthly',
     'Титул: Верный Хранитель','Title: Loyal Keeper',
     'Вы — Верный Хранитель Авроры!','You are the Loyal Keeper of Aurora!',1);

-- ============================================================================
-- 12. DAILY STREAK
-- ============================================================================
REPLACE INTO `aurora_streak_rewards` (`day`,`item_entry`,`item_amount`,`aurora_points`,`shards`)
VALUES
  (1,0,1,0,5),
  (2,0,1,0,10),
  (3,0,1,20,20),
  (4,50030,1,0,0),
  (5,0,1,50,50),
  (6,50031,1,0,0),
  (7,50032,1,200,0);

-- ============================================================================
-- 13. CONTINENTS: disable stock ores/herbs spawn? no. Nothing here.
-- ============================================================================

-- ============================================================================
-- 14. EVENTS
-- ============================================================================
REPLACE INTO `aurora_events`
  (`id`,`internal_name`,`name_ru`,`name_en`,`description_ru`,`description_en`,
   `type`,`duration_hours`,`schedule_type`,`schedule_value`,`map_id`,`min_activity_score`,`world_state_id`,`enabled`)
VALUES
  (1,'auroras_dawn','Рассвет Авроры','Aurora''s Dawn',
     'Каждый вечер Хранители проводят ритуал света','Every evening the Keepers perform a ritual of light',
     'daily',2,'time','18:00',0,0,30001,1),
  (2,'nyctos_breach','Прорыв Ноктиса','Nyctos Breach',
     'Тьма прорывается в мир через разломы','Darkness breaks through the rifts',
     'weekly',4,'time','SAT:20:00',0,100,30002,1),
  (3,'hunt_heralds','Охота на Вестников','Hunt for the Heralds',
     'Вестники Ноктиса скрываются в мире','The Heralds of Nyctos hide in the world',
     'daily',24,'time','00:00',0,50,30003,1),
  (4,'battle_keepers','Битва Хранителей','Battle of the Keepers',
     'Альянс и Орда сражаются за Обсерваторию','Alliance and Horde fight for the Observatory',
     'weekly',6,'time','FRI:19:00',951,200,30004,1),
  (5,'night_remembrance','Ночь Памяти','Night of Remembrance',
     'Души павших героев возвращаются','Souls of fallen heroes return',
     'monthly',72,'calendar','1-3',0,0,30005,1),
  (6,'fair_balance','Ярмарка Равновесия','Fair of Balance',
     'Ярмарка Странника Эониса','The Fair of Wanderer Eonis',
     'monthly',168,'calendar','15-21',0,0,30006,1),
  (7,'eclipse','Затмение','The Eclipse',
     'Редкое событие при пиковой активности','A rare event at peak activity',
     'dynamic',1,'activity','online:300;activity:50000',0,0,30007,1),
  (8,'awakening_ancients','Пробуждение Древних','Awakening of the Ancients',
     'Древние пробуждаются в солнцестояние','The Ancients awaken at the solstice',
     'seasonal',336,'calendar','SOLSTICE',0,0,30008,1),
  (9,'fall_of_altars','Падение Алтарей','Fall of the Altars',
     'Тьма поглощает алтари Ночного города — верните их свету',
     'Darkness swallows the altars of the Night City - return them to the light',
     'weekly',5,'time','SUN:21:00',950,0,30009,1),
  (10,'keeper_conclave','Собор Хранителей','Keeper Conclave',
     'Хранители Равновесия собираются на совет и раздают дары',
     'The Keepers of Balance gather in conclave and hand out gifts',
     'weekly',3,'time','WED:19:00',0,0,30010,1),
  (11,'shard_rain','Звёздный Ливень','Shard Rain',
     'Осколки падших звёзд сыплются на земли Азерота',
     'Shards of fallen stars rain down on the lands of Azeroth',
     'monthly',48,'calendar','10-12',0,0,30011,1);

REPLACE INTO `aurora_event_rewards`
  (`id`,`event_id`,`tier`,`requirement_type`,`requirement_value`,
   `item_entry`,`item_amount`,`aurora_points`,`shards`,`title_dbc_id`,`visual_enchant_id`,`name_ru`,`name_en`)
VALUES
  (1,1,'participation','time',30,0,1,0,2,0,0,'Участие','Participation'),
  (2,1,'bronze','kills',50,0,1,0,5,0,0,'5 осколков','5 Shards'),
  (3,1,'silver','quests',3,0,1,20,10,0,0,'20 Очков Авроры','20 Aurora Points'),
  (4,1,'gold','helps',5,60010,1,0,0,0,0,'Свиток Рассвета','Dawn Scroll'),
  (5,1,'legendary','time',120,0,1,0,0,0,101,'Малое Сияние Авроры','Minor Aurora Glow'),

  (6,9,'participation','time',30,0,1,0,3,0,0,'Участие в защите алтарей','Altar defence participation'),
  (7,9,'bronze','kills',25,0,1,0,8,0,0,'10 осколков','10 Shards'),
  (8,9,'silver','bosses',1,0,1,30,15,0,0,'30 Очков Авроры','30 Aurora Points'),
  (9,9,'gold','time',180,50030,1,50,25,0,0,'Сундук Алтаря','Altar Chest'),

  (10,10,'participation','time',30,0,1,0,3,0,0,'Присутствие на Соборе','Conclave attendance'),
  (11,10,'bronze','time',60,0,1,15,8,0,0,'15 Очков Авроры','15 Aurora Points'),
  (12,10,'silver','quests',2,0,1,35,15,0,0,'35 Очков Авроры','35 Aurora Points'),
  (13,10,'gold','time',180,0,1,80,40,0,0,'Упоение Собора','Conclave Bounty'),

  (14,11,'participation','time',30,0,1,0,4,0,0,'Участие в Ливне','Shard Rain participation'),
  (15,11,'bronze','kills',40,0,1,0,12,0,0,'Вскрытый осколок','Cracked Shard'),
  (16,11,'silver','kills',120,0,1,25,25,0,0,'Серебро Ливня','Shard Silver'),
  (17,11,'legendary','time',600,0,1,0,0,0,102,'Тень Ноктиса','Shadow of Nyctos');

-- ============================================================================
-- 15. SHOP
-- ============================================================================
REPLACE INTO `aurora_shop_items`
  (`id`,`item_entry`,`item_amount`,`cost_shards`,`cost_aurora`,`cost_gold`,`category`,
   `name_ru`,`name_en`,`description_ru`,`description_en`,`is_daily`,`discount`,`enabled`)
VALUES
  (1,50030,1,0,0,100000,'equipment','Сундук Альянса Падших','Alliance Fallen Chest','Эпик ilvl 200','Epic ilvl 200',0,0,1),
  (2,50031,1,0,0,100000,'equipment','Сундук Орды Падших','Horde Fallen Chest','Эпик ilvl 200','Epic ilvl 200',0,0,1),
  (3,50040,1,200,0,0,'equipment','Посох Эониса','Staff of Eonis','Эпик ilvl 213','Epic ilvl 213',0,0,1),
  (4,50050,1,500,0,0,'mounts','Боевой конь Рассвета','Dawn Warhorse','Маунт Альянса','Alliance mount',0,0,1),
  (5,50051,1,500,0,0,'mounts','Волк Сумеречной Стаи','Twilight Pack Wolf','Маунт Орды','Horde mount',0,0,1),
  (6,50201,1,0,1000,0,'mounts','Фантом Авроры','Phantom of Aurora','Уникальный маунт','Unique mount',0,0,1),
  (7,50202,1,0,1500,0,'mounts','Призматический Фантом','Prismatic Phantom','Уникальный маунт','Unique mount',0,0,1),
  (8,50060,1,100,0,0,'pets','Теневой детёныш','Shadow Whelpling','Питомец','Pet',0,0,1),
  (9,50001,10,0,0,50000,'consumables','Осколок падшей звезды x10','Shard of Fallen Star x10','Валюта','Currency',0,0,1),
  (10,50100,10,0,0,100000,'consumables','Очки Авроры x10','Aurora Points x10','Мета-валюта','Meta currency',0,0,1),
  (11,50200,1,0,5000,0,'equipment','Клинок Авроры','Blade of Aurora','Легендарный меч ilvl 284','Legendary sword ilvl 284',0,0,1),
  (12,60001,1,300,0,0,'enchants','Свиток: Сияние Авроры','Scroll: Aurora Glow','Визуальный энчант','Visual enchant',0,0,1),
  (13,60002,1,500,0,0,'enchants','Свиток: Тень Ноктиса','Scroll: Shadow of Nyctos','Визуальный энчант','Visual enchant',0,0,1),
  (14,60003,1,500,0,0,'enchants','Свиток: Пламя Рассвета','Scroll: Dawn Flame','Визуальный энчант','Visual enchant',0,0,1),
  (15,60004,1,1000,0,0,'enchants','Свиток: Призматический Блеск','Scroll: Prismatic Shine','Визуальный энчант','Visual enchant',0,0,1);

-- ============================================================================
-- 16. VISUAL ENCHANTS
-- ============================================================================
REPLACE INTO `aurora_visual_enchants`
  (`id`,`name_ru`,`name_en`,`description_ru`,`description_en`,`spell_visual_id`,
   `item_subclass_mask`,`rarity`,`source`)
VALUES
  (101,'Сияние Авроры','Aurora Glow','Золотое сияние','Golden glow',32000,0,'epic','online'),
  (102,'Тень Ноктиса','Shadow of Nyctos','Чёрный дым','Black smoke',32001,0,'epic','vote'),
  (103,'Пламя Рассвета','Dawn Flame','Огненный шлейф','Fiery trail',32002,0,'epic','vote'),
  (201,'Ледяная Аура','Frost Aura','Ледяные частицы','Ice particles',32003,0,'rare','vote'),
  (202,'Грозовой Разряд','Lightning Strike','Молнии','Lightning',32004,0,'epic','vote'),
  (301,'Призматический Блеск','Prismatic Shine','Радужный перелив','Rainbow shimmer',32005,0,'legendary','vote'),
  (401,'Глас Авроры','Voice of Aurora','Полный визуальный сет','Full Aurora set',32006,0,'legendary','vote'),
  (501,'Клинок Падшего','Blade of the Fallen','Кровавый след','Bloody trail',32007,0,'legendary','achievement');

-- ============================================================================
-- 17. VOTE REWARD MATRIX
-- ============================================================================
REPLACE INTO `aurora_vote_rewards`
  (`category`,`rank`,`title_dbc_id`,`item_entry`,`item_amount`,`aurora_points`,`visual_enchant_id`)
VALUES
  ('pve',1,5010,50200,1,1000,101),
  ('pve',2,5014,50201,1,500,102),
  ('pve',3,5015,0,1,250,103),
  ('pvp',1,5011,50202,1,1000,201),
  ('pvp',2,5014,0,1,500,202),
  ('pvp',3,5015,0,1,250,203),
  ('helper',1,5016,50203,1,500,301),
  ('helper',2,0,0,1,250,0),
  ('helper',3,0,0,1,100,0),
  ('monthly',1,5008,50201,1,2000,401),
  ('monthly',2,0,50202,1,1000,402),
  ('monthly',3,0,0,1,500,403);

-- ============================================================================
-- 18. LORE
-- ============================================================================
REPLACE INTO `aurora_lore_chronicles`
  (`id`,`campaign_id`,`chapter`,`title_ru`,`title_en`,`text_ru`,`text_en`,`unlock_achievement_id`)
VALUES
  (1,1,1,'Пробуждение Малакора','Awakening of Malakor',
     'Тысячу лет назад эльфийский принц Малакор был избран Хранителями как защитник Печати. Но Ноктис нашёл трещину в его сердце — горе по утрате возлюбленной. Малакор предал клятву, заточил души павших воинов в Алтарях Сумерек и стал первым Вестником.',
     'A thousand years ago, the elven prince Malakor was chosen by the Keepers as guardian of the Seal. But Nyctos found a crack in his heart - the grief of losing his beloved. Malakor broke his oath, imprisoned the souls of fallen warriors in the Altars of Twilight, and became the first Herald.',
     10001),
  (2,1,2,'Странник Эонис','The Wanderer Eonis',
     'Странник Эонис, последний из Хранителей того поколения, вышел из Обсерватории, чтобы исправить ошибку. Он не может сражаться сам — его сила в Балансе, а не в клинке. Поэтому он ищет героев.',
     'The Wanderer Eonis, last of the Keepers of that generation, walked out of the Observatory to set things right. He cannot fight himself - his power lies in Balance, not in the blade. So he seeks heroes.',
     10002),
  (3,1,3,'Три пути','Three Paths',
     'Альянс видит в Малакоре угрозу. Орда — соперника. Но Эонис знает: истинная победа — не в уничтожении, а в очищении. Три пути ведут к одной цели. Какой выберешь ты?',
     'The Alliance sees Malakor as a threat. The Horde sees a rival. But Eonis knows: true victory lies not in destruction, but in purification. Three paths lead to one goal. Which will you choose?',
     10033);

-- ============================================================================
-- 19. LOCALIZATION (ruRU)
-- ------------------------------------------------------------
-- The rows above put the Russian wording into `creature_template.name`,
-- `npc_text.text0_0` and `item_template.name`. AzerothCore reads those as the
-- "default" locale, so on an enUS client the same DB would hand the player
-- Russian names. The *_locale tables below carry the localized variant the
-- client asks for; `creature_template_locale` / `npc_text_locale` also keep a
-- description so the tooltip is right on every locale.
--
-- Only ruRU rows are written: ruRU clients pick them up, enUS clients keep the
-- default column, which in this module is already the Russian base wording.
-- Swap the default columns for English if an enUS-first build is required.
-- ============================================================================

-- NOTE: `Name` / `SubName` come from the base row, so the ruRU module keeps
-- Russian as the default and does not split it per locale. The ruRU rows are
-- declared idempotently so a future English base can be added without a schema
-- change.
DELETE FROM `creature_template_locale` WHERE `entry` BETWEEN 150001 AND 150200;
INSERT INTO `creature_template_locale` (`entry`,`locale`,`Name`,`Title`)
SELECT `entry`, l.`locale`, ct.`name`, ct.`subname`
FROM `creature_template` ct
JOIN (SELECT 'ruRU' AS `locale`) l
WHERE ct.`entry` BETWEEN 150001 AND 150200;

-- Gossip greetings: 7001-7003 are the Alliance/Horde/neutral hubs, 7004 the
-- shopkeeper, 7005 Valdor inside the campaign instance.
REPLACE INTO `npc_text` (`ID`,`text0_0`,`lang0`,`Probability0`)
VALUES
  (7005,'Архимаг Вальдор держит Врата приоткрытыми: за ними Ночь Падших ждёт героев.',1,1);

UPDATE `creature_template` SET `gossip_menu_id`=7005 WHERE `entry`=150010;

DELETE FROM `npc_text_locale` WHERE `ID` BETWEEN 7001 AND 7005;
INSERT INTO `npc_text_locale` (`ID`,`Locale`,`Text0_0`)
SELECT `ID`, 'ruRU', `text0_0` FROM `npc_text` WHERE `ID` BETWEEN 7001 AND 7005;

-- Item names for every project entry that carries a Russian base name.
DELETE FROM `item_template_locale` WHERE `ID` BETWEEN 50000 AND 60099;
INSERT INTO `item_template_locale` (`ID`,`locale`,`Name`,`Description`)
SELECT `entry`, 'ruRU', `name`, `description`
FROM `item_template`
WHERE `entry` BETWEEN 50000 AND 60099;

-- The shopkeeper sells the whole catalogue, but five stock items are missing
-- from item_template, so the vendor row would be dropped by the core. The
-- visual-enchant scrolls (60001-60004) are created here.
-- Cloned from an existing currency row: item_template has 138 NOT NULL
-- columns that older snapshots create without defaults, so a short column list
-- fails on stricter MySQL builds with error 1136 (column count mismatch).
DELETE FROM `item_template` WHERE `entry` BETWEEN 60001 AND 60004;

CREATE TEMPORARY TABLE `aurora_tmp_scroll` LIKE `item_template`;
INSERT INTO `aurora_tmp_scroll` SELECT * FROM `item_template` WHERE `entry` = 50001;

UPDATE `aurora_tmp_scroll` SET `entry`=60001, `name`='Свиток: Сияние Авроры',
  `description`='Визуальный энчант «Сияние Авроры»', `Quality`=4, `Stackable`=20,
  `ItemLevel`=80, `RequiredLevel`=1, `BuyPrice`=0, `SellPrice`=0;
INSERT INTO `item_template` SELECT * FROM `aurora_tmp_scroll`;

UPDATE `aurora_tmp_scroll` SET `entry`=60002, `name`='Свиток: Тень Ноктиса',
  `description`='Визуальный энчант «Тень Ноктиса»';
INSERT INTO `item_template` SELECT * FROM `aurora_tmp_scroll`;

UPDATE `aurora_tmp_scroll` SET `entry`=60003, `name`='Свиток: Пламя Рассвета',
  `description`='Визуальный энчант «Пламя Рассвета»';
INSERT INTO `item_template` SELECT * FROM `aurora_tmp_scroll`;

UPDATE `aurora_tmp_scroll` SET `entry`=60004, `name`='Свиток: Призматический Блеск',
  `description`='Визуальный энчант «Призматический Блеск»', `Quality`=5;
INSERT INTO `item_template` SELECT * FROM `aurora_tmp_scroll`;

DROP TEMPORARY TABLE `aurora_tmp_scroll`;

-- Vendors list: the shopkeeper (150200) offers the enabled catalogue. Prices
-- stay 0 in the vendor view because the real price is checked in C++.
REPLACE INTO `npc_vendor` (`entry`,`slot`,`item`,`maxcount`,`incrtime`,`ExtendedCost`)
SELECT 150200, i.`id`, i.`item_entry`, 0, 0, 0
FROM `aurora_shop_items` i
WHERE i.`enabled` = 1
ORDER BY i.`id`;






