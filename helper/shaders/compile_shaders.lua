--[[
o-----------------------------------------------------------------------------o
| compile_shaders.lua                                                         |
(-----------------------------------------------------------------------------)
| Shader Compilation Script for PKO Development                              |
|                                                                             |
| Features:                                                                   |
|   - Copies static VSH files from /static to /shaders                        |
|   - Compiles HLSL to VSH for both DX8 and DX9                              |
|   - Strips all comments from output for obfuscation                        |
|   - Encrypts output with XOR cipher (No Header)                            |
o-----------------------------------------------------------------------------o
--]]

dofile('./hlsl_to_vsh.lua')

--=============================================================================
-- CONFIGURATION
--=============================================================================

-- Paths (relative to helper/shaders folder)
local HLSL_FOLDER = "./hlsl"
local STATIC_FOLDER = "./static"
local VSH_OUTPUT_FOLDER = "./shaders"

-- Set to true to compile for DirectX 8, false for DirectX 9
local COMPILE_FOR_DX8 = false

-- Set to true to strip comments and obfuscate output
local STRIP_COMMENTS = true

-- Set to true to minimize whitespace
local MINIMIZE_WHITESPACE = true

-- Set to true to encrypt shaders
local ENCRYPT_SHADERS = true
local PKO_SHADER_KEY = "X7#m9$KpL2@v5*ZnQ8!w4&YhF3%r6^Dq"
-- local PKO_SHADER_MAGIC = "PKOE" -- Not used (Option 2)

--=============================================================================
-- FILE OPERATION FUNCTIONS
--=============================================================================

function scandir(directory)
    local i, t, popen = 0, {}, io.popen
    local pfile = popen('dir "'..directory..'" /b /a-d 2>nul')
    if not pfile then return t end
    for filename in pfile:lines() do
        i = i + 1
        t[i] = filename
    end
    pfile:close()
    return t
end

function copy_file_content(src, dest)
    local infile = io.open(src, "rb")
    if not infile then return false end
    local content = infile:read("*a")
    infile:close()
    
    local outfile = io.open(dest, "wb")
    if not outfile then return false end
    outfile:write(content)
    outfile:close()
    return true
end

function copy_static_files()
    print("------------------------------------------------------------------------------")
    print("[COPYING] Static VSH files from " .. STATIC_FOLDER)
    
    local files = scandir(STATIC_FOLDER)
    local count = 0
    
    for i, filename in ipairs(files) do
        if filename:match("%.vsh$") then
            local src = STATIC_FOLDER .. "/" .. filename
            local dest = VSH_OUTPUT_FOLDER .. "/" .. filename
            if copy_file_content(src, dest) then
                print("    -> " .. filename .. " [COPIED]")
                count = count + 1
            else
                print("    -> " .. filename .. " [FAILED]")
            end
        end
    end
    print("  Copied " .. count .. " static files.")
end

--=============================================================================
-- OBFUSCATION & ENCRYPTION FUNCTIONS
--=============================================================================

-- Encrypt file content with XOR
function encrypt_file(file_path)
    local file = io.open(file_path, 'rb')
    if not file then return false end
    local content = file:read("*a")
    file:close()
    
    -- In Option 2 (No Header), we assume the file is content-ready for encryption.
    
    local key_len = #PKO_SHADER_KEY
    local encrypted_chars = {}
    
    -- Encrypt body (entire content)
    for i = 1, #content do
        local byte = string.byte(content, i)
        local key_byte = string.byte(PKO_SHADER_KEY, (i-1) % key_len + 1)
        table.insert(encrypted_chars, string.char(bit.bxor(byte, key_byte)))
    end
    
    local final_data = table.concat(encrypted_chars)
    
    file = io.open(file_path, 'wb')
    if not file then return false end
    file:write(final_data)
    file:close()
    return true
end

-- Strip all comments from VSH file
function strip_comments(file_path)
    local file = io.open(file_path, 'r')
    if not file then
        print("Error: Cannot open file for stripping: " .. file_path)
        return false
    end
    
    local lines = {}
    local in_multiline_comment = false
    
    for line in file:lines() do
        local processed_line = line
        
        -- Handle multi-line comments /* */
        if in_multiline_comment then
            local end_comment = processed_line:find("%*/")
            if end_comment then
                processed_line = processed_line:sub(end_comment + 2)
                in_multiline_comment = false
            else
                processed_line = ""
            end
        end
        
        -- Check for start of multi-line comment
        local start_comment = processed_line:find("/%*")
        if start_comment then
            local end_comment = processed_line:find("%*/", start_comment)
            if end_comment then
                -- Single line /* comment */
                processed_line = processed_line:sub(1, start_comment - 1) .. processed_line:sub(end_comment + 2)
            else
                -- Multi-line starts here
                processed_line = processed_line:sub(1, start_comment - 1)
                in_multiline_comment = true
            end
        end
        
        -- Remove single-line comments //
        local comment_start = processed_line:find("//")
        if comment_start then
            processed_line = processed_line:sub(1, comment_start - 1)
        end
        
        -- Trim trailing whitespace
        processed_line = processed_line:gsub("%s+$", "")
        
        -- Only keep non-empty lines (or keep structure with minimal empty lines)
        -- Note: We check if line is empty string
        if processed_line ~= "" or not MINIMIZE_WHITESPACE then
            table.insert(lines, processed_line)
        end
    end
    
    file:close()
    
    -- Remove consecutive empty lines if needed, or just write back
    local final_lines = {}
    local prev_empty = false
    for _, line in ipairs(lines) do
        local is_empty = (line == "")
        if not (is_empty and prev_empty) then
            table.insert(final_lines, line)
        end
        prev_empty = is_empty
    end
    
    -- Write back
    file = io.open(file_path, 'w')
    if not file then
        print("Error: Cannot write file: " .. file_path)
        return false
    end
    
    for _, line in ipairs(final_lines) do
        file:write(line .. "\n")
    end
    file:close()
    
    return true
end

-- Process all VSH files in output folder
function obfuscate_vsh_files(folder)
    print("\n[OBFUSCATION] Processing gathered files...")
    
    local files = scandir(folder)
    local count = 0
    
    for _, filename in ipairs(files) do
        if filename:match("%.vsh$") then
            local full_path = folder .. "/" .. filename
            
            -- 1. Strip comments (Obfuscate)
            if strip_comments(full_path) then
                print("  Stripped: " .. filename)
                count = count + 1
                
                -- 2. Encrypt
                if ENCRYPT_SHADERS then
                    if encrypt_file(full_path) then
                        print("  Encrypted: " .. filename)
                    end
                end
            end
        end
    end
    
    print("[OBFUSCATION] Processed " .. count .. " shader files.\n")
end

--=============================================================================
-- SHADER MAPPINGS
--=============================================================================

local pko_shader_mappings = {
    -- Skinned mesh shaders
    ["pu4nt0_ld"] = {
        {"skinmesh8_1", "/DNUM_SKIN_WEIGHTS=1"},
        {"skinmesh8_1_tt1", "/DNUM_SKIN_WEIGHTS=1 /DUSE_UVMAT0"},
        {"skinmesh8_1_tt2", "/DNUM_SKIN_WEIGHTS=1 /DUSE_UVMAT1"},
        {"skinmesh8_1_tt3", "/DNUM_SKIN_WEIGHTS=1 /DUSE_UVMAT2"}
    },
    ["pb1u4nt0_ld"] = {
        {"skinmesh8_2", "/DNUM_SKIN_WEIGHTS=2"},
        {"skinmesh8_2_tt1", "/DNUM_SKIN_WEIGHTS=2 /DUSE_UVMAT0"},
        {"skinmesh8_2_tt2", "/DNUM_SKIN_WEIGHTS=2 /DUSE_UVMAT1"},
        {"skinmesh8_2_tt3", "/DNUM_SKIN_WEIGHTS=2 /DUSE_UVMAT2"}
    },
    ["pb2u4nt0_ld"] = {
        {"skinmesh8_3", "/DNUM_SKIN_WEIGHTS=3"},
        {"skinmesh8_3_tt1", "/DNUM_SKIN_WEIGHTS=3 /DUSE_UVMAT0"},
        {"skinmesh8_3_tt2", "/DNUM_SKIN_WEIGHTS=3 /DUSE_UVMAT1"},
        {"skinmesh8_3_tt3", "/DNUM_SKIN_WEIGHTS=3 /DUSE_UVMAT2"}
    },
    ["pb3u4nt0_ld"] = {
        {"skinmesh8_4", "/DNUM_SKIN_WEIGHTS=4"},
        {"skinmesh8_4_tt1", "/DNUM_SKIN_WEIGHTS=4 /DUSE_UVMAT0"},
        {"skinmesh8_4_tt2", "/DNUM_SKIN_WEIGHTS=4 /DUSE_UVMAT1"},
        {"skinmesh8_4_tt3", "/DNUM_SKIN_WEIGHTS=4 /DUSE_UVMAT2"}
    },

    -- Outline (inverted hull) shaders - second pass, black, CW cull, no depth write
    ["pu4nt0_ld_outline"]  = { {"skinmesh8_1_outline", ""} },
    ["pb1u4nt0_ld_outline"] = { {"skinmesh8_2_outline", ""} },
    ["pb2u4nt0_ld_outline"] = { {"skinmesh8_3_outline", ""} },
    ["pb3u4nt0_ld_outline"] = { {"skinmesh8_4_outline", ""} },
    ["vs_static_outline"]   = { {"vs_static_outline",   ""} },

    -- Static mesh shaders
    ["vs_pndt0"] = {
        {"vs_pndt0", "/DNO_LIGHTING"},
        {"vs_pnt0", "/DNO_LIGHTING /DNO_DIFFUSE"},
        {"vs_pndt0_t0uvmat", "/DNO_LIGHTING /DUSE_TEX_TRANSFORM"},
        {"vs_pnt0_t0uvmat", "/DNO_LIGHTING /DUSE_TEX_TRANSFORM /DNO_DIFFUSE"}
    },
    ["vs_pndt0_ld"] = {
        {"vs_pndt0_ld", ""},
        {"vs_pnt0_ld", "/DNO_DIFFUSE"},
        {"vs_pndt0_ld_t0uvmat", "/DUSE_TEX_TRANSFORM"},
        {"vs_pnt0_ld_t0uvmat", "/DUSE_TEX_TRANSFORM /DNO_DIFFUSE"}
    },
    ["vs_pndt0_ld_t0uvmat"] = { {"vs_pndt0_ld_t0uvmat_alt", ""} },
    ["vs_pndt0_t0uvmat"] = { {"vs_pndt0_t0uvmat_alt", ""} },
    ["vs_pnt0_ld"] = { {"vs_pnt0_ld_alt", ""} },
    ["vs_pnt0_ld_t0uvmat"] = { {"vs_pnt0_ld_t0uvmat_alt", ""} },
    ["vs_pnt0_t0uvmat"] = { {"vs_pnt0_t0uvmat_alt", ""} }
}

--=============================================================================
-- MAIN FUNCTION
--=============================================================================

function main()
    local path_to_fxc = '"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Utilities/bin/x86/fxc.exe"'
    
    print("==============================================================================")
    print("  PKO Shader Compiler with Obfuscation")
    print("==============================================================================")
    print("")
    print("Configuration:")
    print("  HLSL Source:     " .. HLSL_FOLDER)
    print("  Static Source:   " .. STATIC_FOLDER)
    print("  VSH Output:      " .. VSH_OUTPUT_FOLDER)
    print("  Target DX:       " .. (COMPILE_FOR_DX8 and "DirectX 8" or "DirectX 9"))
    print("  Encryption:      " .. (ENCRYPT_SHADERS and "Yes (Key: X7...)" or "No"))
    print("")
    
    -- 1. Copy static files FIRST (resetting them to clean state)
    copy_static_files()
    
    -- 2. Compile HLSL files
    print("\n------------------------------------------------------------------------------")
    local dx_shader_version = COMPILE_FOR_DX8 and "/Tvs_1_1 /Dvs_1_1" or "/Tvs_2_a /Dvs_2_a"
    local total_compiled = 0
    local total_errors = 0
    local start_time = os.clock()
    
    for hlsl_name, outputs in pairs(pko_shader_mappings) do
        local hlsl_path = HLSL_FOLDER .. "/" .. hlsl_name .. ".hlsl"
        local hlsl_file = io.open(hlsl_path, "r")
        if hlsl_file then
            hlsl_file:close()
            print("[COMPILING] " .. hlsl_name .. ".hlsl")
            for _, output_info in ipairs(outputs) do
                local vsh_name = output_info[1]
                local defines = output_info[2]
                local vsh_path = VSH_OUTPUT_FOLDER .. "/" .. vsh_name .. ".vsh"
                local cmd = path_to_fxc .. " " .. dx_shader_version .. " " .. defines .. " /Fc" .. vsh_path .. " " .. hlsl_path .. " 2>&1"
                local result = os.execute(cmd)
                if result then
                    print("    -> " .. vsh_name .. ".vsh [OK]")
                    total_compiled = total_compiled + 1
                    if COMPILE_FOR_DX8 then hlsl_to_vsh_dx8(vsh_path) end
                else
                    print("    -> " .. vsh_name .. ".vsh [FAILED]")
                    total_errors = total_errors + 1
                end
            end
        else
            print("[SKIPPING] " .. hlsl_name .. ".hlsl (not found)")
        end
    end
    
    -- 3. Obfuscate and Encrypt EVERYTHING in the output folder
    obfuscate_vsh_files(VSH_OUTPUT_FOLDER)
    
    local elapsed = os.clock() - start_time
    print("==============================================================================")
    print("  Compilation Complete!")
    print("==============================================================================")
    print("  Total Compiled:  " .. total_compiled)
    print("  Total Errors:    " .. total_errors)
    print("  Time Elapsed:    " .. string.format("%.2f", elapsed) .. " seconds")
end

-- Run Main
main()
