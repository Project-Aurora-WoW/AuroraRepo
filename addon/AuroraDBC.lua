--[[
    AuroraDBC.lua - клиентский парсер DBC для аддона AuroraUI.

    Зачем: аддон должен знать, установлен ли DBC-патч кампании #1.
    В 3.3.5a Lua не имеет доступа к файловой системе, поэтому прочитать
    MPQ-архив из аддона нельзя. Проверка идёт через то, что клиент уже
    загрузил в память: GetAchievementInfo(id) для ID 10001-10099.
    Есть ответ - патч установлен, nil - нет.

    ParseDBC - полный разбор бинарного WDBC. Нужен, если файл как-то
    попал в Lua (отладка, другой загрузчик). ReadString работает по
    переданной строке, поэтому разбор не зависит от возможностей клиента.
]]

local ADDON_NAME = ...

AuroraDBC = AuroraDBC or {}
local M = AuroraDBC

local format, sub = format, string.sub
-- ============================================================================
-- Разбор бинарного DBC (WDBC)
-- ============================================================================
-- Формат 3.3.5a:
--   char   magic[4]      "WDBC"
--   uint32 records
--   uint32 fields
--   uint32 recordSize    = fields * 4
--   uint32 stringSize
--   rows[records]        recordSize байт на запись, int32 поля
--   strings[stringSize]  UTF-8, с NUL в начале, смещения от начала блока
-- Порядок байт - little endian, как и в остальных файлах клиента.

local function ReadU32(bytes, pos)
    local b1, b2, b3, b4 = sub(bytes, pos, pos), sub(bytes, pos + 1, pos + 1),
                           sub(bytes, pos + 2, pos + 2), sub(bytes, pos + 3, pos + 3)
    if not b4 then
        return nil
    end
    return b1 + b2 * 256 + b3 * 65536 + b4 * 16777216
end

local function ReadString(bytes, strBase, offset)
    if not offset or offset == 0 then
        return ""
    end
    local pos = strBase + offset + 1
    local stop = strBase + #bytes
    local out = {}
    while pos <= stop do
        local ch = sub(bytes, pos, pos)
        if ch == "\0" then
            break
        end
        out[#out + 1] = ch
        pos = pos + 1
    end
    return table.concat(out)
end

-- Разбирает строку DBC. Возвращает таблицу записей или nil + причина.
-- fields - ожидаемое число полей (62 для Achievement, 31 для Criteria).
function M.ParseDBC(bytes, fields)
    if type(bytes) ~= "string" or #bytes < 20 then
        return nil, "файл короче заголовка"
    end
    if sub(bytes, 1, 4) ~= "WDBC" then
        return nil, "нет сигнатуры WDBC"
    end

    local records = ReadU32(bytes, 5)
    local actualFields = ReadU32(bytes, 9)
    local recordSize = ReadU32(bytes, 13)
    local stringSize = ReadU32(bytes, 17)

    if not (records and actualFields and recordSize and stringSize) then
        return nil, "заголовок обрезан"
    end
    if fields and actualFields ~= fields then
        return nil, format("ожидалось %d полей, в файле %d", fields, actualFields)
    end
    if recordSize ~= actualFields * 4 then
        return nil, "recordSize не совпадает с числом полей"
    end

    local headerEnd = 20
    local bodyEnd = headerEnd + records * recordSize
    if #bytes < bodyEnd + stringSize then
        return nil, "размер файла меньше заявленного"
    end

    local strBase = bodyEnd
    local rows = {}
    for r = 0, records - 1 do
        local row = {}
        local base = headerEnd + r * recordSize
        for f = 0, actualFields - 1 do
            row[f + 1] = ReadU32(bytes, base + f * 4 + 1)
        end
        rows[r + 1] = row
    end

    return { records = records, fields = actualFields, rows = rows, strBase = strBase }
end

-- Возвращает имена достижений из разобранного Achievement.dbc.
-- Поле 5 (индекс 4) - название enUS, 6 (индекс 5) - ruRU.
function M.AchievementNames(parsed, strings)
    if not parsed then
        return {}
    end
    local out = {}
    for _, row in ipairs(parsed.rows) do
        local id = row[1]
        local nameEn = ReadString(strings, parsed.strBase, row[5])
        local nameRu = ReadString(strings, parsed.strBase, row[6])
        out[id] = (nameRu ~= "" and nameRu) or nameEn
    end
    return out
end

-- ============================================================================
-- Проверка наличия патча по клиентскому API
-- ============================================================================
-- В 3.3.5a доступны: GetAchievementInfo(id) -> id, name, points, completed, ...
-- Если запись отсутствует в Achievement.dbc, клиент возвращает nil.

local CAMPAIGN_ACHIEVEMENTS = {
    10001, 10002, 10003, 10004, 10010, 10013, 10014, 10020,
    10022, 10024, 10030, 10031, 10032, 10033, 10099,
}

-- Проверяет, знает ли клиент достижение. Возвращает 3 значения:
--   known   - true/false/nil (nil = API недоступен)
--   name    - название, если известно
--   points  - очки
function M.ClientKnowsAchievement(id)
    if type(GetAchievementInfo) ~= "function" then
        return nil
    end
    local _, name, points = GetAchievementInfo(id)
    if not name then
        return false
    end
    return true, name, points
end

-- Итог проверки патча.
-- Возвращает: state ("patched"|"partial"|"missing"|"unknown"), known, total, firstMissing
function M.CheckCampaignPatch()
    local total = #CAMPAIGN_ACHIEVEMENTS
    local known = 0
    local firstMissing = nil

    for _, id in ipairs(CAMPAIGN_ACHIEVEMENTS) do
        local isKnown = M.ClientKnowsAchievement(id)
        if isKnown == nil then
            return "unknown", 0, total, nil
        end
        if isKnown then
            known = known + 1
        elseif not firstMissing then
            firstMissing = id
        end
    end

    if known == 0 then
        return "missing", known, total, CAMPAIGN_ACHIEVEMENTS[1]
    elseif known == total then
        return "patched", known, total, nil
    end
    return "partial", known, total, firstMissing
end

M.CampaignAchievementIds = CAMPAIGN_ACHIEVEMENTS

-- Читает файл рядом с аддоном (только для отладки, если он положен вручную).
-- Штатный клиент 3.3.5a не даёт Lua-доступ к файлам, поэтому функция
-- возвращает nil, если окружение не предоставило чтение.
function M.ReadLocalDBC(fileName)
    if type(ReadFile) ~= "function" then
        return nil, "файловый доступ из Lua недоступен в 3.3.5a"
    end
    return ReadFile(fileName)
end
