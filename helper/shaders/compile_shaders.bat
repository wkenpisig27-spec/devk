@echo off
setlocal
cd /d "%~dp0"

echo.
echo ================================================================================
echo   PKO Shader Compiler - Compiles HLSL to Obfuscated VSH
echo   Uses PowerShell + fxc.exe — no LuaJIT required
echo ================================================================================
echo.
echo Starting compilation...
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0compile_shaders.ps1"
set ERR=%ERRORLEVEL%

echo.
echo ================================================================================
if not "%ERR%"=="0" (
    echo   Failed with exit code %ERR%
) else (
    echo   Done!
)
echo ================================================================================
pause
exit /b %ERR%
