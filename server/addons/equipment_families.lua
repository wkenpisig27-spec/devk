-- equipment_families.lua
-- Family gear IDs: 10000 + tierIdx*100 + (family-1)*12 + slot
-- Families: 1 Guardian, 2 Royal, 3 Wind, 4 Hunter, 5 Shadow, 6 Sanctum

EQUIPMENT_V2_ID_MIN = 10000
EQUIPMENT_V2_ID_MAX = 10671

EQUIPMENT_FAMILY = {
	[1] = { Name = "Guardian", Colour = 1 },
	[2] = { Name = "Royal", Colour = 5 },
	[3] = { Name = "Wind", Colour = 3 },
	[4] = { Name = "Hunter", Colour = 7 },
	[5] = { Name = "Shadow", Colour = 9 },
	[6] = { Name = "Sanctum", Colour = 3 },
}

-- Equip slots checked for set pieces (LOOK link indices)
EQUIPMENT_SET_SLOTS = {
	0, -- HEAD / cap
	2, -- BODY
	3, -- GLOVE
	4, -- SHOES
	5, -- NECK
	6, -- LHAND
	7, -- HAND1 (ring)
	8, -- HAND2 (ring)
	9, -- RHAND
}

-- Flat final-stat set bonuses (applied after ExAttrSet). v1 = plain stats only.
-- Thresholds stack: 6-piece gets 2+4+6 bonuses.
FAMILY_SET_BONUS = {
	[1] = { -- Guardian
		[2] = { { ATTR = ATTR_MXHP, VALUE = 200 } },
		[4] = { { ATTR = ATTR_DEF, VALUE = 30 } },
		[6] = { { ATTR = ATTR_PDEF, VALUE = 4 }, { ATTR = ATTR_HREC, VALUE = 8 } },
	},
	[2] = { -- Royal
		[2] = { { ATTR = ATTR_MXATK, VALUE = 25 }, { ATTR = ATTR_MNATK, VALUE = 20 } },
		[4] = { { ATTR = ATTR_HIT, VALUE = 20 } },
		[6] = { { ATTR = ATTR_MXATK, VALUE = 40 }, { ATTR = ATTR_MNATK, VALUE = 30 } },
	},
	[3] = { -- Wind
		[2] = { { ATTR = ATTR_FLEE, VALUE = 20 } },
		[4] = { { ATTR = ATTR_MSPD, VALUE = 25 } },
		[6] = { { ATTR = ATTR_FLEE, VALUE = 30 }, { ATTR = ATTR_HIT, VALUE = 15 } },
	},
	[4] = { -- Hunter
		[2] = { { ATTR = ATTR_HIT, VALUE = 25 } },
		[4] = { { ATTR = ATTR_CRT, VALUE = 10 } },
		[6] = { { ATTR = ATTR_CRT, VALUE = 15 }, { ATTR = ATTR_MXATK, VALUE = 20 } },
	},
	[5] = { -- Shadow
		[2] = { { ATTR = ATTR_FLEE, VALUE = 15 }, { ATTR = ATTR_HIT, VALUE = 15 } },
		[4] = { { ATTR = ATTR_MXATK, VALUE = 20 }, { ATTR = ATTR_FLEE, VALUE = 15 } },
		[6] = { { ATTR = ATTR_HIT, VALUE = 25 }, { ATTR = ATTR_CRT, VALUE = 8 } },
	},
	[6] = { -- Sanctum
		[2] = { { ATTR = ATTR_MXSP, VALUE = 200 } },
		[4] = { { ATTR = ATTR_SREC, VALUE = 10 }, { ATTR = ATTR_MXHP, VALUE = 100 } },
		[6] = { { ATTR = ATTR_MXSP, VALUE = 300 }, { ATTR = ATTR_HREC, VALUE = 8 } },
	},
}

-- Affix pools per family (secondary rolls). MaxValue scaled by rarity in equips.lua.
FAMILY_AFFIX = {
	[1] = { -- Guardian: tank
		[ITEMATTR_VAL_CON] = { MaxValue = 12, Rate = 160 },
		[ITEMATTR_VAL_MXHP] = { MaxValue = 600, Rate = 140 },
		[ITEMATTR_VAL_DEF] = { MaxValue = 50, Rate = 120 },
		[ITEMATTR_VAL_PDEF] = { MaxValue = 5, Rate = 40 },
		[ITEMATTR_VAL_HREC] = { MaxValue = 12, Rate = 80 },
		[ITEMATTR_VAL_STR] = { MaxValue = 6, Rate = 40 },
		[ITEMATTR_VAL_STA] = { MaxValue = 8, Rate = 50 },
	},
	[2] = { -- Royal: burst
		[ITEMATTR_VAL_STR] = { MaxValue = 12, Rate = 120 },
		[ITEMATTR_VAL_DEX] = { MaxValue = 10, Rate = 100 },
		[ITEMATTR_VAL_MXATK] = { MaxValue = 70, Rate = 140 },
		[ITEMATTR_VAL_MNATK] = { MaxValue = 50, Rate = 100 },
		[ITEMATTR_VAL_HIT] = { MaxValue = 40, Rate = 80 },
		[ITEMATTR_VAL_CRT] = { MaxValue = 8, Rate = 40 },
		[ITEMATTR_VAL_CON] = { MaxValue = 6, Rate = 40 },
	},
	[3] = { -- Wind: speed
		[ITEMATTR_VAL_AGI] = { MaxValue = 14, Rate = 150 },
		[ITEMATTR_VAL_ASPD] = { MaxValue = 35, Rate = 100 },
		[ITEMATTR_VAL_MSPD] = { MaxValue = 40, Rate = 100 },
		[ITEMATTR_VAL_FLEE] = { MaxValue = 45, Rate = 120 },
		[ITEMATTR_VAL_HIT] = { MaxValue = 25, Rate = 60 },
		[ITEMATTR_VAL_DEX] = { MaxValue = 8, Rate = 50 },
	},
	[4] = { -- Hunter: crit / precision
		[ITEMATTR_VAL_LUK] = { MaxValue = 12, Rate = 120 },
		[ITEMATTR_VAL_CRT] = { MaxValue = 18, Rate = 140 },
		[ITEMATTR_VAL_HIT] = { MaxValue = 50, Rate = 130 },
		[ITEMATTR_VAL_MXATK] = { MaxValue = 55, Rate = 100 },
		[ITEMATTR_VAL_DEX] = { MaxValue = 12, Rate = 100 },
		[ITEMATTR_VAL_AGI] = { MaxValue = 8, Rate = 50 },
	},
	[5] = { -- Shadow: PvP skirmish
		[ITEMATTR_VAL_FLEE] = { MaxValue = 40, Rate = 120 },
		[ITEMATTR_VAL_HIT] = { MaxValue = 40, Rate = 120 },
		[ITEMATTR_VAL_ASPD] = { MaxValue = 25, Rate = 70 },
		[ITEMATTR_VAL_MXATK] = { MaxValue = 45, Rate = 90 },
		[ITEMATTR_VAL_CRT] = { MaxValue = 10, Rate = 70 },
		[ITEMATTR_VAL_AGI] = { MaxValue = 10, Rate = 80 },
		[ITEMATTR_VAL_MXHP] = { MaxValue = 300, Rate = 50 },
	},
	[6] = { -- Sanctum: sustain / support
		[ITEMATTR_VAL_STA] = { MaxValue = 14, Rate = 150 },
		[ITEMATTR_VAL_MXSP] = { MaxValue = 600, Rate = 140 },
		[ITEMATTR_VAL_SREC] = { MaxValue = 15, Rate = 120 },
		[ITEMATTR_VAL_CON] = { MaxValue = 10, Rate = 100 },
		[ITEMATTR_VAL_MXHP] = { MaxValue = 400, Rate = 80 },
		[ITEMATTR_VAL_HREC] = { MaxValue = 10, Rate = 70 },
		[ITEMATTR_VAL_DEF] = { MaxValue = 25, Rate = 40 },
	},
}

function IsEquipmentV2ItemID(itemID)
	return itemID ~= nil and itemID >= EQUIPMENT_V2_ID_MIN and itemID <= EQUIPMENT_V2_ID_MAX
end

function GetEquipmentFamilyID(itemID)
	if not IsEquipmentV2ItemID(itemID) then
		return 0
	end
	local within = math.mod(itemID - EQUIPMENT_V2_ID_MIN, 100)
	return math.floor(within / 12) + 1
end

function GetEquipmentTierIndex(itemID)
	if not IsEquipmentV2ItemID(itemID) then
		return -1
	end
	return math.floor((itemID - EQUIPMENT_V2_ID_MIN) / 100)
end

function CountEquipmentFamilyPieces(Player)
	local counts = {}
	for i = 1, table.getn(EQUIPMENT_SET_SLOTS) do
		local slot = EQUIPMENT_SET_SLOTS[i]
		local item = GetChaItem(Player, 1, slot)
		if item ~= nil then
			local id = GetItemID(item)
			local fam = GetEquipmentFamilyID(id)
			if fam > 0 then
				counts[fam] = (counts[fam] or 0) + 1
			end
		end
	end
	return counts
end

function ApplyFamilySetBonuses(Player)
	if Player == nil then
		return
	end
	local counts = CountEquipmentFamilyPieces(Player)
	for fam, count in pairs(counts) do
		local thresholds = FAMILY_SET_BONUS[fam]
		if thresholds ~= nil then
			local steps = { 2, 4, 6 }
			for s = 1, table.getn(steps) do
				local need = steps[s]
				if count >= need and thresholds[need] ~= nil then
					local list = thresholds[need]
					for j = 1, table.getn(list) do
						local b = list[j]
						local cur = GetChaAttr(Player, b.ATTR)
						SetCharaAttr(cur + b.VALUE, Player, b.ATTR)
					end
				end
			end
		end
	end
end

EQUIPMENT_FAMILY_NAME_TO_ID = {
	guardian = 1,
	royal = 2,
	wind = 3,
	hunter = 4,
	shadow = 5,
	sanctum = 6,
}

-- Which weapon slot indices (0-5) to include for a class filter. nil = all weapons.
EQUIPMENT_FAMILY_JOB_WEAPONS = {
	champion = { [0] = true },
	champ = { [0] = true },
	["8"] = { [0] = true },
	crusader = { [1] = true },
	cru = { [1] = true },
	["9"] = { [1] = true },
	sharpshooter = { [2] = true, [3] = true },
	ss = { [2] = true, [3] = true },
	["12"] = { [2] = true, [3] = true },
	cleric = { [4] = true },
	["13"] = { [4] = true },
	seal = { [4] = true },
	sealmaster = { [4] = true },
	["14"] = { [4] = true },
	voyager = { [5] = true },
	["16"] = { [5] = true },
	all = nil,
}

function ResolveEquipmentFamilyID(nameOrId)
	if nameOrId == nil then
		return 0
	end
	local asNum = tonumber(nameOrId)
	if asNum ~= nil and EQUIPMENT_FAMILY[asNum] ~= nil then
		return asNum
	end
	local key = string.lower(tostring(nameOrId))
	return EQUIPMENT_FAMILY_NAME_TO_ID[key] or 0
end

function GetEquipmentFamilySetItemIDs(familyId, tier, jobFilter)
	local tierIndex = -1
	local tiers = { 10, 20, 30, 40, 50, 60, 70 }
	for i = 1, table.getn(tiers) do
		if tiers[i] == tier then
			tierIndex = i - 1
			break
		end
	end
	if familyId < 1 or familyId > 6 or tierIndex < 0 then
		return nil
	end

	local weaponFilter = nil
	if jobFilter ~= nil and tostring(jobFilter) ~= "" then
		local key = string.lower(tostring(jobFilter))
		if EQUIPMENT_FAMILY_JOB_WEAPONS[key] ~= nil then
			weaponFilter = EQUIPMENT_FAMILY_JOB_WEAPONS[key]
		elseif key ~= "all" then
			return nil -- unknown job filter
		end
	end

	local ids = {}
	for slot = 0, 11 do
		local include = true
		if slot <= 5 and weaponFilter ~= nil then
			include = weaponFilter[slot] == true
		end
		if include then
			ids[table.getn(ids) + 1] = EQUIPMENT_V2_ID_MIN + tierIndex * 100 + (familyId - 1) * 12 + slot
		end
	end
	return ids
end

-- giveEvent: ItemInstance event id (default QUEST_AWARD_5 = 7 for solid affix rolls)
function GiveEquipmentFamilySet(role, familyName, tier, jobFilter, giveEvent)
	local familyId = ResolveEquipmentFamilyID(familyName)
	if familyId == 0 then
		BickerNotice(role, "Unknown family. Use: Guardian, Royal, Wind, Hunter, Shadow, Sanctum")
		return false
	end
	tier = tonumber(tier)
	local ids = GetEquipmentFamilySetItemIDs(familyId, tier, jobFilter)
	if ids == nil then
		BickerNotice(role, "Invalid tier/job. Tiers: 10,20,30,40,50,60,70. Jobs: champion,crusader,sharpshooter,cleric,seal,voyager,all")
		return false
	end

	local eventId = tonumber(giveEvent) or 7
	local given = 0
	for i = 1, table.getn(ids) do
		local ok = GiveItem(role, 0, ids[i], 1, eventId)
		if ok ~= nil and ok ~= 0 and ok ~= LUA_FALSE then
			given = given + 1
		end
	end

	local famName = EQUIPMENT_FAMILY[familyId].Name
	local jobNote = ""
	if jobFilter ~= nil and tostring(jobFilter) ~= "" and string.lower(tostring(jobFilter)) ~= "all" then
		jobNote = " (" .. tostring(jobFilter) .. " weapons)"
	end
	BickerNotice(role, "Gave " .. given .. "/" .. table.getn(ids) .. " " .. famName .. " T" .. tier .. " pieces" .. jobNote)
	SystemNotice(role, "[FamilySet] " .. famName .. " T" .. tier .. " -> " .. given .. " items")
	return given > 0
end

print("-- [Loading] equipment_families.lua")
