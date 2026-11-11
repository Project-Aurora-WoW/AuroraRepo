# Aurora — сборка и подключение

Целевая платформа: **AzerothCore master (3.3.5a)**.
Ничего из стокового ядра не патчится — только добавляются новые файлы.

## 1. Куда положить

```
<AzerothCore>/src/server/scripts/Aurora/
├── CMakeLists.txt
├── Core/
│   ├── aurora_config.h
│   ├── aurora_shared.h
│   ├── aurora_shared.cpp
│   ├── aurora_bootstrap.cpp
│   └── aurora_commands.cpp
└── Campaigns/            (добавляется по мере готовности)
```

`src/server/scripts/CMakeLists.txt` в stock AC рекурсивно собирает всё содержимое
`scripts/`, поэтому достаточно скопировать папку `Aurora/` — пересборка подхватит
файлы через `CONFIGURE_DEPENDS`. Если ваша сборка использует явные `add_subdirectory`,
добавьте одну строку:

```cmake
add_subdirectory(Aurora)
```

## 2. Разрешение модулей в ScriptLoader

Файл `<AzerothCore>/src/server/scripts/ScriptLoader.cpp`.

**Способ A — явные объявления (надёжнее).** В начало файла:

```cpp
// AURORA
void AddSC_aurora_bootstrap();
void AddSC_aurora_commandscript();
```

В конец `AddScripts()` — **после** всех стоковых `AddSC_*`, чтобы наши скрипты
получили приоритет при конфликте имён:

```cpp
    // ===== AURORA =====
    AddSC_aurora_bootstrap();
    AddSC_aurora_commandscript();
```

> Почему не через ScriptName в БД: команды (`CommandScript`) не имеют колонки
> `ScriptName` в `creature_template`, поэтому их нельзя включить правкой данных.
> Явный вызов из `LoadScriptNames()` гарантирует загрузку.

**Способ B — если не хотите трогать ScriptLoader.** Добавьте в конец
`<AzerothCore>/src/server/scripts/CMakeLists.txt`:

```cmake
target_sources(scripts PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Aurora/Core/aurora_shared.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Aurora/Core/aurora_bootstrap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Aurora/Core/aurora_commands.cpp
)
```

и в `ScriptLoader.cpp` вызовы из способа A остаются обязательными.

## 3. База данных

Порядок применения:

| # | Файл | База |
|---|---|---|
| 1 | `sql/01_aurora_schema.sql` | world (раздел 1) + character (раздел 2) |
| 2 | `sql/02_aurora_world_data.sql` | world |
| 3 | `sql/03_aurora_spawns.sql` | world |
| 4 | `sql/04_character_procedures.sql` | **character** |
| 5 | `sql/06_aurora_shop_purchases.sql` | character |

```bash
mysql -u root -p acore_world      < sql/01_aurora_schema.sql
mysql -u root -p acore_world      < sql/02_aurora_world_data.sql
mysql -u root -p acore_world      < sql/03_aurora_spawns.sql
mysql -u root -p acore_character  < sql/04_character_procedures.sql
mysql -u root -p acore_character  < sql/06_aurora_shop_purchases.sql
```

`01_aurora_schema.sql` содержит две секции в одном файле: раздел 1 — таблицы
world, раздел 2 — таблицы character. Если применяете автоматически, выполните
разделы на соответствующих базах либо прогоните файл на обе — `CREATE TABLE IF NOT EXISTS`
безопасен, лишние таблицы просто окажутся неиспользуемыми.

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
SELECT COUNT(*) FROM aurora_events;           -- 8
SELECT COUNT(*) FROM aurora_shop_items;       -- 15
SELECT COUNT(*) FROM aurora_visual_enchants;  -- 8
SELECT COUNT(*) FROM quest_template WHERE ID BETWEEN 90001 AND 90999; -- 18

-- character
SHOW PROCEDURE STATUS WHERE Db = DATABASE() AND Name LIKE 'sp_Aurora%';  -- 6
SHOW TABLES LIKE 'aurora_v_weights';          -- 1
```

## 4. Конфигурация

В `worldserver.conf` секция Aurora не обязательна: значения зашиты в
`aurora_config.h`. Если нужна настройка без пересборки, добавьте:

```ini
Aurora.Enable = 1
Aurora.Campaign.01.Enabled = 1
Aurora.Shop.Enabled = 1
Aurora.Voting.Enabled = 1
Aurora.Localization.Default = ruRU
```

(Эти ключи пока не читаются — модуль берёт значения из заголовка. Подключение
Config остаётся на следующий срез.)

## 5. Команды после сборки

```
.aurora                 — версия и статус
.aurora version
.aurora activity        — личная статистика (Activity-First)
.aurora shop            — каталог
.aurora shop buy <id>   — покупка
.shop                   — псевдоним каталога
.aurora vote list       — кандидаты текущего месяца
.aurora vote top        — победители прошлых периодов
.aurora vote cast pve 1 — голос
.vote stats             — псевдоним .aurora activity
.aurora reload shop     — перечитать каталог (RBAC: administrator)
```

Права команд по умолчанию `RBAC_PERM_COMMAND_HELP` — то есть любому игроку.
Чтобы закрыть покупки для обычных игроков, поменяйте флаг у `HandleShopBuyCommand`
на нужный `RBAC_PERM_*` из `RBAC.h`.

## 6. Что требуется проверить на живом ядре

Часть API в AC master меняется между релизами. Перед сборкой сверьте:

- `player->GetSession()->GetSessionDbcLocale()` — тип возврата;
- `ChatHandler::PSendSysMessage` с `{}` — используется `StringFormat`, нужен
  `#include "StringFormat.h"` (уже включён);
- `player->CanStoreNewItem(NULL_BAG, NULL_SLOT, ITEM_NONE, nullptr)` — сигнатура
  в вашей ревизии принимает `Item**`; если нет — замените на проверку
  `player->GetFreeInventorySpace()` (или аналог в вашей ветке);
- `Item::SetState` — не обязателен, уберите, если метод не публичный;
- `Field::IsNull()` — присутствует с AC 2022+.

Если что-то из этого не совпадёт с вашей ревизией — пришлите вывод компилятора,
я поправлю точечно.

## 7. Состав среза v7.1.0 (срез 2)

| Файл | Что делает |
|---|---|
| `Core/aurora_config.h` | константы, диапазоны ID, тайминги |
| `Core/aurora_shared.h/.cpp` | enum'ы, ID-хелперы, реестр карт |
| `Core/aurora_bootstrap.cpp` | старт: проверка таблиц, загрузка каталога магазина |
| `Core/aurora_commands.cpp` | `.aurora shop/vote/activity`, алиасы `.shop`,`.vote` |
| `Core/aurora_activity_bridge.cpp` | счётчики активности, flush раз в минуту, награды за ачивки |
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
только показывает состояние и не дублирует расчёт — так две реализации
не могут разойтись.

## 9. Ограничения этого среза

- Достижения (`10001…10099`, `20001…20005`) **не выдаются**: нужен патч клиентских
  `Achievement.dbc` / `Achievement_Criteria.dbc` / `CharTitles.dbc`. В таблицах
  подготовлены правила и награды, C++ их выдаёт, но клиент не покажет
  достижение, которого нет в DBC.
- Эвенты: таблицы и награды заполнены, планировщик (срез 3) ещё не написан.
- `aurora_shop_purchases` — в файле `06`, применяется на character-базу.
- Спеллы `500010…500016`, `500020`, `500030`, `500001` и маунты `60050/60051`,
  `70002/70003/70004` **должны быть заведены в `spell_dbc`** — скрипт их
  только вызывает. Без них боссы просто не кастуют, но не падают.

## 10. Проверка синтаксиса (что реально было сделано)

Все 10 файлов прогнаны через MSVC 19.51 (`cl /Zs`, C++20) с минимальными
заглушками AC-API. Найдено и исправлено в процессе:

- `InstanceScript`/`Map` вызов несуществующего `UpdateEncounterStateForKilledCreature`;
- `GetSource()` вместо `pair.second` при обходе игроков инстанса;
- `Pending&` вместо `PendingCounters&` в четырёх хуках моста активности;
- `Pending player_guid` — переменная с именем, совпадающим с полем структуры.

Финальный статус: 9 из 10 файлов — чисто. `aurora_commands.cpp` требует
подтверждения на живом ядре по трём местам: сигнатура `CanStoreNewItem`,
необходимость `Item::SetState`, а также способ сборки `ChatCommand`-таблиц
(в зависимости от ревизии `ChildCommands` — `std::vector` или `ChatCommand const*`).

Это проверка **синтаксиса и логики типов**, а не линковка: полноценная сборка
возможна только внутри дерева AzerothCore.
