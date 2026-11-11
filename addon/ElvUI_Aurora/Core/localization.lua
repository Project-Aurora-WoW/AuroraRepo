-- =====================================================================
-- ElvUI_Aurora / Core/localization.lua
-- Shared string table. Locale files fill it in; missing keys fall back to
-- the key itself so a half-translated build never shows "nil".
-- =====================================================================

local E, L, V, P, G = unpack(ElvUI)
local AU = E:GetModule('Aurora')

-- Local L is used instead of the global when ElvUI already defines a key.
AU.L = setmetatable({}, {
    __index = function(tbl, key)
        return key
    end,
})

function AU:T(key)
    return self.L[key] or key
end

function AU:Tr(key)
    return self.L[key] or key
end
