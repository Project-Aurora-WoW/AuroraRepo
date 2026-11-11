-- =====================================================================
-- ElvUI_Aurora / Core/data.lua
-- Static ids shared by every module. Keep in sync with Aurora::enum in
-- src/Aurora/Core/aurora_shared.h and with sql/02_aurora_world_data.sql.
-- =====================================================================

local E, L, V, P, G = unpack(ElvUI)
local AU = E:GetModule('Aurora')

AU.MapID = 950

-- Campaign #1 items
AU.Items = {
    ShardOfFallenStar = 50001,
    AuroraPoints      = 50100,
    BeaconOfLight     = 50010,
    DarkTotem         = 50011,
    MedallionOfNight  = 50020,
    BloodTotemOfNight = 50021,
    WandererCloak     = 50022,
    HeartOfNight      = 50023,
}

-- Quest chains
AU.Quests = {
    Alliance = { 90001, 90002, 90003, 90004, 90005, 90006 },
    Horde    = { 90011, 90012, 90013, 90014, 90015, 90016 },
    Neutral  = { 90021, 90022, 90023, 90024, 90025, 90026 },
}

-- Quest that closes each chain. Completing it marks the campaign done.
AU.ChainFinalQuest = {
    Alliance = 90006,
    Horde    = 90016,
    Neutral  = 90026,
}

-- Achievements (must exist in the patched client Achievement.dbc)
AU.Achievements = {
    FirstStep           = 10001,
    Malakor10           = 10002,
    Malakor25           = 10003,
    NoDeaths            = 10004,
    ShieldBroker        = 10010,
    ShadowHunter        = 10011,
    ShadowGong          = 10012,
    FasterThanDark      = 10013,
    SoloHero            = 10014,
    AltarCapturer       = 10020,
    FallenHero          = 10021,
    LoneWolf            = 10022,
    LightDefender       = 10023,
    Dominion            = 10024,
    PathOfLight         = 10030,
    PathOfBlood         = 10031,
    PathOfBalance       = 10032,
    TripleAgent         = 10033,
    ShardCollector      = 10040,
    LegendaryHunter     = 10041,
    NightCollector      = 10042,
    FullEquip           = 10043,
    HeroOfFallenNight   = 10099,
}

-- Event definitions, mirror of aurora_events
AU.Events = {
    [1] = { name = 'Рассвет Авроры',    type = 'daily'    },
    [2] = { name = 'Прорыв Ноктиса',    type = 'weekly'   },
    [3] = { name = 'Охота на Вестников', type = 'daily'   },
    [4] = { name = 'Битва Хранителей',  type = 'weekly'   },
    [5] = { name = 'Ночь Памяти',       type = 'monthly'  },
    [6] = { name = 'Ярмарка Равновесия', type = 'monthly' },
    [7] = { name = 'Затмение',          type = 'dynamic'  },
    [8] = { name = 'Пробуждение Древних', type = 'seasonal' },
}

-- Worldstate ids: 30001..30008 for events, 30050/30051 for campaign #1
AU.WorldStates = {
    EventBase        = 30000,   -- base + event id
    AltarsControlled = 30050,
    BossShield       = 30051,
}

AU.Colors = {
    Aurora   = { r = 1.00, g = 0.82, b = 0.00 },
    Night    = { r = 0.20, g = 0.10, b = 0.40 },
    Alliance = { r = 0.00, g = 0.40, b = 0.80 },
    Horde    = { r = 0.80, g = 0.10, b = 0.10 },
    Neutral  = { r = 1.00, g = 1.00, b = 1.00 },
    Muted    = { r = 0.60, g = 0.60, b = 0.60 },
}
