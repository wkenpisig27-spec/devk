--[[
o-----------------------------------------------------------------------------o
| obfuscate_vsh.lua                                                           |
(-----------------------------------------------------------------------------)
| Standalone VSH Obfuscation Script                                           |
|                                                                             |
| Use this to strip comments from existing VSH files without recompiling.    |
| Just run: luajit obfuscate_vsh.lua                                          |
o-----------------------------------------------------------------------------o
--]]

--=============================================================================
-- CONFIGURATION
--=============================================================================

-- Path to shader folder (relative to helper/shaders)
local VSH_FOLDER = "../../client/shader"

-- Create backup before obfuscating
local CREATE_BACKUP = true

--=============================================================================
-- FUNCTIONS
--=============================================================================

function strip_comments_from_file(file_path)
    local file = io.open(file_path, 'r')
    if not file then
        return false, "Cannot open file"
    end
    
    local original_content = file:read("*a")
    file:close()
    
    local lines = {}
    local in_multiline_comment = false
    
    for line in original_content:gmatch("[^\r\n]+") do
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
                processed_line = processed_line:sub(1, start_comment - 1) .. processed_line:sub(end_comment + 2)
            else
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
        
        -- Keep the line (even if empty, to preserve structure)
        table.insert(lines, processed_line)
    end
    
    -- Remove consecutive empty lines
    local final_lines = {}
    local prev_empty = false
    for _, line in ipairs(lines) do
        local is_empty = (line:match("^%s*$") ~= nil)
        if not (is_empty and prev_empty) then
            table.insert(final_lines, line)
        end
        prev_empty = is_empty
    end
    
    -- Remove leading/trailing empty lines
    while #final_lines > 0 and final_lines[1]:match("^%s*$") do
        table.remove(final_lines, 1)
    end
    while #final_lines > 0 and final_lines[#final_lines]:match("^%s*$") do
        table.remove(final_lines)
    end
    
    return true, table.concat(final_lines, "\n") .. "\n"
end

function obfuscate_folder(folder)
    print("==============================================================================")
    print("  PKO VSH Obfuscator - Comment Stripper")
    print("==============================================================================")
    print("")
    print("Target folder: " .. folder)
    print("")
    
    local pfile = io.popen('dir "' .. folder .. '" /b /a-d 2>nul')
    if not pfile then
        print("Error: Cannot list directory")
        return
    end
    
    local vsh_files = {}
    for filename in pfile:lines() do
        if filename:match("%.vsh$") then
            table.insert(vsh_files, filename)
        end
    end
    pfile:close()
    
    if #vsh_files == 0 then
        print("No .vsh files found in folder.")
        return
    end
    
    print("Found " .. #vsh_files .. " VSH files.\n")
    
    -- Create backup folder if needed
    if CREATE_BACKUP then
        local backup_folder = folder .. "/backup_" .. os.date("%Y%m%d_%H%M%S")
        os.execute('mkdir "' .. backup_folder .. '" 2>nul')
        
        print("Creating backups in: " .. backup_folder)
        for _, filename in ipairs(vsh_files) do
            os.execute('copy "' .. folder .. '/' .. filename .. '" "' .. backup_folder .. '/' .. filename .. '" >nul 2>&1')
        end
        print("")
    end
    
    print("Stripping comments...\n")
    
    local success_count = 0
    local error_count = 0
    
    for _, filename in ipairs(vsh_files) do
        local full_path = folder .. "/" .. filename
        local ok, result = strip_comments_from_file(full_path)
        
        if ok then
            -- Write the obfuscated content
            local out_file = io.open(full_path, 'w')
            if out_file then
                out_file:write(result)
                out_file:close()
                print("  [OK] " .. filename)
                success_count = success_count + 1
            else
                print("  [ERR] " .. filename .. " - Cannot write file")
                error_count = error_count + 1
            end
        else
            print("  [ERR] " .. filename .. " - " .. result)
            error_count = error_count + 1
        end
    end
    
    print("")
    print("==============================================================================")
    print("  Obfuscation Complete!")
    print("==============================================================================")
    print("")
    print("  Successfully processed: " .. success_count)
    print("  Errors: " .. error_count)
    print("")
end

--=============================================================================
-- MAIN
--=============================================================================

obfuscate_folder(VSH_FOLDER)
