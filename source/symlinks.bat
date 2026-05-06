@echo off
setlocal

:: Must run as Administrator to create symlinks on Windows
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Administrator privileges required to create symlinks.
    echo Right-click symlinks.bat and select "Run as administrator".
    pause
    exit /b 1
)

set "SRC=%~dp0bin\Release"
set "CLIENT=%~dp0..\client\system"
set "SERVER=%~dp0..\server"

echo ========================================
echo  PKODev Binary Symlinks
echo ========================================
echo.
echo Source : %SRC%
echo Client : %CLIENT%
echo Server : %SERVER%
echo.

:: ---- Client binaries ----
call :MKLINK "%CLIENT%\Game.exe"              "%SRC%\game\Game.exe"
call :MKLINK "%CLIENT%\CaLua.dll"             "%SRC%\calua\CaLua.dll"
call :MKLINK "%CLIENT%\MindPower3D_D9R.dll"   "%SRC%\mindpower3d\MindPower3D_D9R.dll"

:: ---- Server binaries ----
call :MKLINK "%SERVER%\AccountServer.exe"     "%SRC%\accountserver\AccountServer.exe"
call :MKLINK "%SERVER%\GateServer.exe"        "%SRC%\gateserver\GateServer.exe"
call :MKLINK "%SERVER%\GameServer.exe"        "%SRC%\gameserver\GameServer.exe"
call :MKLINK "%SERVER%\GroupServer.exe"       "%SRC%\groupserver\GroupServer.exe"

echo.
echo ========================================
echo  Done!
echo ========================================
pause
goto :EOF

:: -------------------------------------------------------
:MKLINK
:: Usage: call :MKLINK "<link path>" "<target path>"
:: Deletes any existing file/link at the destination, then creates a symlink.
if exist %~1 (
    del /F /Q %~1 2>nul
    if exist %~1 rmdir /Q %~1 2>nul
)
if exist %~2 (
    mklink %~1 %~2 >nul
    echo   [OK]   %~nx1
) else (
    echo   [SKIP] %~nx2 not found - build the project first.
)
exit /b 0
