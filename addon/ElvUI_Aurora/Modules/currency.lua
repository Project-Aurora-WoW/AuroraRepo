-- =====================================================================
-- ElvUI_Aurora / Modules/currency.lua
-- Aurora currencies in the ElvUI datatext strip.
--
-- Shard of Fallen Star (50001) and Aurora Points (50100) are ordinary items
-- in Aurora's design, not real client currencies, so the value comes from
-- GetItemCount, refreshed on a timer rather than from a currency event.
-- =====================================================================

local E, L, V, P, G = unpack(ElvUI)
local AU = E:GetModule('Aurora')

local shardText, pointsText

function AU:InitializeCurrency()
    if shardText then
        return
    end

    local panel = CreateFrame('Frame', 'AuroraCurrencyBar', UIParent)
    panel:SetSize(200, 22)
    panel:SetPoint('BOTTOM', UIParent, 'BOTTOM', 0, 140)
    panel:SetFrameStrata('LOW')

    local bg = panel:CreateTexture(nil, 'BACKGROUND')
    bg:SetAllPoints()
    bg:SetColorTexture(0.05, 0.05, 0.10, 0.75)

    shardText = panel:CreateFontString(nil, 'OVERLAY', 'GameFontHighlightSmall')
    shardText:SetPoint('LEFT', panel, 'LEFT', 10, 0)
    shardText:SetTextColor(0.75, 0.65, 1.00)

    pointsText = panel:CreateFontString(nil, 'OVERLAY', 'GameFontHighlightSmall')
    pointsText:SetPoint('RIGHT', panel, 'RIGHT', -10, 0)
    pointsText:SetTextColor(1.00, 0.82, 0.00)

    self:ScheduleRepeatingTimer('CurrencyRefresh', 10.0)
end

function AU:CurrencyRefresh()
    if not shardText or not pointsText then
        return
    end

    local shards = GetItemCount(AU.Items.ShardOfFallenStar, true) or 0
    local points = GetItemCount(AU.Items.AuroraPoints, true) or 0

    shardText:SetFormattedText('%s: %d', 'Осколки', shards)
    pointsText:SetFormattedText('%s: %d', 'Очки Авроры', points)
end
