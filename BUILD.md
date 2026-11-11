# Aurora — сборка и подключение

Целевая платформа: **AzerothCore master (3.3.5a)**.
Ничего из стокового ядра не патчится — модуль ставится как обычный модуль AC.

## 1. Куда положить

```
<AzerothCore>/modules/mod-aurora/
├── CMakeLists.txt
├── data/sql/
│   ├── db-world/        2026_11_10_00 … 07
│   └── db-characters/   2026_11_10_00 … 08
└── src/Aurora/
    ├── aurora_script_loader.cpp
    ├── Core/
    └── Campaigns/Campaign01_NightOfTheFallen/
```

Это стандартная раскладка модулей AzerothCore. Модули подключаются из
`modules/CMakeLists.txt` ядра, поэтому:

- после `git pull` ядро видит модуль без правок `src/`;
- обновление ядра не затрагивает код модуля;
- удаление модуля — просто удаление каталога.

## 2. Сборка

```bash
./acore.sh compiler build
```

(или обычный `cmake --build build --target worldserver`). CMake модуля
добавляет свои `.cpp` к существующей цели `scripts` — второй библиотеки не
создаётся, ничего с ядром не конфликтует.

В логе конфигурации должна появиться строка:

```
[AURORA] module mod-aurora: <список исходников>
```

## 3. База данных

Файлы лежат парами с одинаковым именем: дата `2026_11_10` и порядковый
номер `00…08`. Порядок применения — по номеру в пределах каждой базы.

| # | `db-world` | `db-characters` |
|---|---|---|
| 00 | схема: таблицы `aurora_*` для world | схема: таблицы `aurora_*` для character |
| 01 | данные мира: кампании, NPC, предметы, квесты, магазин, эвенты | — |
| 02 | спавны NPC и алтарей, порталы | — |
| 03 | — | процедуры `sp_Aurora*` + `aurora_v_weights` |
| 04 | — | журнал покупок, счётчики кампании |
| 05 | — | `aurora_shop_purchases` |
| 06 | тексты gossip | — |
| 07 | RBAC-права `.aurora shop buy` / `.aurora vote cast` | — |
| 08 | кастомные спеллы кампании (`spell_dbc` 500001–500030) | `aurora_pending_grants` |

```bash
mysql -u root -p acore_world      < modules/mod-aurora/data/sql/db-world/2026_11_10_00.sql
mysql -u root -p acore_world      < modules/mod-aurora/data/sql/db-world/2026_11_10_01.sql
...
mysql -u root -p acore_character  < modules/mod-aurora/data/sql/db-characters/2026_11_10_08.sql
```

**Процедуры — только на character.** Все таблицы, которые они читают и
пишут (`aurora_player_activity`, `aurora_vote_candidates`, `aurora_votes`,
`aurora_vote_winners`), создаются в character-схеме. Единственная зависимость
от world — веса в `aurora_activity_weights`; они зеркалируются в
`aurora_v_weights` на character-базе функцией `RefreshWeightMirror()` в
`aurora_voting_engine.cpp` при старте и каждый час. Поэтому веса
по-прежнему правятся **только в world**.

Проверка:

```sql
-- world
SELECT COUNT(*) FROM aurora_campaigns;        -- 17
SELECT COUNT(*) FROM aurora_events;           -- 9
SELECT COUNT(*) FROM aurora_shop_items;       -- 15
SELECT COUNT(*) FROM aurora_visual_enchants;  -- 8
SELECT COUNT(*) FROM quest_template WHERE ID BETWEEN 90001 AND 90999; -- 18

-- character
SHOW PROCEDURE STATUS WHERE Db = DATABASE() AND Name LIKE 'sp_Aurora%';  -- 6
SHOW TABLES LIKE 'aurora_v_weights';          -- 1
```

### Автоапдейтер

Файлы можно отдать автоапдейтеру AC: скопировать `db-world/*.sql` в
`$AC_DATA_DIR/sql/updates/world/` и `db-characters/*.sql` в
`$AC_DATA_DIR/sql/updates/character/`, сохранив имена `YYYY_MM_DD_NN.sql`
(AC применяет их по порядку даты/номера и пишет в таблицу `updates`). Имена
уже в этом формате и с датой позже текущей ревизии ядра, поэтому подхватятся
после рестарта при включённом `Updater.Updates.EnableDatabases`.

## 4. Конфигурация

Ключи в `worldserver.conf` читаются один раз при старте
(`aurora_settings.cpp`). Если ключ отсутствует, берётся значение из
`aurora_config.h`:

```ini
Aurora.Enable = 1
Aurora.Core.Enabled = 1
Aurora.Campaign.01.Enabled = 1
Aurora.Shop.Enabled = 1
Aurora.Voting.Enabled = 1
Aurora.Events.Enabled = 1
Aurora.OnlineRewards.Enabled = 1
Aurora.Localization.Default = ruRU
```

`Aurora.Enable = 0` выключает модуль целиком: bootstrap не трогает таблицы,
магазин и голосование отвечают «отключено».

## 5. Команды после сборки

```
.aurora                 — версия и статус
.aurora version
.aurora activity        — личная статистика (Activity-First)
.aurora shop            — каталог
.aurora shop buy <id>   — покупка
.aurora campaign        — прогресс кампании, серия входов, часы онлайн
.aurora vote list       — кандидаты текущего месяца
.aurora vote top        — победители прошлых периодов
.aurora vote cast pve 1 — голос
.aurora reload shop     — перечитать каталог (RBAC: administrator)
```

Права на чтение — `RBAC_PERM_COMMAND_HELP` (любой игрок). Покупка и голос
закрыты собственными правами `RBAC_PERM_COMMAND_AURORA_SHOP` /
`RBAC_PERM_COMMAND_AURORA_VOTE`, которые выдаются файлом `db-world/07`.

## 6. Что требует проверки на живом ядре

Часть API в AC master меняется между релизами. Перед сборкой сверьте:

- `player->GetSession()->GetSessionDbcLocale()` — тип возврата;
- `ChatHandler::PSendSysMessage` с `{}` — используется `StringFormat`, нужен
  `#include "StringFormat.h"` (уже включён);
- `player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, entry, count)` — модуль
  использует вариант с `ItemPosCountVec dest`, он же нужен для `StoreNewItem`;
- `Field::IsNull()` — присутствует с AC 2022+.

Если что-то из этого не совпадёт с вашей ревизией — пришлите вывод компилятора.

## 7. Состав модуля v7.1.0

| Файл | Что делает |
|---|---|
| `Core/aurora_config.h` | константы, диапазоны ID, тайминги |
| `Core/aurora_shared.h/.cpp` | enum'ы, ID-хелперы, реестр карт |
| `Core/aurora_bootstrap.cpp` | старт: проверка таблиц, загрузка каталога магазина |
| `Core/aurora_commands.cpp` | `.aurora shop/vote/campaign/activity/version/reload` |
| `Core/aurora_activity_bridge.cpp` | счётчики активности, flush раз в минуту, отложенные награды |
| `Core/aurora_event_manager.cpp` | планировщик эвентов (time / calendar / activity) |
| `Core/aurora_event_rewards.cpp` | ярусы наград, выдача онлайн и офлайн-игрокам |
| `Core/aurora_online_rewards.cpp` | награды за онлайн по порогам |
| `Core/aurora_daily_streak.cpp` | 7-дневная серия входов |
| `Core/aurora_voting_engine.cpp` | пересчёт баллов и выплаты победителям |
| `Core/aurora_visual_enchants.cpp` | визуальные энчанты: владение и применение |
| `Core/aurora_lore.cpp` | хроники кампании |
| `Campaigns/…/campaign01.h/.cpp` | общие объявления кампании и таблица эффектов алтарей |
| `Campaigns/…/night_of_the_fallen.cpp` | InstanceScript: захват алтарей, снятие щита, энрейдж |
| `Campaigns/…/boss_malakor.cpp` | Малакор: щит, переход в бой, адды, энрейдж |
| `Campaigns/…/boss_vespera.cpp` | Веспера: хилит Стражей, вешает дебаффы |
| `Campaigns/…/npc_twilight_warden.cpp` | Страж: держит алтарь, лишится при отводе |
| `Campaigns/…/go_shadow_altar.cpp` | Алтарь: интеракция, проверка Стража, подсказки |

## 8. Логика кампании #1

1. Игроки входят в инстанс — запускается таймер кампании.
2. У каждого из 4 алтарей стоит Страж Сумерек. Пока Страж жив, алтарь запечатан.
3. Убить Стража → встать на алтарь → удерживать 10 секунд → алтарь захвачен.
   Прогресс при уходе **убывает**, а не сбрасывается.
4. Другая фракция на том же алтаре блокирует прогресс.
5. 3 алтаря у одной фракции → с Малакора снимается `SPELL_DARK_SHIELD`,
   он переходит в бой, включается ротация (теневая стрела, адды, аура).
6. Через 10 минут после снятия щита — энрейдж.
7. 4 алтаря → бонус −75% брони и +100% урона по боссу.

Вся логика захвата живёт в одном месте (InstanceScript). `go_shadow_altar.cpp`
только показывает состояние и не дублирует расчёт.

## 9. Ограничения

- Достижения (`10001…10099`, `20001…20005`) выдаются сервером, но клиент
  покажет их только после патча `Achievement.dbc` / `Achievement_Criteria.dbc` /
  `CharTitles.dbc` — см. `docs/DBC_PATCH.md`.
- Спеллы `500010…500016`, `500020`, `500030`, `500001` и маунты
  `60050/60051`, `70002/70003/70004` **должны быть заведены в `spell_dbc`** —
  скрипт их только вызывает.
- Ключи `Aurora.*` в `worldserver.conf` не читаются.

## 10. Проверка синтаксиса

Файлы модуля прогонялись через MSVC (`cl /Zs`, C++20) с минимальными
заглушками AC-API. Это проверка **синтаксиса и логики типов**, а не линковка:
полноценная сборка возможна только внутри дерева AzerothCore.
