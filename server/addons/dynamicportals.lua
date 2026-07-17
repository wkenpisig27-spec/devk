--[[
	Dynamic Portal System
	Creates temporary or permanent interactive portals at runtime via Lua.

	Player-owned portals (CreateDynamicPortalCha) are private:
	  - Visible only to the creator and their current party members
	  - Only the creator and party members can walk in and trigger them
	  - Display name is auto-prefixed: "[OwnerName] Your Portal Name"

	Global portals (CreateDynamicPortal) are visible to everyone on the map.
]]

DynamicPortal = {}

-- Default portal visual (item entity ID 193 is the standard map switch portal model)
DynamicPortal.DEFAULT_ITEM_ID = 193

-- Create a global portal at map coordinates (centimeters)
function DynamicPortal.CreateGlobal(MapCopy, x, y, callback, name, lifetime, itemID)
	return CreateDynamicPortal(MapCopy, x, y, callback, name, lifetime, itemID or DynamicPortal.DEFAULT_ITEM_ID)
end

-- Create a player-owned portal near the character
function DynamicPortal.CreateForPlayer(Player, offsetX, offsetY, callback, name, lifetime, itemID)
	local MapCopy = GetChaMapCopy(Player)
	local x, y = GetChaPos(Player)
	return CreateDynamicPortalCha(
		Player,
		MapCopy,
		x + (offsetX or 200),
		y + (offsetY or 0),
		callback,
		name,
		lifetime,
		itemID or DynamicPortal.DEFAULT_ITEM_ID
	)
end

-- Destroy all portals owned by a player
function DynamicPortal.DestroyPlayerPortals(Player)
	return DestroyDynamicPortalCha(Player)
end

-- Callback: show a notice when the portal is used
function DynamicPortal_OnNotice(Player, MapCopy)
	SystemNotice(Player, "Dynamic Portal activated!")
end

-- Callback: teleport player to a city birth point
function DynamicPortal_TeleportCity(Player, MapCopy, cityName)
	if cityName and cityName ~= "" then
		MoveCity(Player, cityName)
	else
		SystemNotice(Player, "Portal destination is not configured.")
	end
end

-- Pre-built callback for Argent City teleport
function DynamicPortal_ToArgent(Player, MapCopy)
	DynamicPortal_TeleportCity(Player, MapCopy, "Argent City")
end

-- Pre-built callback for Shaitan City teleport
function DynamicPortal_ToShaitan(Player, MapCopy)
	DynamicPortal_TeleportCity(Player, MapCopy, "Shaitan City")
end

-- Item-use handler: spawns a 30-second player portal in front of the user
function ItemUse_DynamicPortal(Player, Item)
	local portalID = DynamicPortal.CreateForPlayer(Player, 200, 0, "DynamicPortal_OnNotice", "Test Portal", 30)
	if portalID > 0 then
		SystemNotice(Player, "Portal created! ID: "..portalID)
	else
		SystemNotice(Player, "Failed to create portal.")
		UseItemFailed(Player)
	end
end

-- Item-use handler: removes all portals created by the player
function ItemUse_DestroyPortal(Player, Item)
	local count = DynamicPortal.DestroyPlayerPortals(Player)
	SystemNotice(Player, "Destroyed "..count.." portal(s).")
end

-- GM command: /portal [,lifetime] — create a test portal in front of you
cmd.list['portal'] = {
	gm = 99,
	param = {},
	func = function(role, param)
		local lifetime = 60
		if param.n >= 1 then
			lifetime = tonumber(param[1]) or 60
		end
		local portalID = DynamicPortal.CreateForPlayer(role, 200, 0, "DynamicPortal_OnNotice", "GM Portal", lifetime)
		if portalID > 0 then
			BickerNotice(role, "Portal created (ID: "..portalID..", lifetime: "..lifetime.."s)")
		else
			BickerNotice(role, "Failed to create portal.")
		end
	end
}

-- GM command: /portaltp <city> [,lifetime]  or  /portaltp,<city>[,lifetime]
cmd.list['portaltp'] = {
	gm = 99,
	param = {'string'},
	func = function(role, param)
		local cityName = param[1]
		local lifetime = 120
		if param.n >= 2 then
			lifetime = tonumber(param[2]) or 120
		end
		local callbackName = "DynamicPortal_GMTo_"..string.gsub(cityName, " ", "_")
		_G[callbackName] = function(Player, MapCopy)
			DynamicPortal_TeleportCity(Player, MapCopy, cityName)
		end
		local portalID = DynamicPortal.CreateForPlayer(role, 200, 0, callbackName, "Portal to "..cityName, lifetime)
		if portalID > 0 then
			BickerNotice(role, "Teleport portal to "..cityName.." created (ID: "..portalID..")")
		else
			BickerNotice(role, "Failed to create teleport portal.")
		end
	end
}

-- GM command: /portalclear — destroy all portals owned by the GM
cmd.list['portalclear'] = {
	gm = 99,
	param = {},
	func = function(role, param)
		local count = DynamicPortal.DestroyPlayerPortals(role)
		BickerNotice(role, "Destroyed "..count.." portal(s).")
	end
}

-- GM command: /portalglobal,x,y,callback,name [,lifetime] — spawn a global portal at coords
cmd.list['portalglobal'] = {
	gm = 99,
	param = {'number', 'number', 'string', 'string'},
	func = function(role, param)
		local MapCopy = GetChaMapCopy(role)
		local lifetime = -1
		if param.n >= 5 then
			lifetime = tonumber(param[5]) or -1
		end
		local portalID = DynamicPortal.CreateGlobal(MapCopy, param[1], param[2], param[3], param[4], lifetime)
		if portalID > 0 then
			BickerNotice(role, "Global portal created (ID: "..portalID..")")
		else
			BickerNotice(role, "Failed to create global portal.")
		end
	end
}
