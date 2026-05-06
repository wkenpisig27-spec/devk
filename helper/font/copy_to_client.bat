@echo off
REM ============================================================================
REM Copy Generated Fonts to Client
REM ============================================================================

setlocal

set OUTPUT_DIR=%~dp0output
set CLIENT_FONT_DIR=%~dp0..\..\client\font

echo ============================================================================
echo Copy Fonts to Client
echo ============================================================================
echo.
echo Source: %OUTPUT_DIR%
echo Destination: %CLIENT_FONT_DIR%
echo.

REM Check if output folder has files
set FILE_COUNT=0
for %%F in ("%OUTPUT_DIR%\*.fnt") do set /a FILE_COUNT+=1
for %%F in ("%OUTPUT_DIR%\*.png") do set /a FILE_COUNT+=1

if %FILE_COUNT%==0 (
    echo ERROR: No font files found in output folder!
    echo.
    echo Please run generate_all.bat first.
    echo.
    pause
    exit /b 1
)

echo Found %FILE_COUNT% files to copy.
echo.
echo Files to be copied:
for %%F in ("%OUTPUT_DIR%\*.fnt") do echo   - %%~nxF
for %%F in ("%OUTPUT_DIR%\*.png") do echo   - %%~nxF
echo.

set /p CONFIRM="Copy files to client/font? (Y/N): "
if /i not "%CONFIRM%"=="Y" (
    echo Cancelled.
    pause
    exit /b 0
)

echo.
echo Copying files...

copy /y "%OUTPUT_DIR%\*.fnt" "%CLIENT_FONT_DIR%\" >nul
copy /y "%OUTPUT_DIR%\*.png" "%CLIENT_FONT_DIR%\" >nul

echo.
echo ============================================================================
echo Done! Files copied to client/font/
echo ============================================================================
echo.
echo Don't forget to rebuild the client!
echo.

pause
