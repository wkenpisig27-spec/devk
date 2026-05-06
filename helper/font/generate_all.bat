@echo off
REM ============================================================================
REM BMFont Auto-Generator for PKODev
REM ============================================================================
REM 
REM USAGE:
REM 1. Drop your .ttf font file into the "input/" folder
REM 2. Run this batch file
REM 3. Find generated fonts in "output/" folder
REM 4. Copy contents of output/ to client/font/
REM 
REM This will generate all required game font sizes automatically!
REM ============================================================================

setlocal enabledelayedexpansion

REM === CONFIGURATION ===
set BMFONT_PATH=%~dp0bmfont64.exe
set INPUT_DIR=%~dp0input
set OUTPUT_DIR=%~dp0output
set CONFIG_DIR=%~dp0bmfont_config
set TEMP_CONFIG=%~dp0temp_font.bmfc

REM Font sizes required by the game
set FONT_SIZES=12 13 14 16 20 28

REM Check if BMFont exists
if not exist "%BMFONT_PATH%" (
    set BMFONT_PATH=%~dp0bmfont32.exe
)

if not exist "%BMFONT_PATH%" (
    echo.
    echo ERROR: bmfont64.exe or bmfont32.exe not found!
    echo.
    echo Please download BMFont from: https://www.angelcode.com/products/bmfont/
    echo And copy bmfont64.exe to: %~dp0
    echo.
    pause
    exit /b 1
)

echo ============================================================================
echo BMFont Auto-Generator for PKODev
echo ============================================================================
echo.
echo Using BMFont: %BMFONT_PATH%
echo Input folder: %INPUT_DIR%
echo Output folder: %OUTPUT_DIR%
echo.

REM Check if input folder exists
if not exist "%INPUT_DIR%" (
    mkdir "%INPUT_DIR%"
    echo Created input folder. Please add your .ttf font files there.
    pause
    exit /b 1
)

REM Check if output folder exists
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

REM Find TTF files in input folder
set FONT_COUNT=0
for %%F in ("%INPUT_DIR%\*.ttf") do (
    set /a FONT_COUNT+=1
    set "FONT_FILE=%%~nxF"
    set "FONT_NAME=%%~nF"
    set "FONT_FULL_PATH=%%F"
)

for %%F in ("%INPUT_DIR%\*.otf") do (
    set /a FONT_COUNT+=1
    set "FONT_FILE=%%~nxF"
    set "FONT_NAME=%%~nF"
    set "FONT_FULL_PATH=%%F"
)

if %FONT_COUNT%==0 (
    echo.
    echo ERROR: No font files found in input folder!
    echo.
    echo Please copy your .ttf or .otf font file to:
    echo   %INPUT_DIR%
    echo.
    pause
    exit /b 1
)

if %FONT_COUNT% GTR 1 (
    echo.
    echo WARNING: Multiple font files found. Using the last one: %FONT_FILE%
    echo For best results, keep only one font file in the input folder.
    echo.
)

echo Found font: %FONT_FILE%
echo.

REM Clean output folder
echo Cleaning output folder...
del /q "%OUTPUT_DIR%\*.fnt" 2>nul
del /q "%OUTPUT_DIR%\*.png" 2>nul

REM Generate fonts for each size
echo.
echo Generating fonts...
echo.

set SUCCESS_COUNT=0
set TOTAL_COUNT=0

for %%S in (%FONT_SIZES%) do (
    set /a TOTAL_COUNT+=1
    echo [%%S pt] Generating gamedefault_%%S...
    
    REM Create temporary config file
    call :CreateConfig "%%S" "%FONT_FILE%" "%FONT_NAME%" "gamedefault_%%S"
    
    REM Run BMFont
    "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\gamedefault_%%S.fnt"
    
    if exist "%OUTPUT_DIR%\gamedefault_%%S.fnt" (
        echo         SUCCESS: gamedefault_%%S.fnt created
        set /a SUCCESS_COUNT+=1
    ) else (
        echo         FAILED: Could not generate gamedefault_%%S
    )
)

REM Also generate default.fnt (copy of 12pt)
echo.
echo [12 pt] Generating default.fnt...
call :CreateConfig "12" "%FONT_FILE%" "%FONT_NAME%" "default"
"%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\default.fnt"
if exist "%OUTPUT_DIR%\default.fnt" (
    echo         SUCCESS: default.fnt created
    set /a SUCCESS_COUNT+=1
)
set /a TOTAL_COUNT+=1

REM Generate Arial_12 (using same font)
echo.
echo [12 pt] Generating Arial_12.fnt...
call :CreateConfig "12" "%FONT_FILE%" "%FONT_NAME%" "Arial_12"
"%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\Arial_12.fnt"
if exist "%OUTPUT_DIR%\Arial_12.fnt" (
    echo         SUCCESS: Arial_12.fnt created
    set /a SUCCESS_COUNT+=1
)
set /a TOTAL_COUNT+=1

REM Cleanup temp config
del "%TEMP_CONFIG%" 2>nul

echo.
echo ============================================================================
echo Generation Complete!
echo ============================================================================
echo.
echo Font used: %FONT_FILE%
echo Generated: %SUCCESS_COUNT% / %TOTAL_COUNT% font files
echo.
echo Output location: %OUTPUT_DIR%
echo.

REM List generated files
echo Generated files:
echo.
for %%F in ("%OUTPUT_DIR%\*.fnt") do (
    echo   - %%~nxF
)
for %%F in ("%OUTPUT_DIR%\*.png") do (
    echo   - %%~nxF
)

echo.
echo ============================================================================
echo NEXT STEPS:
echo ============================================================================
echo.
echo 1. Review the generated files in: output\
echo 2. Copy ALL files (.fnt and .png) to: client\font\
echo    Or run: copy_to_client.bat
echo 3. Rebuild and test the client
echo.

pause
exit /b 0

REM ============================================================================
REM Function: CreateConfig
REM Creates a temporary BMFont config file
REM Parameters: %1=size, %2=fontFile, %3=fontName, %4=outputName
REM ============================================================================
:CreateConfig
set CFG_SIZE=%~1
set CFG_FONT_FILE=%~2
set CFG_FONT_NAME=%~3
set CFG_OUTPUT=%~4

(
echo # AngelCode Bitmap Font Generator configuration file
echo fileVersion=1
echo.
echo # font settings
echo fontName=%CFG_FONT_NAME%
echo fontFile=%INPUT_DIR%\%CFG_FONT_FILE%
echo charSet=0
echo fontSize=-%CFG_SIZE%
echo aa=1
echo scaleH=100
echo useSmoothing=1
echo isBold=0
echo isItalic=0
echo useUnicode=1
echo disableBoxChars=1
echo outputInvalidCharGlyph=0
echo dontIncludeKerningPairs=0
echo useHinting=1
echo renderFromOutline=1
echo useClearType=0
echo autoFitNumPages=0
echo autoFitFontSizeMin=0
echo autoFitFontSizeMax=0
echo.
echo # character alignment
echo paddingDown=1
echo paddingUp=1
echo paddingRight=1
echo paddingLeft=1
echo spacingHoriz=1
echo spacingVert=1
echo useFixedHeight=0
echo forceZero=0
echo widthPaddingFactor=0.00
echo.
echo # output file
echo outWidth=512
echo outHeight=512
echo outBitDepth=32
echo fontDescFormat=0
echo fourChnlPacked=0
echo textureFormat=png
echo textureCompression=0
echo alphaChnl=0
echo redChnl=4
echo greenChnl=4
echo blueChnl=4
echo invA=0
echo invR=0
echo invG=0
echo invB=0
echo.
echo # outline
echo outlineThickness=0
echo.
echo # selected chars - Basic ASCII + Extended Latin
echo chars=32-126,160-255,8211-8212,8216-8217,8220-8221,8226,8230,8364
echo.
echo # icon settings
echo iconImages=
echo iconImages=
) > "%TEMP_CONFIG%"

goto :eof
