# Aurora — /home/claude/workspace

[Продолжение работы над проектом «Аврора»](https://github.com/maxorlov59rus/aurora-wow)

Серверная часть для AzerothCore (3.3.5a) и клиентский аддон.

## Что внутри

**`src/Aurora/`** — серверный модуль, встраивается в `AzerothCore/src/server/scripts/Aurora/`

| Файл | Назначение |
|---|---|
| `aurora_script_loader.cpp` | `AddAuroraScripts()` — точка входа для сгенерированного ScriptLoader ядра |
| `Core/aurora_config.h` | константы, диапазоны ID, тайминги |
| `Core/aurora_shared.*` | enum'ы и ID-хелперы |
| `Core/aurora_bootstrap.cpp` | проверка таблиц при старте, загрузка каталога магазина |
| `Core/aurora_commands.cpp` | `.aurora shop / vote / activity / version / reload` |
| `Core/aurora_activity_bridge.cpp` | счётчики активности, batched flush раз в минуту |
| `Core/aurora_event_manager.cpp` | планировщик 8 эвентов (time / calendar / activity) |
| `Core/aurora_online_rewards.cpp` | награды за онлайн по порогам |
| `Core/aurora_daily_streak.cpp` | 7-дневная серия входов |
| `Core/aurora_voting_engine.cpp` | пересчёт баллов и выплаты победителям |
| `Campaigns/Campaign01_NightOfTheFallen/` | инстанс, боссы, алтари, Стражи |

**`sql/`** — шесть файлов, применяются в порядке номеров

| Файл | База |
|---|---|
| `01_aurora_schema.sql` | world (раздел 1) + character (раздел 2) |
| `02_aurora_world_data.sql` | world |
| `03_aurora_spawns.sql` | world |
| `04_character_procedures.sql` | **character** |
| `05_character_extra_tables.sql` | character |
| `06_aurora_shop_purchases.sql` | character |

**`addon/AuroraUI/`** — готовый к установке клиентский аддон (без внешних зависимостей).

**`addon/ElvUI_Aurora/`** — альтернативная версия на ElvUI (требует установленного ElvUI).

## Статус

`worldserver.exe` собирается целиком вместе с модулем. Аддон установлен в клиент, но в игре ещё не проверялся.

## Сборка

См. [BUILD.md](BUILD.md).

## Лицензия

GNU GPL v2 — как у самого AzerothCore, поскольку модуль компилируется в один бинарник с ним.
