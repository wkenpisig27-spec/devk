@echo off
REM ============================================================================
REM PKO - Windows Deploy Script
REM ============================================================================
REM Creates symbolic links from build output to client/ and server/ directories.
REM Must run as Administrator (symlinks require elevated privileges).
REM
REM Usage:
REM   deploy-windows.bat              Deploy Release build
REM   deploy-windows.bat Debug        Deploy Debug build
REM ============================================================================

setlocal

set SCRIPT_DIR=%~dp0
set SOURCE_DIR=%SCRIPT_DIR%..
set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Release

echo ============================================
echo   PKO - Windows Deploy (%CONFIG%)
echo ============================================
echo.

REM === Server binaries ===
echo --- Server Binaries ---
if "%CONFIG%"=="Debug" (
    call :MKLINK "%SOURCE_DIR%\..\server\AccountServer_D.exe" "%SOURCE_DIR%\bin\Debug\accountserver\AccountServer.exe"
    call :MKLINK "%SOURCE_DIR%\..\server\AccountServer_D.pdb" "%SOURCE_DIR%\bin\Debug\accountserver\AccountServer.pdb"
    call :MKLINK "%SOURCE_DIR%\..\server\GroupServer_D.exe" "%SOURCE_DIR%\bin\Debug\groupserver\GroupServer.exe"
    call :MKLINK "%SOURCE_DIR%\..\server\GroupServer_D.pdb" "%SOURCE_DIR%\bin\Debug\groupserver\GroupServer.pdb"
    call :MKLINK "%SOURCE_DIR%\..\server\GateServer_D.exe" "%SOURCE_DIR%\bin\Debug\gateserver\GateServer.exe"
    call :MKLINK "%SOURCE_DIR%\..\server\GateServer_D.pdb" "%SOURCE_DIR%\bin\Debug\gateserver\GateServer.pdb"
    call :MKLINK "%SOURCE_DIR%\..\server\GameServer_D.exe" "%SOURCE_DIR%\bin\Debug\gameserver\GameServer.exe"
    call :MKLINK "%SOURCE_DIR%\..\server\GameServer_D.pdb" "%SOURCE_DIR%\bin\Debug\gameserver\GameServer.pdb"
) else (
    call :MKLINK "%SOURCE_DIR%\..\server\AccountServer.exe" "%SOURCE_DIR%\bin\Release\accountserver\AccountServer.exe"
    call :MKLINK "%SOURCE_DIR%\..\server\AccountServer.pdb" "%SOURCE_DIR%\bin\Release\accountserver\AccountServer.pdb"
    call :MKLINK "%SOURCE_DIR%\..\server\GroupServer.exe" "%SOURCE_DIR%\bin\Release\groupserver\GroupServer.exe"
    call :MKLINK "%SOURCE_DIR%\..\server\GroupServer.pdb" "%SOURCE_DIR%\bin\Release\groupserver\GroupServer.pdb"
    call :MKLINK "%SOURCE_DIR%\..\server\GateServer.exe" "%SOURCE_DIR%\bin\Release\gateserver\GateServer.exe"
    call :MKLINK "%SOURCE_DIR%\..\server\GateServer.pdb" "%SOURCE_DIR%\bin\Release\gateserver\GateServer.pdb"
    call :MKLINK "%SOURCE_DIR%\..\server\GameServer.exe" "%SOURCE_DIR%\bin\Release\gameserver\GameServer.exe"
    call :MKLINK "%SOURCE_DIR%\..\server\GameServer.pdb" "%SOURCE_DIR%\bin\Release\gameserver\GameServer.pdb"
)

REM === Client binaries ===
echo.
echo --- Client Binaries ---
if "%CONFIG%"=="Debug" (
    call :MKLINK "%SOURCE_DIR%\..\client\system\Game_D.exe" "%SOURCE_DIR%\bin\Debug\game\Game.exe"
    call :MKLINK "%SOURCE_DIR%\..\client\system\Game_D.pdb" "%SOURCE_DIR%\bin\Debug\game\Game.pdb"
    call :MKLINK "%SOURCE_DIR%\..\client\system\MindPower3D_D9D.dll" "%SOURCE_DIR%\bin\Debug\mindpower3d\MindPower3D_D9D.dll"
    call :MKLINK "%SOURCE_DIR%\..\client\system\MindPower3D_D9D.pdb" "%SOURCE_DIR%\bin\Debug\mindpower3d\MindPower3D_D9D.pdb"
    call :MKLINK "%SOURCE_DIR%\..\client\system\CaLua_D.dll" "%SOURCE_DIR%\bin\Debug\calua\CaLua.dll"
    call :MKLINK "%SOURCE_DIR%\..\client\system\CaLua_D.pdb" "%SOURCE_DIR%\bin\Debug\calua\CaLua.pdb"
) else (
    call :MKLINK "%SOURCE_DIR%\..\client\system\Game.exe" "%SOURCE_DIR%\bin\Release\game\Game.exe"
    call :MKLINK "%SOURCE_DIR%\..\client\system\Game.pdb" "%SOURCE_DIR%\bin\Release\game\Game.pdb"
    call :MKLINK "%SOURCE_DIR%\..\client\system\MindPower3D_D9R.dll" "%SOURCE_DIR%\bin\Release\mindpower3d\MindPower3D_D9R.dll"
    call :MKLINK "%SOURCE_DIR%\..\client\system\MindPower3D_D9R.pdb" "%SOURCE_DIR%\bin\Release\mindpower3d\MindPower3D_D9R.pdb"
    call :MKLINK "%SOURCE_DIR%\..\client\system\CaLua.dll" "%SOURCE_DIR%\bin\Release\calua\CaLua.dll"
    call :MKLINK "%SOURCE_DIR%\..\client\system\CaLua.pdb" "%SOURCE_DIR%\bin\Release\calua\CaLua.pdb"
)

echo.
echo ============================================
echo   Deploy complete!
echo ============================================
pause
goto :EOF

:MKLINK
if exist %~1 del /F /Q %~1
if exist %~2 (
    mklink %~1 %~2
) else (
    echo WARNING: Source not found: %~2
)
exit /b 0
