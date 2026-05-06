@echo off
echo ========================================
echo Creating table symlinks (server -> client)
echo ========================================
echo.

:: Create table directory if it doesn't exist
if not exist "%cd%\scripts\table" mkdir "%cd%\scripts\table"

:: Server Table Files -> Client
call :MKLINK "%cd%\scripts\table\AreaSet.txt" "%cd%\..\server\resource\AreaSet.txt"
call :MKLINK "%cd%\scripts\table\Characterinfo.txt" "%cd%\..\server\resource\Characterinfo.txt"
call :MKLINK "%cd%\scripts\table\characterposeinfo.txt" "%cd%\..\server\resource\characterposeinfo.txt"
call :MKLINK "%cd%\scripts\table\chaticons.txt" "%cd%\..\server\resource\chaticons.txt"
call :MKLINK "%cd%\scripts\table\ElfSkillInfo.txt" "%cd%\..\server\resource\ElfSkillInfo.txt"
call :MKLINK "%cd%\scripts\table\eventsound.txt" "%cd%\..\server\resource\eventsound.txt"
call :MKLINK "%cd%\scripts\table\forgeitem.txt" "%cd%\..\server\resource\forgeitem.txt"
call :MKLINK "%cd%\scripts\table\hairs.txt" "%cd%\..\server\resource\hairs.txt"
call :MKLINK "%cd%\scripts\table\iteminfo.txt" "%cd%\..\server\resource\iteminfo.txt"
call :MKLINK "%cd%\scripts\table\itempre.txt" "%cd%\..\server\resource\itempre.txt"
call :MKLINK "%cd%\scripts\table\ItemRefineEffectInfo.txt" "%cd%\..\server\resource\ItemRefineEffectInfo.txt"
call :MKLINK "%cd%\scripts\table\ItemRefineInfo.txt" "%cd%\..\server\resource\ItemRefineInfo.txt"
call :MKLINK "%cd%\scripts\table\itemtype.txt" "%cd%\..\server\resource\itemtype.txt"
call :MKLINK "%cd%\scripts\table\MagicGroupInfo.txt" "%cd%\..\server\resource\MagicGroupInfo.txt"
call :MKLINK "%cd%\scripts\table\MagicSingleinfo.txt" "%cd%\..\server\resource\MagicSingleinfo.txt"
call :MKLINK "%cd%\scripts\table\mapinfo.txt" "%cd%\..\server\resource\mapinfo.txt"
call :MKLINK "%cd%\scripts\table\MonsterInfo.txt" "%cd%\..\server\resource\MonsterInfo.txt"
call :MKLINK "%cd%\scripts\table\MonsterList.txt" "%cd%\..\server\resource\MonsterList.txt"
call :MKLINK "%cd%\scripts\table\mountinfo.txt" "%cd%\..\server\resource\mountinfo.txt"
call :MKLINK "%cd%\scripts\table\musicinfo.txt" "%cd%\..\server\resource\musicinfo.txt"
call :MKLINK "%cd%\scripts\table\notifyset.txt" "%cd%\..\server\resource\notifyset.txt"
call :MKLINK "%cd%\scripts\table\NPCList.txt" "%cd%\..\server\resource\NPCList.txt"
call :MKLINK "%cd%\scripts\table\objevent.txt" "%cd%\..\server\resource\objevent.txt"
call :MKLINK "%cd%\scripts\table\ResourceInfo.txt" "%cd%\..\server\resource\ResourceInfo.txt"
call :MKLINK "%cd%\scripts\table\sceneffectinfo.txt" "%cd%\..\server\resource\sceneffectinfo.txt"
call :MKLINK "%cd%\scripts\table\sceneobjinfo.txt" "%cd%\..\server\resource\sceneobjinfo.txt"
call :MKLINK "%cd%\scripts\table\selectcha.txt" "%cd%\..\server\resource\selectcha.txt"
call :MKLINK "%cd%\scripts\table\ServerSet.txt" "%cd%\..\server\resource\ServerSet.txt"
call :MKLINK "%cd%\scripts\table\shadeinfo.txt" "%cd%\..\server\resource\shadeinfo.txt"
call :MKLINK "%cd%\scripts\table\shipinfo.txt" "%cd%\..\server\resource\shipinfo.txt"
call :MKLINK "%cd%\scripts\table\shipiteminfo.txt" "%cd%\..\server\resource\shipiteminfo.txt"
call :MKLINK "%cd%\scripts\table\skilleff.txt" "%cd%\..\server\resource\skilleff.txt"
call :MKLINK "%cd%\scripts\table\skillinfo.txt" "%cd%\..\server\resource\skillinfo.txt"
call :MKLINK "%cd%\scripts\table\StoneInfo.txt" "%cd%\..\server\resource\StoneInfo.txt"
call :MKLINK "%cd%\scripts\table\StringSet.txt" "%cd%\..\server\resource\StringSet.txt"
call :MKLINK "%cd%\scripts\table\TerrainInfo.txt" "%cd%\..\server\resource\TerrainInfo.txt"

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