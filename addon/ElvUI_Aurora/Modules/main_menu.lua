-- =====================================================================
-- ElvUI_Aurora / Modules/main_menu.lua
-- Minimap button + tabbed window.
--
-- Chat commands are handled by the SERVER (aurora_commands.cpp), not here:
-- the addon only sends "/aurora ..." so the server stays the single source
-- of truth for prices, permissions and vote validity.
-- =====================================================================

local E, L, V, P, G = unpack(ElvUI)
local AU = E:GetModule('Aurora')

local menuButton
local mainMenu

local TABS = {
    { id = 'campaign',   label = 'Кампания' },
    { id = 'activity',   label = 'Активность' },
    { id = 'shop',       label = 'Магазин' },
    { id = 'vote',       label = 'Голосование' },
    { id = 'events',     label = 'Эвенты' },
}

function AU:InitializeMainMenu()
    if menuButton then
        return
    end

    menuButton = CreateFrame('Button', 'AuroraMinimapButton', Minimap)
    menuButton:SetSize(32, 32)
    menuButton:SetFrameStrata('MEDIUM')
    menuButton:SetPoint('TOPLEFT', Minimap, 'TOPLEFT', 2, -96)

    local icon = menuButton:CreateTexture(nil, 'BACKGROUND')
    icon:SetSize(20, 20)
    icon:SetPoint('CENTER')
    icon:SetTexture('Interface\\Icons\\INV_Misc_Summonable_Boss_Token')

    local border = menuButton:CreateTexture(nil, 'OVERLAY')
    border:SetPoint('TOPLEFT', -2, 2)
    border:SetPoint('BOTTOMRIGHT', 2, -2)
    border:SetTexture('Interface\\Minimap\\UI-Minimap-Background')

    menuButton:SetScript('OnEnter', function(self)
        GameTooltip:SetOwner(self, 'ANCHOR_LEFT')
        GameTooltip:AddLine('Проект Аврора', 1.0, 0.82, 0.0)
        GameTooltip:AddLine('Кампания #1: Ночь Падших', 0.9, 0.9, 0.9)
        GameTooltip:AddLine('ЛКМ: открыть меню', 0.6, 0.6, 0.6)
        GameTooltip:Show()
    end)

    menuButton:SetScript('OnLeave', function()
        GameTooltip:Hide()
    end)

    menuButton:SetScript('OnClick', function()
        AU:ToggleMainMenu()
    end)

    self:CreateMainMenuFrame()
end

function AU:CreateMainMenuFrame()
    mainMenu = CreateFrame('Frame', 'AuroraMainMenu', UIParent)
    mainMenu:SetSize(520, 360)
    mainMenu:SetPoint('CENTER', UIParent, 'CENTER', 0, 40)
    mainMenu:SetFrameStrata('DIALOG')
    mainMenu:EnableMouse(true)
    mainMenu:SetMovable(true)
    mainMenu:RegisterForDrag('LeftButton')
    mainMenu:SetScript('OnDragStart', mainMenu.StartMoving)
    mainMenu:SetScript('OnDragStop', mainMenu.StopMovingOrSizing)
    mainMenu:Hide()

    local bg = mainMenu:CreateTexture(nil, 'BACKGROUND')
    bg:SetAllPoints()
    bg:SetColorTexture(0.04, 0.04, 0.10, 0.95)

    local title = mainMenu:CreateFontString(nil, 'OVERLAY', 'GameFontNormalLarge')
    title:SetPoint('TOP', mainMenu, 'TOP', 0, -14)
    title:SetTextColor(1.0, 0.82, 0.0)
    title:SetText('ПРОЕКТ АВРОРА')

    local close = CreateFrame('Button', nil, mainMenu, 'UIPanelCloseButton')
    close:SetPoint('TOPRIGHT', mainMenu, 'TOPRIGHT', -2, -2)

    mainMenu.tabs = {}
    mainMenu.content = mainMenu:CreateFontString(nil, 'OVERLAY', 'GameFontHighlight')
    mainMenu.content:SetPoint('TOPLEFT', mainMenu, 'TOPLEFT', 24, -110)
    mainMenu.content:SetPoint('BOTTOMRIGHT', mainMenu, 'BOTTOMRIGHT', -24, 24)
    mainMenu.content:SetJustifyH('LEFT')
    mainMenu.content:SetJustifyV('TOP')

    for index, tab in ipairs(TABS) do
        local button = CreateFrame('Button', nil, mainMenu)
        button:SetSize(92, 24)
        button:SetPoint('TOPLEFT', mainMenu, 'TOPLEFT', 20 + (index - 1) * 96, -76)

        local buttonBg = button:CreateTexture(nil, 'BACKGROUND')
        buttonBg:SetAllPoints()
        buttonBg:SetColorTexture(0.15, 0.15, 0.25, 0.9)

        local label = button:CreateFontString(nil, 'OVERLAY', 'GameFontNormalSmall')
        label:SetPoint('CENTER')
        label:SetText(tab.label)

        button:SetScript('OnClick', function()
            AU:ShowTab(tab.id)
        end)

        mainMenu.tabs[tab.id] = button
    end
end

function AU:ToggleMainMenu()
    if not mainMenu then
        return
    end

    if mainMenu:IsShown() then
        mainMenu:Hide()
    else
        self:ShowTab('campaign')
        mainMenu:Show()
    end
end

function AU:ShowTab(tabId)
    if not mainMenu then
        return
    end

    -- Tab content is a short pointer to the server command. The addon does
    -- not cache shop prices or rankings: the server checks every purchase.
    local text

    if tabId == 'campaign' then
        text = '|cffFFD100Ночь Падших|r\n\n' ..
               'Вход: портал у южного Танариса (фаза 0x0200).\n' ..
               'Захватите 3 из 4 алтарей, чтобы снять щит с Малакора.\n\n' ..
               'Индикатор состояния — в правом верхнем углу.'
    elseif tabId == 'activity' then
        text = '|cffFFD100Ваша активность|r\n\n' ..
               'Наберите в чате: |cffffff00/aurora activity|r\n\n' ..
               'Уровень персонажа на рейтинг не влияет —\n' ..
               'считается только реальная активность.'
    elseif tabId == 'shop' then
        text = '|cffFFD100Магазин|r\n\n' ..
               'Каталог: |cffffff00/aurora shop|r\n' ..
               'Покупка: |cffffff00/aurora shop buy <id>|r\n\n' ..
               'Цены и наличие проверяет сервер.'
    elseif tabId == 'vote' then
        text = '|cffFFD100Голосование|r\n\n' ..
               'Кандидаты: |cffffff00/aurora vote list|r\n' ..
               'Голос: |cffffff00/aurora vote cast pve 1|r\n' ..
               'Итоги: |cffffff00/aurora vote top|r\n\n' ..
               'Один голос в категории в сутки.'
    elseif tabId == 'events' then
        text = '|cffFFD100Эвенты|r\n\n' ..
               'Список активных эвентов — в отдельном окне.\n' ..
               'Нажмите кнопку ниже.'
        self:ToggleEventsFrame()
    end

    mainMenu.content:SetText(text or '')
end
