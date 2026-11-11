--[[
    Aurora UI - Ночь Падших
    Клиентский HUD модуля «Аврора» для 3.3.5a.

    Аддон только отображает серверное состояние и отправляет команды.
    Никаких расчётов цен, рангов и захвата алтарей на клиенте нет:
    всё проверяет сервер (aurora_commands.cpp).

    Точки данных:
      * валюты            — GetItemCount(50001 / 50100)
      * алтари и щит       — worldstate 30050 / 30051
      * события            — worldstate 30001..30008
      * магазин, голоса    — чат-команды .aurora ...
]]

local ADDON_NAME = ...

local AuroraDBC = AuroraDBC or {}

local ITEM_SHARDS = 50001
local ITEM_AURORA = 50100

local WS_ALTARS_CONTROLLED = 30050
local WS_BOSS_SHIELD       = 30051
local WS_EVENT_BASE        = 30000

local EVENTS = {
    [1] = "Рассвет Авроры",
    [2] = "Прорыв Ноктиса",
    [3] = "Охота на Вестников",
    [4] = "Битва Хранителей",
    [5] = "Ночь Памяти",
    [6] = "Ярмарка Равновесия",
    [7] = "Затмение",
    [8] = "Пробуждение Древних",
}

-- ============================================================================
-- Вспомогательные функции
-- ============================================================================
local function GetWorldState(id)
    if not GetWorldStateUIInfo then
        return 0
    end

    local info = { GetWorldStateUIInfo(id) }
    if not info or not info[1] then
        return 0
    end

    return tonumber(info[2]) or 0
end

local function PlayerHasAura(spellId)
    for i = 1, 40 do
        local name, _, _, _, _, _, _, _, _, spell = UnitAura("player", i)
        if not name then
            break
        end
        if spell == spellId then
            return true
        end
    end
    return false
end

local function CountItem(itemId)
    local count = 0
    for bag = 0, 4 do
        local slots = GetContainerNumSlots(bag)
        for slot = 1, slots do
            local link = GetContainerItemLink(bag, slot)
            if link then
                local id = tonumber(link:match("item:(%d+)"))
                if id == itemId then
                    local _, stack = GetContainerItemInfo(bag, slot)
                    count = count + (stack or 0)
                end
            end
        end
    end
    return count
end

local function MakePanel(name, width, height)
    local frame = CreateFrame("Frame", name, UIParent)
    frame:SetWidth(width)
    frame:SetHeight(height)

    local bg = frame:CreateTexture(nil, "BACKGROUND")
    bg:SetAllPoints(frame)
    bg:SetTexture(0.04, 0.04, 0.10, 0.85)

    local border = frame:CreateTexture(nil, "BORDER")
    border:SetPoint("TOPLEFT", -1, 1)
    border:SetPoint("BOTTOMRIGHT", 1, -1)
    border:SetTexture(1, 0.82, 0, 0.35)

    return frame
end
-- ============================================================================
-- Парсер DBC кампании (см. AuroraDBC.lua)
-- ============================================================================
local function GetCampaignPatchState()
    return AuroraDBC.CheckCampaignPatch()
end

-- ============================================================================
-- 1. Трекер кампании
-- ============================================================================
local tracker = MakePanel("AuroraTracker", 210, 74)
tracker:SetPoint("TOPRIGHT", UIParent, "TOPRIGHT", -220, -220)
tracker:SetFrameStrata("LOW")
tracker:Hide()

tracker.title = tracker:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
tracker.title:SetPoint("TOP", tracker, "TOP", 0, -8)
tracker.title:SetTextColor(1.0, 0.82, 0.0)
tracker.title:SetText("АВРОРА · НОЧЬ ПАДШИХ")

tracker.altars = tracker:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
tracker.altars:SetPoint("TOPLEFT", tracker, "TOPLEFT", 12, -26)
tracker.altars:SetText("Алтари: 0/4")

tracker.shield = tracker:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
tracker.shield:SetPoint("TOPLEFT", tracker, "TOPLEFT", 12, -44)
tracker.shield:SetText("Щит: —")

tracker.hint = tracker:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
tracker.hint:SetPoint("TOPLEFT", tracker, "TOPLEFT", 12, -60)
tracker.hint:SetTextColor(0.6, 0.6, 0.6)
tracker.hint:SetText("")

local function UpdateTracker()
    local _, instanceType = GetInstanceInfo()
    local inAurora = (instanceType == "raid") and (GetRealZoneText() ~= nil)

    local altars = GetWorldState(WS_ALTARS_CONTROLLED)
    local shield = GetWorldState(WS_BOSS_SHIELD)

    if altars == 0 and shield == 0 then
        tracker:Hide()
        return
    end

    tracker:Show()
    tracker.altars:SetFormattedText("Алтари: %d/4", altars)

    if altars >= 3 then
        tracker.altars:SetTextColor(0.2, 1.0, 0.2)
    elseif altars > 0 then
        tracker.altars:SetTextColor(1.0, 0.82, 0.0)
    else
        tracker.altars:SetTextColor(0.9, 0.9, 0.9)
    end

    if shield == 1 then
        tracker.shield:SetText("Щит Малакора: активен")
        tracker.shield:SetTextColor(0.9, 0.4, 0.4)
        tracker.hint:SetText("Захватите 3 алтаря")
    else
        tracker.shield:SetText("Щит Малакора: снят")
        tracker.shield:SetTextColor(0.2, 1.0, 0.2)
        tracker.hint:SetText("Босс уязвим")
    end
end

-- ============================================================================
-- 2. Валюты
-- ============================================================================
local currencyBar = MakePanel("AuroraCurrencyBar", 210, 22)
currencyBar:SetPoint("BOTTOM", UIParent, "BOTTOM", 0, 140)
currencyBar:SetFrameStrata("LOW")

currencyBar.shards = currencyBar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
currencyBar.shards:SetPoint("LEFT", currencyBar, "LEFT", 10, 0)
currencyBar.shards:SetTextColor(0.75, 0.65, 1.0)
currencyBar.shards:SetText("Осколки: 0")

currencyBar.points = currencyBar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
currencyBar.points:SetPoint("RIGHT", currencyBar, "RIGHT", -10, 0)
currencyBar.points:SetTextColor(1.0, 0.82, 0.0)
currencyBar.points:SetText("Аврора: 0")

local function UpdateCurrency()
    currencyBar.shards:SetFormattedText("Осколки: %d", CountItem(ITEM_SHARDS))
    currencyBar.points:SetFormattedText("Аврора: %d", CountItem(ITEM_AURORA))
end

-- ============================================================================
-- 3. Окно эвентов
-- ============================================================================
local eventsFrame = MakePanel("AuroraEventsFrame", 420, 300)
eventsFrame:SetPoint("CENTER", UIParent, "CENTER", 0, 60)
eventsFrame:SetFrameStrata("DIALOG")
eventsFrame:EnableMouse(true)
eventsFrame:SetMovable(true)
eventsFrame:RegisterForDrag("LeftButton")
eventsFrame:SetScript("OnDragStart", eventsFrame.StartMoving)
eventsFrame:SetScript("OnDragStop", eventsFrame.StopMovingOrSizing)
eventsFrame:Hide()

local evTitle = eventsFrame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
evTitle:SetPoint("TOP", eventsFrame, "TOP", 0, -14)
evTitle:SetTextColor(1.0, 0.82, 0.0)
evTitle:SetText("ЭВЕНТЫ АВРОРЫ")

local evClose = CreateFrame("Button", nil, eventsFrame, "UIPanelCloseButton")
evClose:SetPoint("TOPRIGHT", eventsFrame, "TOPRIGHT", -2, -2)

eventsFrame.rows = {}
for eventId = 1, 8 do
    local row = eventsFrame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
    row:SetPoint("TOPLEFT", eventsFrame, "TOPLEFT", 20, -46 - (eventId - 1) * 26)
    row:SetJustifyH("LEFT")
    eventsFrame.rows[eventId] = row
end

local function RefreshEvents()
    local anyActive = false

    for eventId = 1, 8 do
        local row = eventsFrame.rows[eventId]
        local state = GetWorldState(WS_EVENT_BASE + eventId)
        local name = EVENTS[eventId] or "?"

        if state == 1 then
            anyActive = true
            row:SetText("|cff00ff00●|r " .. name)
        else
            row:SetText("|cff555555" .. name .. "|r")
        end
    end

    return anyActive
end

-- ============================================================================
-- 4. Кнопка на миникарте и меню
-- ============================================================================
local menuButton = CreateFrame("Button", "AuroraMinimapButton", Minimap)
menuButton:SetWidth(32)
menuButton:SetHeight(32)
menuButton:SetFrameStrata("MEDIUM")
menuButton:SetPoint("TOPLEFT", Minimap, "TOPLEFT", 2, -96)

local icon = menuButton:CreateTexture(nil, "BACKGROUND")
icon:SetWidth(20)
icon:SetHeight(20)
icon:SetPoint("CENTER")
icon:SetTexture("Interface\\Icons\\INV_Misc_Summonable_Boss_Token")

menuButton:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_LEFT")
    GameTooltip:AddLine("Проект Аврора", 1.0, 0.82, 0.0)
    GameTooltip:AddLine("Кампания #1: Ночь Падших", 0.9, 0.9, 0.9)
    GameTooltip:AddLine("ЛКМ: меню", 0.6, 0.6, 0.6)
    GameTooltip:Show()
end)

menuButton:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)

local menu = MakePanel("AuroraMenu", 460, 300)
menu:SetPoint("CENTER", UIParent, "CENTER", 0, 40)
menu:SetFrameStrata("DIALOG")
menu:EnableMouse(true)
menu:SetMovable(true)
menu:RegisterForDrag("LeftButton")
menu:SetScript("OnDragStart", menu.StartMoving)
menu:SetScript("OnDragStop", menu.StopMovingOrSizing)
menu:Hide()

local menuTitle = menu:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
menuTitle:SetPoint("TOP", menu, "TOP", 0, -14)
menuTitle:SetTextColor(1.0, 0.82, 0.0)
menuTitle:SetText("ПРОЕКТ АВРОРА")

local menuClose = CreateFrame("Button", nil, menu, "UIPanelCloseButton")
menuClose:SetPoint("TOPRIGHT", menu, "TOPRIGHT", -2, -2)

local menuText = menu:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
menuText:SetPoint("TOPLEFT", menu, "TOPLEFT", 24, -80)
menuText:SetPoint("BOTTOMRIGHT", menu, "BOTTOMRIGHT", -24, 24)
menuText:SetJustifyH("LEFT")
menuText:SetJustifyV("TOP")

local MENU_TEXT = {
    campaign = "|cffFFD100Ночь Падших|r\n\n" ..
               "Вход: портал у южного Танариса.\n" ..
               "Захватите 3 из 4 алтарей, чтобы снять щит.\n\n" ..
               "Состояние — в трекере справа сверху.\n\n" ..
               "Прогресс цепочек и алтарей:\n" ..
               "|cffffff00/aurora campaign|r",

    activity = "|cffFFD100Активность|r\n\n" ..
               "Наберите: |cffffff00/aurora activity|r\n\n" ..
               "Уровень персонажа на рейтинг не влияет.",

    shop     = "|cffFFD100Магазин|r\n\n" ..
               "Каталог: |cffffff00/aurora shop|r\n" ..
               "Покупка: |cffffff00/aurora shop buy <id>|r\n\n" ..
               "Цены проверяет сервер.",

    vote     = "|cffFFD100Голосование|r\n\n" ..
               "Кандидаты: |cffffff00/aurora vote list|r\n" ..
               "Голос: |cffffff00/aurora vote cast pve 1|r\n\n" ..
               "Один голос в категории в сутки.",

    events   = "|cffFFD100Эвенты|r\n\n" ..
               "Открыть список активных эвентов.",
}

local function ShowTab(tabId)
    menuText:SetText(MENU_TEXT[tabId] or "")

    if tabId == "events" then
        if RefreshEvents() or true then
            eventsFrame:Show()
        end
    end
end

local TABS = {
    { id = "campaign", label = "Кампания" },
    { id = "activity", label = "Активность" },
    { id = "shop",     label = "Магазин" },
    { id = "vote",     label = "Голосование" },
    { id = "events",   label = "Эвенты" },
}

for index, tab in ipairs(TABS) do
    local button = CreateFrame("Button", nil, menu)
    button:SetWidth(86)
    button:SetHeight(24)
    button:SetPoint("TOPLEFT", menu, "TOPLEFT", 20 + (index - 1) * 88, -52)

    local buttonBg = button:CreateTexture(nil, "BACKGROUND")
    buttonBg:SetAllPoints(button)
    buttonBg:SetTexture(0.15, 0.15, 0.25, 0.9)

    local label = button:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    label:SetPoint("CENTER")
    label:SetText(tab.label)

    button:SetScript("OnClick", function()
        ShowTab(tab.id)
    end)
end

menuButton:SetScript("OnClick", function()
    if menu:IsShown() then
        menu:Hide()
    else
        ShowTab("campaign")
        menu:Show()
    end
end)

-- ============================================================================
-- 5. Тикеры
-- ============================================================================
local ticker = CreateFrame("Frame")
ticker:SetScript("OnUpdate", function(self, elapsed)
    self.elapsed = (self.elapsed or 0) + elapsed

    if self.elapsed < 2.0 then
        return
    end
    self.elapsed = 0

    UpdateTracker()
    UpdateCurrency()

    if eventsFrame:IsShown() then
        RefreshEvents()
    end
end)

local boot = CreateFrame("Frame")
boot:RegisterEvent("PLAYER_ENTERING_WORLD")
boot:RegisterEvent("ZONE_CHANGED_NEW_AREA")
boot:RegisterEvent("UPDATE_WORLD_STATES")
boot:RegisterEvent("BAG_UPDATE")
boot:SetScript("OnEvent", function()
    UpdateTracker()
    UpdateCurrency()
end)

-- ============================================================================
-- 6. Слэш-команды
-- ============================================================================
SLASH_AURORAUI1 = "/aui"
SlashCmdList["AURORAUI"] = function(msg)
    local cmd = (msg or ""):lower():match("^%s*(%S*)")

    if cmd == "events" then
        RefreshEvents()
        eventsFrame:Show()
    elseif cmd == "tracker" then
        UpdateTracker()
    elseif cmd == "menu" then
        ShowTab("campaign")
        menu:Show()
    elseif cmd == "patch" then
        local state, known, total, firstMissing = GetCampaignPatchState()
        if state == "patched" then
            DEFAULT_CHAT_FRAME:AddMessage(format("|cffFFD100[Aurora UI]|r DBC-патч установлен: %d из %d достижений.", known, total))
        elseif state == "partial" then
            DEFAULT_CHAT_FRAME:AddMessage(format("|cffFFD100[Aurora UI]|r DBC-патч неполный: %d из %d, первое отсутствующее ID %d.", known, total, firstMissing))
        elseif state == "unknown" then
            DEFAULT_CHAT_FRAME:AddMessage("|cffFFD100[Aurora UI]|r Проверка DBC недоступна.")
        else
            DEFAULT_CHAT_FRAME:AddMessage("|cffFFD100[Aurora UI]|r DBC-патч не установлен: достижения кампании клиенту неизвестны.")
        end
        menu:Show()
    else
        DEFAULT_CHAT_FRAME:AddMessage("|cffFFD100[Aurora UI]|r /aui menu | events | tracker")
    end
end

local msg = DEFAULT_CHAT_FRAME
msg:AddMessage("|cffFFD100[Aurora UI]|r Модуль 7.1.0 загружен. /aui menu")
