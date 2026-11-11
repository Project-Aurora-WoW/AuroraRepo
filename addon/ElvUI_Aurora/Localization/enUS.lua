-- =====================================================================
-- ElvUI_Aurora / Localization/enUS.lua
-- =====================================================================

local E, L, V, P, G = unpack(ElvUI)
local AU = E:GetModule('Aurora')

local strings = {
    ['Oskolki']              = 'Shards',
    ['Ochki Avrory']         = 'Aurora Points',
    ['Altari']               = 'Altars',
    ['Shchit Malakora']      = 'Malakor shield',
}

for key, value in pairs(strings) do
    AU.L[key] = value
end
