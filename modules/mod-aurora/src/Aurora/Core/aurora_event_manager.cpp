// ============================================================================
// PROJECT AURORA v7.1.0
// aurora_event_manager.cpp - event scheduler.
//
// Schedule types supported by `aurora_events.schedule_value`:
//   time      "HH:MM"        -> daily at HH:MM local server time
//             "SAT:HH:MM"    -> weekly, 3-letter day prefix
//             "FRI:HH:MM", "MON:HH:MM", ...
//   calendar  "D1-D2"        -> repeating within a month (e.g. 1-3, 15-21)
//             "SOLSTICE"     -> 20..22 of March/June/September/December
//   activity  "online:N;activity:M" -> dynamic, starts when the live server
//             has at least N players online and total activity score >= M
//
// Design decisions that differ from the original specification:
//   * Scheduling lives here, in the world process, not in a MySQL EVENT.
//     A DB scheduler cannot reach the character database and needs SUPER.
//   * Start/stop transitions are computed from the clock every tick, so a
//     restart at 18:05 still starts an 18:00 daily event that is still
//     running, instead of silently skipping it for the whole day.
//   * Event state is persisted in `aurora_active_events` so a client that
//     logs in after a restart still sees the right tracker state.
// ============================================================================

#include "ScriptMgr.h"
#include "Player.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "Chat.h"
#include "Language.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "WorldState.h"

#include "aurora_shared.h"

using namespace Aurora;

namespace
{
    constexpr uint32 SCHEDULE_TICK_MS = 10000;
    constexpr uint32 ACTIVITY_TICK_MS = 300000;  // 5 min server-activity rollup
    constexpr uint32 ECLIPSE_MIN_ONLINE   = 300;
    constexpr double ECLIPSE_MIN_ACTIVITY = 50000.0;
    constexpr uint32 ANNOUNCE_RETRY_MS = 600000; // do not spam the event banner

    struct EventDefinition
    {
        uint32      id            = 0;
        std::string internalName;
        std::string nameRu;
        std::string nameEn;
        std::string descRu;
        std::string descEn;
        std::string type;           // daily / weekly / monthly / seasonal / dynamic
        uint32      durationHours = 0;
        std::string scheduleType;   // time / calendar / activity
        std::string scheduleValue;
        uint32      mapId         = 0;
        uint32      worldStateId  = 0;
        int32       weekday       = -1;  // 0 = Sunday, -1 = every day
        int32       hour          = -1;
        int32       minute        = 0;
        int32       monthDayFrom  = 0;
        int32       monthDayTo    = 0;
        bool        solstice      = false;
        uint32      minOnline     = 0;
        double      minActivity   = 0.0;
    };

    struct ActiveEvent
    {
        uint32   rowId      = 0;   // aurora_active_events.id
        uint32   eventId    = 0;
        time_t   startedAt  = 0;
        time_t   endsAt     = 0;
        bool     announced  = false;
    };

    std::unordered_map<uint32, EventDefinition> g_definitions;
    std::unordered_map<uint32, ActiveEvent>     g_active;   // keyed by eventId

    // ------------------------------------------------------------------------
    // Time helpers
    // ------------------------------------------------------------------------
    tm LocalNow()
    {
        time_t now = time(nullptr);
        tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        return localTime;
    }

    int32 ParseWeekdayPrefix(std::string const& value)
    {
        static const char* names[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

        if (value.size() < 4 || value[3] != ':')
            return -1;

        std::string prefix = value.substr(0, 3);
        for (int32 i = 0; i < 7; ++i)
            if (prefix == names[i])
                return i;

        return -1;
    }

    bool ParseTimeOfDay(std::string const& value, int32& hour, int32& minute)
    {
        int h = 0, m = 0;
        if (std::sscanf(value.c_str(), "%d:%d", &h, &m) != 2)
            return false;
        if (h < 0 || h > 23 || m < 0 || m > 59)
            return false;

        hour = h;
        minute = m;
        return true;
    }

    // ------------------------------------------------------------------------
    // Definition parsing - happens once at startup, not per tick.
    // ------------------------------------------------------------------------
    bool ParseSchedule(EventDefinition& definition)
    {
        std::string const& value = definition.scheduleValue;

        if (definition.scheduleType == "time")
        {
            int32 prefix = ParseWeekdayPrefix(value);
            std::string timePart = (prefix >= 0) ? value.substr(4) : value;

            definition.weekday = prefix;
            return ParseTimeOfDay(timePart, definition.hour, definition.minute);
        }

        if (definition.scheduleType == "calendar")
        {
            if (value == "SOLSTICE")
            {
                definition.solstice = true;
                return true;
            }

            int from = 0, to = 0;
            if (std::sscanf(value.c_str(), "%d-%d", &from, &to) != 2)
                return false;

            if (from < 1 || to < from || to > 31)
                return false;

            definition.monthDayFrom = from;
            definition.monthDayTo = to;
            return true;
        }

        if (definition.scheduleType == "activity")
        {
            // "online:300;activity:50000"
            uint32 online = 0;
            double activity = 0.0;
            std::sscanf(value.c_str(), "online:%u;activity:%lf", &online, &activity);

            definition.minOnline = online;
            definition.minActivity = activity;
            return true;
        }

        return false;
    }

    // ------------------------------------------------------------------------
    // "Should this event be running right now?"
    // ------------------------------------------------------------------------
    bool IsInScheduleWindow(EventDefinition const& definition)
    {
        tm const localTime = LocalNow();

        if (definition.scheduleType == "time")
        {
            if (definition.weekday >= 0 && localTime.tm_wday != definition.weekday)
                return false;

            if (definition.hour < 0)
                return false;

            // Window may cross midnight when duration > 24h; handled by
            // comparing the elapsed minutes since the scheduled start.
            long startMinutes = definition.hour * 60 + definition.minute;
            long nowMinutes = localTime.tm_hour * 60 + localTime.tm_min;
            long durationMinutes = static_cast<long>(definition.durationHours) * 60;

            long delta = nowMinutes - startMinutes;
            if (delta < 0 && durationMinutes > 1440)
                delta += 1440;

            return delta >= 0 && delta < durationMinutes;
        }

        if (definition.scheduleType == "calendar")
        {
            if (definition.solstice)
            {
                int const month = localTime.tm_mon + 1;
                int const day = localTime.tm_mday;
                bool const inSolsticeMonth =
                    (month == 3 || month == 6 || month == 9 || month == 12);

                return inSolsticeMonth && day >= 20 && day <= 22;
            }

            int const day = localTime.tm_mday;
            return day >= definition.monthDayFrom && day <= definition.monthDayTo;
        }

        if (definition.scheduleType == "activity")
        {
            if (definition.minOnline == 0 && definition.minActivity <= 0.0)
                return false;

            uint32 online = static_cast<uint32>(ObjectAccessor::GetPlayers().size());
            if (online < definition.minOnline)
                return false;

            QueryResult result = WorldDatabase.Query(
                "SELECT `total_activity_score` FROM `aurora_server_activity` "
                "WHERE `date` = CURDATE()");

            double score = 0.0;
            if (result)
                score = result->Fetch()[0].Get<double>();

            return score >= definition.minActivity;
        }

        return false;
    }

    // ------------------------------------------------------------------------
    // Persistence
    // ------------------------------------------------------------------------
    void PersistStart(EventDefinition const& definition, time_t start, time_t end)
    {
        WorldDatabase.Execute(
            "INSERT INTO `aurora_active_events` "
            "(`event_id`, `started_at`, `ends_at`, `status`) "
            "VALUES ({}, FROM_UNIXTIME({}), FROM_UNIXTIME({}), 'active')",
            definition.id, start, end);
    }

    void PersistStop(EventDefinition const& definition)
    {
        WorldDatabase.Execute(
            "UPDATE `aurora_active_events` SET `status` = 'completed' "
            "WHERE `event_id` = {} AND `status` = 'active'", definition.id);
    }

    // ------------------------------------------------------------------------
    // Announcements
    // ------------------------------------------------------------------------
    void Announce(EventDefinition const& definition, bool started)
    {
        std::string const& name = definition.nameRu;
        std::string message = started
            ? "|cffFFD100[ЭВЕНТ]|r |cffffff00" + name + "|r начался! " + definition.descRu
            : "|cffFFD100[ЭВЕНТ]|r |cffffff00" + name + "|r завершился.";

        ChatHandler handler(nullptr);
        handler.SendWorldText(message.c_str());
    }
    void SetWorldState(EventDefinition const& definition, uint32 value)
    {
        uint32 state = definition.worldStateId > 0
            ? definition.worldStateId
            : (30000 + definition.id);

        sWorldState->setWorldState(state, value);
    }
}

    // Participation credit: every player online during an active event gets
    // one row in aurora_event_progress per active event instance.
    void CreditParticipation()
    {
        for (auto const& pair : g_active)
        {
            uint32 const rowId = pair.second.rowId;
            if (rowId == 0)
                continue;

            for (auto const& playerPair : ObjectAccessor::GetPlayers())
            {
                Player* player = playerPair.second;
                if (!player || !player->IsInWorld())
                    continue;

                CharacterDatabase.Execute(
                    "INSERT INTO `aurora_event_progress` "
                    "(`active_event_id`, `player_guid`, `participation_sec`) "
                    "VALUES ({}, {}, {}) "
                    "ON DUPLICATE KEY UPDATE "
                    "`participation_sec` = `participation_sec` + VALUES(`participation_sec`)",
                    rowId, player->GetGUID().GetCounter(), SCHEDULE_TICK_MS / 1000);
            }
        }
    }

    void AuroraGrantEventRewards(uint32 activeEventId, uint32 eventId);

class aurora_event_manager : public WorldScript, public PlayerScript
{
public:
    aurora_event_manager()
        : WorldScript("aurora_event_manager_world"),
          PlayerScript("aurora_event_manager_player") { }

    void OnStartup() override
    {
        LoadDefinitions();
        RecoverActiveEvents();

        LOG_INFO("module", "[Aurora] Event manager ready: {} definitions, {} active.",
                 g_definitions.size(), g_active.size());
    }

    void OnShutdown() override
    {
        for (auto const& pair : g_active)
            PersistStop(g_definitions[pair.first]);
    }

    void OnUpdate(uint32 diff) override
    {
        if (!IsEventsEnabled())
            return;

        _timer += diff;
        _activityTimer += diff;

        // Daily server-activity rollup, independent from the schedule tick.
        if (_activityTimer >= ACTIVITY_TICK_MS)
        {
            _activityTimer = 0;
            UpdateServerActivity();
        }

        if (_timer < SCHEDULE_TICK_MS)
            return;

        _timer = 0;
        Tick();
    }
    // ------------------------------------------------------------------------
    // Login: tell the player what is running, once per session
    // ------------------------------------------------------------------------
    void OnPlayerLogin(Player* player) override
    {
        if (!player || g_active.empty())
            return;

        tm const localTime = LocalNow();
        ChatHandler handler(player->GetSession());

        for (auto const& pair : g_active)
        {
            auto itr = g_definitions.find(pair.first);
            if (itr == g_definitions.end())
                continue;

            EventDefinition const& definition = itr->second;
            long remaining = static_cast<long>(pair.second.endsAt) - static_cast<long>(time(nullptr));
            if (remaining <= 0)
                continue;

            handler.PSendSysMessage(
                "|cffFFD100[ЭВЕНТ]|r |cffffff00%s|r активен, осталось %ld ч %ld мин.",
                definition.nameRu.c_str(), remaining / 3600, (remaining % 3600) / 60);
        }

        (void)localTime;
    }

private:
    // ------------------------------------------------------------------------
    // Startup
    // ------------------------------------------------------------------------
    void LoadDefinitions()
    {
        g_definitions.clear();

        QueryResult result = WorldDatabase.Query(
            "SELECT `id`, `internal_name`, `name_ru`, `name_en`, `description_ru`, "
            "`description_en`, `type`, `duration_hours`, `schedule_type`, "
            "`schedule_value`, `map_id`, `world_state_id` "
            "FROM `aurora_events` WHERE `enabled` = 1");

        if (!result)
        {
            LOG_ERROR("module", "[Aurora] No enabled rows in `aurora_events` - scheduler is idle.");
            return;
        }

        do
        {
            Field* fields = result->Fetch();

            EventDefinition definition;
            definition.id           = fields[0].Get<uint32>();
            definition.internalName = fields[1].Get<std::string>();
            definition.nameRu       = fields[2].Get<std::string>();
            definition.nameEn       = fields[3].Get<std::string>();
            definition.descRu       = fields[4].Get<std::string>();
            definition.descEn       = fields[5].Get<std::string>();
            definition.type         = fields[6].Get<std::string>();
            definition.durationHours = fields[7].Get<uint32>();
            definition.scheduleType = fields[8].Get<std::string>();
            definition.scheduleValue = fields[9].Get<std::string>();
            definition.mapId        = fields[10].Get<uint32>();
            definition.worldStateId = fields[11].Get<uint32>();

            if (!ParseSchedule(definition))
            {
                LOG_ERROR("module", "[Aurora] Event '{}' has an unparsable schedule '{}' "
                                    "({}) - skipped.", definition.internalName,
                          definition.scheduleValue, definition.scheduleType);
                continue;
            }

            g_definitions[definition.id] = definition;
        } while (result->NextRow());
    }

    // Restore events that were active before the last restart, so the client
    // tracker and the world state stay consistent.
    void RecoverActiveEvents()
    {
        g_active.clear();

        QueryResult result = WorldDatabase.Query(
            "SELECT a.`id`, a.`event_id`, UNIX_TIMESTAMP(a.`started_at`), "
            "UNIX_TIMESTAMP(a.`ends_at`) "
            "FROM `aurora_active_events` a WHERE a.`status` = 'active'");

        if (!result)
            return;

        time_t const now = time(nullptr);

        do
        {
            Field* fields = result->Fetch();

            ActiveEvent active;
            active.rowId     = fields[0].Get<uint32>();
            active.eventId   = fields[1].Get<uint32>();
            active.startedAt = static_cast<time_t>(fields[2].Get<uint64>());
            active.endsAt    = static_cast<time_t>(fields[3].Get<uint64>());
            active.announced = true;

            auto itr = g_definitions.find(active.eventId);
            if (itr == g_definitions.end())
                continue;

            if (now >= active.endsAt)
            {
                PersistStop(itr->second);
                continue;
            }

            g_active[active.eventId] = active;
            SetWorldState(itr->second, 1);

            LOG_INFO("module", "[Aurora] Event '{}' restored ({} min left).",
                     itr->second.internalName, (active.endsAt - now) / 60);
        } while (result->NextRow());
    }

    // ------------------------------------------------------------------------
    // Tick
    // ------------------------------------------------------------------------
    void Tick()
    {
        time_t const now = time(nullptr);

        // 1) stop what has ended
        std::vector<uint32> toStop;
        for (auto const& pair : g_active)
            if (now >= pair.second.endsAt)
                toStop.push_back(pair.first);

        for (uint32 eventId : toStop)
            StopEvent(eventId);

        // 1b) a running event whose window closed (server was down at the end
        // of its schedule, or the row was edited) must not run forever.
        std::vector<uint32> toExpire;
        for (auto const& pair : g_active)
        {
            auto itr = g_definitions.find(pair.first);
            if (itr == g_definitions.end())
                continue;

            if (!IsInScheduleWindow(itr->second)
                && pair.second.endsAt > now + static_cast<time_t>(itr->second.durationHours) * 3600)
                toExpire.push_back(pair.first);
        }

        for (uint32 eventId : toExpire)
            CancelEvent(eventId);

        // 2) start what should be running
        for (auto const& pair : g_definitions)
        {
            uint32 eventId = pair.first;
            EventDefinition const& definition = pair.second;

            if (g_active.count(eventId))
                continue;

            if (!IsInScheduleWindow(definition))
                continue;

            StartEvent(definition);
        }

        // 4) credit participation time to everyone online during an active event
        CreditParticipation();


        // 3) repeat the banner for long events that are still running
        for (auto const& pair : g_active)
        {
            ActiveEvent const& active = pair.second;
            if (active.announced)
                continue;

            auto itr = g_definitions.find(active.eventId);
            if (itr == g_definitions.end())
                continue;

            Announce(itr->second, true);
            g_active[active.eventId].announced = true;
        }
    }

    void StartEvent(EventDefinition const& definition)
    {
        time_t const now = time(nullptr);
        time_t end = now + static_cast<time_t>(definition.durationHours) * 3600;

        PersistStart(definition, now, end);

        ActiveEvent active;
        active.eventId   = definition.id;
        active.startedAt = now;
        active.endsAt    = end;
        active.announced = true;

        g_active[definition.id] = active;
        SetWorldState(definition, 1);
        Announce(definition, true);

        LOG_INFO("module", "[Aurora] Event '{}' started, ends in {} h.",
                 definition.internalName, definition.durationHours);
    }

    // Same as StopEvent, but the instance is marked 'cancelled' instead of
    // 'completed' because it never reached its scheduled end. Rewards are not
    // granted: the event did not run its full duration.
    void CancelEvent(uint32 eventId)
    {
        auto activeItr = g_active.find(eventId);
        if (activeItr == g_active.end())
            return;

        ActiveEvent const active = activeItr->second;
        g_active.erase(activeItr);

        auto itr = g_definitions.find(eventId);
        if (itr == g_definitions.end())
            return;

        WorldDatabase.Execute(
            "UPDATE `aurora_active_events` SET `status` = 'cancelled' "
            "WHERE `id` = {} AND `status` = 'active'",
            active.rowId);

        // The instance ran and players took part, so the participation they
        // earned is paid out instead of being silently dropped. Only the
        // time-based tiers can be reached before the scheduled end, which is
        // exactly what `AuroraGrantEventRewards` re-checks per player.
        AuroraGrantEventRewards(active.rowId, eventId);

        SetWorldState(itr->second, 0);
        Announce(itr->second, false);

        LOG_INFO("module", "[Aurora] Event '{}' cancelled: its schedule window closed "
                           "while the instance was still running.", itr->second.internalName);
    }

    void StopEvent(uint32 eventId)
    {
        auto activeItr = g_active.find(eventId);
        if (activeItr == g_active.end())
            return;

        g_active.erase(activeItr);

        auto itr = g_definitions.find(eventId);
        if (itr == g_definitions.end())
            return;

        // Grant participation rewards before the progress rows are orphaned.
        AuroraGrantEventRewards(activeItr->second.rowId, eventId);

        PersistStop(itr->second);
        SetWorldState(itr->second, 0);
        Announce(itr->second, false);

        LOG_INFO("module", "[Aurora] Event '{}' ended.", itr->second.internalName);
    }

    // ------------------------------------------------------------------------
    // Daily server activity rollup.
    //  peak_online   - highest simultaneous player count seen today
    //  activity      - sum of every player total_score
    //  eclipse       - dynamic event trigger, latched once per day
    // Read by IsInScheduleWindow() for schedule_type = "activity".
    // ------------------------------------------------------------------------
    void UpdateServerActivity()
    {
        uint32 const online = static_cast<uint32>(ObjectAccessor::GetPlayers().size());

        QueryResult scores = CharacterDatabase.Query(
            "SELECT COALESCE(SUM(`total_score`), 0) FROM `aurora_player_activity`");

        double total = 0.0;
        if (scores)
            total = scores->Fetch()[0].Get<double>();

        CharacterDatabase.Execute(
            "INSERT INTO `aurora_server_activity` "
            "(`date`, `peak_online`, `total_activity_score`) VALUES (CURDATE(), {}, {}) "
            "ON DUPLICATE KEY UPDATE "
            "`peak_online` = GREATEST(`peak_online`, VALUES(`peak_online`)), "
            "`total_activity_score` = VALUES(`total_activity_score`)",
            online, total);

        // Latch the eclipse trigger so the dynamic event fires once a day even
        // if the threshold is crossed during several rollups.
        if (online >= ECLIPSE_MIN_ONLINE && total >= ECLIPSE_MIN_ACTIVITY)
        {
            CharacterDatabase.Execute(
                "UPDATE `aurora_server_activity` SET `eclipse_triggered` = 1, "
                "`eclipse_time` = IFNULL(`eclipse_time`, NOW()) "
                "WHERE `date` = CURDATE() AND `eclipse_triggered` = 0");
        }

        LOG_DEBUG("module", "[Aurora] Server activity: online={}, score={:.2f}.", online, total);
    }

    uint32 _timer = 0;
    uint32 _activityTimer = 0;
};

void AddSC_aurora_event_manager()
{
    new aurora_event_manager();
}
