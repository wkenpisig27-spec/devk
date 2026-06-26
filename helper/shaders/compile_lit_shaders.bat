@echo off
setlocal
cd /d "%~dp0"

echo.
echo ================================================================================
echo   PKO Lit Shader Compiler (cel / outline / static lit)
echo   Uses PowerShell + fxc.exe — no LuaJIT required
echo ================================================================================
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0compile_lit_shaders.ps1"
set ERR=%ERRORLEVEL%

echo.
if not "%ERR%"=="0" (
    echo Compilation failed with exit code %ERR%.
    pause
    exit /b %ERR%
)

echo Press any key to exit...
pause >nul
exit /b 0
