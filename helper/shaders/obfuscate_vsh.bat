@echo off
echo.
echo ================================================================================
echo   PKO VSH Obfuscator - Strips Comments from Existing VSH Files
echo ================================================================================
echo.
echo This will strip all comments from VSH files in client/shader/
echo A backup will be created before processing.
echo.
echo Press any key to continue or Ctrl+C to cancel...
pause > nul
echo.

luajit obfuscate_vsh.lua

echo.
echo Press any key to exit...
pause > nul
