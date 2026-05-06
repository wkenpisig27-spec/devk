--[[
o-----------------------------------------------------------------------------o
| top-decompile v.1.0.7                                                       |
(-----------------------------------------------------------------------------)
| By deguix         / An Utility for TOP/PKO/KOP | Compatible w/ LuaJIT 2.0.5 |
|                  -----------------------------------------------------------|
|                                                                             |
|   This script decompiles the .bin files for most game versions. If any      |
| game version isn't supported, please let me know. Configuration section is  |
| found at the user.lua file.                                                 |
o-----------------------------------------------------------------------------o
--]]

print('- Loading file: decompiler.lua v.1.0.7 ')
print('---------------------------------------')

--From: http://lua-users.org/lists/lua-l/2010-01/msg01427.html
function pack(...)
	return { ... }, select("#", ...)
end

table.pack = pack
if table.unpack == nil then
	table.unpack = unpack
end

print('- Loading file: lua_functional.lua (from lua_functional v1.102)')
lf = dofile('./lua_functional.lua')

print('- Loading file: bit.lua (from luabit v0.4)')
dofile('./bit.lua')
print('- Loading file: hex.lua (from luabit v0.4)')
dofile('./hex.lua')

--lua "bugs":
---No existing structure to handle named returned parameters directly - table parameters input = 1 table parameter.
---Table construction not accepting multiple values returned from a function as items of the table (very counter-intuitive).
---Table construction not accepting function definitions (even though the regular variable definition of functions look almost the same).

--[[
This function is an extension to the original tostring function, and returns
the string representation of many values including tables.
--]]
function mytostring(x)
	local s
	if type(x) == "table" then
		s = "{"
		j = 0
		for i, v in pairs(x) do
			if i ~= "_G" and v ~= x then
				if ((type(rawget (x, i)) == "table") or (type(rawget (x, i)) == "number")) then
					temp = tostring(i) .. "=" .. mytostring(rawget (x, i))
				else
					temp = tostring(i) .. "='" .. mytostring(rawget (x, i)).. "'"
				end
				if j ~= 0 then
					s = s .. "," .. temp
				else
					s = s .. temp
				end
				j = j + 1
			end
		end
		return s .. "}"
	else return tostring(x)
	end
end

--[[
This function uses the new tostring function extension.
--]]
function myprint(a)
	print(mytostring(a))
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

--from util
--Fixed size string split
function string.split_fixedsize(text, size)
	local list = {}
	local pos = 1
	while 1 do
		local _string = string.sub(text, pos, pos+size)
		if string.len(_string) <= 0 then
			break
		end
		table.insert(list, _string)
		pos = pos+size+1
	end
	return list
end

function table.update(_table, array)
	for i,v in ipairs(array) do
		local exists_i = -1 -- -1 means doesn't exist.
		for j,w in ipairs(_table) do
			if w[1] == v[1] then
				exists_i = j
				--print(v[1], w[2], v[2])
				_table[j] = v --simplified from v[2] = w[2]
			end
		end
		if exists_i == -1 then
			--print(v[1])
			table.insert(_table, v)
		end
	end
	for i,v in pairs(array) do
		if type(i) == 'string' then
			_table[i] = v
		end
	end
	return _table
end

function table.index_by_first_value(_table, value)
	for i,v in ipairs(_table) do
		if v == value then
			return i
		end
	end
	return -1
end

--equivalent to "atoi" related functions
function bytestr_to_int(bytestr,endian,signed) -- use length of string to determine 8,16,32,64 bits
    local t=bytestr
    if endian=="big" then --reverse bytes
        local tt={}
        for k=1,table.getn(t) do
            tt[table.getn(t)-k+1]=t[k]
        end
        t=tt
    end
    local n=0
    for k in ipairs(t) do
        n=n+t[k]*2^((k-1)*8)
    end
    if signed then
        n = (n > 2^((table.getn(t)*8)-1) -1) and (n - 2^(table.getn(t)*8)) or n -- if last bit set, negative.
    end
    return n
end

--equivalent to "atoi" related functions
function bytes_to_int(str,endian,signed) -- use length of string to determine 8,16,32,64 bits
    local t={string.byte(str,1,-1)}
    if endian=="big" then --reverse bytes
        local tt={}
        for k=1,table.getn(t) do
            tt[table.getn(t)-k+1]=t[k]
        end
        t=tt
    end
    local n=0
    for k=1,table.getn(t) do
        n=n+t[k]*2^((k-1)*8)
    end
    if signed then
        n = (n > 2^((table.getn(t)*8)-1) -1) and (n - 2^(table.getn(t)*8)) or n -- if last bit set, negative.
    end
    return n
end

function bytestr_to_int_s(str,endian)
	return bytestr_to_int(str,endian,true)
end

--[[
function select(index, ...) --implemented in lua 5.1
	local return_table = {}
	if index == "#" then
		table.getn(arg)
	else
		for i,v in ipairs(arg) do
			if i > index then
				table.insert(return_table, v)
			end
		end
	end
	return unpack(return_table)
end
--]]

function int_to_bytes(num,endian,signed)
    if num<0 and not signed then num=-num print "warning, dropping sign from number converting to unsigned" end
    local res={}
    local n = math.ceil(select(2,math.frexp(num))/8) -- number of bytes to be used.
    if signed and num < 0 then
        num = num + 2^n
    end
    for k=n,1,-1 do -- 256 = 2^8 bits per char.
        local mul=2^(8*(k-1))
        res[k]=math.floor(num/mul)
        num=num-res[k]*mul
    end
    assert(num==0)
    if endian == "big" then
        local t={}
        for k=1,n do
            t[k]=res[n-k+1]
        end
        res=t
    end
    return string.char(unpack(res))
end

--stot and vice-versa.
function transform_to_bytes(data)
	local data_table = {}
	for i=1,string.len(data) do
		table.insert(data_table, string.byte(data, i))
	end
	return data_table
end

function transform_from_bytes(data_table)
	local data = ''
	for i,s in ipairs(data_table) do
		data = data .. string.char(s)
	end
	return data
end

--general function for decryption
function decrypt(encryption_key, data_items)
	for i,s in ipairs(data_items) do
		data_items[i] = ((s - encryption_key[((i-1) % table.getn(encryption_key))+1]) + 256) % 256
	end
	return data_items
end

function decrypt_bin(tsv_bin_file, decrypted_tsv_bin_file)
	local bin_file_encryption_key = {152,157,159,104,224,102,171,112,233,209,224,224,203,221,209,203,213,207} --converted from hexadecimal to decimal (lua 5.0 doesn't support hexadecimal numbers directly)

	local myfile = io.open(tsv_bin_file, 'rb')
	if myfile == nil then
		print(tsv_bin_file..' doesn\'t exist')
		return
	end
	local _data = myfile:read('*all')
	myfile:close()
	
	local _items_length = transform_to_bytes(string.sub(_data,1,4))
	local _items = string.split_fixedsize(string.sub(_data,5), bytestr_to_int(transform_to_bytes(string.sub(_data,1,4)))-1)
	
	for i,s in ipairs(_items) do
		_items[i] = decrypt(bin_file_encryption_key, transform_to_bytes(s))
	end

	local myfile2 = io.open(decrypted_tsv_bin_file, 'wb')
	myfile2:write(transform_from_bytes(_items_length)) --to make sure transformation is working, otherwise, use string.sub(_data,1,4) directly.
	for i,s in ipairs(_items) do
		myfile2:write(transform_from_bytes(s))
	end
	myfile2:close()
end

--from chunkspy
from_double = function(x)
  local sign = 1
  local mantissa = string.byte(x, 7) % 16
  for i = 6, 1, -1 do mantissa = mantissa * 256 + string.byte(x, i) end
  if string.byte(x, 8) > 127 then sign = -1 end
  local exponent = (string.byte(x, 8) % 128) * 16 +
                   math.floor(string.byte(x, 7) / 16)
  if exponent == 0 then return 0 end
  mantissa = (math.ldexp(mantissa, -52) + 1) * sign
  return math.ldexp(mantissa, exponent - 1023)
end

--from chunkspy
from_float = function(x)
  local sign = 1
  local mantissa = string.byte(x, 3) % 128
  for i = 2, 1, -1 do mantissa = mantissa * 256 + string.byte(x, i) end
  if string.byte(x, 4) > 127 then sign = -1 end
  local exponent = (string.byte(x, 4) % 128) * 2 +
                   math.floor(string.byte(x, 3) / 128)
  if exponent == 0 then return 0 end
  mantissa = (math.ldexp(mantissa, -23) + 1) * sign
  return math.ldexp(mantissa, exponent - 127)
end

function round(num, idp)
    local mult = 10^(idp or 0)
    if num >= 0 then return math.floor(num * mult + 0.5) / mult
    else return math.ceil(num * mult - 0.5) / mult end
end

--Another recode is under way, improvements to be expected:
---made parameters be part of the main table (in the form: {name,type_table,param=value}), to take advantage of lua's table structure.
---t,s,l becomes t as table with tables with 4 values: type, size of type, amount of types, and flag value. This should make it easier to have multiple types. This is not really an improvement, but internally, it makes the code easier to manage.
----type table has locked structure to reduce typing.
----"color" type removed.
----String now depends on l to determine the amount of characters it has, not s.
----New flag added to take care of the previous rcolor type (flag reverses the order of the type values).
----stp and v are the optional parts of the small type table (so you can type {'ulong',v=10} for example, stp is rarely used).
---Infinite marked addresses.

--maybe this way: {{{'ubyte', 2}, {'ushort', 2}}, {{'str','2',l=2}}} for 1,2,1,2;my,st
--or like this: semicollon(2*commma(2*ubyte+2*ushort)+2*str(2))
--or like this: ;(2*,(2*ubyte,2*ushort),2*str(2)) --type math
--or like this: ;{{2*t_ubyte,2*t_ushort},2*t_str(2)} --partial type math
--or like this: t_(t_(2*t_ubyte(),2*t_ushort()),2*t_str(2)) --partial type math
--_m(2,_m(2,'ubyte'),_m(2,'ushort')),_m(2,'str', l=2)
-- new testing module: type math.
--adding type 'pad' to replace any data to be skipped within range, 'save' to save spot, and 'load' to restore spot.

local tsv = {t={}}

function s_semicolon(n)
	n.s = string.sub(n.s,1,-string.len(n.j))
	n.j = ';'
	if string.len(n.s) > 0 then
		n.s = n.s..n.j
	end
	return n
end

function s_comma(n)
	n.s = string.sub(n.s,1,-string.len(n.j))
	n.j = ','
	if string.len(n.s) > 0 then
		n.s = n.s..j
	end
	return n
end

function s_null(n)
	n.s = string.sub(n.s,1,-string.len(n.j))
	n.j = ''
	return n
end

function pad(n)
	n.i=n.i+1
	return n
end

function save(n) --remember, save indexes are always sequential
	table.insert(n.t.s, n.i)
	return n
end

function _load(n,f)
	n.i = n.t.s[f]
	return n
end

function static(n,f)
	n.c = 0
	if f==nil then
		f = 0
	end
	n.s = n.s..tostring(f)..n.j
	return n
end

function func(n,f)
	return f(n)
end

function str_end(n)
	n.c=0
	return n
end

function set_i(n,f)
	n.i=f
	return n
end

--TODO: Need to be able to make string's flags work.

function _integer(n,k,u,f)
	local _t = {}
	for l=n.i,n.i+k-1,1 do
		table.insert(_t, n.t[l])
	end

	local number = 0

	if u == 1 then
		number = bytestr_to_int_s(_t)
	else
		number = bytestr_to_int(_t)
	end
	
	n.i=n.i+k
	if (f~=nil) then
		if (((bit.band(f,1) == 1) and ((number == (2^(k*8))-1) or (number == -1))) or
			((bit.band(f,4) == 4) and (number == 0))) then
			if ((bit.band(f,32) == 32) and (bit.band(n.c,1) ~= 1) and (n.s == '')) then
				n.s = n.s..'0'..n.j
				n.c = bit.bor(n.c,1)
			end
			return n
		end
		
		if (bit.band(f,2) == 2) then
			function broken_byte_summation(iteractions)
				if iteractions < 0 then
					return 0
				end
				return (205*(2^(iteractions*8)))+broken_byte_summation(iteractions-1)
			end
			
			if ((number == 0) or (number == broken_byte_summation(k))) then
				return n
			end
		end
		
		if (bit.band(f,16) == 16) then
			number = number * 100
		end
		
		if (bit.band(f,64) == 64) then --move backwards flag
			n.i=n.i-(2*k) --because it was already done once
		end
		
		--TODO: Alternative:
		--[[
			- Add 1 more variable to structure (aka another register)
			- Make any n.i increase depend on this new variable.
			- Create new function to switch directions.
			
			- NOT an alternative to save/load.
		--]]
	end
	
	n.s = n.s..tostring(number)..n.j
	return n
end

function _float(n,k,u,f)
	local _t = {}
	for l=n.i,n.i+k,1 do
		table.insert(_t, n.t[l])
	end
	n.i=n.i+k
	local number = 0
	if f == nil then
		f = 3
	end
	if u == 1 then --double
		number = round(from_double(transform_from_bytes(_t)),f)
	else
		number = round(from_float(transform_from_bytes(_t)),f)
	end
	if number == round(number,0) then
		n.s = n.s..string.format('%.1f',number)..n.j
	else
		n.s = n.s..tostring(number)..n.j
	end
	return n
end

function bool(n,f) return _integer(n,1,0,f) end
function ubyte(n,f) return _integer(n,1,0,f) end
function byte(n,f) return _integer(n,1,1,f) end
function ushort(n,f) return _integer(n,2,0,f) end
function short(n,f) return _integer(n,2,1,f) end
function ulong(n,f) return _integer(n,4,0,f) end
function long(n,f) return _integer(n,4,1,f) end
function uquad(n,f) return _integer(n,8,0,f) end
function quad(n,f) return _integer(n,8,1,f) end

function float(n,f) return _float(n,4,0,f) end
function double(n,f) return _float(n,8,1,f) end

-- from: http://lua-users.org/wiki/CommonFunctions
-- remove trailing and leading whitespace from string.
-- http://en.wikipedia.org/wiki/Trim_(8programming)
function trim(s)
  -- from PiL2 20.4
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

function char(n,f)
	if ((n.t[n.i] ~= 0) and (bit.band(n.c,1) ~= 1)) then --c == 1 = null byte detected.
		n.s = n.s..string.char(n.t[n.i])..n.j
	else
		n.c = bit.bor(n.c,1)
	end
	
	if (f~=nil) then
		if (bit.band(n.c,1) == 1) then
			if n.s == '' then
				if (bit.band(f,1) == 1) then
					n.s = n.s..'0'
				elseif (bit.band(f,8) == 8) then
					n.s = n.s..' '
				end
			elseif ((bit.band(f,2) == 2) and (bit.band(n.c,2) ~= 2)) then
				n.s = trim(n.s)
				n.c = bit.bor(n.c,2)
			end
		end
	end
	
	n.i = n.i + 1
	return n
end

--reiterates function x amount of times
lf.reiterate = function(funct, x)
	local myfuncs = {}
	for i=1,x,1 do
		table.insert(myfuncs, funct)
	end
	return lf.compose(unpack(myfuncs))
end

--opposite ordering of function composition
lf.ocompose = function(...)
    local myfuncs = table.pack(...)
    return function(...)
        local results = table.pack(...)
        local ind = 1
        while myfuncs[ind] do
            results = table.pack(myfuncs[ind](table.unpack(results)))
            ind = ind+1
        end
        return table.unpack(results)
    end
end

--test
--print(lf.ocompose(s_j_comma,lf.reiterate(pad,2),lf.reiterate(lf.apply(ubyte,8),2))(1,{'10','20','30','40','50','60','70','80'},'',''))
--simplification of names of functions needed:

local oo = lf.ocompose
local r = lf.reiterate
local sjc = s_comma
local sjs = s_semicolon
local sjn = s_null
local a = lf.apply --it's worse to insert at the beginning than inserting at end (recreation of tables takes a long time).
local oa2 = function(f,v) return lf.applyn(f, 2, v) end --2 is the index of the flag parameter
--local oa1 = function(f,v) return lf.applyn(f, 1, v) end --a is more efficient in this case
local _s = function (a,b) a = b; return a end
local nop = function(x) return x end
local c = lf.curry

local key_con = function (x,y) return x..y end
local key_con_t = function (x,y) return x..'\t'..y end
local key_con_n = function (x,y) return x..'\n'..y end

--[[
This function is an extension to the original tostring function, and returns
the string representation of many values including tables.
--]]
function mytostring(x)
	local s
	if type(x) == "table" then
		s = "{"
		j = 0
		for i, v in pairs(x) do
			if i ~= "_G" and v ~= x then
				if ((type(rawget (x, i)) == "table") or (type(rawget (x, i)) == "number")) then
					temp = tostring(i) .. "=" .. mytostring(rawget (x, i))
				else
					temp = tostring(i) .. "='" .. mytostring(rawget (x, i)).. "'"
				end
				if j ~= 0 then
					s = s .. "," .. temp
				else
					s = s .. temp
				end
				j = j + 1
			end
		end
		return s .. "}"
	else return tostring(x)
	end
end

--[[
This function uses the new tostring function extension.
--]]
function myprint(a)
	print(mytostring(a))
end

function sub_f(n,f)
	--n.t = oo(transform_to_bytes,(f['encryption'] and a(decrypt,bin_file_encryption_key)))(x)
	n.t['s'],n.i,n.s,n.j,n.c = {},1,'',',',0
	return lf.map(function (y)
		n = y[2](n)
		local temp = string.sub(n.s,1,-string.len(n.j)-1)
		n.s,n.j,n.c='',',',0
		return temp
	end,f,ipairs)
end

function decompile_bin(tsv_bin_structure, tsv_bin_structure_version, tsv_bin_file, decompiled_tsv_bin_file)
	local bin_file_encryption_key = {152,157,159,104,224,102,171,112,233,209,224,224,203,221,209,203,213,207} --converted from hexadecimal to decimal (lua 5.0 doesn't support hexadecimal numbers directly) -- it doesn't change for now
	
	local real_tsv_bin_structure = {}
	for i,v in ipairs(tsv_bin_structure) do
		if table.index_by_first_value(v['versions'],tsv_bin_structure_version) > 0 then
			real_tsv_bin_structure = table.update(real_tsv_bin_structure,v)
		end
	end

	local myfile = io.open(tsv_bin_file, 'rb')
	if myfile == nil then
		print(tsv_bin_file..' doesn\'t exist')
		return {}
	end
	local _data = myfile:read('*all')
	myfile:close()

	local n = {i=1,t={},s='',j=',',c=0}
	
	--returns 1 line of the interpreted struct
	local struct_interpreter = function (x)
		n.t = oo(transform_to_bytes,(real_tsv_bin_structure['encryption'] and a(decrypt,bin_file_encryption_key)))(x)
		n.t['s'],n.i,n.s,n.j,n.c = {},1,'',',',0
		return lf.map(function (y)
			n = y[2](n)
			local temp = string.sub(n.s,1,-string.len(n.j)-1)
			n.s,n.j,n.c='',',',0
			return temp
		end,real_tsv_bin_structure,ipairs)
	end
	
	--returns whole file interpreted
	local _table = lf.map(
		struct_interpreter,
		string.split_fixedsize(
			string.sub(_data,5),
			ulong({i=1,t=transform_to_bytes(string.sub(_data,1,4)),s='',j='',c=0}).s-1),
		ipairs)
	
	--return the names of fields of struct
	table.insert(_table, 1, lf.map(function (t) return t[1] end,real_tsv_bin_structure,ipairs))
	
	local myfile2 = io.open(decompiled_tsv_bin_file, 'wb')
	for i,v in ipairs(_table) do
		if i == 1 then
			myfile2:write('//')
		end
		for j,w in ipairs(v) do
			if j == 1 then
				myfile2:write(w)
			else
				myfile2:write('\t'..w)
			end
		end
		if i ~= table.getn(_table) then
			myfile2:write('\n')
		end
	end
	myfile2:close()
	return _table
end

--Structures
areaset = {
	{
		['versions'] =			{1,2,3,4,5,6,7,8,9},
		['encryption'] = 		false,
		{'ID',					oo(r(pad,4),ulong)},
		{'Name',				oo(sjn,r(oa2(char,8),72),r(pad,28))},
		{'RGB color',			oo(r(ubyte,3),pad)},
		{'Sound effect ID',		long}, --move 2 foward, do 3 backward ubytes, (i at this moment is at -1), so move foward 5.
		{'Color RGB',			oo(r(pad,2),r(oa2(ubyte,64),3),r(pad,5))}, --flag 64 on ubyte = reverse orientation, not implemented separately because there are no others that use this.
		{'Lighting RGB value', 	oo(r(pad,2),r(oa2(ubyte,64),3),r(pad,5))}, --same as: 	oo(r(pad,2),save,r(oa2(ubyte,64),3),oa2(_load,1),r(pad,2))} except without save/load
		{'Lighting angle',		oo(r(float,3))},
		{'Is it city?',			oo(ubyte,r(pad,3))}
	},
	{
		['versions'] = 			{4,5,6,7,8},
		['encryption'] = 		true
	}
}

characterinfo = {
	{
		['versions'] =						{1,2,3,4,5,6,7,8,9},
		['encryption'] = 					false,
		{'ID',								oo(r(pad,4),ulong)},
		{'Name',							oo(sjn,r(char,72),r(pad,49))},
		{'Short Name',						oo(sjn,r(char,16),pad)},
		{'Model Type',						ubyte},
		{'Logic Type',						ubyte},
		{'Framework Number',				ushort},
		{'Suite Serial',					ushort},
		{'Suite Quantity',					ushort},
		{'Part 00',							ushort},
		{'Part 01',							ushort},
		{'Part 02',							ushort},
		{'Part 03',							ushort},
		{'Part 04',							ushort},
		{'Part 05',							ushort},
		{'Part 06',							ushort},
		{'Part 07',							ushort},
		{'FeffID',							oo(r(ushort,2))},
		{'EeffID',							oo(ushort,r(pad,4))},
		{'Special Effect Action Serial',	oo(r(ushort,3))},
		{'Shadow',							ushort},
		{'Action ID',						ushort},
		{'Transparency',					oo(ubyte,pad)},
		{'Moving Sound Effect',				short},
		{'Breathing Sound Effect',			short},
		{'Death Sound Effect',				short},
		{'Can it be controlled?',			ubyte},
		{'Area Limited?',					ubyte},
		{'Altitude Excursion',				short},
		{'Item types that can equip',		oo(r(short,6),r(pad,30))},
		{'Length',							float},
		{'Width',							float},
		{'Height',							float},
		{'Collision Range',					ushort},
		{'Birth',							oo(r(ubyte,2),pad)},
		{'Death',							oo(r(ubyte,2),pad)},
		{'Birth Effect',					ushort},
		{'Death Effect',					ushort},
		{'Hibernate Action',				oo(ubyte,pad)},
		{'Death Instant Action',			oo(ubyte,pad)},
		{'Remaining HP Effect Display',		oo(r(ulong,3))},
		{'Attack can swerve',				ubyte},
		{'Confirm to blow away',			oo(ubyte,r(pad,2))},
		{'Script',							ulong},
		{'Weapon Used',						ulong},
		{'Skill ID',						oo(save, r(oo(long,r(pad,4)),11))}, --check why this is not working --r( ,11)
		{'Skill Rate',						oo(oa2(_load,1), r(oo(r(pad,4),long),11))},
		{'Drop ID',							oo(save, r(oo(long,r(pad,4)),10))},
		{'Drop Rate',						oo(oa2(_load,2), r(oo(r(pad,4),long),10))},
		{'Quantity Limit',					oo(save,r(pad,80),ulong)},
		{'Fatality Rate',					ulong},
		{'Prefix Lvl',						ulong},
		{'Quest Drop ID',					oo(save, oa2(_load,3), r(oo(long,r(pad,4)),10))},
		{'Quest Drop Rate',					oo(oa2(_load,3), r(oo(r(pad,4),long),10))},
		{'AI',								oo(oa2(_load,4),ulong)},
		{'Turn?',							oo(ubyte,r(pad,3))},
		{'Vision',							ulong},
		{'Noise',							ulong},
		{'GetExp',							ulong},
		{'Light',							oo(ubyte,r(pad,3))},
		{'MobExp',							ulong},
		{'Level',							ulong},
		{'Max HP',							long},
		{'Current HP',						long},
		{'Max SP',							long},
		{'Current SP',						long},
		{'Minimum Attack',					long},
		{'Maximum Attack',					long},
		{'Physical Resistance',				long},
		{'Defense',							long},
		{'Hit Rate',						long},
		{'Dodge Rate',						long},
		{'Critical Chance',					long},
		{'Drop Rate Chance',				long},
		{'HP Recovery Per Cycle',			long},
		{'SP Recovery Per Cycle',			long},
		{'Attack Speed',					long},
		{'Attack Distance',					long},
		{'Chase Distance',					long},
		{'Movement Speed',					long},
		{'Col',								long},
		{'Strength',						long},
		{'Agility',							long},
		{'Accuracy',						long},
		{'Constitution',					long},
		{'Spirit',							long},
		{'Luck',							long},
		{'Left_Rad',						ulong},
		{'Guild ID',						oo(sjn,r(char,32),pad)},
		{'Title',							oo(sjn,r(char,32),pad)},
		{'Class',							oo(sjn,r(char,16),r(pad,2))},
		{'Experience',						ulong},
		{'Next Level Experience',			oo(r(pad,4),ulong)},
		{'Reputation',						ulong},
		{'AP',								ushort},
		{'TP',								ushort},
		{'GD',								ulong},
		{'SPRI',							ulong},
		{'Story',							ulong},
		{'Max Sail',						ulong},
		{'Sail',							ulong},
		{'StaSA',							ulong},
		{'SCSM',							ulong},
		{'TStrength',						long},
		{'TAgility',						long},
		{'TAccuracy',						long},
		{'TConstitution',					long},
		{'TSpirit',							long},
		{'TLuck',							long},
		{'TMax HP',							long},
		{'TMax SP',							long},
		{'TAttack',							long},
		{'TDefense',						long},
		{'THit Rate',						long},
		{'TDodge Rate',						long},
		{'TDrop Rate Chance',				long},
		{'TCritical Rate Chance',			long},
		{'THP Recovery',					long},
		{'TSP Recovery',					long},
		{'TAttack Speed',					long},
		{'TAttack Distance',				long},
		{'TMovement Speed',					long},
		{'TSPRI',							ulong},
		{'TSCSM',							oo(ulong, r(pad,4))}
	},
	{
		['versions'] = 						{2},
		
		{'Name',							oo(sjn,r(char,72),r(pad,64))},
		{'Logic Type',						oo(ubyte,pad)},
		{'TSCSM',							ulong},
		{'chasf',							oo(r(float,3),pad)}
	},
	{
		['versions'] = 						{3,4,5,6,7,8,9},
		
		{'Name',							oo(sjn,r(char,72),r(pad,72))},
		{'Short Name',						oo(sjn,r(char,32),r(pad,8))},
		{'Logic Type',						ubyte},
		{'Item types that can equip',		oo(r(short,6),r(pad,28))},
		{'TSCSM',							oo(ulong,r(pad,8))},
		{'chasf',							oo(r(float,3),r(pad,12))}
	},
	{
		['versions'] = 						{4,5,6,7,8},
		['encryption'] = 					true
	},
	{
		['versions'] = 						{5},
		
		{'chasf',							oo(r(float,3),r(pad,4))}
	}
}

characterposeinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Pose',			oo(sjn,r(char,72),r(pad,28))},
		{'Fists',			ushort},
		{'Sword',			ushort},
		{'2H Sword',		ushort},
		{'Dual Swords', 	ushort},
		{'Firegun',			ushort},
		{'Bow',				ushort},
		{'Dagger',			oo(ushort,r(pad,2))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

chaticons = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Small On',		oo(sjn,r(char,72),r(pad,44))},
		{'Small On x',		ulong},
		{'Small On y',		ulong},
		{'Small Off',		oo(sjn,r(char,16))},
		{'Small Off x', 	ulong},
		{'Small Off y',		ulong},
		{'Big',				oo(sjn,r(char,16))},
		{'Big x', 			ulong},
		{'Big y',			ulong},
		{'Very big',		oo(sjn,r(char,16))},
		{'Very big x', 		ulong},
		{'Very big y',		ulong}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

elfskillinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'Kind ID',			oo(save,r(pad,4),ulong)},
		{'Level',			oo(oa2(_load,1),ulong)}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

eventsound = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'Sound ID',		ulong}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

forgeitem = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'Level',			oo(save,r(pad,108),ubyte)},
		{'Failed Level',	oo(save,oa2(_load,1),r(pad,8),sjn,r(char,72))},
		{'Success Rate',	oo(oa2(_load,2),pad,ubyte,r(pad,5))},
		{'Requirement 1',	oo(ushort,ubyte,pad)},
		{'Requirement 2',	oo(ushort,ubyte,pad)},
		{'Requirement 3',	oo(ushort,ubyte,pad)},
		{'Requirement 4',	oo(ushort,ubyte,pad)},
		{'Requirement 5',	oo(ushort,ubyte,pad)},
		{'Requirement 6',	oo(ushort,ubyte,pad)},
		{'Required Gold',	oo(save,oa2(_load,2),r(pad,3),ulong,oa2(_load,3))} --might have a "remnant data" bug
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

hairs = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'Color',			oo(sjn,r(char,8),r(pad,4))},
		{'Required Item 0', r(ulong,2)},
		{'Required Item 1',	r(ulong,2)},
		{'Required Item 2',	r(ulong,2)},
		{'Required Item 3',	r(ulong,2)},
		{'Required Gold',	ulong},
		{'Model',			ulong},
		{'Failed Model',	oo(ulong,r(pad,8))},
		{'For Lance?',		ubyte},
		{'For Carsise?',	ubyte},
		{'For Phyllis?',	ubyte},
		{'For Ami?',		oo(ubyte,r(pad,4))}
	},
	{
		['versions'] = 		{3,4,5,6,7,8,9},
		{'Color',			oo(sjn,r(char,32),r(pad,8))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

iteminfo = {
	{
		['versions'] =							{1,2,3,4,5,6,7,8,9},
		['encryption'] = 						false,
		{'ID',									oo(r(pad,4),ulong)},
		{'Name',								oo(sjn,r(char,76),r(pad,108))},
		{'Icon Name',							oo(sjn,r(char,16),pad)},
		{'Model (Ground)',						oo(sjn,r(char,16),r(pad,3))},
		{'Model (Lance)',						oo(sjn,r(char,16),r(pad,3))},
		{'Model (Carsise)',						oo(sjn,r(char,16),r(pad,3))},
		{'Model (Phyllis)',						oo(sjn,r(char,16),r(pad,3))},
		{'Model (Ami)',							oo(sjn,r(char,16),r(pad,3))},
		{'Ship Symbol',							ushort},
		{'Ship Size',							ushort},
		{'Item Type',							ushort},
		{'Obtain Prefix Rate',					static},
		{'Set ID',								static},
		{'Forging Level',						ubyte},
		{'Stable Level',						ubyte},
		{'Repairable',							ubyte},
		{'Tradable',							ubyte},
		{'Pick-upable',							ubyte},
		{'Droppable',							ubyte},
		{'Deletable',							oo(ubyte,save,r(pad,35))},
		{'Max Stack Size',						ulong},
		{'Instance',							ubyte},
		{'Price',								oo(save,oa2(_load,1),r(pad,3),ulong)},
		{'Character Types',						oo(r(byte,4))},
		{'Character Level',						ushort},
		{'Character Classes',					oo(r(byte,19),r(pad,3))},
		{'Character Nick',						static},
		{'Character Reputation',				static},
		{'Equipable Slots',						oo(oa2(_load,2),r(byte,10))},
		{'Item Switch Locations',				r(byte,10)},
		{'Item Obtain Into Location Determine',	ubyte},
		{'+STR %',								short},
		{'+AGI %',								short},
		{'+ACC %',								short},
		{'+CON %',								short},
		{'+SPR %',								short},
		{'+LUK %',								short},
		{'+Attack Speed %',						short},
		{'+Attack Range %',						short},
		{'+Min Attack %',						short},
		{'+Max Attack %',						short},
		{'+Defense %',							short},
		{'+Max HP %',							short},
		{'+Max SP %',							short},
		{'+Dodge Rate %',						short},
		{'+Hit Rate %',							short},
		{'+Critical Rate %',					short},
		{'+Treasure Drop Rate %',				short},
		{'+HP Recovery %',						short},
		{'+SP Recovery %',						short},
		{'+Movement Speed %',					short},
		{'+Resource Gathering Rate %',			short},
		{'+STR (Min,Max)',						r(short,2)},
		{'+AGI',								r(short,2)},
		{'+ACC',								r(short,2)},
		{'+CON',								r(short,2)},
		{'+SPR',								r(short,2)},
		{'+LUK',								r(short,2)},
		{'+Attack Speed',						r(short,2)},
		{'+Attack Range',						r(short,2)},
		{'+Min Attack',							r(short,2)},
		{'+Max Attack',							r(short,2)},
		{'+Defense',							r(short,2)},
		{'+Max HP',								r(short,2)},
		{'+Max SP',								r(short,2)},
		{'+Dodge',								r(short,2)},
		{'+Hit Rate',							r(short,2)},
		{'+Critical Rate',						r(short,2)},
		{'+Treasure Drop Rate',					r(short,2)},
		{'+HP Recovery',						r(short,2)},
		{'+SP Recovery',						r(short,2)},
		{'+Movement Speed',						r(short,2)},
		{'+Resource Gathering Rate',			r(short,2)},
		{'+Physical Resist',					r(short,2)},
		{'Affected By Left Hand Efs',			short},
		{'+Energy',								oo(save,r(pad,4),r(ushort,2),save)},
		{'+Durability',							oo(oa2(_load,3),r(ushort,2))},
		{'Gem Sockets',							oo(oa2(_load,4),ushort)},
		{'Ship Durability Recovery',			static},
		{'Ship Cannon Amount',					static},
		{'Ship Member Count',					static},
		{'Ship Label',							static},
		{'Ship Cargo Capacity',					static},
		{'Ship Fuel Consumption',				static},
		{'Ship Attack Speed',					static},
		{'Ship Movement Speed',					static},
		{'Usage Script',						oo(sjn,r(char,32),r(pad,2))},
		{'Display Effect',						ushort},
		{'Bind Effect',							oo(save,r(oo(ushort,r(pad,2)),8))},
		{'Bind Effect 2',						oo(oa2(_load,5),r(oo(r(pad,2),ushort),8))},
		{'1st Inv Slot Effect',					r(ushort,2)},
		{'Drop Model Effect',					r(ushort,2)},
		{'Item Usage Effect',					r(ushort,2)},
		{'Description',							oo(sjn,r(oa2(char,1),128),r(pad,12))}
	},
	{
		['versions'] = 							{2,3,4,5,6,7,8,9},
		{'Description',							oo(sjn,r(oa2(char,1),256),r(pad,12))}
	},
	{
		['versions'] = 							{4,5,6,7,8}, --TODO in client source: make it 14 like with other 2.0 files.
		{'Equipable Slots',						oo(oa2(_load,2),r(byte,14))},
		{'Item Switch Locations',				r(byte,14)}
	},
	{
		['versions'] = 							{7},
		{'Item Type',							oo(ushort,r(pad,2))},
		{'Deletable',							oo(ubyte,save,r(pad,33))},
		{'Price',								oo(save,oa2(_load,1),pad,ulong)}
	},
	{
		['versions'] = 							{4,5,6,8},
		['encryption'] = 						true
	}
}

itempre = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

itemrefineeffectinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'Glow ID',		ulong},
		{'Effect ID 1 (Lance)',	oo(ushort,save,r(pad,6))},
		{'Effect ID 1 (Carsise)',	oo(ushort,save,r(pad,6))},
		{'Effect ID 1 (Phyllis)',	oo(ushort,save,r(pad,6))},
		{'Effect ID 1 (Ami)',		oo(ushort,save,r(pad,6))},
		{'Dummy 1',				oo(ubyte,save)},
		{'Effect ID 2 (Lance)',	oo(oa2(_load,1),ushort,save)},
		{'Effect ID 2 (Carsise)',	oo(oa2(_load,2),ushort,save)},
		{'Effect ID 2 (Phyllis)',	oo(oa2(_load,3),ushort,save)},
		{'Effect ID 2 (Ami)',		oo(oa2(_load,4),ushort,save)},
		{'Dummy 2',				oo(oa2(_load,5),ubyte,save)},
		{'Effect ID 3 (Lance)',	oo(oa2(_load,6),ushort,save)},
		{'Effect ID 3 (Carsise)',	oo(oa2(_load,7),ushort,save)},
		{'Effect ID 3 (Phyllis)',	oo(oa2(_load,8),ushort,save)},
		{'Effect ID 3 (Ami)',		oo(oa2(_load,9),ushort,save)},
		{'Dummy 3',				oo(oa2(_load,10),ubyte,save)},
		{'Effect ID 4 (Lance)',	oo(oa2(_load,11),ushort)},
		{'Effect ID 4 (Carsise)',	oo(oa2(_load,12),ushort)},
		{'Effect ID 4 (Phyllis)',	oo(oa2(_load,13),ushort)},
		{'Effect ID 4 (Ami)',		oo(oa2(_load,14),ushort)},
		{'Dummy 4',				oo(oa2(_load,15),ubyte,r(pad,16))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

itemrefineinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'1?',				ushort},
		{'2?',				ushort},
		{'3?',				ushort},
		{'4?',				ushort},
		{'5?',				ushort},
		{'6?',				ushort},
		{'7?',				ushort},
		{'8?',				ushort},
		{'9?',				ushort},
		{'10?',				ushort},
		{'11?',				ushort},
		{'12?',				ushort},
		{'13?',				ushort},
		{'14?',				ushort},
		{'15?',				oa2(float,1)},
		{'16?',				oa2(float,1)},
		{'17?',				oa2(float,1)},
		{'18?',				oo(oa2(float,1),r(pad,12))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

itemtype = unpack{itempre}

magicgroupinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong,r(pad,100))},
		{'Name',			oo(sjn,r(char,32))},
		{'Number of Types',	ulong},
		{'Types',			r(oa2(long,1),8)},
		{'Quantities',		oo(r(long,8),r(pad,4))},
		{'?',				ulong}
	},
	{
		['versions'] = 		{6,7},
		['encryption'] = 	true
	}
}

magicsingleinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,60))},
		{'1?',				ulong},
		{'2?',				oo(sjn,r(char,24),str_end,r(oo(oa2(static,','),r(char,24),str_end),7))}, --oo(sjs,r(oo(sjn,r(char,24)),8)) --oo(oo(sjn,r(char,24)),r(oo(sjc,char,sjn,r(char,23)),7))
		{'3?',				ulong},
		{'4?',				ulong},
		{'5?',				oo(sjn,r(char,24),str_end,r(oo(oa2(static,','),r(char,24),str_end),7))},
		{'6?',				r(long,8)},
		{'7?',				ulong},
		{'8?',				ulong},
		{'9?',				oo(sjn,r(char,24))}
	},
	{
		['versions'] = 		{6,7},
		['encryption'] = 	true
	}
}

mapinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'File',			oo(sjn,r(char,72),r(pad,28))},
		{'Name',			oo(sjn,r(char,16),save,r(pad,23))},
		{'1?',				ubyte},
		{'Coords',			oo(oa2(_load,1),r(ulong,2),r(pad,12))},
		{'2?',				oa2(static,'')},
		{'RGB color',		r(ubyte,3)}
	},
	{
		['versions'] = 		{6},
		{'Name',			oo(sjn,r(char,32),save,r(pad,23))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

musicinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'?',				ulong}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

notifyset = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'?',				oo(sjn,r(char,72),r(pad,28))},
		{'Hint Mode',		ubyte},
		{'Message',			oo(sjn,r(char,64),r(pad,3))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

objevent = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'1?',				oo(sjn,r(char,72),r(pad,50))},
		{'2?',				ushort},
		{'3?',				ushort},
		{'4?',				ushort},
		{'5?',				ushort},
		{'6?',				ushort},
		{'7?',				ushort},
		{'8?',				ushort},
		{'9?',				oo(ubyte,r(pad,3))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

resourceinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'?',				ulong}
	},
	{
		['versions'] = 		{6,7},
		['encryption'] = 	true
	}
}


--[[ Functional translation prototype:

resourceinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'?',				ulong}
	},
	{
		['versions'] = 		{6,7},
		['encryption'] = 	true
	}
}

-- without names or versions or encryption:
resourceinfo = row(oo(r(pad,4),ulong),oo(sjn,r(char,72),r(pad,28)),ulong)

-- with name:
resourceinfo = row(column('ID',oo(r(pad,4),ulong)),column('ID',oo(sjn,r(char,72),r(pad,28))),column('?',ulong))

-- with version and encryption:
resourceinfo = struct(if(version(6,7),decrypt),row(column('ID',oo(r(pad,4),ulong)),column('Name',oo(sjn,r(char,72),r(pad,28))),column('?',ulong)))

struct(if(version(6,7),decrypt))

struct(if(version(6,7),decrypt),r(pad,4),name('ID',ulong),sjn,name('Name',r(char,72)),r(pad,28),name('?',ulong))

--name = capture like in "regular expressions"...

---

dismantling if:

ifmonad(isnumber(4),continueiftrue,print('test'))

or

monad(isnumber(true,4),print('test')) or monad(true,isnumber(4),print('test'))

so that seems equivalent as giving each function a parameter that indicates the direction it's running:

+(true,1,1) = 2, while +(false,1,1) = 0 (because it's doing what - does, which is the opposite).

or it could be done by pointers, so that:
(+,1,2) = +(1,2)

monad(_(isnumber,4),_(print,'test'))

--- monad if is for later

what's needed:

struct,row,column,if functions.
--]]



sceneffectinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'File',			oo(sjn,r(char,72),r(pad,28))},
		{'Name',			oo(sjn,r(char,16))},
		{'Image Name',		oo(sjn,r(char,16),r(pad,4))},
		{'Flag',			ulong},
		{'ObjTypeID',		oo(ulong,r(pad,4))},
		{'Dummy',			oo(r(oa2(long,33),8))},
		{'Dummy 2',			long},
		{'1?',				ulong},
		{'2?',				float},
		{'3?',				float},
		{'4?',				float}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

--SceneObj Type:
--from enum at Scene.h in client game source (definition of meanings at SceneObjSet.h):
--[[
enum
{
    SCENEOBJ_TYPE_NORMAL = 0 ,
    SCENEOBJ_TYPE_POSE       ,
    SCENEOBJ_TYPE_TERRAIN    ,
    SCENEOBJ_TYPE_POINTLIGHT ,
    SCENEOBJ_TYPE_ENVLIGHT   ,
    SCENEOBJ_TYPE_FOG        ,
    SCENEOBJ_TYPE_ENVSOUND	 ,
	MAX_SCENEOBJ_TYPE		 ,		// ???????
};--]]

--[[ Varied Data #1
- SCENEOBJ_TYPE_POINTLIGHT = 3: Point Color (3 bytes)
- SCENEOBJ_TYPE_ENVLIGHT = 4: Environment Color (3 bytes)
- SCENEOBJ_TYPE_ENVSOUND = 6: Environment Sound Name (12 length string)
- SCENEOBJ_TYPE_NORMAL = 0: List: 
-- Amount of pairs that follows (int).
-- {Object to fade sequence (int),
-- Fading coeficient (float)}
--]]
function specialdata(n) --to convert from datatypes?
	local n_a = table.copy(n) --FIXED 1.0.7: n.s was grabbing n_a.s string (because only table reference was copied - 1 of many lua gotchas).
	n_a.i = n_a.i - 4
	n_a = ulong(n_a)
	
	if n_a.s == '0,' then
		return oo(oa2(set_i,n.i+76),r(oa2(ulong,36),12),oa2(set_i,n.i))(n)
	elseif n_a.s == '3,' or n_a.s == '4,' then
		return oo(r(ubyte,3),oa2(set_i,n.i))(n) --i = 128 to 136 (3 numbers), set i back to 128
	elseif n_a.s == '6,' then
		return oo(oa2(set_i,n.i+48),sjn,r(char,12),oa2(set_i,n.i))(n)
	end
	n.s = '0'..n.j
	return n
end

--[[ Varied Data #2
- SCENEOBJ_TYPE_POINTLIGHT = 3: List:
-- Range (int)
-- Attenuation (float)
-- Animation Controller ID (int)
- SCENEOBJ_TYPE_ENVSOUND = 6: List of distances (max 1)
--]]
function specialdata2(n) --to convert from datatypes?
	local n_a = table.copy(n) --FIXED 1.0.7: n.s was grabbing n_a.s string (because only table reference was copied - 1 of many lua gotchas).
	n_a.i = n_a.i - 4
	n_a = ulong(n_a)
	
	if n_a.s == '3,' then
		return oo(oa2(set_i,n.i+12),ulong,float,ulong,oa2(set_i,n.i))(n)
	elseif n_a.s == '6,' then
		return oo(oa2(set_i,n.i+60),ulong,oa2(set_i,n.i))(n)
	end
	n.s = '0'..n.j
	return n
end

sceneobjinfo = { --removed unused fields since script conversion
	{
		['versions'] =					{1,2,3,4,5,6,7,8,9},
		['encryption'] = 				false,
		{'ID',							oo(r(pad,4),ulong)},
		{'File',						oo(sjn,r(char,72),r(pad,28))},
		{'Name',						oo(sjn,r(char,16))},
		{'Type',						ulong},
		{'Varied Data #1',				specialdata},
		{'Varied Data #2',				oo(specialdata2,r(pad,24))},
		{'Attach Effect ID',			oo(save,r(pad,4),ulong)},
		{'Enable Environment Light',	oo(r(pad,4),ulong,save)},
		{'Enable Point Light',			oo(save,oa2(_load,2),ulong)},
		{'Style',						oo(oa2(_load,1),ulong)},
		{'Flag',						oo(oa2(_load,3),ulong)},
		{'Size Flag',					oo(ulong,r(pad,20))},
		{'Shade Flag',					oo(ulong,r(pad,72))}
		--{'Is Really Big',				unknown - must have been used in an earlier pko build - must be byte},
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

--TODO: continue starting at selectcha

selectcha = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,32))},
		{'1?',				oo(ulong,save,r(pad,256))},
		{'2?',				oo(ulong,save)},
		{'3?',				oo(oa2(_load,1),r(ulong,56))},
		{'4?',				oo(oa2(_load,2),r(pad,252),ulong,r(pad,252))},
		{'5?',				oo(ulong,r(pad,252))},
		{'6?',				oo(ulong,r(pad,1300))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

serverset = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'Region',			oo(save,r(pad,80),sjn,r(oa2(char,1),16),r(pad,4))},
		{'IP #1',			oo(oa2(_load,1),sjn,r(oa2(char,1),16))},
		{'IP #2',			oo(sjn,r(oa2(char,1),16))},
		{'IP #3',			oo(sjn,r(oa2(char,1),16))},
		{'IP #4',			oo(sjn,r(oa2(char,1),16))},
		{'IP #5',			oo(sjn,r(oa2(char,1),16))}
	},
	{
		['versions'] = 		{3,4,5,6,7,8,9},
		{'Region',			oo(save,r(pad,80),sjn,r(oa2(char,1),16),save)},
		{'Description',		oo(oa2(_load,2),pad,sjn,r(char,256),r(pad,4))},
		{'?',				oo(sjn,r(char,16),r(pad,3))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

shadeinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'1?',				oo(sjn,r(char,72),r(pad,28))},
		{'2?',				oo(sjn,r(char,16),r(pad,4))},
		{'3?',				float},
		{'4?',				ulong},
		{'5?',				ulong},
		{'6?',				ulong},
		{'7?',				ulong},
		{'8?',				ulong},
		{'9?',				ulong},
		{'10?',				ulong},
		{'11?',				ulong},
		{'12?',				ulong},
		{'13?',				ulong}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

function ship15id(i,t,s,j,c)
	i,t,s,j,c = oo(r(pad(i,t,s,j,c),4),ulong)
	s = tostring(tonumber(s)+1)
	return i,t,s,j,c
end

shipinfo = {
	{
		['versions'] =				{1,2,3,4,5,6,7,8,9},
		['encryption'] = 			false,
		{'ID',						ship15id},
		{'Name',					oo(sjn,r(char,72),r(pad,92))},
		{'Item ID',					oo(save,r(pad,128),ushort)},
		{'Char ID',					ushort},
		{'Action ID',				oo(ushort,r(pad,10))},
		{'Hull',					oo(save,r(pad,128),ushort,save)},
		{'Engine',					oo(oa2(_load,2),r(pad,32),r(oa2(short,4),16))},
		{'Mobility',				oo(oa2(_load,2),r(oa2(short,4),16))},
		{'Cannon',					oo(r(pad,32),r(oa2(short,4),16))},
		{'Component',				oo(r(oa2(short,4),12))},
		{'Level Restriction',		oo(oa2(_load,3),ushort,r(pad,2))},
		{'Class Restriction',		oo(short,r(pad,30))},
		{'Durability',				ushort},
		{'Durability Recovery',		ushort},
		{'Defense',					ushort},
		{'Physical Resist',			ushort},
		{'Min Attack',				ushort},
		{'Max Attack',				ushort},
		{'Attack Range',			ushort},
		{'Reloading Time',			ushort},
		{'Cannon Area of Effect',	ushort},
		{'Cargo Capacity',			ushort},
		{'Fuel',					ushort},
		{'Fuel Consumption',		ushort},
		{'Attack Speed',			ushort},
		{'Movement Speed',			oo(ushort,save)},
		{'Description',				oo(oa2(_load,1),sjn,r(char,128))},
		{'Remark',					oo(oa2(_load,4),ushort)}
	},
	{
		['versions'] = 				{2,3,4,5,6,7,8,9},
		{'ID',						oo(r(pad,4),ulong)}
	},
	{
		['versions'] = 				{3,4,5,6,7,8,9},
		{'Hull',					oo(save,r(pad,152),ushort,save)},
		{'Engine',					oo(oa2(_load,2),r(pad,38),r(oa2(short,4),19))},
		{'Mobility',				oo(oa2(_load,2),r(oa2(short,4),19))},
		{'Cannon',					oo(r(pad,38),r(oa2(short,4),19))},
		{'Component',				r(oa2(short,4),15)},
		{'Class Restriction',		oo(short,r(pad,36))},
		{'Remark',					oo(oa2(_load,4),ushort,r(pad,2))}
	},
	{
		['versions'] = 				{4,5,6,7,8},
		['encryption'] = 			true
	}
}

shipiteminfo = {
	{
		['versions'] =				{1,2,3,4,5,6,7,8,9},
		['encryption'] = 			false,
		{'ID',						ship15id},
		{'Name',					oo(sjn,r(char,72),r(pad,92))},
		{'Model',					oo(save,r(pad,128),ulong)},
		{'Propeller 1',				ushort},
		{'Propeller 2',				ushort},
		{'Propeller 3',				ushort},
		{'Propeller 4',				ushort},
		{'Price',					ulong},
		{'Durability',				ushort},
		{'Durability Recovery',		ushort},
		{'Defense',					ushort},
		{'Physical Resist',			ushort},
		{'Min Attack',				ushort},
		{'Max Atatck',				ushort},
		{'Attack Range',			ushort},
		{'Reloading Time',			ushort},
		{'Cannon Area of Effect',	ushort},
		{'Cargo Capacity',			ushort},
		{'Fuel',					ushort},
		{'Fuel Consumption',		ushort},
		{'Attack Speed',			ushort},
		{'Movement Speed',			oo(ushort,save)},
		{'Description',				oo(oa2(_load,1),sjn,r(char,128))},
		{'Remark',					oo(oa2(_load,2),ushort,r(pad,2))}
	},
	{
		['versions'] = 				{2,3,4,5,6,7,8,9},
		{'ID',						oo(r(pad,4),ulong)},
	},
	{
		['versions'] = 				{4,5,6,7,8},
		['encryption'] = 			true
	}
}

skilleff = {
	{
		['versions'] =							{1,2,3,4,5,6,7,8,9},
		['encryption'] = 						false,
		{'ID',									oo(r(pad,4),ulong)},
		{'Name',								oo(sjn,r(char,72),r(pad,46))},
		{'Activation Interval',					short},
		{'Transfer Persists Duration',			oo(sjn,r(oa2(char,1),32))},
		{'Use Effect Script',					oo(sjn,r(oa2(char,1),32))},
		{'Remove Effect Script',				oo(sjn,r(oa2(char,1),32))},
		{'Can it be Manually Cancelled?',		ubyte},
		{'Can Move?',							ubyte},
		{'Can use Skill?',						ubyte},
		{'Can use Normal Attack?',				ubyte},
		{'Can Trade?',							ubyte},
		{'Can use Items?',						ubyte},
		{'Can Attack?',							ubyte},
		{'Can be Attacked?',					ubyte},
		{'Can be Item Target?',					ubyte},
		{'Can be Skill Target?',				ubyte},
		{'Can be Invisible?',					ubyte},
		{'Can be Seen as Yourself?',			ubyte},
		{'Can use Inventory?',					ubyte},
		{'Can Talk to NPC?',					ubyte},
		{'Remove Effect ID',					ubyte},
		{'Screen Effect',						ubyte},
		{'Client Performance',					oo(ubyte,r(pad,3))},
		{'Client Display ID',					short},
		{'Ground Status Effect',				short},
		{'Center Display',						ubyte},
		{'Knock out Display',					ubyte},
		{'Special Effect of Recipe',			ushort},
		{'Dummy 1',								short},
		{'Display Effect When Attacked With?',	ubyte},
		{'Dummy 2',								oo(ubyte,r(pad,8))}
	},
	{
		['versions'] = 							{4,5,6,7,8},
		['encryption'] = 						true
	},
	{
		['versions'] = 							{6,7},
		{'Name',								oo(sjn,r(oa2(char,1),72),r(pad,48))},
		{'Dummy 2',								oo(ubyte,r(pad,10))}
	},
	{
		['versions'] = 							{8},
		{'Name',								oo(sjn,r(oa2(char,1),72),r(pad,46))},
		{'Dummy 2',								oo(ubyte,r(pad,12))}
	}
}

skillinfo = {
	{
		['versions'] =							{1,2,3,4,5,6,7,8,9},
		['encryption'] = 						false,
		{'ID',									oo(r(pad,4),ulong)},
		{'Name',								oo(sjn,r(char,72),r(pad,47))},
		{'Not Life Skill?',						byte},
		{'Class Requirement',					oo(sjn,byte,oa2(static,','),byte,r(oo(oa2(static,';'),byte,oa2(static,','),byte),3),r(pad,10))},
		{'Left Hand Requirement',				oo(sjn,short,oa2(static,','),short,r(oo(oa2(static,';'),short,oa2(static,','),short),7))},
		{'Right Hand Requirement',				oo(sjn,short,oa2(static,','),short,r(oo(oa2(static,';'),short,oa2(static,','),short),7))},
		{'Armor Requirement',					oo(sjn,short,oa2(static,','),short,r(oo(oa2(static,';'),short,oa2(static,','),short),7))},
		{'Conch Usage',							oo(r(short,3),r(pad,42))},
		{'Skill Phase',							byte},
		{'Skill Type',							oo(byte,save,r(pad,22))},
		{'Useful/Harmful',						oo(byte,save)},
		{'Learning Level',						oo(oa2(_load,1),short)},
		{'Prerequisite',						oo(sjn,short,oa2(static,','),short,r(oo(oa2(static,';'),short,oa2(static,','),short),2))},
		{'Skill Points Consumption',			byte},
		{'Discharged Status',					byte},
		{'Apply Point',							oo(byte,pad)},
		{'Cast Range',							ushort},
		{'Process Target',						byte},
		{'Attack Mode',							byte},
		{'Angle',								oo(oa2(_load,2),pad,short)},
		{'Radius',								short},
		{'Region Shape',						byte},
		{'Prophase Management Formula',			oo(sjn,r(char,32),save,r(pad,133))},
		{'Add Surface Formula',					oo(sjn,r(char,32),save,pad)},
		{'SP Formula',							oo(oa2(_load,3),pad,sjn,r(char,32),pad)},
		{'Durability Consumption Formula',		oo(sjn,r(char,32),pad)},
		{'Top-up Consumption Formula',			oo(sjn,r(char,32),pad)},
		{'Region Formula',						oo(sjn,r(char,32),pad)},
		{'Discharge Phase Formula',				oo(oa2(_load,4),pad,sjn,r(char,32),pad)},
		{'Effect Phase Formula',				oo(sjn,r(char,32),pad)},
		{'Positive Effect Formula',				oo(sjn,r(char,32),pad)},
		{'Opposing Effect Formula',				oo(sjn,r(char,32),r(pad,2))},
		{'Bind Status ID Can Removed Manually',	oo(ulong,r(pad,22))},
		{'Self Parameter',						static},
		{'Self Effect',							static},
		{'Consumable',							static},
		{'Duration',							static},
		{'Target Parameter',					static},
		{'Splash Parameter',					short},
		{'Duration on Target',					short},
		{'Splash Persists Effect',				short},
		{'Morph ID',							short},
		{'Summon ID',							oo(short,r(pad,2))},
		{'Discharge Duration',					static},
		{'Repeated Discharge Duration Formula',	oo(sjn,r(char,32),pad)},
		{'Damage Effect',						oo(save,r(pad,3),short)},
		{'Play Effect',							oo(byte,pad)},
		{'Action',								r(short,10)},
		{'Keyframe',							short},
		{'Attack Sound Effect',					short},
		{'Character Dummy',						oo(r(short,2),r(pad,2))},
		{'Character Effect',					oo(r(ushort,2),r(pad,2))},
		{'Base Standard Value',					oo(r(ushort,2),r(pad,2))},
		{'Item Dummy',							short},
		{'Item Effect',							r(short,2)},
		{'Item Effect 2',						r(short,2)},
		{'Path of Flight Keyframe',				short},
		{'Character Dummy 2',					short},
		{'Item Dummy 2',						short},
		{'Path of Flight Effect',				short},
		{'Path of Flight Speed',				short},
		{'Attacked Sound Effect',				short},
		{'Dummy',								short},
		{'Character Attacked Effect',			short},
		{'Effect Duration Point',				oo(byte,pad)},
		{'Surface Attacked Effect',				short},
		{'Water Surface Effect',				short},
		{'Icon',								oo(sjn,r(char,16),pad)},
		{'Play Count',							oo(byte,save)},
		{'Command',								oo(oa2(_load,5),r(byte,2))},
		{'Description',							oo(oa2(_load,6),sjn,r(char,128))},
		{'Effect',								oo(sjn,r(char,128))},
		{'Consumption',							oo(sjn,r(char,128),r(pad,46))}
	},
	{
		['versions'] = 							{4,5,6,7,8},
		['encryption'] = 						true
	},
	{
		['versions'] = 							{6},
		{'Consumption',							oo(sjn,r(char,128),r(pad,50))}
	}
}

stoneinfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'File',			oo(sjn,r(char,72),r(pad,28))},
		{'Item ID',			ulong},
		{'Equip Slots',		r(ulong,3)},
		{'Type',			ulong},
		{'Script',			oo(sjn,r(char,64))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	},
	{
		['versions'] = 		{9},
		{'Equip Slots',		r(ulong,15)}, --whatever is the amount of equipment slots available
	}
}

terraininfo = {
	{
		['versions'] =		{1,2,3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'File',			oo(sjn,r(char,72),r(pad,28))},
		{'1?',				oo(byte,r(pad,7))},
		{'2?',				oo(byte,r(pad,3))}
	},
	{
		['versions'] = 		{6,7},
		['encryption'] = 	true
	}
}

helpinfoset = {
	{
		['versions'] =		{3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong)},
		{'Name',			oo(sjn,r(char,72),r(pad,28))},
		{'Description',		oo(sjn,r(r(char,1024),2))} --stack overflow if using 2048
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

monsterlist = {
	{
		['versions'] =		{3,4,5,6,7,8,9},
		['encryption'] = 	false,
		{'ID',				oo(r(pad,4),ulong,r(pad,100))},
		{'Name',			oo(sjn,r(char,128))},
		{'Level',			oo(sjn,r(char,128))},
		{'Coordinates',		r(ulong,2)},
		{'Map',				oo(sjn,r(char,128))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

npclist = unpack{monsterlist}

monsterinfo = {
	{
		['versions'] =			{4,5,6,7,8,9},
		['encryption'] = 		false,
		{'ID',					oo(r(pad,4),ulong)},
		{'Name',				oo(sjn,r(char,72),r(pad,60))},
		{'Start Coordinates',	r(oa2(ulong,16),2)},
		{'End Coordinates',		r(oa2(ulong,16),2)},
		{'Char IDs',			oo(r(ulong,2),r(pad,24))},
		{'Map',					oo(sjn,r(char,16),r(pad,16))}
	},
	{
		['versions'] = 		{4,5,6,7,8},
		['encryption'] = 	true
	}
}

--[[
Lit files:

-- item.lit:
  Specifies the base for blending weapon glow animation - used by "Glow ID" in itemrefineeffectinfo.

- struct lwItemLitFileHead
{
    DWORD version;
    DWORD type;
    DWORD mask[4];
}; = 24bytes (SKIP)

- DWORD item_num; = 4 bytes (use as beginning of file - like with regular bins)

- struct lwItemLitInfo (repeat as many times as item_num specifies)
{
    typedef vector<lwLitInfo*> _LitBuf;
    typedef _LitBuf::iterator _LitBuf_It;

    DWORD id; = 4 bytes
    char descriptor[DR_MAX_NAME]; = 64 bytes
    char file[DR_MAX_FILE]; = 128 bytes
    _LitBuf lit; = check lwLitInfo after lit_num
};

- DWORD lit_num; = 4 bytes

- struct lwLitInfo (repeat as many times as lit_num specifies)
{
    DWORD id; = 4 bytes
    char file[DR_MAX_FILE]; = 128 bytes
    DWORD anim_type; = 4 bytes
    DWORD transp_type; = 4 bytes
    float opacity; = 4 bytes
};

Example of itemlit tsv (based on hexed item.lit - 5 items):
#Header (skipped):
##Version: 1
##Type: 1
##Mask: {0,0,0,0}
#Amount of following items: 5
#ID	Descriptor	File	LitInfo (Amount of follows){ID,Files,Animation Type, Transparency Type, Opacity}
0	BF D5 (empty)	01010005.lgo	(0 items)
1	BA EC (empty)	04090030.lgo	(4 items)0,red.tga|a.tga|bon2.tga,5,1,1.0;1,red.tga|ne_Ribbon2.tga,5,1,1.0;2,red.tga|ne_Ribbon2.tga,5,1,1.0;3,red.tga|ne_Ribbon2.tga,5,1,1.0
2	C0 B6 (empty)	01010006.lgo	(4 items)0,blue.tga|e_Ribbon3.tga,5,1,1.0;1,blue.tga|e_Ribbon3.tga,5,1,1.0;2,blue.tga|e_Ribbon3.tga,5,1,1.0;3,blue.tga|e_Ribbon3.tga,5,1,1.0
3	BB C6 (empty)	01010006.lgo	(4 items)0,yellow.TGA|Ribbon4.tga,5,1,1.0;1,yellow.TGA|Ribbon4.tga,5,1,1.0;2,yellow.TGA|Ribbon4.tga,5,1,1.0;3,yellow.TGA|Ribbon4.tga,5,1,1.0
4	C2 CC (empty)	01010005.lgo	(4 items)0,green.tga|tga,5,1,1.0;1,green.tga|tga,5,1,1.0;2,green.tga|tga,5,1,1.0;3,green.tga|tga,5,1,1.0

Example of itemlit tsv (based on hexed item.lit - 5 items):
#(24 bytes of header SKIPPED) (Amount of following items)
#ID	Descriptor	File	(Amount of follows){ID,Files,Animation Type, Transparency Type, Opacity}
0	BF D5 (empty)	01010005.lgo	(0 items)
1	BA EC (empty)	04090030.lgo	(4 items)0,red.tga,5,1,1.0;1,red.tga,5,1,1.0;2,red.tga,5,1,1.0;3,red.tga,5,1,1.0
2	C0 B6 (empty)	01010006.lgo	(4 items)0,blue.tga,5,1,1.0;1,blue.tga,5,1,1.0;2,blue.tga,5,1,1.0;3,blue.tga,5,1,1.0
3	BB C6 (empty)	01010006.lgo	(4 items)0,yellow.TGA,5,1,1.0;1,yellow.TGA,5,1,1.0;2,yellow.TGA,5,1,1.0;3,yellow.TGA,5,1,1.0
4	C2 CC (empty)	01010005.lgo	(4 items)0,green.tga,5,1,1.0;1,green.tga,5,1,1.0;2,green.tga,5,1,1.0;3,green.tga,5,1,1.0

#ID	Descriptor	File	{ID,Files,Animation Type, Transparency Type, Opacity}
0		01010005.lgo	
1		04090030.lgo	red.tga,5,1,1.0;red.tga,5,1,1.0;red.tga,5,1,1.0;red.tga,5,1,1.0
2		01010006.lgo	blue.tga,5,1,1.0;blue.tga,5,1,1.0;blue.tga,5,1,1.0;blue.tga,5,1,1.0
3		01010006.lgo	yellow.TGA,5,1,1.0;yellow.TGA,5,1,1.0;yellow.TGA,5,1,1.0;yellow.TGA,5,1,1.0
4		01010005.lgo	green.tga,5,1,1.0;green.tga,5,1,1.0;green.tga,5,1,1.0;green.tga,5,1,1.0

There can only be 4 max lwLitInfo's because there are only 4 groups of textures in itemrefineeffectinfo (which
  determine the sequence of animations).

-- It's actually a binary file that needs decompiling, except there's no way to compile it unless by script!

--TODO: to modify to have variable sized bin files.

---

-- lit.tx:

Provides blending animation to many in-game events - most notably the thunderstorm at sea.

class LitMgr
{
private:
    LitInfo* _lit_seq;
    DWORD _lit_num;
};

struct LitInfo
{
    DWORD obj_type;
    DWORD anim_type;
    DWORD sub_id;
    DWORD color_op;
    char file[128];
    char mask[128];
    char str_buf[8][128];
    DWORD str_num;
};

- Read structure - It's a space separated valued file:

(64 max byte Name) (Number of items)
(64 max byte Name) (Object Type) [See following]

If (Object Type) == 0 -- character
	(Animation Type) (Animation File) (Mask Texture Lit File) (Amount of Lit Texture Files) [(Lit Texture File) ...]

If (Object Type) == 1 or 2 -- scene or item
	(Animation Type) (Animation File) (Image Attachment) (Color Operation) (Amount of Lit Texture Files) [(Lit Texture File) ...]

Lit_readme.txt mentions first what (Color Operation) can be, then after, what the (Animation Type) can be. Copying it here:

    // Control
    D3DTOP_DISABLE              = 1,      // disables stage
    D3DTOP_SELECTARG1           = 2,      // the default
    D3DTOP_SELECTARG2           = 3,      // the default

    // Modulate
    D3DTOP_MODULATE             = 4,      // multiply args together
    D3DTOP_MODULATE2X           = 5,      // multiply and  1 bit
    D3DTOP_MODULATE4X           = 6,      // multiply and  2 bits

    // Add
    D3DTOP_ADD                  =  7,   // add arguments together
    D3DTOP_ADDSIGNED            =  8,   // add with -0.5 bias
    D3DTOP_ADDSIGNED2X          =  9,   // as above but left  1 bit
    D3DTOP_SUBTRACT             = 10,   // Arg1 - Arg2, with no saturation
    D3DTOP_ADDSMOOTH            = 11,   // add 2 args, subtract product
                                        // Arg1 + Arg2 - Arg1*Arg2
                                        // = Arg1 + (1-Arg1)*Arg2


There are currently 5 type of image attachment.
0: 360 Frame,  action [0.0 - 1.0]
1: 120 Frame,  Rotating animation, UV[0 - 360]
2: 360 Frame,  Offset motion + rotating motion
3: 720 Frame,  Rotating animation
4: 120 Frame,  Movement screen

Example of a structuraly valid lit.tx:

test 4
mytestitem1 0 0 anim.lgo mask.tga 1 tex1.tga
mytestitem2 1 4 anim2.lgo 0 9 3 tex2.tga tex3.tga tex4.tga
mytestitem3 2 4 anim3.lgo 0 9 0
mytestitem4 3

NOTES:
- 1 lit texture is loaded with (Object Type) 2 = item.
- All items with (Object Type) 1 = scene are ignored (its part is commented in source code).
--]]

top_binary_structures = {
	['areaset'] = areaset,
	['characterinfo'] = characterinfo,
	['characterposeinfo'] = characterposeinfo,
	['chaticons'] = chaticons,
	['elfskillinfo'] = elfskillinfo,
	['eventsound'] = eventsound,
	['forgeitem'] = forgeitem,
	['hairs'] = hairs,
	['iteminfo'] = iteminfo,
	['itempre'] = itempre,
	['itemrefineeffectinfo'] = itemrefineeffectinfo,
	['itemrefineinfo'] = itemrefineinfo,
	['itemtype'] = itemtype,
	['magicgroupinfo'] = magicgroupinfo,
	['magicsingleinfo'] = magicsingleinfo,
	['mapinfo'] = mapinfo,
	['monsterinfo'] = monsterinfo,
	['musicinfo'] = musicinfo,
	['notifyset'] = notifyset,
	['objevent'] = objevent,
	['resourceinfo'] = resourceinfo,
	['sceneffectinfo'] = sceneffectinfo,
	['sceneobjinfo'] = sceneobjinfo,
	['serverset'] = serverset,
	['selectcha'] = selectcha,
	['shadeinfo'] = shadeinfo,
	['shipinfo'] = shipinfo,
	['shipiteminfo'] = shipiteminfo,
	['skilleff'] = skilleff,
	['skillinfo'] = skillinfo,
	['stoneinfo'] = stoneinfo,
	['terraininfo'] = terraininfo,
	['helpinfoset'] = helpinfoset,
	['monsterlist'] = monsterlist,
	['npclist'] = npclist
}

function decompile_folder_bin(folder, version)
	print('Decompiling folder: '..folder)
	local start_decompile_time = os.clock()
	for i,v in pairs(top_binary_structures) do
		local start_time = os.clock()
		print(i..'.bin ->')
		decompile_bin(v, version, folder..'/'..i..'.bin', folder..'/'..i..'.txt')
		print('->'..i..'.txt ('..(os.clock() - start_time)..' s)')
	end
	print('Folder decompiled successfully in '..(os.clock() - start_decompile_time)..'s: '..folder)
end

function decrypt_folder_bin(folder)
	print('Decrypting folder: '..folder)
	local start_decrypt_time = os.clock()
	for i,v in pairs(top_binary_structures) do
		local start_time = os.clock()
		print(i..'.bin -> ')
		decrypt_bin(folder..'/'..i..'.bin', folder..'/'..i..'_un.bin')
		print('-> '..i..'_un.bin ('..(os.clock() - start_time)..' s)')
	end
	print('Folder decrypted successfully in '..(os.clock() - start_decrypt_time)..'s: '..folder)
end

function compile_gamefolder_bin(game_folder)
	print('Game folder to compile: '..game_folder)
	print('System command start. Running game in compile mode.')
	os.execute('cd "'..game_folder..'" & "'..game_folder..'/system/game.exe" startgame table_bin')
	print('System command end. If messages between start and this appear, assume failure.')
end

--from: http://snippets.luacode.org/snippets/Table_Slice_116
function table_slice(values,i1,i2)
    local res = {}
    local n = #values
    -- default values for range
    i1 = i1 or 1
    i2 = i2 or n
    if i2 < 0 then
        i2 = n + i2 + 1
    elseif i2 > n then
        i2 = n
    end
    if i1 < 1 or i1 > n then
        return {}
    end
    local k = 1
    for i = i1,i2 do
        res[k] = values[i]
        k = k + 1
    end
    return res
end

--from: http://stackoverflow.com/questions/8722620/comparing-two-index-tables-by-index-value-in-lua
function is_table_equal(t1, t2)
  if #t1 ~= #t2 then return false end
  for i=1,#t1 do
    if t1[i] ~= t2[i] then return false end
  end
  return true
end

function decrypt_texture(texture_file)
	local block_size = 44
	local file = io.open(texture_file, 'r+b')
	local start_data = file:read(block_size)
	file:seek("end", -block_size-4) --ignore the "texture_encryption_key"
	local end_data = file:read(block_size)
	local texture_encryption_key_data = file:read(4)

	local texture_encryption_key = {109,112,46,120} --match 'mp.x' ending string
	if is_table_equal(transform_to_bytes(string.sub(texture_encryption_key_data,1,4)), texture_encryption_key) then
		local start_bytes_data = transform_to_bytes(string.sub(start_data,1,block_size))
		local end_bytes_data = transform_to_bytes(string.sub(end_data,1,block_size))

		local temp_bytes_data = {}
		for k,v in ipairs(table_slice(start_bytes_data,1,block_size)) do temp_bytes_data[k] = v end
		for k,v in ipairs(table_slice(end_bytes_data,1,block_size)) do start_bytes_data[k] = v end
		for k,v in ipairs(temp_bytes_data) do end_bytes_data[k] = v end
		
		file:seek("set")
		file:write(transform_from_bytes(start_bytes_data))
	    file:seek("end", -block_size-4) --ignore the "texture_encryption_key"
		file:write(transform_from_bytes(end_bytes_data))
		file:write("\0")
		file:close()
		return true
	end
	file:close()
	return false
end

function scandir_f(folder, func, linux)
    local i, t = 0, {}
	local pdir, pfile --ÌìÊ¹1.tga and ÌìÊ¹2.tga
	if linux then
		pdir = io.popen('ls -A "'..folder..'"')
	else
		pdir = io.popen('dir "'.. folder .. '" /b /ad')
	end
	for _directory in pdir:lines() do
		t[i] = scandir_f(folder .. "/" .. _directory, func, linux)
		i = i + 1
	end
    pdir:close()
	
	if linux then
		pfile = io.popen('ls "'..folder..'"')
	else
		pfile = io.popen('dir "'.. folder .. '" /b /a-d')
	end
    for filename in pfile:lines() do
        t[i] = filename
		func(folder .. "/" .. filename)
		i = i + 1
    end
    pfile:close()
    return t
end

function decrypt_folder_texture(folder, linux)
	linux = linux or false
	print('Decrypting folder (inc subfolders): '..folder)
	local start_decrypt_time = os.clock()
	
	function timed_texture_decryption(file_name)
		local start_time = os.clock()
		local file_name_length = string.len(file_name)
		local file_name_extension = string.lower(string.sub(file_name,file_name_length-3,file_name_length))
		if (file_name_extension == '.tga') or (file_name_extension == '.bmp') or (file_name_extension == '.dds') then
			print(file_name)
			decrypt_texture(file_name)
			print('('..(os.clock() - start_time)..' s)')
		end
	end
	scandir_f(folder, timed_texture_decryption, linux)
	print('Folder decrypted successfully in '..(os.clock() - start_decrypt_time)..'s: '..folder)
end