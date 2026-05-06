
--utils
local lf = dofile('./lua_functional.lua')
local oa2 = function(f,v) return lf.applyn(f, 2, v) end --2 is the index of the flag parameter

--Luajit bug workaround:
local items_number_max_per_function = 1500 --This is not measured exactly because there's no way to do that - do the best closest thing here.

function hashtoarray(t, empty_item, acceptable_numbers)
	local new_t = {}
	local highest_number = 0
	for i,v in pairs(t) do
		local possible_number = tonumber(i)
		if possible_number ~= nil and acceptable_numbers(possible_number) then
			new_t[possible_number] = v
			table.remove(new_t[possible_number], 1) --remove id field
			if possible_number > highest_number then
				highest_number = possible_number
			end
		end
	end
	for i=1,highest_number do
		if new_t[i] == nil then
			new_t[i] = empty_item
		end
	end
	return new_t
end


--[[
This function is an extension to the original tostring function, and returns
the string representation of many values including tables.
--]]
function serialize_array(x,addedsep,myfile)
	if addedsep == nil then
		addedsep = ""
	end
	if type(x) == "table" then
		local s = ""
		local sep = ""
		local afterfirstsep = "," .. addedsep
		local j = 1
		if not myfile then
			s = "{"
			addedsep = ""
		end
		for i, v in ipairs(x) do
			--LUAJIT BUG WORKAROUND: it runs out of memory on large strings - gc is extremely inefficient on large strings as well.
			--adjust this number if running out of memory - try to get it as high as possible on ssd,
			--even if it slows down to protect the ssd from excessive writes. gc is so inefficient with
			--this data that even 1000's writes on a regular hdd is faster than ram memory!
			--I left it at 1000000 because I use SSD, put it around 1000 or 10000 for regular hdd.
			
			--Slowdown is fixed in the newest of the new git luajit 2.1 version - but until then, workaround is in place.
			if myfile then 
				if string.len(s) > 1000000 then
					myfile:write(s)
					s = ""
				end
			end
			if i ~= "_G" and v ~= x then
				if myfile then
					sep = ",\r\n\t"
					if (i % items_number_max_per_function) == 1 and x[-1] ~= nil and type(x[-1]) == "table" then
						if i == 1 then
							s = s .. "local _table = {}\r\n"
						else
							s = s .. "\r\n} end\r\n"
						end
						s = s .. "_table["..j.."] = function()\r\n\treturn {[-1] = " .. serialize_array(rawget(x, -1),addedsep) .. "," .. "\r\n\t"
						j = j + 1
						sep = ""
					end
				else
					if i > 1 then
						sep = afterfirstsep
					end
				end
				if ((type(rawget (x, i)) == "table") or (type(rawget (x, i)) == "number") or (type(rawget (x, i)) == "boolean")) then
					temp = serialize_array(rawget (x, i), addedsep)
				else
					temp = "'" .. string.gsub(serialize_array(rawget (x, i), addedsep), "'", "\\'") .. "'"
				end
				s = s .. sep .. temp
			end
		end
		if myfile then
			return s .. "}\r\nend\r\nreturn merge_tables(_table)"
		else
			return s .. "}"
		end
	else return tostring(x)
	end
end

--[[
This function uses the new tostring function extension.
--]]
function myprint(a)
	print(serialize_array(a))
end

--[[
Split text into a list consisting of the strings in text,
separated by strings matching delimiter (which may be a pattern).
example: string.split(",%s*", "Anna, Bob, Charlie,Dolores")
--]]
--Changed to version from: https://stackoverflow.com/questions/1426954/split-string-in-lua
function string.split(sep, inputstr)
        if sep == nil then
                sep = "%s"
        end
        local t={}
        for str in string.gmatch(inputstr, "([^"..sep.."]+)") do
                table.insert(t, str)
        end
        return t
end

--[[
table.copy( t )
returns a exact copy of table t
--]]
function table.copy( t, lookup_table )
	lookup_table = lookup_table or {}
	local tcopy = {}
	if not lookup_table[t] then
		lookup_table[t] = tcopy
	end
	for i,v in pairs( t ) do
		if type( i ) == "table" then
			if lookup_table[i] then
				i = lookup_table[i]
			else
				i = table.copy( i, lookup_table )
			end
		end
		if type( v ) ~= "table" then
			tcopy[i] = v
		else
			if lookup_table[v] then
				tcopy[i] = lookup_table[v]
			else
				tcopy[i] = table.copy( v, lookup_table )
			end
		end
	end
	return tcopy
end

function table.merge(main_table, sub_table)
	for k,v in pairs(sub_table) do main_table[#main_table+k] = v end
end

function ipairs_cs(tbl, cs) --as per http://lua-users.org/wiki/GeneralizedPairsAndIpairs
  -- Iterator function
  local function stateless_iter(tbl, i)
    -- Implement your own index, value selection logic
    i = i + 1
    local v = tbl[i]
    if v then return i, v end
  end

  -- return iterator function, table, and starting point
  return stateless_iter, tbl, cs
end

--lua data types

function number(v)
	return tonumber(v)
end

function _string(v)
	return v
end

function boolean(v)
	if v == '1' then --only 1 is valid as true
		return true
	else
		return false
	end
end

function array(v, f) --f = {func, separator}
	local finished_subitem = {}
	for i,w in ipairs(string.split(f[2], v)) do
		table.insert(finished_subitem, f[1](w))
	end
	return finished_subitem
end

function i(a)
	return a
end

function txt_to_tsv(txt_structure, txt_file) --will output transliteration of txt file to lua.
	tsv_structure = tsv_structure or i
	local myfile = io.open(txt_file, 'rb')
	local _data = myfile:read('*all')
	myfile:close()

	local finished_table = {}
	local finished_item = {}
	for i,v in ipairs(txt_structure) do
		table.insert(finished_item, v[1]) -- run structured function on the data
	end
	table.insert(finished_table, -1, finished_item)
	
	local input_table = string.split('\r\n', _data)
	--Bug: Luajit will run out of memory after placing many items into table per function call
	--Workaround: Instead of calling loop directly, call it in another function, make it make the table, then join them (much slower).
	--In regular lua: Only the txt_to_tsv_loop call without the number_of_items_max parameter use would be necessary!
	local cs = 0
	while true do
		local cont, finished_table_part = txt_to_tsv_loop(input_table, cs, txt_structure, items_number_max_per_function)
		for i,v in ipairs(finished_table_part) do
			--print(txt_file, i, v[1], v[2], cont)
			
			finished_table[tonumber(v[1])] = v
		end
		if not cont then
			break
		end
		cs = cs + items_number_max_per_function
	end
	
	--local myfile2 = io.open(tsv_file, 'wb')
	
	--only array is normalized, apply efficient translation:
	
	--myfile2:write(serialize_withspecialneg1(array, myfile2))
	--myfile2:write(serialize_withspecialneg1(finished_table))
	--myfile2:close()
	
	return hashtoarray(finished_table, {}, function(x) if x == -1 or x > 0 then return true end return false end)
end

function txt_to_tsv_loop(input_table, cs, txt_structure, number_of_items_max)
	local finished_table = {}
	for i,v in ipairs_cs(input_table, cs) do
		if string.sub(v, 1, 2) ~= '//' then
			local item_table = string.split('\t', v)
			if item_table[1] ~= nil then
				local finished_item = {}
				for j,w in ipairs(item_table) do
					local test = txt_structure[j][2](w)
					if test ~= nil and w ~= nil then
						table.insert(finished_item, txt_structure[j][2](w)) -- run structured function on the data
					end
				end
				if i-cs > number_of_items_max then
					return true,finished_table
				end --finished_table[tonumber(item_table[1])-cs] = finished_item
				table.insert(finished_table, finished_item) --make table index = 'id' field (for now).
			end
		end
	end
	return false,finished_table
end

local txt = {}
local _txt_to_tsv = {}

--Structures
txt.areaset = {
	{'id',					number},
	{'name',				_string},
	{'color',				oa2(array,{number,','})},
	{'music_file',			number},
	{'color2',				oa2(array,{number,','})},
	{'light_color', 		oa2(array,{number,','})},
	{'light_angle',			oa2(array,{number,','})},
	{'iscity',				boolean}
}

_txt_to_tsv.areaset = function(t)
	
end

txt.characterposeinfo = {
	{'id',					number},
	{'name',				_string},
	{'fists',				number},
	{'sword',				number},
	{'two_handed_sword',	number},
	{'dual_swords',			number},
	{'firegun',				number},
	{'bow',					number},
	{'dagger',				number}
}

txt.chaticons = {
	{'id',					number},
	{'big_icon',			_string},
	{'big_icon_x',			number},
	{'big_icon_y',			number},
	{'small_on_icon',		_string},
	{'small_on_icon_x',		number},
	{'small_on_icon_y',		number},
	{'small_off_icon',		_string},
	{'small_off_icon_x',	number},
	{'small_off_icon_y',	number},
	{'very_big_icon',		_string},
	{'very_big_icon_x',		number},
	{'very_big_icon_y',		number}
}

txt.elfskillinfo = {
	{'id',		number},
	{'name',	_string},
	{'kind_id',	number},
	{'level',	number}
}

txt.eventsound = {
	{'id',			number},
	{'name',		_string},
	{'sound_id',	number}
}

txt.forgeitem = {
	{'id',				number},
	{'level',			_string}, --TODO: Make this a number (can't right now because ffi needs id 0 for indexing)
	{'success_rate',	number},
	{'requirement_1',	oa2(array,{number,','})},
	{'requirement_2',	oa2(array,{number,','})},
	{'requirement_3',	oa2(array,{number,','})},
	{'requirement_4',	oa2(array,{number,','})},
	{'requirement_5',	oa2(array,{number,','})},
	{'requirement_6',	oa2(array,{number,','})},
	{'price',			number}
}

txt.hairs = {
	{'id',				number},
	{'name',			_string},
	{'color',			_string},
	{'item1',			oa2(array,{number,','})},
	{'item2',			oa2(array,{number,','})},
	{'item3',			oa2(array,{number,','})},
	{'item4',			oa2(array,{number,','})},
	{'price',			number},
	{'model',			number},
	{'failed_model',	number},
	{'for_lance',		number},
	{'for_carsise',		number},
	{'for_phyllis',		number},
	{'for_ami',			number},
}

txt.helpinfoset = {
	{'id',			number},
	{'name',		_string},
	{'description', _string}
}

txt.itempre = {
	{'id',			number},
	{'name',		_string}
}

txt.itemrefineeffectinfo = {
	{'id',				number},
	{'name',			_string},
	{'unknown_1',		number},
	{'lance_id_1',		number},
	{'carsise_id_1',	number},
	{'phyllis_id_1',	number},
	{'ami_id_1',		number},
	{'unknown_2',		number},
	{'lance_id_2',		number},
	{'carsise_id_2',	number},
	{'phyllis_id_2',	number},
	{'ami_id_2',		number},
	{'unknown_3',		number},
	{'lance_id_3',		number},
	{'carsise_id_3',	number},
	{'phyllis_id_3',	number},
	{'ami_id_3',		number},
	{'unknown_4',		number},
	{'lance_id_4',		number},
	{'carsise_id_4',	number},
	{'phyllis_id_4',	number},
	{'ami_id_4',		number},
	{'unknown',			number}
}

txt.itemrefineinfo = {
	{'id',							number},
	{'name',						_string},
	{'red_effect',					number},
	{'blue_effect',					number},
	{'green_effect',				number},
	{'yellow_effect',				number},
	{'red_blue_effect',				number},
	{'red_green_effect',			number},
	{'red_yellow_effect',			number},
	{'blue_green_effect',			number},
	{'blue_yellow_effect',			number},
	{'green_yellow_effect',			number},
	{'red_blue_green_effect',		number},
	{'red_blue_yellow_effect',		number},
	{'red_green_yellow_effect',		number},
	{'blue_green_yellow_effect',	number},
	{'height',						number},
	{'width',						number},
	{'length',						number},
	{'size_increment',				number},
}

txt.itemtype = {
	{'id',			number},
	{'name',		_string}
}

txt.magicgroupinfo = {
	{'id',				number},
	{'name',			_string},
	{'types_amount',	number},
	{'types',			oa2(array,{number,','})},
	{'amount',			oa2(array,{number,','})},
	{'unknown',			number}
}

txt.magicsingleinfo = {
	{'id',					number},
	{'name',				_string},
	{'effect_files_amount',	number},
	{'effect_files',		oa2(array,{_string,','})},
	{'frames',				number},
	{'effects_amount',		number},
	{'effects',				oa2(array,{_string,','})},
	{'unknown_list',		oa2(array,{number,','})},
	{'unknown',				number},
	{'unknown2',			number},
	{'end_effect',			_string},
}

txt.mapinfo = {
	{'id',					number},
	{'name',				_string},
	{'full_name',			_string},
	{'iscity',				boolean},
	{'center_coordintes',	oa2(array,{number,','})},
	{'empty',				_string},
	{'color',				oa2(array,{number,','})},
}

txt.monsterinfo = {
	{'id',					number},
	{'id2',					number},
	{'start_coordinates',	oa2(array,{number,','})},
	{'end_coordinates',		oa2(array,{number,','})},
	{'char_ids',			oa2(array,{number,','})},
	{'map_file_name',		_string},
}

txt.monsterlist = {
	{'id',			number},
	{'name',		_string},
	{'level',		number},
	{'coordinates',	oa2(array,{number,','})},
	{'map',			_string}
}

txt.musicinfo = {
	{'id',		number},
	{'file',	_string},
	{'iswav',	boolean}
}

txt.objevent = {
	{'id',			number},
	{'name',		_string},
	{'unknown1',	number},
	{'unknown2',	number},
	{'unknown3',	number},
	{'unknown4',	number},
	{'unknown5',	number},
	{'unknown6',	number},
	{'unknown7',	number},
	{'unknown8',	oa2(array,{number,','})}
}

txt.notifyset = {
	{'id',			number},
	{'name',		_string},
	{'is_enabled',	boolean},
	{'description',	_string}
}

txt.npclist = {
	{'id',			number},
	{'name',		_string},
	{'area',		_string},
	{'coordinates',	oa2(array,{number,','})},
	{'map',			_string}
}

txt.resourceinfo = {
	{'id',		number},
	{'file',	_string},
	{'unknown',	number}
}

txt.sceneffectinfo = {
	{'id',				number},
	{'file',			_string},
	{'name',			_string},
	{'image_name',		_string},
	{'flag',			number},
	{'obj_type_id',		number},
	{'unknown_list',	oa2(array,{number,','})},
	{'unknown2',		number},
	{'unknown3',		number},
	{'unknown4', 		number},
	{'unknown5',		number},
	{'unknown6',		number}
}

txt.sceneobjinfo = {
	{'id',				number},
	{'file',			_string},
	{'name',			_string},
	{'type',			number},
	{'var1',			_string}, --assume string for further handling
	{'var2',			_string}, --assume string for further handling
	{'effect_id',		number},
	{'has_env_light',	boolean},
	{'has_point_light',	boolean},
	{'style',			number},
	{'flag',			number},
	{'size_flag',		number},
	{'shade_flag',		number}
}

txt.selectcha = {
	{'id',				number},
	{'name',			_string},
	{'char_id',			number},
	{'unknown',			number},
	{'unknown2_list',	oa2(array,{number,','})},
	{'armor',			number},
	{'gloves',			number},
	{'boots',			number},
}

txt.serverset = {
	{'id',			number},
	{'name',		_string},
	{'sub_region',	_string},
	{'ip',			_string},
	{'alt_ip',		_string},
	{'alt_ip2',		_string},
	{'alt_ip3',		_string},
	{'alt_ip4',		_string},
	{'region',		_string},
	{'group',		_string}
}

txt.shadeinfo = {
	{'id',			number},
	{'file',		_string},
	{'name',		_string},
	{'unknown',		number},
	{'unknown2',	number},
	{'unknown3',	number},
	{'unknown4',	number},
	{'unknown5',	number},
	{'unknown6',	number},
	{'unknown7',	number},
	{'unknown8',	number},
	{'unknown9',	number},
	{'unknown10',	number},
	{'unknown11',	number},
}

txt.shipinfo = {
	{'id',						number},
	{'name',					_string},
	{'item_id',					number},
	{'char_id',					number},
	{'action_id',				number},
	{'hull',					number},
	{'engine',					oa2(array,{number,','})},
	{'mobility',				oa2(array,{number,','})},
	{'cannon',					oa2(array,{number,','})},
	{'component',				oa2(array,{number,','})},
	{'level_restriction',		number},
	{'class_restriction',		number},
	{'durability',				number},
	{'durability_recovery',		number},
	{'defense',					number},
	{'physical_resistance',		number},
	{'min_attack',				number},
	{'max_attack',				number},
	{'attack_range',			number},
	{'reloading_time',			number},
	{'cannon_area_of_effect',	number},
	{'cargo_capacity',			number},
	{'fuel',					number},
	{'fuel_consumption',		number},
	{'attack_speed',			number},
	{'movement_speed',			number},
	{'description',				number},
	{'remark',					number},
}

txt.shipiteminfo = {
	{'id',						number},
	{'name',					_string},
	{'model',					_string},
	{'propeller_1',				number},
	{'propeller_2',				number},
	{'propeller_3',				number},
	{'propeller_4',				number},
	{'price',					number},
	{'durability',				number},
	{'durability_recovery',		number},
	{'defense',					number},
	{'physical_resist',			number},
	{'min_attack',				number},
	{'max_attack',				number},
	{'attack_range',			number},
	{'reloading_time',			number},
	{'cannon_area_of_effect',	number},
	{'cargo_capacity',			number},
	{'fuel',					number},
	{'fuel_consumption',		number},
	{'attack_speed',			number},
	{'movement_speed',			number},
	{'description',				number},
	{'remark',					number},
}

txt.skilleff = {
	{'id',										number},
	{'name',									_string},
	{'activation_interval',						number},
	{'func_transition',							_string},
	{'func_start',								_string},
	{'func_end',								_string},
	{'is_manually_cancellable',					number},
	{'can_player_move',							boolean},
	{'can_player_use_skills',					boolean},
	{'can_player_normal_attack',				boolean},
	{'can_player_trade',						boolean},
	{'can_player_use_items',					boolean},
	{'can_player_attack',						boolean},
	{'can_player_be_attacked',					boolean},
	{'can_player_be_item_targetable',			boolean},
	{'can_player_be_skill_targetable',			boolean},
	{'can_player_be_invisible',					boolean},
	{'can_player_be_seen_as_himself',			boolean},
	{'can_player_use_inventory',				boolean},
	{'can_player_talk_to_npc',					boolean},
	{'remove_effect_id',						number},
	{'screen_effect',							number},
	{'client_performance',						number},
	{'client_display_id',						number},
	{'ground_status_effect',					number},
	{'center_display',							number},
	{'knockout_display',						number},
	{'special_effect_of_recipe',				number},
	{'unknown',									number},
	{'display_effect_when_attacking_yourself',	number},
	{'unknown2',								number}
}

txt.skillinfo = {
	{'id',										number},
	{'name',									_string},
	{'is_not_life_skill',						number},
	{'class_requirements',						oa2(array,{oa2(array,{number,','}),';'})},
	{'left_hand_requirements',					oa2(array,{oa2(array,{number,','}),';'})},
	{'right_hand_requirements',					oa2(array,{oa2(array,{number,','}),';'})},
	{'armor_requirement',						oa2(array,{oa2(array,{number,','}),';'})},
	{'conch_usage',							    oa2(array,{number,','})},
	{'skill_phase',							    number},
	{'skill_type',							    number},
	{'is_useful',								number},
	{'learning_level',							number},
	{'skill_prerequisites',						oa2(array,{oa2(array,{number,','}),';'})},
	{'skill_points_consumption',				number},
	{'discharged_status',						number},
	{'apply_point',								number},
	{'cast_range',								number},
	{'process_target',							number},
	{'attack_mode',								number},
	{'angle',									number},
	{'radius',									number},
	{'region_shape',							number},
	{'func_pre_targetting',						_string},
	{'func_aoe_effect',							_string},
	{'func_sp',									_string},
	{'func_durability',							_string},
	{'func_energy',								_string},
	{'func_aoe_range',							_string},
	{'func_start',								_string},
	{'func_end',								_string},
	{'func_use',								_string},
	{'func_unuse',								_string},
	{'is_bind_status_manually_removable',		number},
	{'self_parameter',							number},
	{'self_effect',								number},
	{'consumable',								number},
	{'duration',								number},
	{'target_parameter',						number},
	{'splash_parameter',						number},
	{'duration_on_target',						number},
	{'splash_persists_effect',					number},
	{'morph_id',								number},
	{'summon_id',								number},
	{'discharge_duration',						number},
	{'func_cooldown',							_string},
	{'damage_effect',							number},
	{'play_effect',								number},
	{'action',									oa2(array,{number,','})},
	{'keyframe',								number},
	{'attack_sound_effect',						number},
	{'character_dummy',							oa2(array,{number,','})},
	{'character_effect',						oa2(array,{number,','})},
	{'base_standard_value',						oa2(array,{number,','})},
	{'item_dummy',								number},
	{'item_effect',								oa2(array,{number,','})},
	{'item_effect2',							oa2(array,{number,','})},
	{'path_of_flight_keyframe',					number},
	{'character_dummy',							number},
	{'item_dummy2',								number},
	{'path_of_flight_effect',					number},
	{'path_of_flight_speed',					number},
	{'attacked_sound_effect',					number},
	{'dummy',									number},
	{'character_attacked_effect',				number},
	{'effect_duration_point',					number},
	{'surface_attacked_effect',					number},
	{'water_surface_effect',					number},
	{'icon',									_string},
	{'play_count',								number},
	{'command',									oa2(array,{number,','})},
	{'description',								_string},
	{'effect',									_string},
	{'consumption',								_string}
}

txt.stoneinfo = {
	{'id',					number},
	{'name',				_string},
	{'item_id',				number},
	{'slots',				oa2(array,{number,','})},
	{'base_color',			number},
	{'description_script',	_string},
	{'stats',				oa2(array,{oa2(array,{number,','}),';'})}, --non-native
	--NOTE: There's no way to add stats automatically. They have to be added manually afterwards!
}

txt.terraininfo = {
	{'id',		number},
	{'file',	_string},
	{'width',	number},
	{'height',	number}
}

txt.iteminfo = {
	{'id',								number},
	{'name',							_string},
	{'icon_name',						_string},
	{'model_ground',					_string},
	{'model_lance',						_string},
	{'model_carsise',					_string},
	{'model_phyllis',					_string},
	{'model_ami',						_string},
	{'ship_symbol',						number},
	{'ship_size',						number},
	{'type',							number},
	{'prefixing_rate',					number},
	{'set_id',							number},
	{'forging_level',					number},
	{'stable_level',					number},
	{'isrepairable',					boolean},
	{'istradeable',						boolean},
	{'ispickable',						boolean},
	{'isdroppable',						boolean},
	{'isdeletable',						boolean},
	{'max_stack_amount',				number},
	{'instance',						number},
	{'price',							number},
	{'char_types',						oa2(array,{number,','})},
	{'char_level',						number},
	{'char_classes',					oa2(array,{number,','})},
	{'char_name',						_string},
	{'char_reputation',					_string},
	{'equippable_slots',				oa2(array,{number,','})},
	{'item_switch_slots',				oa2(array,{number,','})},
	{'obtain_in_determined_location',	number},
	{'str_mul',							number},
	{'agi_mul',							number},
	{'acc_mul',							number},
	{'con_mul',							number},
	{'spr_mul',							number},
	{'luk_mul',							number},
	{'attack_speed_mul',				number},
	{'attack_range_mul',				number},
	{'min_attack_mul',					number},
	{'max_attack_mul',					number},
	{'defense_mul',						number},
	{'max_hp_mul',						number},
	{'max_sp_mul',						number},
	{'dodge_rate_mul',					number},
	{'hit_rate_mul',					number},
	{'critical_rate_mul',				number},
	{'treasure_drop_rate_mul',			number},
	{'hp_recovery_mul',					number},
	{'sp_recovery_mul',					number},
	{'movement_speed_mul',				number},
	{'resource_gathering_rate_mul',		number},
	{'str',								oa2(array,{number,','})},
	{'agi',								oa2(array,{number,','})},
	{'acc',								oa2(array,{number,','})},
	{'con',								oa2(array,{number,','})},
	{'spr',								oa2(array,{number,','})},
	{'luk',								oa2(array,{number,','})},
	{'attack_speed',					oa2(array,{number,','})},
	{'attack_range',					oa2(array,{number,','})},
	{'min_attack',						oa2(array,{number,','})},
	{'max_attack',						oa2(array,{number,','})},
	{'defense',							oa2(array,{number,','})},
	{'max_hp',							oa2(array,{number,','})},
	{'max_sp',							oa2(array,{number,','})},
	{'dodge',							oa2(array,{number,','})},
	{'hit_rate',						oa2(array,{number,','})},
	{'critical_rate',					oa2(array,{number,','})},
	{'treasure_drop_rate',				oa2(array,{number,','})},
	{'hp_recovery',						oa2(array,{number,','})},
	{'sp_recovery',						oa2(array,{number,','})},
	{'movement_speed',					oa2(array,{number,','})},
	{'resource_gathering_rate',			oa2(array,{number,','})},
	{'physical_resist',					oa2(array,{number,','})},
	{'left_hand_weapon_effectiveness',  number},
	{'energy',							oa2(array,{number,','})},
	{'durability',						oa2(array,{number,','})},
	{'gem_sockets',						number},
	{'ship_durability_recovery',		number},
	{'ship_cannon_amount',				number},
	{'ship_member_count',				number},
	{'ship_label',						number},
	{'ship_cargo_capacity',				number},
	{'ship_fuel_consumption',			number},
	{'ship_attack_speed',				number},
	{'ship_movement_speed',				number},
	{'func_use',						_string},
	{'display_effect',					number},
	{'bind_effect',						oa2(array,{number,','})},
	{'bind_effect2',					oa2(array,{number,','})},
	{'first_inv_slot_effect',		    oa2(array,{number,','})},
	{'drop_model_effect',			    oa2(array,{number,','})},
	{'item_usage_effect',			    oa2(array,{number,','})},
	{'description',						_string},
}

txt.characterinfo = {
	{'id',							number},
	{'name',                        _string},
	{'short_name',                  _string},
	{'model_type',                  number},
	{'ai_logic_type',               number},
	{'framework_number',            number},
	{'suite_serial',                number},
	{'serial_amount',               number},
	{'item_slot_0',                 number},
	{'item_slot_1',                 number},
	{'item_slot_2',                 number},
	{'item_slot_3',                 number},
	{'item_slot_4',                 number},
	{'item_slot_5',                 number},
	{'item_slot_6',                 number},
	{'item_slot_7',                 number},
	{'effect_ids',                  oa2(array,{number,','})},
	{'e_effect_id',                 number},
	{'special_action_ids',          oa2(array,{number,','})},
	{'has_shadow',                  number},
	{'action_id',                   number},
	{'opacity',                     number},
	{'moving_sound_id',             number},
	{'standing_sound_id',           number},
	{'death_sound_id',              number},
	{'is_controllable',             number},
	{'is_limited_to_area',          number},
	{'base_altitude',               number},
	{'item_types_equippable',       oa2(array,{number,','})},
	{'length',                      number},
	{'width',                       number},
	{'height',                      number},
	{'collision_range',             number},
	{'birth',                       oa2(array,{number,','})},
	{'death',                       oa2(array,{number,','})},
	{'birth_effect',                oa2(array,{number,','})},
	{'death_effect',                oa2(array,{number,','})},
	{'hibernate_action',            number},
	{'death_instant_action',        number},
	{'remaining_hp_effect_display', oa2(array,{number,','})},
	{'is_attack_dodgeable',         number},
	{'is_attack_pusheable',         number},
	{'script',                      number},
	{'weapon_used',                 number},
	{'skill_ids',                   oa2(array,{number,','})},
	{'skill_rates',                 oa2(array,{number,','})},
	{'drop_item_ids',               oa2(array,{number,','})},
	{'drop_item_rates',             oa2(array,{number,','})},
	{'max_amount_drops',            number},
	{'fatality_rate',               number},
	{'prefix_lvl',                  number},
	{'quest_drop_item_ids',         oa2(array,{number,','})},
	{'quest_drop_item_rates',       oa2(array,{number,','})},
	{'ai',                          number},
	{'is_turnable',                 number},
	{'sight_range',                 number},
	{'noise',                       number},
	{'get_exp',                     number},
	{'light',                       number},
	{'mob_exp',                     number},
	{'level',                       number},
	{'max_hp',                      number},
	{'hp',                          number},
	{'max_sp',                      number},
	{'sp',                          number},
	{'min_attack',                  number},
	{'max_attack',                  number},
	{'pr',                          number},
	{'defense',                     number},
	{'hit_rate',                    number},
	{'dodge_rate',                  number},
	{'critical_chance',             number},
	{'drop_rate_chance',            number},
	{'hp_recovery',                 number},
	{'sp_recovery',                 number},
	{'attack_speed',                number},
	{'attack_distance',             number},
	{'chase_distance',              number},
	{'movement_speed',              number},
	{'col',                         number},
	{'str',                         number},
	{'agi',                         number},
	{'acc',                         number},
	{'con',                         number},
	{'spr',                         number},
	{'luk',                         number},
	{'left_rad',                    number},
	{'guild_id',                    _string},
	{'title',                       _string},
	{'class',                       _string},
	{'exp',                         number},
	{'next_lvl_exp',                number},
	{'reputation',                  number},
	{'ap',                          number},
	{'tp',                          number},
	{'gd',                          number},
	{'spri',                        number},
	{'story',                       number},
	{'max_sail',                    number},
	{'sail',                        number},
	{'stasa',                       number},
	{'scsm',                        number},
	{'tstr',                        number},
	{'tagi',                        number},
	{'tacc',                        number},
	{'tcon',                        number},
	{'tspr',                        number},
	{'tluk',                        number},
	{'tmax_hp',                     number},
	{'tmax_sp',                     number},
	{'tattack',                     number},
	{'tdefense',                    number},
	{'thit_rate',                   number},
	{'tdodge_rate',                 number},
	{'tdrop_rate',                  number},
	{'tcritical_chance',            number},
	{'thp_recovery',                number},
	{'tsp_recovery',                number},
	{'tattack_speed',               number},
	{'tattack_distance',            number},
	{'tmovement_speed',             number},
	{'tspri',                       number},
	{'tscsm',                       number},
	{'chasf',                       oa2(array,{number,','})}
}

local txt_names = {
	'areaset',
	'characterposeinfo',
	'chaticons',
	'elfskillinfo',
	'eventsound',
	'forgeitem',
	'hairs',
	'helpinfoset',
	'itempre',
	'itemrefineeffectinfo',
	'itemrefineinfo',
	'itemtype',
	'magicgroupinfo',
	'magicsingleinfo',
	'mapinfo',
	'monsterinfo',
	'monsterlist',
	'musicinfo',
	'objevent',
	'notifyset',
	'npclist',
	'resourceinfo',
	'sceneffectinfo',
	'sceneobjinfo',
	'selectcha',
	'serverset',
	'shadeinfo',
	'shipinfo',
	'shipiteminfo',
	'skilleff',
	'skillinfo',
	'stoneinfo',
	'terraininfo',
	'iteminfo',
	'characterinfo'
}

--[[
Negative indexes prevent name and number colision with item indexes/names. They each mean the following:
- -1 = sub-item key number to sub-item key names - gives a name to the column of the subitem - not used directly usually. tsv.blarg[-1][5] = 'blarg'
- -2 = sub-item key name to sub-item key number - gives the number to the sub-item key to access per name - always used directly. tsv.blarg[-1]['blarg']  = 5
- -3 = item key name to item table - gives a way to access item table using the item table's name field. Usually used. tsv.blarg['my item table'].
--]]

value_name_keys_mt = {}
function value_name_keys_mt.gen__index(p_t) --Gives item tables access to the main table column names.
	return function (t,key)
		--print('index:',t,key)
		if type(key) == "string" then
			return rawget(t, p_t[-1][key])
		end
		return rawget(t, key)
	end
end

function value_name_keys_mt.gen__newindex(p_t) --Gives item tables access to the main table column names.
	return function (t,key,value)
		if type(key) == "string" then
			--print(key, rawget(p_t[-1], key))
			return rawset(t, rawget(p_t[-1], key), value)
		end
		return rawset(t, key, value)
	end
end

function table_values_as_keys(_table)
	for i,v in ipairs(_table) do
		_table[v] = i --regular column items will have number indexes and string values, so this perfectly implements the opposite.
	end
	return
end
function table_names_and_column_names_as_keys(_table)
	vnk_mt = {}
	vnk_mt.__index = value_name_keys_mt.gen__index(_table)
	vnk_mt.__newindex = value_name_keys_mt.gen__newindex(_table)
	
	for i,v in ipairs(_table) do
		setmetatable(v, vnk_mt)
		if v[2] ~= nil and type(v[2]) == 'string' then
			--all v[2] are strings (except on a few tsv) - so no matter what they won't replace the number indexes - thus this is safe.
			--also, v is not copied, just its pointer is (like with all tables), so this is also memory efficient.
			--won't be used if it's not a string, like with monsterinfo.
			_table[v[2]] = v
		end
	end
	return
end

local tsv_inter = {}

--[[
table.copy( t )
returns a exact copy of table t
--]]
function table.copy( t, lookup_table )
	lookup_table = lookup_table or {}
	local tcopy = {}
	if not lookup_table[t] then
		lookup_table[t] = tcopy
	end
	for i,v in pairs( t ) do
		if type( i ) == "table" then
			if lookup_table[i] then
				i = lookup_table[i]
			else
				i = table.copy( i, lookup_table )
			end
		end
		if type( v ) ~= "table" then
			tcopy[i] = v
		else
			if lookup_table[v] then
				tcopy[i] = lookup_table[v]
			else
				tcopy[i] = table.copy( v, lookup_table )
			end
		end
	end
	return tcopy
end

txt.def_trans = {['p']=table.copy}

--print(tsv_inter.musicinfo[1]['file'])

txt.areaset_trans = {['p']=function(_1)
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		if _2[i]['music_file'] == -1 then
			_2[i]['music_file'] = 0 --TODO: to change to a string as well (to implement ffi).
		else
			_2[i]['music_file'] = tsv_inter.musicinfo[_2[i]['music_file']]['file']
		end
		
		--Step by step restructuring (replacing method - efficient for arrays):
		local temp = _2[i]['light_angle']
		_2[i]['light_angle'] = _2[i]['light_color']
		_2[i]['light_color'] = _2[i]['color2']
		_2[i]['color2'] = _2[i]['color']
		_2[i]['color'] = _2[i]['iscity']
		_2[i]['iscity'] = temp
	end
	--Step by step restructuring (replacing method - efficient for arrays - NU for string index not updated, string will index old number correctly):
	_2[-1][_2[-1]['light_angle']] = 'light_color'
	_2[-1][_2[-1]['light_color']] = 'color2'
	_2[-1][_2[-1]['color2']] = 'color'
	_2[-1][_2[-1]['color']] = 'iscity'
	_2[-1][_2[-1]['iscity']] = 'light_angle'
	return _2
end}

txt.chaticons_trans = {['p']=function(_1)
	local _2 = {[-1]={'big_icon','small_on_icon','small_off_icon','very_big_icon'}}
	table_values_as_keys(_2[-1])
	--every new table or table copy needs a new metatable
	local vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_1 do
		_2[i] = {}
		setmetatable(_2[i], vnk_mt_)
		for j,v in ipairs(_2[-1]) do --ipairs is necessary here because there are both string and number keys
			if (v == 'big_icon') or (_1[i][v] ~= '0') then
				_2[i][j] = {_1[i][v],{_1[i][v..'_x'],_1[i][v..'_y']}} --using j on assignment instead of v because it's faster (there's no way of just using v unfortunately)
			else
				_2[i][j] = {}
			end
		end
	end
	return _2
end}


txt.characterinfo_trans = {['p']=function(_1)
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		--TODO: Remove the -1 items in many items
		table.remove(_2[i], _2[-1]['short_name'])
	end
	table.remove(_2[-1],_2[-1]['short_name'])
	return _2
end}

txt.forgeitem_trans = {['p']=function(_1) --in this case, it's easier to create a new table than removing so many columns...
	local _2 = {[-1]={'level',	'success_rate',	'price'}}
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_1 do
		_2[i] = {_1[i]['level'], _1[i]['success_rate'], _1[i]['price']}
		setmetatable(_2[i], vnk_mt_)
	end
	return _2
end}

txt.iteminfo_trans = {['p']=function(_1)
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		--TODO: Restructure (many things could be simplified here)
		table.remove(_2[i], _2[-1]['func_use'])
		table.remove(_2[i], _2[-1]['char_types'])
	end
	table.remove(_2[-1],_2[-1]['func_use'])
	table.remove(_2[-1],_2[-1]['char_types'])
	
	table.insert(_2[-1],'func_use')
	return _2
end}

txt.magicgroupinfo_trans = {['p']=function(_1) --in this case, it's easier to create a new table than removing so many columns...
	local _2 = {[-1]={'name',	'types',	'unknown'}}
	table_values_as_keys(_2[-1])
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_1 do
		_2[i] = {_1[i]['name'], {}, _1[i]['unknown']}
		setmetatable(_2[i], vnk_mt_)
		for j,v in ipairs(_1[i]['types']) do
			_2[i]['types'][j] = {_1[i]['types'][j], _1[i]['amount'][j]}
		end
	end
	return _2
end}

txt.magicsingleinfo_trans = {['p']=function(_1) --in this case, it's easier to create a new table than removing so many columns...
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		--basically the number of items is ignored by this script, except if item is 0.
		--(Really makes no sense to ignore files, even though game works like this)
		
		--there's no way someone would do ,file instead of file, (please don't)
		local j
		if _2[i]['effect_files'] ~= nil then
			for j = #_2[i]['effect_files'], 1, -1  do
				if _2[i]['effect_files'][j] == '' then
					_2[i]['effect_files'][j] = nil
				end
			end
		end
		
		if _1[i]['effects'] ~= nil then
			if #_1[i]['effects'] == 1 and _1[i]['effects'][1] == '0' and _1[i]['effects_amount'] == 0 then
				_2[i]['effects'] = {}
			end
			
			for j = #_2[i]['effects'], 1, -1  do
				if _2[i]['effects'][j] == '' then
					_2[i]['effects'][j] = nil
				end
			end
		end
		
		if _1[i]['unknown_list'] ~= nil then
			if #_1[i]['unknown_list'] == 1 and _1[i]['unknown_list'][1] == -1 then
				_2[i]['unknown_list'] = {}
			end
			
			for j = #_2[i]['unknown_list'], 1, -1  do
				if _2[i]['unknown_list'][j] == -1 then
					_2[i]['unknown_list'][j] = nil
				end
			end
		end
		
		table.remove(_2[i], _2[-1]['effects_amount'])
		table.remove(_2[i], _2[-1]['effect_files_amount'])
	end
	table.remove(_2[-1],_2[-1]['effects_amount'])
	table.remove(_2[-1],_2[-1]['effect_files_amount'])
	return _2
end}

txt.monsterinfo_trans = {['p']=function(_1) --in this case, it's easier to create a new table than removing so many columns...
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		table.remove(_2[i], _2[-1]['id2'])
	end
	table.remove(_2[-1],_2[-1]['id2'])
	return _2
end}

txt.npclist_trans = {['p']=function(_1)
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		--Step by step restructuring (replacing method - efficient for arrays):
		local temp = _2[i]['coordinates']
		_2[i]['coordinates'] = _2[i]['area']
		_2[i]['area'] = temp
	end
	--Step by step restructuring (replacing method - efficient for arrays - NU for string index not updated, string will index old number correctly):
	_2[-1][_2[-1]['area']] = 'coordinates'
	_2[-1][_2[-1]['coordinates']] = 'area'
	return _2
end}

txt.sceneobjinfo_trans = {['p']=function(_1)
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		
		--Define var1 depending on value of type
		if _2[i]['type'] == 0 then
			_2[i]['var1'] = oa2(array,{number,','})(_1[i]['var1'])
		elseif _2[i]['type'] == 3 then
			_2[i]['var1'] = oa2(array,{number,','})(_1[i]['var1'])
		elseif _2[i]['type'] == 4 then
			_2[i]['var1'] = oa2(array,{number,','})(_1[i]['var1'])
		elseif _2[i]['type'] ~= 6 then --6 is _string, as initially defined --TODO: Fix 6 (at decompiler)
			_2[i]['var1'] = number(_1[i]['var1'])
		end
		
		--Define var2 depending on value of type
		if _2[i]['type'] == 3 then
			_2[i]['var2'] = oa2(array,{number,','})(_1[i]['var2'])
		else
			_2[i]['var2'] = number(_1[i]['var2'])
		end
	end
	return _2
end}

txt.skilleff_trans = {['p']=function(_1)
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		table.remove(_2[i], _2[-1]['func_end'])
		table.remove(_2[i], _2[-1]['func_start'])
		table.remove(_2[i], _2[-1]['func_transition'])
	end
	table.remove(_2[-1],_2[-1]['func_end'])
	table.remove(_2[-1],_2[-1]['func_start'])
	table.remove(_2[-1],_2[-1]['func_transition'])
	table.insert(_2[-1],'func_transition')
	table.insert(_2[-1],'func_start')
	table.insert(_2[-1],'func_end')
	return _2
end}

txt.skillinfo_trans = {['p']=function(_1)
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		table.remove(_2[i], _2[-1]['func_cooldown'])
		table.remove(_2[i], _2[-1]['func_unuse'])
		table.remove(_2[i], _2[-1]['func_use'])
		table.remove(_2[i], _2[-1]['func_end'])
		table.remove(_2[i], _2[-1]['func_start'])
		table.remove(_2[i], _2[-1]['func_aoe_range'])
		table.remove(_2[i], _2[-1]['func_energy'])
		table.remove(_2[i], _2[-1]['func_durability'])
		table.remove(_2[i], _2[-1]['func_sp'])
		table.remove(_2[i], _2[-1]['func_aoe_effect'])
		table.remove(_2[i], _2[-1]['func_pre_targetting'])
	end
	table.remove(_2[-1],_2[-1]['func_cooldown'])
	table.remove(_2[-1],_2[-1]['func_unuse'])
	table.remove(_2[-1],_2[-1]['func_use'])
	table.remove(_2[-1],_2[-1]['func_end'])
	table.remove(_2[-1],_2[-1]['func_start'])
	table.remove(_2[-1],_2[-1]['func_aoe_range'])
	table.remove(_2[-1],_2[-1]['func_energy'])
	table.remove(_2[-1],_2[-1]['func_durability'])
	table.remove(_2[-1],_2[-1]['func_sp'])
	table.remove(_2[-1],_2[-1]['func_aoe_effect'])
	table.remove(_2[-1],_2[-1]['func_pre_targetting'])
	
	table.insert(_2[-1],'func_pre_targetting')
	table.insert(_2[-1],'func_aoe_effect')
	table.insert(_2[-1],'func_sp')
	table.insert(_2[-1],'func_durability')
	table.insert(_2[-1],'func_energy')
	table.insert(_2[-1],'func_aoe_range')
	table.insert(_2[-1],'func_start')
	table.insert(_2[-1],'func_end')
	table.insert(_2[-1],'func_use')
	table.insert(_2[-1],'func_unuse')
	table.insert(_2[-1],'func_cooldown')
	table.insert(_2[-1],'func_miss')
	return _2
end}

txt.stoneinfo_trans = {['p']=function(_1)
	local _2 = table.copy(_1)
	--every new table or table copy needs a new metatable
	vnk_mt_ = {}
	vnk_mt_.__index = value_name_keys_mt.gen__index(_2)
	vnk_mt_.__newindex = value_name_keys_mt.gen__newindex(_2)
	for i = 1, #_2 do
		setmetatable(_2[i], vnk_mt_)
		table.remove(_2[i], _2[-1]['description_script'])
		table.remove(_2[i], _2[-1]['item_id'])
	end
	table.remove(_2[-1],_2[-1]['description_script'])
	table.remove(_2[-1],_2[-1]['item_id'])
	
	--Stats
	--TODO: Make attribute numbers into names here.
	
	--NOTE: There's no way to add stats automatically. They have to be added manually afterwards!
	return _2
end}

function txt_to_tsv_folder(table_folder)
	--needs 2 loops because some data from some tsv depend on other tsv
	
	--txt -> lua table (in memory)
	for i,v in ipairs(txt_names) do
		print(table_folder .. v .. '.txt')
		tsv_inter[v] = txt_to_tsv(txt[v], table_folder .. v .. '.txt')
		table_values_as_keys(tsv_inter[v][-1]) --can also be used to reset the column indexes
		table_names_and_column_names_as_keys(tsv_inter[v]) --names get set to -3, while column names are in items themselves
	end	
	
	--lua table (in memory) -> table (serialized)
	for i,v in ipairs(txt_names) do
		print('-> '..table_folder .. v .. '.lua')
		local file = io.open(table_folder..v..'.lua', 'wb')
		if txt[v..'_trans'] ~= nil then
			tsv_inter[v] = txt[v..'_trans'].p(tsv_inter[v])
		else
			tsv_inter[v] = tsv_inter[v]
		end
		file:write(serialize_array(tsv_inter[v], '\t', file))
		file:close()
	end
end