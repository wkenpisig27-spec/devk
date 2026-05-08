EQUIPMENT_LV = 30

-- Rarity tiers — determined by how many bonus stats an item rolls (0 = Common, 5 = Legendary)
RARITY_NAMES = {
    [0] = "Common",
    [1] = "Uncommon",
    [2] = "Rare",
    [3] = "Epic",
    [4] = "Legendary",
    [5] = "Legendary"
}

function GetItemRarity(title)
    return RARITY_NAMES[math.max(0, math.min(5, title))] or "Common"
end

function randomBiased(min, max, bias)
    local v = math.random() ^ bias
    return math.floor(v * max) + min
end

function WeightedRandomIndex(table)
    local total = 0
    for i, v in ipairs(table) do
        total = total + v.Rate
    end
    local result = math.random(1, total)

    total = 0
    for i, v in ipairs(table) do
        total = total + v.Rate
        if total >= result then
            return i
        end
    end
end

ITEMSTAT = {
    [enumItemTypeClothing] = {
        [ITEMATTR_VAL_STR] = {MaxValue = 7, Rate = 70},
        [ITEMATTR_VAL_DEX] = {MaxValue = 7, Rate = 70},
        [ITEMATTR_VAL_AGI] = {MaxValue = 7, Rate = 70},
        [ITEMATTR_VAL_CON] = {MaxValue = 15, Rate = 150},
        [ITEMATTR_VAL_STA] = {MaxValue = 15, Rate = 150},
        [ITEMATTR_VAL_DEF] = {MaxValue = 60, Rate = 50},
        [ITEMATTR_VAL_HIT] = {MaxValue = 25, Rate = 15},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 25, Rate = 15},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 25, Rate = 15},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 10, Rate = 30},
        [ITEMATTR_VAL_SREC] = {MaxValue = 10, Rate = 30},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 15, Rate = 1},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 15, Rate = 1},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 6, Rate = 1}
    },
    [enumItemTypeGlove] = {
        [ITEMATTR_VAL_STR] = {MaxValue = 7, Rate = 70},
        [ITEMATTR_VAL_DEX] = {MaxValue = 15, Rate = 170},
        [ITEMATTR_VAL_AGI] = {MaxValue = 7, Rate = 70},
        [ITEMATTR_VAL_CON] = {MaxValue = 7, Rate = 100},
        [ITEMATTR_VAL_STA] = {MaxValue = 7, Rate = 100},
        [ITEMATTR_VAL_DEF] = {MaxValue = 20, Rate = 50},
        [ITEMATTR_VAL_HIT] = {MaxValue = 50, Rate = 80},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 20, Rate = 15},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 20, Rate = 15},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 10, Rate = 10},
        [ITEMATTR_VAL_SREC] = {MaxValue = 10, Rate = 10},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 40, Rate = 5},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 15, Rate = 1},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 1, Rate = 1}
    },
    [enumItemTypeBoot] = {
        [ITEMATTR_VAL_STR] = {MaxValue = 7, Rate = 70},
        [ITEMATTR_VAL_DEX] = {MaxValue = 7, Rate = 70},
        [ITEMATTR_VAL_AGI] = {MaxValue = 15, Rate = 100},
        [ITEMATTR_VAL_CON] = {MaxValue = 7, Rate = 100},
        [ITEMATTR_VAL_STA] = {MaxValue = 7, Rate = 100},
        [ITEMATTR_VAL_DEF] = {MaxValue = 20, Rate = 50},
        [ITEMATTR_VAL_HIT] = {MaxValue = 20, Rate = 40},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 50, Rate = 80},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 20, Rate = 15},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 10, Rate = 10},
        [ITEMATTR_VAL_SREC] = {MaxValue = 10, Rate = 10},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 10, Rate = 1},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 40, Rate = 5},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 1, Rate = 1}
    },
    [enumItemTypeHair] = {
        [ITEMATTR_VAL_STR] = {MaxValue = 5, Rate = 70},
        [ITEMATTR_VAL_DEX] = {MaxValue = 5, Rate = 70},
        [ITEMATTR_VAL_AGI] = {MaxValue = 5, Rate = 70},
        [ITEMATTR_VAL_CON] = {MaxValue = 8, Rate = 150},
        [ITEMATTR_VAL_STA] = {MaxValue = 15, Rate = 150},
        [ITEMATTR_VAL_DEF] = {MaxValue = 40, Rate = 50},
        [ITEMATTR_VAL_HIT] = {MaxValue = 25, Rate = 15},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 25, Rate = 15},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 25, Rate = 15},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 10, Rate = 30},
        [ITEMATTR_VAL_SREC] = {MaxValue = 10, Rate = 30},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 15, Rate = 1},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 15, Rate = 1},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 1, Rate = 1}
    },
    [enumItemTypeSword] = {
        -- sword
        [ITEMATTR_VAL_STR] = {MaxValue = 10, Rate = 70},
        [ITEMATTR_VAL_DEX] = {MaxValue = 7, Rate = 70},
        [ITEMATTR_VAL_AGI] = {MaxValue = 7, Rate = 100},
        [ITEMATTR_VAL_CON] = {MaxValue = 5, Rate = 100},
        [ITEMATTR_VAL_STA] = {MaxValue = 7, Rate = 150},
        [ITEMATTR_VAL_DEF] = {MaxValue = 20, Rate = 10},
        [ITEMATTR_VAL_HIT] = {MaxValue = 40, Rate = 40},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 40, Rate = 40},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 60, Rate = 60},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 100, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 100, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_SREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 10, Rate = 1},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 10, Rate = 1},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 1, Rate = 1}
    },
    [enumItemTypeGlave] = {
        -- gr8sword
        [ITEMATTR_VAL_STR] = {MaxValue = 15, Rate = 70},
        [ITEMATTR_VAL_DEX] = {MaxValue = 7, Rate = 120},
        [ITEMATTR_VAL_AGI] = {MaxValue = 7, Rate = 100},
        [ITEMATTR_VAL_CON] = {MaxValue = 9, Rate = 90},
        [ITEMATTR_VAL_STA] = {MaxValue = 7, Rate = 150},
        [ITEMATTR_VAL_DEF] = {MaxValue = 50, Rate = 40},
        [ITEMATTR_VAL_HIT] = {MaxValue = 20, Rate = 30},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 20, Rate = 30},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 60, Rate = 60},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 300, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 300, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_SREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 20, Rate = 1},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 10, Rate = 1},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 1, Rate = 1}
    },
    [enumItemTypeBow] = {
        -- bow
        [ITEMATTR_VAL_STR] = {MaxValue = 5, Rate = 130},
        [ITEMATTR_VAL_DEX] = {MaxValue = 15, Rate = 110},
        [ITEMATTR_VAL_AGI] = {MaxValue = 10, Rate = 100},
        [ITEMATTR_VAL_CON] = {MaxValue = 9, Rate = 90},
        [ITEMATTR_VAL_STA] = {MaxValue = 7, Rate = 150},
        [ITEMATTR_VAL_DEF] = {MaxValue = 20, Rate = 30},
        [ITEMATTR_VAL_HIT] = {MaxValue = 30, Rate = 60},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 20, Rate = 40},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 60, Rate = 60},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 200, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 200, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_SREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 20, Rate = 3},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 20, Rate = 3},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 1, Rate = 1}
    },
    [enumItemTypeHarquebus] = {
        -- gun
        [ITEMATTR_VAL_STR] = {MaxValue = 5, Rate = 130},
        [ITEMATTR_VAL_DEX] = {MaxValue = 15, Rate = 110},
        [ITEMATTR_VAL_AGI] = {MaxValue = 10, Rate = 100},
        [ITEMATTR_VAL_CON] = {MaxValue = 9, Rate = 90},
        [ITEMATTR_VAL_STA] = {MaxValue = 7, Rate = 150},
        [ITEMATTR_VAL_DEF] = {MaxValue = 20, Rate = 30},
        [ITEMATTR_VAL_HIT] = {MaxValue = 30, Rate = 60},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 20, Rate = 40},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 60, Rate = 60},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 200, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 200, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_SREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 20, Rate = 3},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 20, Rate = 3},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 1, Rate = 1}
    },
    [enumItemTypeStylet] = {
        -- dagger
        [ITEMATTR_VAL_STR] = {MaxValue = 10, Rate = 150},
        [ITEMATTR_VAL_DEX] = {MaxValue = 5, Rate = 110},
        [ITEMATTR_VAL_AGI] = {MaxValue = 5, Rate = 100},
        [ITEMATTR_VAL_CON] = {MaxValue = 13, Rate = 100},
        [ITEMATTR_VAL_STA] = {MaxValue = 15, Rate = 80},
        [ITEMATTR_VAL_DEF] = {MaxValue = 40, Rate = 60},
        [ITEMATTR_VAL_HIT] = {MaxValue = 30, Rate = 20},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 40, Rate = 40},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 60, Rate = 60},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 200, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_SREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 20, Rate = 3},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 10, Rate = 2},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 1, Rate = 1}
    },
    [enumItemTypeCosh] = {
        -- staff
        [ITEMATTR_VAL_STR] = {MaxValue = 12, Rate = 150},
        [ITEMATTR_VAL_DEX] = {MaxValue = 5, Rate = 110},
        [ITEMATTR_VAL_AGI] = {MaxValue = 5, Rate = 100},
        [ITEMATTR_VAL_CON] = {MaxValue = 12, Rate = 90},
        [ITEMATTR_VAL_STA] = {MaxValue = 15, Rate = 150},
        [ITEMATTR_VAL_DEF] = {MaxValue = 40, Rate = 60},
        [ITEMATTR_VAL_HIT] = {MaxValue = 30, Rate = 20},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 40, Rate = 40},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 60, Rate = 60},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 200, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 500, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_SREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 20, Rate = 3},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 10, Rate = 2},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 1, Rate = 1}
    },
    [enumItemTypeShield] = {
        -- shield
        [ITEMATTR_VAL_STR] = {MaxValue = 7, Rate = 150},
        [ITEMATTR_VAL_DEX] = {MaxValue = 7, Rate = 110},
        [ITEMATTR_VAL_AGI] = {MaxValue = 7, Rate = 100},
        [ITEMATTR_VAL_CON] = {MaxValue = 15, Rate = 90},
        [ITEMATTR_VAL_STA] = {MaxValue = 7, Rate = 150},
        [ITEMATTR_VAL_DEF] = {MaxValue = 40, Rate = 50},
        [ITEMATTR_VAL_HIT] = {MaxValue = 30, Rate = 50},
        [ITEMATTR_VAL_FLEE] = {MaxValue = 40, Rate = 50},
        [ITEMATTR_VAL_MXATK] = {MaxValue = 60, Rate = 50},
        [ITEMATTR_VAL_MXHP] = {MaxValue = 400, Rate = 50},
        [ITEMATTR_VAL_MXSP] = {MaxValue = 400, Rate = 50},
        [ITEMATTR_VAL_HREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_SREC] = {MaxValue = 5, Rate = 30},
        [ITEMATTR_VAL_ASPD] = {MaxValue = 10, Rate = 3},
        [ITEMATTR_VAL_MSPD] = {MaxValue = 10, Rate = 2},
        [ITEMATTR_VAL_PDEF] = {MaxValue = 3, Rate = 1}
    }
}

-- Max value scale per rarity tier: defines the hard power ceiling for each tier.
-- An Uncommon item can never roll a stat higher than 30% of its MaxValue.
-- A Legendary+ item can roll up to the full MaxValue.
RARITY_MAX_SCALE = {
    [1] = 0.30,  -- Uncommon:   up to 30% of stat max
    [2] = 0.50,  -- Rare:       up to 50% of stat max
    [3] = 0.70,  -- Epic:       up to 70% of stat max
    [4] = 0.85,  -- Legendary:  up to 85% of stat max
    [5] = 1.00,  -- Legendary+: full max, every stat
}

-- Bias per rarity tier: higher tier = lower bias = values spread toward the tier's ceiling.
RARITY_BIAS = {
    [1] = 2.5,  -- Uncommon:   clusters near floor of 30%
    [2] = 2.0,  -- Rare:       moderate spread within 50%
    [3] = 1.6,  -- Epic:       decent spread within 70%
    [4] = 1.3,  -- Legendary:  tends toward 85% ceiling
    [5] = 1.0,  -- Legendary+: uniform — any value equally likely
}

-- Two-pass: count fired stats first to know the rarity tier,
-- then roll values using that tier's scale and bias.
-- This guarantees higher rarity = more stats AND higher stat values.
function CalculateExtraStats(Level, ItemStat, StatRate)
    local ItemRates = {}
    for i, v in pairs(ItemStat) do
        ItemRates[#ItemRates + 1] = {Value = i, Rate = v.Rate}
    end

    -- Pass 1: count how many stats fire (determines rarity tier)
    local firedSlots = 0
    for i = 1, 5 do
        if Percentage_Random(StatRate[i]) == 1 then
            firedSlots = firedSlots + 1
        else
            break
        end
    end

    -- Pass 2: roll values using rarity-scaled ceiling and bias
    local scale = RARITY_MAX_SCALE[firedSlots] or 0.30
    local bias  = RARITY_BIAS[firedSlots]      or 2.5
    local StatVar = {}
    for i = 1, firedSlots do
        local Index  = WeightedRandomIndex(ItemRates)
        local statID = ItemRates[Index].Value
        local maxVal = math.max(1, math.floor(ItemStat[statID].MaxValue * scale))
        local value  = math.max(1, randomBiased(1, maxVal, bias))
        StatVar[#StatVar + 1] = {ID = statID, Num = value}
        table.remove(ItemRates, Index)
    end

    local title = #StatVar
    return StatVar, title
end

ITEMRATE = {
    [MONSTER_BAOLIAO] = {
        -- Monster Drops
        1,
        0.5,
        0.3,
        0.15,
        0.1
    },
    [QUEST_AWARD_3] = {
        -- Equipment Chests
        0.2,
        0.15,
        0.1,
        0.06,
        0.03
    },
    [QUEST_AWARD_5] = {
        -- Mystic & Beautiful Chests
        0.5,
        0.3,
        0.15,
        0.1,
        0.05
    },
    [PLAYER_HSSRA] = {
        -- Black Market
        0.2,
        0.15,
        0.1,
        0.05,
        0.03
    },
    [PLAYER_ZSITEM] = {
        -- Star of Unity
        0.3,
        0.2,
        0.15,
        0.1,
        0.05
    }
}

-- Item color codes by rarity tier (title = stat count, 0–5)
-- These map to the client's item name color display.
ITEMCOLOUR = {
    [0] = 0,  -- Common     (white)
    [1] = 1,  -- Uncommon   (green)
    [2] = 3,  -- Rare       (blue)
    [3] = 7,  -- Epic       (purple)
    [4] = 5,  -- Legendary  (orange)
    [5] = 9   -- Legendary+ (gold, 5-stat)
}

function Creat_Item(item, item_type, item_lv, item_event)
    if item_lv >= EQUIPMENT_LV and ITEMSTAT[item_type] and ITEMRATE[item_event] then
        Reset_item_add()
        local attr, title = CalculateExtraStats(item_lv, ITEMSTAT[item_type], ITEMRATE[item_event])
        for i, v in pairs(attr) do
            SetAttributeEditable(item, i - 1, v.ID)
            SetItemAttr(item, v.ID, GetItemAttr(item, v.ID) + v.Num)
        end
        local colour = ITEMCOLOUR[title] or 0
        local prefix = (35 + title) * 10 + (colour * 1000)
        SetItemAttr(item, ITEMATTR_MAXENERGY, prefix)
    end
    return 0
end