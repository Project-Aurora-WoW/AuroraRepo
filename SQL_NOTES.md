# Aurora — что и куда применяется

Документ описывает, как раскладываются файлы по базам и что нельзя делать без патча клиента.

## Файлы
| Файл | База | Что делает |
|---|---|---|
| `data/sql/db-world/2026_11_10_00.sql` | WORLD | Создаёт таблицы `aurora_*` для мира. Стоковые таблицы не трогает. |
| `data/sql/db-world/2026_11_10_01.sql` | WORLD | Кампании, локации, NPC/GO шаблоны, предметы, квесты, магазин, эвенты, награды. |
| `data/sql/db-world/2026_11_10_02.sql` | WORLD | Спавны NPC и алтарей на карте 950 и порталы в хабах. |
| `data/sql/db-world/2026_11_10_06.sql` | WORLD | Тексты gossip-меню квестодателей. |
| `data/sql/db-world/2026_11_10_07.sql` | WORLD | RBAC-права `.aurora shop buy` и `.aurora vote cast`. |
| `data/sql/db-characters/2026_11_10_00.sql` | **CHARACTER** | Создаёт таблицы `aurora_*` для прогресса игроков. |
| `data/sql/db-characters/2026_11_10_03.sql` | **CHARACTER** | Все процедуры `sp_Aurora*` + зеркало весов `aurora_v_weights`. |
| `data/sql/db-characters/2026_11_10_04.sql` | CHARACTER | Журнал покупок, расчёты голосов, счётчики кампании, активности. |
| `data/sql/db-characters/2026_11_10_05.sql` | CHARACTER | Журнал покупок магазина (совместимость). |
| `data/sql/db-characters/2026_11_10_08.sql` | CHARACTER | Отложенные награды: титулы, энчанты, предметы, очки. |
| `data/sql/db-world/2026_11_10_08.sql` | WORLD | Кастомные спеллы кампании и маунтов в `spell_dbc`. |
| `data/sql/db-world/2026_11_10_09.sql` | WORLD | Пул ежедневных поручений `aurora_daily_quest_pool`. |
| `data/sql/db-world/2026_11_10_11.sql` | WORLD | Перенос кампании #1 на карту 951. |
| `data/sql/db-characters/2026_11_10_09.sql` | CHARACTER | Состояние ежедневных поручений `aurora_daily_quests`. |
| `data/sql/db-characters/2026_11_10_10.sql` | CHARACTER | Счётчики продаж магазина `aurora_shop_sales`. |

## Правило разделения баз

- **world** — контент и справочники: каталог магазина, эвенты, награды, спеллы,
  квесты, спавны. World-таблица общая для всех рилмов и **не должна мутироваться
  в рантайме**: код никогда не делает туда `UPDATE`/`INSERT` из игрового действия.
- **character** — всё, что принадлежит игроку или конкретному запуску: прогресс,
  счётчики, журналы, состояние поручений, продажи.
- **Межбазовых внешних ключей не бывает.** Таблица в character, ссылающаяся на
  world-таблицу (например `aurora_event_progress.active_event_id` →
  `aurora_active_events`), держит только обычный `KEY`, а целостность
  обеспечивает код.
- Планировщик эвентов пишет `aurora_active_events` в **world**: это состояние
  рилма, а не игрока.

**Почему процедуры на character, а не на world.** Всё, что они читают и пишут
(`aurora_player_activity`, `aurora_vote_candidates`, `aurora_votes`,
`aurora_vote_winners`), создаётся в character-схеме. Ранее файл лежал на world,
и процедуры физически не видели свои таблицы. Единственная зависимость от world —
веса в `aurora_activity_weights`: они зеркалируются в `aurora_v_weights`
(character) функцией `RefreshWeightMirror()` в `aurora_voting_engine.cpp` при
старте и каждый час. Править веса по-прежнему нужно **только в world**.

## Стандартное размещение (Docs/DataDir)

Кашенные отчёты AC подхватывают SQL из `$AC_DATA_DIR/sql/updates/`. Положите файлы как

```
sql/updates/aurora/2026_11_10_00.sql   -> 01_aurora_schema.sql
sql/updates/aurora/2026_11_10_01.sql   -> 02_aurora_world_data.sql
...
```

и убедитесь, что в `worldserver.conf` путь `Updater.Updates.EnableDatabases` охватывает world.

## Что НЕ реализуется через БД

1. **Достижения.** В 3.3.5a клиент рисует их из `Achievement.dbc` / `Achievement_Criteria.dbc`. Сервер не выбирает строку из таблицы — он проверяет `criteria` из DBC. Поэтому:
   - В `aurora_achievement_criteria` лежит только правило, каким критерием что проверять.
   - Сами ID 10001…10099 должны быть добавлены в клиентские DBC (патч `patch-4.MPQ`), иначе сервер не сможет выдать достижение, а клиент — показать.
   - Таблицу `achievement_dbc`, которую предлагало ТЗ, не создаём: такой таблицы нет в AC и её использование было ошибкой.
2. **Титулы.** Аналогично: `CharTitles.dbc`. `aurora_titles` лишь реестр соответствия «внутреннее имя → DBC id», чтобы C++ не хардкодил их в 12 местах.
3. **Награды за квест.** Хранятся в `quest_template` (`RewardItem1..4` + `RewardAmount1..4`), а не в отдельной `quest_reward_item`. Файл 02 заполняет эти колонки.
4. **Квест-цепочки.** `RewardNextQuest` в `quest_template`. Плюс C++ закрепляет автопереход, чтобы длинная цепочка не рвалась при прерывании квеста.

## Ожидаемая версия схемы

`worldserver` при старте сравнивает `world_db_version.version` с последним файлом в `sql/updates/world/`. Все файлы Aurora должны быть пронумерованы датой-временем больше текущей ревизии вашей сборки, иначе они не применятся автоматически.

## Проверка после применения

```sql
-- WORLD
SELECT COUNT(*) FROM aurora_campaigns;            -- 1 (кампания #1)
SELECT COUNT(*) FROM aurora_events;               -- ожидается 8
SELECT COUNT(*) FROM aurora_shop_items;           -- по каталогу
SELECT COUNT(*) FROM aurora_visual_enchants;      -- ожидается 8

-- CHARACTER
SELECT COUNT(*) FROM aurora_player_activity;      -- 0 при первом запуске
```

## Известные ограничения

- Поля `phaseMask` у `creature`/`gameobject` в AC поддерживаются (в отличие от `creature_template`), поэтому фазы задаются только в спавнах — см. файл 03.
- `guid` у `creature` и `gameobject` — PK без AUTO_INCREMENT. Спавны используют фиксированные guid из диапазона 900000+, чтобы не столкнуться со стоковыми.
