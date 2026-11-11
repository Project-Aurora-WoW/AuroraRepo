-- =====================================================================
-- ElvUI_Aurora / Modules/tracker.lua
-- Campaign #1 tracker: altar control + boss shield state.
--
-- Data source: two worldstates written by the instance script
--   WS_ALTARS_CONTROLLED = 30050  (0..4)
--   WS_BOSS_SHIELD       = 30051  (1 = shield up, 0 = broken)
-- The tracker only reads. All campaign state is decided on the server.
-- =====================================================================

local E, L, V, P, G = unpack(ElvUI)
local AU = E:GetModule('Aurora')

local tracker

local function GetWorldState(id)
    if not C_WorldState or not C_WorldState.IsWorldStateActive or not C_WorldState.GetWorldStateValue then
        return 0
    end

    if not C_WorldState.IsWorldStateActive(id) then
        return 0
    end

    return C_WorldState.GetWorldStateValue(id) or 0
end

function AU:InitializeTracker()
    if tracker then
        return
    end

    tracker = CreateFrame('Frame', 'AuroraTracker', UIParent)
    tracker:SetSize(210, 78)
    tracker:SetPoint('TOPRIGHT', UIParent, 'TOPRIGHT', -220, -220)
    tracker:SetFrameStrata('LOW')
    tracker:EnableMouse(false)

    local bg = tracker:CreateTexture(nil, 'BACKGROUND')
    bg:SetAllPoints()
    bg:SetColorTexture(0.05, 0.05, 0.10, 0.82)

    tracker.title = tracker:CreateFontString(nil, 'OVERLAY', 'GameFontNormalSmall')
    tracker.title:SetPoint('TOP', tracker, 'TOP', 0, -8)
    tracker.title:SetTextColor(1.0, 0.82, 0.0)
    tracker.title:SetText('АВРОРА · НОЧЬ ПАДШИХ')

    tracker.altars = tracker:CreateFontString(nil, 'OVERLAY', 'GameFontHighlightSmall')
    tracker.altars:SetPoint('TOPLEFT', tracker, 'TOPLEFT', 12, -28)
    tracker.altars:SetText('Алтари: 0/4')

    tracker.shield = tracker:CreateFontString(nil, 'OVERLAY', 'GameFontHighlightSmall')
    tracker.shield:SetPoint('TOPLEFT', tracker, 'TOPLEFT', 12, -46)
    tracker.shield:SetText('Щит Малакора: активен')

    tracker.hint = tracker:CreateFontString(nil, 'OVERLAY', 'GameFontHighlightSmall')
    tracker.hint:SetPoint('TOPLEFT', tracker, 'TOPLEFT', 12, -62)
    tracker.hint:SetTextColor(0.6, 0.6, 0.6)
    tracker.hint:SetText('')

    tracker:Hide()

    self:RegisterEvent('PLAYER_ENTERING_WORLD', 'TrackerRefresh')
    self:RegisterEvent('ZONE_CHANGED_NEW_AREA', 'TrackerRefresh')

    self:ScheduleRepeatingTimer('TrackerRefresh', 2.0)
end

function AU:TrackerRefresh()
    if not tracker then
        return
    end

    if GetMapID and GetMapID() ~= AU.MapID then
        tracker:Hide()
        return
    end

    tracker:Show()

    local altars = GetWorldState(AU.WorldStates.AltarsControlled)
    local shieldUp = GetWorldState(AU.WorldStates.BossShield)

    tracker.altars:SetText(('Алтари: %d/4'):format(altars))

    if altars >= 3 then
        tracker.altars:SetTextColor(0.2, 1.0, 0.2)
    elseif altars > 0 then
        tracker.altars:SetTextColor(1.0, 0.82, 0.0)
    else
        tracker.altars:SetTextColor(0.9, 0.9, 0.9)
    end

    if shieldUp == 1 then
        tracker.shield:SetText('Щит Малакора: активен')
        tracker.shield:SetTextColor(0.9, 0.4, 0.4)
        tracker.hint:SetText('Захватите 3 алтаря, чтобы снять щит')
    else
        tracker.shield:SetText('Щит Малакора: снят')
        tracker.shield:SetTextColor(0.2, 1.0, 0.2)
        tracker.hint:SetText('Босс уязвим')
    end
end
