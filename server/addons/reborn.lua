--*------------------------------------------*--
--* dir parent	: plugin	     	   	     *--
--* File name	: rebornSystem.lua     		 *--
--* Coded by 	: Valdiney Eviles			 *--
--* Discord		: Eviles#2759				 *--
--* PKOdev		: @Eviles					 *--
--*------------------------------------------*--

--[[
    [Introduction]:
    - Can add more than 1 reborn option to NPC, automatically generate new text/page based on table "Reborn.List";
    - You can set a min/max level needed to reborn character;
    - You can specific an amount gold needed to reborn;
    - You can specific an item and quantity needed to reborn;
    - You can give players rewards after reborn: gold, base attribute point or items;
    - NPC Talk and system messages easy to edit on an array;
    - Player must not wearing equipment in order to reborn;
    - Rebirth skills are keep after reborn;

	[TODO]:
	- Load reborn information with DataFile, so can handle earned player EXP as wish;
	- Reduce experience gain based on reborn level
	
	[Installation]:
		1.	Load this addon file using dofile.
		2.	Create a new NPC in your map as like the example below:
			###	Character Reborn	1	564	0	217275,277575	217275,277575	180	Argent City	1	1	RebornNPC	0
		3.	Go to: \resource\script\MisSdk\
		3.1	Open the file: NpcSdk.lua
		3.2	Search for: return ChangeItem(character,npc)
		3.3	Add the following below:
			elseif item.func == Reborn.Push then
				return Reborn.Push(character,item.p1)
		4.	That's all!
]]

Reborn = {}
Reborn.Conf = {}
Reborn.List = {}

Reborn.Conf.Active = true -- @ Enable/disable the NPC.
Reborn.Conf.Page = 2 -- @ Start page to show reset list (don't touch!)

-- @ This are conditions and rewards for first reset option in NPC
-- @ You can easily add new options to the NPC by duplicating this and editing the key.
-- @ The NPC will automatically generate text and pages based on this.
Reborn.List[1] = {
	MinLv = 20,
	MaxLv = 30,
    MaxReborn = 10,
	Need = {
		Gold = 0,
		ItemID = 885,
		Quantity = 1,
	},
	Reward = {
        AttrPoint = 5,
		Gold = 0,
		Item = {
			[1] = {ItemID = nil, Quantity = nil, Quality = nil},
			[2] = {ItemID = nil, Quantity = nil, Quality = nil},
		}
	}
}

-- @ This are talks that used in NPC and error messages.
Reborn.Talk = {
	"Character Reborn: Hello! I can reborn your character and you can obtain great rewards.",
	"Reborn Character",
	"Maybe not now, bye!",
	"Character Reborn: What level range you character is actually?",
	"Reborn for: Lv%d-%d",
	"[Next]",
	"[Back]",
	"Character Reborn: By doing that, your character will be Lv1 again.",
	"Reborn now",
	"Please remove your equipment!",
	"Character need to be from Lv%d to Lv%d!",
	"You need to have %dg Gold to reborn your character!",
	"Requires %dx %s to reborn your character!",
	"<Character Reborn>: {%s}, succesfully reborn his character. Blessing from the whole server {%s}, hope you have a safe journey!",
	"Character Reborn: I am sorry, the system is currently unavailable.",
}

-- @ NPC Function
function RebornNPC()
	if (Reborn.Conf.Active) then
		Talk(1, Reborn.Talk[1])
		Text(1, Reborn.Talk[2], JumpPage, Reborn.Conf.Page)
		Text(1, Reborn.Talk[3], CloseTalk)
		Reborn.GenerateText()
	else
		Talk(1, Reborn.Talk[15])
	end
end

-- @ Generate the text according to reborn list.
-- @ A limit of 7 items per page.
function Reborn.GenerateText()
	local Page, Count, Total = Reborn.Conf.Page, 0, 0
	for i, v in pairs(Reborn.List) do
		Total = Total + 1
	end
	local Pages = math.ceil(Total/7) + 2
	Talk(Page, Reborn.Talk[4])	
	for i,v in pairs(Reborn.List) do
		if (Count == 7) then
			Text(Page, Reborn.Talk[6], JumpPage, Page + 1)			
			Page = Page + 1
			Talk(Page, Reborn.Talk[4])					
			Count = 0
		end
		local Range = string.format(Reborn.Talk[5], Reborn.List[i].MinLv, Reborn.List[i].MaxLv)
		Text(Page, Range, JumpPage, Pages)
		Reborn.GeneratePage(Pages, i, Page)
		Pages = Pages + 1
		Count = Count + 1
	end
end

-- @ Generates the pages for NPC.
function Reborn.GeneratePage(Pages, i, BackPage)
	Talk(Pages, Reborn.Talk[8])
	Text(Pages, Reborn.Talk[9], Reborn.Push, i)	
	Text(Pages, Reborn.Talk[7], JumpPage, BackPage)
end

-- @ Checking if player has all requeriments to reborn.
function Reborn.Push(Player, i)
	if (IsEquip(Player) == LUA_TRUE) then
		BickerNotice(Player, Reborn.Talk[10])
		return
	end
	if (GetChaAttr(Player, ATTR_LV) < Reborn.List[i].MinLv or GetChaAttr(Player, ATTR_LV) > Reborn.List[i].MaxLv) then
		BickerNotice(Player, string.format(Reborn.Talk[11], Reborn.List[i].MinLv, Reborn.List[i].MaxLv))
		return
	end
	if (GetChaAttr(Player, ATTR_GD) < Reborn.List[i].Need.Gold) then
		BickerNotice(Player, string.format(Reborn.Talk[12], Reborn.List[i].Need.Gold))
		return
	end
	if (CheckBagItem(Player, Reborn.List[i].Need.ItemID) < Reborn.List[i].Need.Quantity) then
		BickerNotice(Player, string.format(Reborn.Talk[13], Reborn.List[i].Need.Quantity, GetItemName(Reborn.List[i].Need.ItemID)))
		return
	end
	Reborn.Begin(Player, i)
end

-- @ Check and return reborn level
function Reborn.Level(Player)
    local PID = GetCharID(Player)
	local Dir = GetResPath(string.format("../PlayerData/Reborn/%d.dat", PID))
	local Table = DataFile:Init(Dir, Table):Load()
	local Level = Table[PID].Count
	if (Table[PID] ~= nil) then
		if Level > 0 then
			return Level
		else
			return 0
		end
	end
end

-- @ Resets the character level to 1 and it's base attributes.
-- @ Take required items and gold.
-- @ Give rewards to character.
-- @ Announce in system that the character succesfully reborn.
function Reborn.Begin(Player, i)
    local PID = GetCharID(Player)
	local Dir = GetResPath(string.format("../PlayerData/Reborn/%d.dat", PID))
	local Table = DataFile:Init(Dir, Table):Load()
	if (Table[PID] == nil) then
		Table[PID] = {}
		Table[PID] = {Count = 1}
		DataFile:Init(Dir, Table):Save()
	end
	
	Table[PID].Count = Table[PID].Count + 1
	DataFile:Init(Dir, Table):Save()
	
    local cha_skill = GetChaAttr(Player, ATTR_TP)
    local clear_skill = ClearAllFightSkill(Player)
    cha_skill = cha_skill + clear_skill

	local rebornBonus = Reborn.List[i].Reward.AttrPoint * Table[PID].Count
	SetChaAttr(Player, ATTR_BSTR, rebornBonus)
	SyncChar(Player, 4)	
	SetChaAttr(Player, ATTR_BCON, rebornBonus)
	SyncChar(Player, 4)	
	SetChaAttr(Player, ATTR_BAGI, rebornBonus)
	SyncChar(Player, 4)	
	SetChaAttr(Player, ATTR_BDEX, rebornBonus)
	SyncChar(Player, 4)	
	SetChaAttr(Player, ATTR_BSTA, rebornBonus)
	SyncChar(Player, 4)	
	SetChaAttr(Player, ATTR_AP, 0)	
	SyncChar(Player, 4)	
	SetChaAttr(Player, ATTR_CEXP, 0)
	SyncChar(Player, 4)
	SetChaAttr(Player, ATTR_LV, 1)	
	SyncChar(Player, 4)		
	TakeMoney(Player, 0, Reborn.List[i].Need.Gold)
	TakeItem(Player, 0, Reborn.List[i].Need.ItemID, Reborn.List[i].Need.Quantity)
	PlayEffect(Player, 361)
	SystemNotice(Player, string.format("Congratulations! Your reborn level is now Lv%d", Reborn.Level(Player) - 1))
	Notice(string.format(Reborn.Talk[14], GetChaDefaultName(Player), GetChaDefaultName(Player)))
	Reborn.Rewards(Player, i)
	Reborn.RebirthSkill(Player)
	RefreshCha(Player)
end

-- @ Reading rewards from the table and giving them to player.
function Reborn.Rewards(Player, i)
	if (Reborn.List[i].Reward.Gold > 0) then
		AddMoney(Player, 0, Reborn.List[i].Need.Gold)
	end
	for k = 1, table.getn(Reborn.List[i].Reward.Item) do
		if (Reborn.List[i].Reward.Item[k].ItemID ~= nil) then
			if (GetChaFreeBagGridNum(Player) > 0) then
				GiveItem(Player, 0, Reborn.List[i].Reward.Item[k].ItemID, Reborn.List[i].Reward.Item[k].Quantity, Reborn.List[i].Reward.Item[k].Quality)
			else
				GiveItemX(Player, 0, Reborn.List[i].Reward.Item[k].ItemID, Reborn.List[i].Reward.Item[k].Quantity, Reborn.List[i].Reward.Item[k].Quality)
			end
		end
	end
end

-- @ Checking rebirth skills, so we add them back if character is rebirthed.
function Reborn.RebirthSkill(Player)
	local rebirth_exp = GetChaAttr( Player, ATTR_CSAILEXP)
	local cha_job = GetChaAttr(Player, ATTR_JOB)	
	if (rebirth_exp >= 12000 and RebirthEXP and cha_job == 9) then
		AddChaSkill(Player , 459, 3, 3, 0)
		AddChaSkill(Player , 453, 3, 3, 0)
	elseif (rebirth_exp >= 12000 and cha_job == 8) then
		AddChaSkill(Player, 459, 3, 3, 0)
		AddChaSkill(Player, 455, 3, 3, 0)
	elseif (rebirth_exp >= 12000 and cha_job == 16) then
		AddChaSkill(Player , 459, 3, 3, 0)
		AddChaSkill(Player , 454, 3, 3, 0)
	elseif (rebirth_exp >= 12000 and cha_job == 12) then
		AddChaSkill(Player, 459, 3, 3, 0)
		AddChaSkill(Player, 456, 3, 3, 0)
	elseif (rebirth_exp >= 12000 and cha_job == 13) then
		AddChaSkill(Player, 459, 3, 3, 0)
		AddChaSkill(Player, 458, 3, 3, 0)
	elseif (rebirth_exp >= 12000 and cha_job == 14) then
		AddChaSkill(Player, 459, 3, 3, 0)
		AddChaSkill(Player, 457, 3, 3, 0)
	elseif (rebirth_exp >= 9000 and rebirth_exp < 12000 and cha_job == 9) then
		AddChaSkill(Player, 459, 2, 2, 0)
		AddChaSkill(Player, 453, 2, 2, 0)
	elseif (rebirth_exp >= 9000 and rebirth_exp < 12000 and cha_job == 8) then
		AddChaSkill(Player, 459, 2, 2, 0)
		AddChaSkill(Player, 455, 2, 2, 0)
	elseif (rebirth_exp >= 9000 and rebirth_exp < 12000 and cha_job == 16) then
		AddChaSkill(Player, 459, 2, 2, 0)
		AddChaSkill(Player, 454, 2, 2, 0)
	elseif (rebirth_exp >= 9000 and rebirth_exp < 12000 and cha_job == 12) then
		AddChaSkill(Player, 459, 2, 2, 0)
		AddChaSkill(Player, 456, 2, 2, 0)
	elseif (rebirth_exp >= 9000 and rebirth_exp < 12000 and cha_job == 13) then
		AddChaSkill(Player, 459, 2, 2, 0)
		AddChaSkill(Player, 458, 2, 2, 0)
	elseif (rebirth_exp >= 9000 and rebirth_exp < 12000 and cha_job == 14) then
		AddChaSkill(Player, 459, 2, 2, 0)
		AddChaSkill(Player, 457, 2, 2, 0)
	elseif (rebirth_exp > 0 and rebirth_exp < 9000) then
		AddChaSkill(Player, 459, 1, 1, 0)
		GiveItem_zsbook(Player)	
	end
end