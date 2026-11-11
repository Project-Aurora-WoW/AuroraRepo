-- =====================================================================
-- ElvUI_Aurora / Modules/events.lua
-- Active event display.
--
-- Data source: WorldState ids 30001..30008, set by
-- aurora_event_manager.cpp to 1 while an event runs and 0 otherwise.
-- The client does NOT compute schedules: it only reflects server state, so
-- a player with a wrong system clock still sees the correct event.
-- =====================================================================

local E, L, V, P, G = unpack(ElvUI)
local AU = E:GetModule('Aurora')

local eventsFrame
local eventBanner

local function IsEventActive(eventId)
    if not C_WorldState or not C_WorldState.IsWorldStateActive then
        return false
    end

    local stateId = AU.WorldStates.EventBase + eventId
    if not C_WorldState.IsWorldStateActive(stateId) then
        return false
    end

    return (C_WorldState.GetWorldStateValue(stateId) or 0) == 1
end

function AU:InitializeEvents()
    if eventsFrame then
        return
    end

    eventsFrame = CreateFrame('Frame', 'AuroraEventsFrame', UIParent)
    eventsFrame:SetSize(420, 300)
    eventsFrame:SetPoint('CENTER', UIParent, 'CENTER', 0, 60)
    eventsFrame:SetFrameStrata('DIALOG')
    eventsFrame:EnableMouse(true)
    eventsFrame:SetMovable(true)
    eventsFrame:RegisterForDrag('LeftButton')
    eventsFrame:SetScript('OnDragStart', eventsFrame.StartMoving)
    eventsFrame:SetScript('OnDragStop', eventsFrame.StopMovingOrSizing)
    eventsFrame:Hide()

    local bg = eventsFrame:CreateTexture(nil, 'BACKGROUND')
    bg:SetAllPoints()
    bg:SetColorTexture(0.04, 0.04, 0.10, 0.94)

    local title = eventsFrame:CreateFontString(nil, 'OVERLAY', 'GameFontNormalLarge')
    title:SetPoint('TOP', eventsFrame, 'TOP', 0, -14)
    title:SetTextColor(1.0, 0.82, 0.0)
    title:SetText('ЭВЕНТЫ ПРОЕКТА АВРОРА')

    local close = CreateFrame('Button', nil, eventsFrame, 'UIPanelCloseButton')
    close:SetPoint('TOPRIGHT', eventsFrame, 'TOPRIGHT', -2, -2)

    eventsFrame.rows = {}

    for eventId = 1, 8 do
        local row = eventsFrame:CreateFontString(nil, 'OVERLAY', 'GameFontHighlight')
        row:SetPoint('TOPLEFT', eventsFrame, 'TOPLEFT', 20, -46 - (eventId - 1) * 26)
        row:SetJustifyH('LEFT')
        eventsFrame.rows[eventId] = row
    end

    -- Login / zone banner, reused instead of spamming chat
    eventBanner = CreateFrame('Frame', nil, UIParent)
    eventBanner:SetSize(340, 46)
    eventBanner:SetPoint('TOP', UIParent, 'TOP', 0, -140)
    eventBanner:Hide()

    local bannerBg = eventBanner:CreateTexture(nil, 'BACKGROUND')
    bannerBg:SetAllPoints()
    bannerBg:SetColorTexture(0.05, 0.05, 0.12, 0.85)

    eventBanner.text = eventBanner:CreateFontString(nil, 'OVERLAY', 'GameFontNormal')
    eventBanner.text:SetPoint('CENTER')

    self:ScheduleRepeatingTimer('EventRefresh', 5.0)
end

function AU:EventRefresh()
    if not eventsFrame then
        return
    end

    local anyActive = false

    for eventId = 1, 8 do
        local row = eventsFrame.rows[eventId]
        local definition = AU.Events[eventId]
        if row and definition then
            if IsEventActive(eventId) then
                anyActive = true
                row:SetText(('|cff00ff00●|r %s |cff888888(%s)'):format(definition.name, definition.type))
            else
                row:SetText(('|cff555555● %s|r'):format(definition.name))
            end
        end
    end

    if anyActive and eventsFrame:IsShown() then
        eventsFrame:Show()
    end
end

function AU:ToggleEventsFrame()
    if not eventsFrame then
        return
    end

    if eventsFrame:IsShown() then
        eventsFrame:Hide()
    else
        self:EventRefresh()
        eventsFrame:Show()
    end
end
