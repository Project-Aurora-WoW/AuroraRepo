-- ============================================================================
-- PROJECT AURORA v7.1.0 - STORED PROCEDURES
-- Target: AzerothCore master (3.3.5a), MySQL 8.0+
--
-- !!! WHERE TO APPLY: **CHARACTER** database !!!
--
-- Why the character database and not world
--   Everything these procedures touch lives in the character schema:
--     aurora_player_activity, aurora_vote_candidates, aurora_votes,
--     aurora_vote_winners, aurora_player_progress.
--   The one cross-schema dependency is `aurora_activity_weights`, which is
--   world data. It is NOT joined here: its base_points values are read once
--   per call into local variables via a helper view `aurora_v_weights`.
--   That helper is created on the character database and refreshed from the
--   world table by the C++ side (see aurora_voting_engine.cpp).
--
--   This removes the earlier defect where procedures were loaded into world
--   and then could not see the character tables they operate on.
--
-- DESIGN RULES
--   1. No procedure scans stock AC character tables (characters, mail,
--      character_instance, guild, ...). All counters are maintained live by
--      aurora_activity_bridge.cpp, so this file only derives scores.
--   2. Scores are recomputed from counters + weights. The weights table is
--      the single source of truth; tuning is a data change, not a rebuild.
--   3. Formulas follow the design document:
--        PvE    = bosses*15 + raids*25 + playtime_hours*2 + quests*5
--                 (+ dungeons*10, + achievements*3)
--        PvP    = kills*2 + arena_wins*15 + bg_wins*12 + honor/100
--        Social = mails*3 + trades*5 + auctions*1
-- ============================================================================

SET NAMES utf8mb4;

-- ----------------------------------------------------------------------------
-- Weight mirror. The C++ engine refreshes this from `world.aurora_activity_weights`
-- on startup and on every hourly tick, so operators still change weights in
-- the world database only.
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `aurora_v_weights` (
  `activity`    VARCHAR(32) NOT NULL,
  `base_points` INT UNSIGNED NOT NULL DEFAULT 1,
  `updated_at`  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`activity`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Mirror of world.aurora_activity_weights';

DROP PROCEDURE IF EXISTS `sp_AuroraRecalculateScores`;

DELIMITER //

CREATE PROCEDURE `sp_AuroraRecalculateScores`(IN p_min_playtime_sec BIGINT UNSIGNED)
BEGIN
    -- All DECLARE statements must come first: MySQL requires declarations
    -- before any executable statement inside BEGIN ... END.
    DECLARE v_week   DATE;
    DECLARE v_boss   INT UNSIGNED DEFAULT 15;
    DECLARE v_raid   INT UNSIGNED DEFAULT 25;
    DECLARE v_time   INT UNSIGNED DEFAULT 2;
    DECLARE v_quest  INT UNSIGNED DEFAULT 5;
    DECLARE v_dung   INT UNSIGNED DEFAULT 10;
    DECLARE v_ach    INT UNSIGNED DEFAULT 3;
    DECLARE v_plk    INT UNSIGNED DEFAULT 2;
    DECLARE v_arw    INT UNSIGNED DEFAULT 15;
    DECLARE v_bgw    INT UNSIGNED DEFAULT 12;
    DECLARE v_mail   INT UNSIGNED DEFAULT 3;
    DECLARE v_trade  INT UNSIGNED DEFAULT 5;
    DECLARE v_auc    INT UNSIGNED DEFAULT 1;

    SET v_week = DATE_SUB(CURDATE(), INTERVAL WEEKDAY(CURDATE()) DAY);

    SELECT COALESCE(MAX(CASE WHEN `activity` = 'boss_kill'       THEN `base_points` END), v_boss),
           COALESCE(MAX(CASE WHEN `activity` = 'raid_enter'      THEN `base_points` END), v_raid),
           COALESCE(MAX(CASE WHEN `activity` = 'playtime_minute' THEN `base_points` END), v_time),
           COALESCE(MAX(CASE WHEN `activity` = 'quest_complete'  THEN `base_points` END), v_quest),
           COALESCE(MAX(CASE WHEN `activity` = 'dungeon_enter'   THEN `base_points` END), v_dung),
           COALESCE(MAX(CASE WHEN `activity` = 'achievement_earn' THEN `base_points` END), v_ach),
           COALESCE(MAX(CASE WHEN `activity` = 'player_kill'     THEN `base_points` END), v_plk),
           COALESCE(MAX(CASE WHEN `activity` = 'arena_game'      THEN `base_points` END), v_arw),
           COALESCE(MAX(CASE WHEN `activity` = 'bg_game'         THEN `base_points` END), v_bgw),
           COALESCE(MAX(CASE WHEN `activity` = 'mail_send'       THEN `base_points` END), v_mail),
           COALESCE(MAX(CASE WHEN `activity` = 'trade_complete'  THEN `base_points` END), v_trade),
           COALESCE(MAX(CASE WHEN `activity` = 'auction_bid'     THEN `base_points` END), v_auc)
    INTO v_boss, v_raid, v_time, v_quest, v_dung, v_ach,
         v_plk, v_arw, v_bgw, v_mail, v_trade, v_auc
    FROM `aurora_v_weights`;

    UPDATE `aurora_player_activity`
    SET
        `pve_score` = ROUND(
              (`bosses_killed`       * v_boss)
            + (`raids_entered`       * v_raid)
            + ((`total_playtime_sec` / 3600.0) * v_time)
            + (`quests_completed`    * v_quest)
            + (`dungeons_entered`    * v_dung)
            + (`achievements_earned` * v_ach)
        , 2),
        `pvp_score` = ROUND(
              (`players_killed`      * v_plk)
            + (`arena_wins`          * v_arw)
            + (`battleground_wins`   * v_bgw)
            + (`honor_earned` / 100.0)
        , 2),
        `social_score` = ROUND(
              (`mails_sent`          * v_mail)
            + (`trades_completed`    * v_trade)
            + ((`auction_bids` + `auction_sales`) * v_auc)
        , 2),
        `total_score` = ROUND(
              ROUND((`bosses_killed` * v_boss) + (`raids_entered` * v_raid)
                  + ((`total_playtime_sec` / 3600.0) * v_time)
                  + (`quests_completed` * v_quest) + (`dungeons_entered` * v_dung)
                  + (`achievements_earned` * v_ach), 2)
            + ROUND((`players_killed` * v_plk) + (`arena_wins` * v_arw)
                  + (`battleground_wins` * v_bgw) + (`honor_earned` / 100.0), 2)
            + ROUND((`mails_sent` * v_mail) + (`trades_completed` * v_trade)
                  + ((`auction_bids` + `auction_sales`) * v_auc), 2)
        , 2),
        `activity_period` = v_week
    WHERE `total_playtime_sec` >= p_min_playtime_sec;

    -- Anyone below the eligibility threshold keeps zero ranking scores but
    -- retains counters, so they appear the moment they cross the threshold.
    UPDATE `aurora_player_activity`
    SET `pve_score` = 0, `pvp_score` = 0, `social_score` = 0, `total_score` = 0
    WHERE `total_playtime_sec` < p_min_playtime_sec;
END//

DROP PROCEDURE IF EXISTS `sp_AuroraRebuildCandidates`//

CREATE PROCEDURE `sp_AuroraRebuildCandidates`(IN p_period VARCHAR(16), IN p_limit TINYINT UNSIGNED)
BEGIN
    IF p_period IS NULL OR p_period = '' THEN
        SET p_period = DATE_FORMAT(CURDATE(), '%Y-%m');
    END IF;
    IF p_limit IS NULL OR p_limit = 0 THEN
        SET p_limit = 50;
    END IF;

    DELETE FROM `aurora_vote_candidates` WHERE `period` = p_period;

    INSERT INTO `aurora_vote_candidates` (`period`,`category`,`rank`,`player_guid`,`score`)
    SELECT p_period, 'pve', rn, player_guid, pve_score
    FROM (
        SELECT player_guid, pve_score,
               ROW_NUMBER() OVER (ORDER BY pve_score DESC, player_guid ASC) AS rn
        FROM `aurora_player_activity`
        WHERE pve_score > 0 AND total_playtime_sec >= 3600
    ) t
    WHERE t.rn <= p_limit;

    INSERT INTO `aurora_vote_candidates` (`period`,`category`,`rank`,`player_guid`,`score`)
    SELECT p_period, 'pvp', rn, player_guid, pvp_score
    FROM (
        SELECT player_guid, pvp_score,
               ROW_NUMBER() OVER (ORDER BY pvp_score DESC, player_guid ASC) AS rn
        FROM `aurora_player_activity`
        WHERE pvp_score > 0 AND total_playtime_sec >= 3600
    ) t
    WHERE t.rn <= p_limit;

    INSERT INTO `aurora_vote_candidates` (`period`,`category`,`rank`,`player_guid`,`score`)
    SELECT p_period, 'helper', rn, player_guid, social_score
    FROM (
        SELECT player_guid, social_score,
               ROW_NUMBER() OVER (ORDER BY social_score DESC, player_guid ASC) AS rn
        FROM `aurora_player_activity`
        WHERE social_score > 0 AND total_playtime_sec >= 3600
    ) t
    WHERE t.rn <= p_limit;

    INSERT INTO `aurora_vote_candidates` (`period`,`category`,`rank`,`player_guid`,`score`)
    SELECT p_period, 'monthly', rn, player_guid, total_score
    FROM (
        SELECT player_guid, total_score,
               ROW_NUMBER() OVER (ORDER BY total_score DESC, player_guid ASC) AS rn
        FROM `aurora_player_activity`
        WHERE total_score > 0 AND total_playtime_sec >= 3600
    ) t
    WHERE t.rn <= p_limit;
END//

DROP PROCEDURE IF EXISTS `sp_AuroraFinalizeVotes`//

CREATE PROCEDURE `sp_AuroraFinalizeVotes`(IN p_period VARCHAR(16))
BEGIN
    IF p_period IS NULL OR p_period = '' THEN
        SET p_period = DATE_FORMAT(CURDATE() - INTERVAL 1 MONTH, '%Y-%m');
    END IF;

    DELETE FROM `aurora_vote_winners` WHERE `period` = p_period;

    INSERT INTO `aurora_vote_winners` (`period`,`category`,`rank`,`player_guid`,`votes_count`)
    SELECT p_period, category, rn, candidate_guid, votes_count
    FROM (
        SELECT candidate_guid, category, COUNT(*) AS votes_count,
               ROW_NUMBER() OVER (PARTITION BY category
                                  ORDER BY COUNT(*) DESC, candidate_guid ASC) AS rn
        FROM `aurora_votes`
        WHERE DATE_FORMAT(vote_date, '%Y-%m') = p_period
        GROUP BY candidate_guid, category
    ) t
    WHERE t.rn <= 3;
END//

DROP PROCEDURE IF EXISTS `sp_AuroraGrantVoteRewards`//

CREATE PROCEDURE `sp_AuroraGrantVoteRewards`(IN p_period VARCHAR(16))
BEGIN
    -- Returns the reward rows the C++ side must hand out. The procedure does
    -- not touch item, mail or title tables: those grants go through the
    -- normal MailDraft / SetTitle path so the client updates correctly.
    SELECT w.`category`, w.`rank`, w.`player_guid`,
           r.`title_dbc_id`, r.`item_entry`, r.`item_amount`,
           r.`aurora_points`, r.`visual_enchant_id`
    FROM `aurora_vote_winners` w
    JOIN `aurora_vote_rewards` r
      ON r.`category` = w.`category` AND r.`rank` = w.`rank`
    WHERE w.`period` = p_period AND w.`reward_claimed` = 0;
END//

DROP PROCEDURE IF EXISTS `sp_AuroraExpireVotes`//

CREATE PROCEDURE `sp_AuroraExpireVotes`(IN p_keep_days INT UNSIGNED)
BEGIN
    IF p_keep_days IS NULL OR p_keep_days = 0 THEN
        SET p_keep_days = 7;
    END IF;
    DELETE FROM `aurora_votes`
    WHERE `vote_date` < DATE_SUB(CURDATE(), INTERVAL p_keep_days DAY);
END//

DROP PROCEDURE IF EXISTS `sp_AuroraEligibleVoters`//

CREATE PROCEDURE `sp_AuroraEligibleVoters`()
BEGIN
    SELECT `player_guid`, `total_playtime_sec`, `total_score`, `activity_period`
    FROM `aurora_player_activity`
    WHERE `total_playtime_sec` >= 3600
    ORDER BY `total_score` DESC;
END//

DELIMITER ;

-- ============================================================================
-- SCHEDULING
-- No MySQL EVENT is created here on purpose:
--   * enabling the event scheduler needs SUPER;
--   * all three calls already happen from the world process, which can also
--     mail rewards through the real MailDraft path.
-- The C++ engine (aurora_voting_engine.cpp) calls, against CharacterDatabase:
--   CALL sp_AuroraRecalculateScores(3600);
--   CALL sp_AuroraRebuildCandidates('YYYY-MM', 50);
--   CALL sp_AuroraExpireVotes(7);
--   CALL sp_AuroraFinalizeVotes('YYYY-MM');      -- weekly
--   CALL sp_AuroraGrantVoteRewards('YYYY-MM');   -- weekly
-- ============================================================================
