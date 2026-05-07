@echo off
echo ========================================
echo Creating table symlinks (server -> client)
echo ========================================
echo.

:: Use script's own directory so running as admin doesn't default to System32
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

:: Create table directory if it doesn't exist
if not exist "%SCRIPT_DIR%\scripts\table" mkdir "%SCRIPT_DIR%\scripts\table"

:: Server Table Files -> Client
call :MKLINK "%SCRIPT_DIR%\scripts\table\AreaSet.txt" "%SCRIPT_DIR%\..\server\resource\AreaSet.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\Characterinfo.txt" "%SCRIPT_DIR%\..\server\resource\Characterinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\characterposeinfo.txt" "%SCRIPT_DIR%\..\server\resource\characterposeinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\chaticons.txt" "%SCRIPT_DIR%\..\server\resource\chaticons.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\ElfSkillInfo.txt" "%SCRIPT_DIR%\..\server\resource\ElfSkillInfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\eventsound.txt" "%SCRIPT_DIR%\..\server\resource\eventsound.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\forgeitem.txt" "%SCRIPT_DIR%\..\server\resource\forgeitem.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\hairs.txt" "%SCRIPT_DIR%\..\server\resource\hairs.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\iteminfo.txt" "%SCRIPT_DIR%\..\server\resource\iteminfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\itempre.txt" "%SCRIPT_DIR%\..\server\resource\itempre.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\ItemRefineEffectInfo.txt" "%SCRIPT_DIR%\..\server\resource\ItemRefineEffectInfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\ItemRefineInfo.txt" "%SCRIPT_DIR%\..\server\resource\ItemRefineInfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\itemtype.txt" "%SCRIPT_DIR%\..\server\resource\itemtype.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\MagicGroupInfo.txt" "%SCRIPT_DIR%\..\server\resource\MagicGroupInfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\MagicSingleinfo.txt" "%SCRIPT_DIR%\..\server\resource\MagicSingleinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\mapinfo.txt" "%SCRIPT_DIR%\..\server\resource\mapinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\MonsterInfo.txt" "%SCRIPT_DIR%\..\server\resource\MonsterInfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\MonsterList.txt" "%SCRIPT_DIR%\..\server\resource\MonsterList.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\mountinfo.txt" "%SCRIPT_DIR%\..\server\resource\mountinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\musicinfo.txt" "%SCRIPT_DIR%\..\server\resource\musicinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\notifyset.txt" "%SCRIPT_DIR%\..\server\resource\notifyset.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\NPCList.txt" "%SCRIPT_DIR%\..\server\resource\NPCList.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\objevent.txt" "%SCRIPT_DIR%\..\server\resource\objevent.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\ResourceInfo.txt" "%SCRIPT_DIR%\..\server\resource\ResourceInfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\sceneffectinfo.txt" "%SCRIPT_DIR%\..\server\resource\sceneffectinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\sceneobjinfo.txt" "%SCRIPT_DIR%\..\server\resource\sceneobjinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\selectcha.txt" "%SCRIPT_DIR%\..\server\resource\selectcha.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\ServerSet.txt" "%SCRIPT_DIR%\..\server\resource\ServerSet.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\shadeinfo.txt" "%SCRIPT_DIR%\..\server\resource\shadeinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\shipinfo.txt" "%SCRIPT_DIR%\..\server\resource\shipinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\shipiteminfo.txt" "%SCRIPT_DIR%\..\server\resource\shipiteminfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\skilleff.txt" "%SCRIPT_DIR%\..\server\resource\skilleff.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\skillinfo.txt" "%SCRIPT_DIR%\..\server\resource\skillinfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\StoneInfo.txt" "%SCRIPT_DIR%\..\server\resource\StoneInfo.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\StringSet.txt" "%SCRIPT_DIR%\..\server\resource\StringSet.txt"
call :MKLINK "%SCRIPT_DIR%\scripts\table\TerrainInfo.txt" "%SCRIPT_DIR%\..\server\resource\TerrainInfo.txt"

echo.
echo ========================================
echo Table symlinks created!
echo ========================================
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