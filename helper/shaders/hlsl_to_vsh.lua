--[[
o-----------------------------------------------------------------------------o
| hlsl_to_vsh                                                                 |
(-----------------------------------------------------------------------------)
| By deguix                / An Utility for top-recode | Compatible w/ LuaJIT |
|                         ----------------------------------------------------|
|   Converts hlsl files to vsh files - compiles hlsl into vsh DX9/DX8 shaders.|
o-----------------------------------------------------------------------------o
--]]

function hlsl_to_vsh_dx8(shader_file)
	local file = io.open(shader_file, 'r+')
	local data = file:read("*a")
	data = data:gsub("c(%d-)%[", "c%[%1+")
	data = data:gsub("[^/][^/]dcl_(.-)\n", "  //dcl_%1\n")
	data = data:gsub("[^/][^T]vs_1_1", "  vs.1.1")
	file:seek("set")
	file:write(data)
	file:close()
	return true
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

function hlsl_to_vsh_folder(hlsl_folder, vsh_folder, dx8)
	local path_to_fxc = '"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Utilities/bin/x86/fxc.exe"'
	
	dx8 = dx8 or false
	print('Converting folder\'s .hlsl files in "' ..hlsl_folder.. '" (inc subfolders)\r\nto .vsh in "'..vsh_folder..'".')
	local start_convert_time = os.clock()
	
	function timed_conversion(full_path_to_hlsl)
		local start_time = os.clock()
		--Modified from code found here:
		--https://stackoverflow.com/questions/5243179/what-is-the-neatest-way-to-split-out-a-path-name-into-its-components-in-lua/12191225
		local path, file_name, file_extension = string.match(full_path_to_hlsl, "(.-)([^\\/]-)%.?([^%.\\/]*)$")
		if (file_extension == 'hlsl') then
			local dx_shader_version = "/Tvs_2_a /Dvs_2_a"
			if dx8 then
				dx_shader_version = "/Tvs_1_1 /Dvs_1_1"
			end

			--Now hlsl files make many vsh at once:
			local vsh_name_define_table = {
				["effect"] = {
					{"eff1", ""},
					{"eff2", "/DNO_VERTEXUVS"}
				},
				["vs_pndt0"] = {
					{"vs_pndt0_ld", ""},
					{"vs_pnt0_ld", "/DNO_DIFFUSE"},
					{"vs_pndt0_ld_t0uvmat", "/DUSE_TEX_TRANSFORM"},
					{"vs_pnt0_ld_t0uvmat", "/DUSE_TEX_TRANSFORM /DNO_DIFFUSE"},
					{"vs_pndt0", "/DNO_LIGHTING"},
					{"vs_pnt0", "/DNO_LIGHTING /DNO_DIFFUSE"},
					{"vs_pndt0_t0uvmat", "/DNO_LIGHTING /DUSE_TEX_TRANSFORM"},
					{"vs_pnt0_t0uvmat", "/DNO_LIGHTING /DUSE_TEX_TRANSFORM /DNO_DIFFUSE"}
				},
				["skinmesh"] = {
					{"skinmesh8_1", "/DNEW_PALETTE_C /DNUM_SKIN_WEIGHTS=1"},
					{"skinmesh8_2", "/DNEW_PALETTE_C /DNUM_SKIN_WEIGHTS=2"},
					{"skinmesh8_3", "/DNEW_PALETTE_C /DNUM_SKIN_WEIGHTS=3"},
					{"skinmesh8_4", "/DNEW_PALETTE_C /DNUM_SKIN_WEIGHTS=4"}
				},
				["skinmesh8_1"] = {
					{"skinmesh8_1", "/DNEW_PALETTE_C /DNUM_SKIN_WEIGHTS=1"}
				},
				["skinmesh8_2"] = {
					{"skinmesh8_2", "/DNEW_PALETTE_C /DNUM_SKIN_WEIGHTS=1"}
				},
				["skinmesh8_3"] = {
					{"skinmesh8_3", "/DNEW_PALETTE_C /DNUM_SKIN_WEIGHTS=1"}
				},
				["skinmesh8_4"] = {
					{"skinmesh8_4", "/DNEW_PALETTE_C /DNUM_SKIN_WEIGHTS=1"}
				},
				["font"] = {{"font", ""}},
				["minimap"] = {{"minimap", ""}},
				["shade_effect"] = {{"shadeeff", "/DPROPER_TEXCOORD1"}},
			};
			
			for i,_table in ipairs(vsh_name_define_table[file_name]) do
				print(file_name)
				local full_path_to_vsh = vsh_folder..'/'.._table[1]..'.vsh'
				if not os.execute(path_to_fxc..' '..dx_shader_version..' '.._table[2]..' /Fc'..full_path_to_vsh..' '..full_path_to_hlsl) then
					print("error when compiling with fxc")
				else
					if dx8 then
						hlsl_to_vsh_dx8(full_path_to_vsh)
					end
					print('('..(os.clock() - start_time)..' s)')
				end
			end
		end
	end
	scandir_f(hlsl_folder, timed_conversion)
	print('Folder converted successfully in '..(os.clock() - start_convert_time)..'s: '..vsh_folder)
end