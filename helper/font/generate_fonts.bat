@echo off
REM ============================================================================
REM BMFont Batch Generator for PKODev
REM ============================================================================
REM 
REM INSTRUCTIONS:
REM 1. Run this batch file from helper/font/
REM 2. Font files will be generated directly to client/font/
REM 
REM The script will generate all required .fnt and .png files
REM ============================================================================

setlocal

REM === CONFIGURATION ===
set BMFONT_PATH=%~dp0bmfont64.exe
set OUTPUT_DIR=%~dp0..\..\client\font
set CONFIG_DIR=%~dp0bmfont_config

REM Check if BMFont exists
if not exist "%BMFONT_PATH%" (
    set BMFONT_PATH=%~dp0bmfont32.exe
)

if not exist "%BMFONT_PATH%" (
    echo.
    echo ERROR: bmfont64.exe or bmfont32.exe not found!
    echo.
    echo Please copy bmfont64.exe to: %~dp0
    echo.
    echo Download BMFont from: https://www.angelcode.com/products/bmfont/
    echo.
    pause
    exit /b 1
)

echo ============================================================================
echo BMFont Generator for PKODev
echo ============================================================================
echo.
echo Using BMFont: %BMFONT_PATH%
echo Output folder: %OUTPUT_DIR%
echo.

REM === GENERATE FONTS ===

echo [1/6] Generating gamedefault_12...
"%BMFONT_PATH%" -c "%CONFIG_DIR%\gamedefault_12.bmfc" -o "%OUTPUT_DIR%\gamedefault_12.fnt"
if errorlevel 1 (
    echo WARNING: gamedefault_12 generation may have issues
) else (
    echo OK
)

echo [2/6] Generating gamedefault_13...
"%BMFONT_PATH%" -c "%CONFIG_DIR%\gamedefault_13.bmfc" -o "%OUTPUT_DIR%\gamedefault_13.fnt"
if errorlevel 1 (
    echo WARNING: gamedefault_13 generation may have issues
) else (
    echo OK
)

echo [3/6] Generating gamedefault_20...
"%BMFONT_PATH%" -c "%CONFIG_DIR%\gamedefault_20.bmfc" -o "%OUTPUT_DIR%\gamedefault_20.fnt"
if errorlevel 1 (
    echo WARNING: gamedefault_20 generation may have issues
) else (
    echo OK
)

echo [4/6] Generating gamedefault_28...
"%BMFONT_PATH%" -c "%CONFIG_DIR%\gamedefault_28.bmfc" -o "%OUTPUT_DIR%\gamedefault_28.fnt"
if errorlevel 1 (
    echo WARNING: gamedefault_28 generation may have issues
) else (
    echo OK
)

echo [5/6] Generating Arial_12...
"%BMFONT_PATH%" -c "%CONFIG_DIR%\Arial_12.bmfc" -o "%OUTPUT_DIR%\Arial_12.fnt"
if errorlevel 1 (
    echo WARNING: Arial_12 generation may have issues
) else (
    echo OK
)

echo [6/6] Generating default...
"%BMFONT_PATH%" -c "%CONFIG_DIR%\default.bmfc" -o "%OUTPUT_DIR%\default.fnt"
if errorlevel 1 (
    echo WARNING: default generation may have issues
) else (
    echo OK
)

echo.
echo ============================================================================
echo Generation complete!
echo ============================================================================
echo.
echo Generated files in %OUTPUT_DIR%:
echo.
dir /b "%OUTPUT_DIR%\*.fnt" 2>nul
dir /b "%OUTPUT_DIR%\*.png" 2>nul
echo.
echo Fonts are now ready for the game client!
echo.
pause
