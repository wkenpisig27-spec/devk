@echo off
REM ============================================================================
REM PKO - Windows Build Script (MSBuild)
REM ============================================================================
REM Usage:
REM   build-windows.bat              Build Release x64
REM   build-windows.bat Debug        Build Debug x64
REM   build-windows.bat Release clean    Clean Release build
REM ============================================================================

setlocal

set SCRIPT_DIR=%~dp0
set SOURCE_DIR=%SCRIPT_DIR%..
set SOLUTION=%SOURCE_DIR%\source.sln
set CONFIG=%~1
set ACTION=%~2

if "%CONFIG%"=="" set CONFIG=Release
if "%ACTION%"=="clean" (
    set TARGET=/t:Clean
) else (
    set TARGET=
)

REM Find MSBuild
set MSBUILD=
for %%P in (
    "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
) do (
    if exist %%P (
        set MSBUILD=%%P
        goto :found
    )
)
echo ERROR: MSBuild not found. Install Visual Studio 2022.
exit /b 1

:found
echo ============================================
echo   PKO - Windows Build
echo ============================================
echo   Solution:   %SOLUTION%
echo   Config:     %CONFIG% ^| x64
echo   MSBuild:    %MSBUILD%
if not "%TARGET%"=="" echo   Action:     Clean
echo ============================================
echo.

%MSBUILD% "%SOLUTION%" /p:Configuration=%CONFIG% /p:Platform=x64 %TARGET% /m /v:minimal

if %ERRORLEVEL%==0 (
    echo.
    echo ============================================
    echo   BUILD SUCCESSFUL
    echo ============================================
    echo.
    echo Binaries: %SOURCE_DIR%\bin\%CONFIG%\
    echo.
    if "%ACTION%"=="clean" (
        echo Build cleaned.
    ) else (
        echo Server binaries:
        if exist "%SOURCE_DIR%\bin\%CONFIG%\accountserver\AccountServer.exe" echo   [OK] AccountServer.exe
        if exist "%SOURCE_DIR%\bin\%CONFIG%\gateserver\GateServer.exe" echo   [OK] GateServer.exe
        if exist "%SOURCE_DIR%\bin\%CONFIG%\groupserver\GroupServer.exe" echo   [OK] GroupServer.exe
        if exist "%SOURCE_DIR%\bin\%CONFIG%\gameserver\GameServer.exe" echo   [OK] GameServer.exe
        echo.
        echo Client binaries:
        if exist "%SOURCE_DIR%\bin\%CONFIG%\game\Game.exe" echo   [OK] Game.exe
        if exist "%SOURCE_DIR%\bin\%CONFIG%\mindpower3d\MindPower3D_D9R.dll" echo   [OK] MindPower3D_D9R.dll
        if exist "%SOURCE_DIR%\bin\%CONFIG%\calua\CaLua.dll" echo   [OK] CaLua.dll
    )
) else (
    echo.
    echo ============================================
    echo   BUILD FAILED (exit code: %ERRORLEVEL%)
    echo ============================================
)

endlocal
