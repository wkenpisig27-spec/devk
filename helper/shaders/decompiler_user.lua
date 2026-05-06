--[[
o-----------------------------------------------------------------------------o
| top-decompile v.1.0.7                                                       |
(-----------------------------------------------------------------------------)
| By deguix         / An Utility for TOP/PKO/KOP | Compatible w/ LuaJIT 2.1.0 |
|                  -----------------------------------------------------------|
|                                                                             |
|   This script decompiles the .bin files for most game versions. If any      |
| game version isn't supported, please let me know.                           |
o-----------------------------------------------------------------------------o
--]]

--[[
	Version numbers:
	
	- 9 for compiled client (with working binaries, not released yet) - basically 4 without encryption.
	- 8 for PKO (latest).
	- 7 for KOP (latest).
	- 6 for TOPII (latest).
	- 5 for PKO with v.2.4 server only (decompiling server bins will result in resolved resource strings, which can then be used on other versions).
	- 4 for PKO with v.2.4 (used by the newer v2 servers).
	- 3 for TOPII with v.2.0.0 (used by all older v2 servers).
	- 2 for TOPI (latest).
	- 1 for TOPI with v.1.3.5 (used by a couple of servers).
	
	Note: Please, make sure you delete the bins except for versions 3, 4, and 8 resourceinfo (those versions do not compile resourceinfo) and versions
	1 and 2 stringset (those versions do not compile stringset) before recompilation, because newer client versions are being very silent on errors
	on certain bins.
--]]

--[[
	decrypt_bin(source_binary_file,target_decrypted_binary_file)
	Decrypts the specified binary file, even if its already decrypted by itself.
	
	Parameters:
		source_binary_file: the full path to the source binary file using foward slashes. Example: 'C:/Games/TOP/scripts/table/iteminfo.bin'.
		target_decrypted_binary_file: the full path to the target decrypted binary file using foward slashes. Example: 'C:/Games/TOP/scripts/table/iteminfo_un.bin'.
--]]
--decrypt_bin('C:/Games/PKO-2.4-client/scripts/table/areaset.bin', 'C:/Games/PKO-2.4-client/scripts/table/areaset_un.bin')

--[[
	decrypt_folder_bin(folder,version)
	Decrypts all files in a folder, even if they are already decrypted by themselves.
	
	Parameters:
		folder: path to the folder using foward slashes and no ending slash. Example: 'C:/Games/TOP/scripts/table'.
--]]
--decrypt_folder_bin('C:/Games/PKO-2.4-client/scripts/table')

--[[
	decompile_bin(structure,version,source_binary_file,target_text_file)
	Decompile the specified binary file, using the specified structure and version, to a specified text file.
	
	Parameters:
		strcuture: one of the structures found in decompiler.lua - look at one of the values in the top_binary_structures table. Example: iteminfo.
		version: version of the binary files. Look in the "Version numbers" list above. Example (for top 1.38): 2.
		source_binary_file: the full path to the source binary file using foward slashes. Example: 'C:/Games/TOP/scripts/table/iteminfo.bin'.
		target_text_file: the full path to the target text file using foward slashes. Example: 'C:/Games/TOP/scripts/table/iteminfo.txt'.
--]]
--decompile_bin(areaset, 5, 'C:/Games/PKO-2.4-client/scripts/table/areaset.bin', 'C:/Games/PKO-2.4-client/scripts/table/areaset.txt')

--[[
	decompile_folder_bin(folder,version)
	Decompiles all files in a folder.
	
	Parameters:
		folder: path to the folder using foward slashes and no ending slash. Example: 'C:/Games/TOP/scripts/table'.
		version: version of the binary files. Look in the "Version numbers" list above. Example (for top 1.38): 2.
--]]

--[[
	compile_gamefolder_bin(gamefolder)
	Compiles all files in gamefolder/script/table.
	
	Parameters:
		folder: path to the folder using foward slashes and no ending slash. Example: 'C:/Games/TOP/scripts/table'.
		version: version of the binary files. Look in the "Version numbers" list above. Example (for top 1.38): 2.
--]]

--[[
	decrypt_texture(source_image_file[,target_image_file])
	Decrypts the specified source image file to target file. Does nothing but copy if file is already decrypted.
	Replaces itself if target_image_file is not specified.
	
	Parameters:
		source_binary_file: the full path to the source image file using foward slashes. Example: 'C:/Games/TOP/texture/terrain/abrick01.bmp'.
		(optional) target_decrypted_binary_file: the full path to the target decrypted image file using foward slashes. Example: 'C:/Games/TOP/texture/terrain/abrick01_decrypted.bmp'.
--]]

--[[
	decrypt_folder_texture(folder[,linux])
	Decrypts all image files in a folder if they're not decrypted already. Sub-folders are not followed (yet).
	
	Parameters:
		folder: path to the folder using foward slashes and no ending slash. Example: 'C:/Games/TOP/texture/terrain'.
		(optional) linux: set to true if using this in a linux system (cmdline program in linux is named differently). Example: true.
--]]

dofile('./decompiler.lua') --loads the decompiler - don't change this

--decrypt_folder_bin('E:/Games/top-recode/client_/scripts/table-top2-orig')
--decompile_folder_bin('D:/games/Pirates Online V0.6/scripts/table',4)
--decompile_folder_bin('E:/Games/top-recode/client/scripts/table',9)
--decrypt_folder_bin('/home/deguix/.wine32/drive_c/games/top/client/scripts/table')
--decompile_folder_bin('D:/Games/FoxLv/upload/scripts/table',7)
--decompile_folder_bin('/mnt/sda2/Games/TOP2/scripts/table',6)
--decompile_folder_bin('/home/deguix/.wine32/drive_c/games/FoxLv/upload/scripts/table',8)
--decrypt_folder_bin('D:/games/Pirates Online V0.6/scripts/table')
--compile_gamefolder_bin('D:/Games/PKO-2.4-client')
--decompile_folder_bin('E:/Games/Ultimate Pirates Online/scripts/table',2)
--decrypt_texture('/home/deguix/.wine32/drive_c/games/top/client/texture/character/0000000000.bmp')
--decrypt_texture('C:/games/top-recode/client/texture/ui/LoadProgress.tga')
decrypt_folder_texture('C:/games/top-recode/client/texture', false)