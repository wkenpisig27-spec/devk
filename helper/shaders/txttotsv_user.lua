--[[
o-----------------------------------------------------------------------------o
| txttotsv                                                                    |
(-----------------------------------------------------------------------------)
| By deguix                / An Utility for top-recode | Compatible w/ LuaJIT |
|                         ----------------------------------------------------|
|   Converts txt files into lua files.                                        |
o-----------------------------------------------------------------------------o
--]]

--[[
	txt_to_tsv_folder(folder[,linux])
	Converts whole table folder into lua files.
	
	Parameters:
		folder: path to the folder using foward slashes and no ending slash. Example: 'C:/Games/TOP/texture/terrain'.
--]]

dofile('./txttotsv.lua') --loads the txttotsv - don't change this

txt_to_tsv_folder('../client/scripts/table/')