-- ============================================================================
-- PROJECT AURORA v7.1.0 - Gossip menu item texts
-- Target: AzerothCore master (3.3.5a), WORLD database
-- Load AFTER 01_aurora_schema.sql
-- Idempotent: pure REPLACE INTO (safe to re-run, never duplicates rows).
--
-- Moves quest-giver gossip item texts out of C++ (npc_campaign01_questgivers.cpp)
-- into the world DB so the wording can be edited without a server rebuild.
-- `action` maps to the C++ handler action ('chain' / 'leave').
-- Greeting texts remain in `npc_text` (see 02_aurora_world_data.sql, ids 7001-7003).
-- ============================================================================

SET NAMES utf8mb4;

-- ----------------------------------------------------------------------------
-- 1. GOSSIP ITEM TEXTS (3 quest givers x 2 items each)
-- creature_entry 150100 - Командир Лайондрел (Alliance chain 90001..90006)
-- creature_entry 150101 - Варгок Кровавый Клык (Horde chain 90011..90016)
-- creature_entry 150102 - Странник Эонис (Neutral chain 90021..90026)
-- icon 0 = GOSSIP_ICON_CHAT; sort_order 0 = chain, 1 = leave.
-- ----------------------------------------------------------------------------
REPLACE INTO `aurora_gossip_texts`
  (`id`,`creature_entry`,`action`,`text_ru`,`text_en`,`icon`,`sort_order`,`enabled`)
VALUES
  (700101,150100,'chain','Расскажи о Ночи Падших.','Tell me about the Night of the Fallen.',0,0,1),
  (700102,150100,'leave','Прощай.','Goodbye.',0,1,1),
  (700201,150101,'chain','Ночь ждёт. Идём.','The night awaits. Let''s go.',0,0,1),
  (700202,150101,'leave','Прощай.','Goodbye.',0,1,1),
  (700301,150102,'chain','Равновесие зовёт.','Balance calls.',0,0,1),
  (700302,150102,'leave','Прощай.','Goodbye.',0,1,1);