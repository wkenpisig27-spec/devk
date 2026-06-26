@echo off
setlocal
cd /d "%~dp0"

echo.
echo ================================================================================
echo   PKO VSH Obfuscator - Strips Comments from Existing VSH Files
echo   Uses PowerShell — no LuaJIT required
echo ================================================================================
echo.
echo This will strip all comments from VSH files in client/shader/
echo A backup folder will be created before processing.
echo.
pause

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0obfuscate_vsh.ps1"
set ERR=%ERRORLEVEL%

echo.
pause
exit /b %ERR%
