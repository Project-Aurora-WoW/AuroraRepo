-- =====================================================================
-- ElvUI_Aurora / Core/init.lua
-- Module bootstrap. Written against the real ElvUI 3.3.5 API:
--   E:GetModule() / E:NewModule(name, mixins...) / E:RegisterModule()
-- The original draft used E:AuraRegisterModuleCallback(), which does not
-- exist; every module is registered explicitly in Aurora:Initialize().
-- =====================================================================

local E, L, V, P, G = unpack(ElvUI)
local AU = E:NewModule('Aurora', 'AceEvent-3.0', 'AceTimer-3.0')

_G.Aurora = AU

AU.Version = '7.1.0'
AU.initialized = false

function AU:Initialize()
    if self.initialized then
        return
    end
    self.initialized = true

    self.db = E.db.aurora or {}
    E.db.aurora = self.db

    if self.db.enabled == false then
        return
    end

    self:InitializeTracker()
    self:InitializeEvents()
    self:InitializeCurrency()
    self:InitializeMainMenu()

    self:RegisterEvent('PLAYER_ENTERING_WORLD', 'OnEnteringWorld')

    E:Print(L['|cffFFD100[Aurora]|r Модуль v7.1.0 загружен.'] or '[Aurora] v7.1.0 loaded.')
end

function AU:OnEnteringWorld()
    self:UnregisterEvent('PLAYER_ENTERING_WORLD')

    if GetLocale() ~= nil then
        local locale = GetLocale()
        self.locale = (locale == 'ruRU') and 'ruRU' or 'enUS'
    end

    self:RefreshAll()
end

function AU:RefreshAll()
    if self.TrackerRefresh then
        self:TrackerRefresh()
    end
    if self.EventRefresh then
        self:EventRefresh()
    end
    if self.CurrencyRefresh then
        self:CurrencyRefresh()
    end
end

-- Called by ElvUI when the profile changes; re-reads the saved settings.
function AU:ProfileUpdate()
    self.db = E.db.aurora or {}
    self:RefreshAll()
end

E:RegisterModule(AU:GetName())
