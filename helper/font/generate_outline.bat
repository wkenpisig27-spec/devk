@echo off
REM ============================================================================
REM BMFont Outline Font Generator for PKODev
REM ============================================================================
REM 
REM This script generates fonts with PRE-BAKED OUTLINES for optimal performance.
REM Instead of drawing text 9 times at runtime (8 outline + 1 main), we bake
REM the outline directly into the font texture for single-pass rendering.
REM 
REM USAGE:
REM 1. Ensure input/ folder has the TTF/OTF font files
REM 2. Run this batch file
REM 3. Find generated outline fonts in "output/"
REM 4. Copy to client/font/
REM 
REM OUTPUT FILES:
REM   nameoutline.fnt          - Character/NPC name font with black outline
REM   titleoutline.fnt         - Map title/banner font with black outline
REM   
REM PERFORMANCE BENEFIT:
REM   Before: 9 draw calls per outlined text (8 outline + 1 main)
REM   After:  1 draw call per outlined text (outline pre-baked in texture)
REM ============================================================================

setlocal enabledelayedexpansion

REM === CONFIGURATION ===
set BMFONT_PATH=%~dp0bmfont64.exe
set INPUT_DIR=%~dp0input
set OUTPUT_DIR=%~dp0output
set TEMP_CONFIG=%~dp0temp_outline.bmfc

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
echo BMFont Outline Font Generator for PKODev
echo ============================================================================
echo.
echo This generates fonts with PRE-BAKED OUTLINES for optimal performance.
echo.
echo Using BMFont: %BMFONT_PATH%
echo Input folder: %INPUT_DIR%
echo Output folder: %OUTPUT_DIR%
echo.

REM Ensure output folder exists
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

set TOTAL_FONTS=0
set SUCCESS_COUNT=0

REM ============================================================================
REM Generate Outline Fonts
REM ============================================================================

echo.
echo === Generating Pre-Baked Outline Fonts ===
echo.

REM --- Name Font with Outline (14pt Black weight) ---
REM Used for character/NPC names above heads
if exist "%INPUT_DIR%\gamedefaultmidblack.ttf" (
    set /a TOTAL_FONTS+=1
    echo [14pt] Generating nameoutline (character/NPC names)...
    call :CreateOutlineConfig "14" "%INPUT_DIR%\gamedefaultmidblack.ttf" "Metropolis-Black" "nameoutline" "1" "0,0,0,255"
    "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\nameoutline.fnt"
    if exist "%OUTPUT_DIR%\nameoutline.fnt" (
        echo         SUCCESS - Pre-baked 1px black outline
        set /a SUCCESS_COUNT+=1
    ) else (
        echo         FAILED
    )
) else (
    echo WARNING: gamedefaultmidblack.ttf not found, skipping nameoutline
)

REM --- Small Name Font with Outline (12pt Black weight) ---
REM Alternative smaller name font
if exist "%INPUT_DIR%\gamedefaultsmblack.ttf" (
    set /a TOTAL_FONTS+=1
    echo [12pt] Generating namesmoutline (smaller names)...
    call :CreateOutlineConfig "12" "%INPUT_DIR%\gamedefaultsmblack.ttf" "Metropolis-Black" "namesmoutline" "1" "0,0,0,255"
    "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\namesmoutline.fnt"
    if exist "%OUTPUT_DIR%\namesmoutline.fnt" (
        echo         SUCCESS - Pre-baked 1px black outline
        set /a SUCCESS_COUNT+=1
    ) else (
        echo         FAILED
    )
) else (
    echo WARNING: gamedefaultsmblack.ttf not found, skipping namesmoutline
)

REM --- Title Font with Outline (28pt Black weight) ---
REM Used for map banners and large titles
if exist "%INPUT_DIR%\titleblack.ttf" (
    set /a TOTAL_FONTS+=1
    echo [28pt] Generating titleoutline (map banners)...
    call :CreateOutlineConfig "28" "%INPUT_DIR%\titleblack.ttf" "Metropolis-Black" "titleoutline" "2" "0,0,0,255"
    "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\titleoutline.fnt"
    if exist "%OUTPUT_DIR%\titleoutline.fnt" (
        echo         SUCCESS - Pre-baked 2px black outline
        set /a SUCCESS_COUNT+=1
    ) else (
        echo         FAILED
    )
) else (
    echo WARNING: titleblack.ttf not found, skipping titleoutline
)

REM --- Splash Font with Outline (40pt Black weight) ---
REM Used for big splash text like level up
if exist "%INPUT_DIR%\splashblack.ttf" (
    set /a TOTAL_FONTS+=1
    echo [40pt] Generating splashoutline (level up text)...
    call :CreateOutlineConfig "40" "%INPUT_DIR%\splashblack.ttf" "Metropolis-Black" "splashoutline" "2" "0,0,0,255"
    "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\splashoutline.fnt"
    if exist "%OUTPUT_DIR%\splashoutline.fnt" (
        echo         SUCCESS - Pre-baked 2px black outline
        set /a SUCCESS_COUNT+=1
    ) else (
        echo         FAILED
    )
) else (
    echo WARNING: splashblack.ttf not found, skipping splashoutline
)

REM --- Regular Font with Outline (12pt for UI hints) ---
if exist "%INPUT_DIR%\gamedefaultsm.ttf" (
    set /a TOTAL_FONTS+=1
    echo [12pt] Generating hintoutline (UI tooltips)...
    call :CreateOutlineConfig "12" "%INPUT_DIR%\gamedefaultsm.ttf" "Metropolis" "hintoutline" "1" "0,0,0,255"
    "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\hintoutline.fnt"
    if exist "%OUTPUT_DIR%\hintoutline.fnt" (
        echo         SUCCESS - Pre-baked 1px black outline
        set /a SUCCESS_COUNT+=1
    ) else (
        echo         FAILED
    )
) else (
    echo WARNING: gamedefaultsm.ttf not found, skipping hintoutline
)

REM ============================================================================
REM Cleanup and Summary
REM ============================================================================
:Done

REM Cleanup temp config
del "%TEMP_CONFIG%" 2>nul

echo.
echo ============================================================================
echo Outline Font Generation Complete!
echo ============================================================================
echo.
echo Generated: %SUCCESS_COUNT% / %TOTAL_FONTS% outline font files
echo.
echo Output location: %OUTPUT_DIR%
echo.

REM List generated outline files
echo Generated outline fonts:
for %%F in ("%OUTPUT_DIR%\*outline*.fnt") do (
    echo   - %%~nxF
)

echo.
echo ============================================================================
echo PERFORMANCE IMPROVEMENT
echo ============================================================================
echo.
echo Before (runtime outline):
echo   - Character name: 9 draw calls (8 outline directions + 1 main text)
echo   - 50 players on screen = 450 draw calls just for names!
echo.
echo After (pre-baked outline):
echo   - Character name: 1 draw call (outline baked into texture)
echo   - 50 players on screen = 50 draw calls for names
echo   - 9x REDUCTION in draw calls!
echo.
echo ============================================================================
echo NEXT STEPS
echo ============================================================================
echo.
echo 1. Copy outline fonts to client:
echo    copy "%OUTPUT_DIR%\*outline*.fnt" "..\..\client\font\"
echo    copy "%OUTPUT_DIR%\*outline*.png" "..\..\client\font\"
echo.
echo 2. Update font.clu to use outline fonts:
echo    NAME_FONT = "nameoutline"  -- Pre-baked outline
echo.
echo 3. Update C++ code to use regular Render() instead of ORender()
echo    for fonts that have pre-baked outlines.
echo.
pause
exit /b 0

REM ============================================================================
REM Function: CreateOutlineConfig
REM Creates a BMFont config file with outline settings
REM 
REM Parameters:
REM   %1 = Font size (e.g., "14")
REM   %2 = Font file path
REM   %3 = Font name
REM   %4 = Output name
REM   %5 = Outline thickness (1-4 recommended)
REM   %6 = Outline color (R,G,B,A format, e.g., "0,0,0,255" for black)
REM ============================================================================
:CreateOutlineConfig
set CFG_SIZE=%~1
set CFG_FONT_PATH=%~2
set CFG_FONT_NAME=%~3
set CFG_OUTPUT=%~4
set CFG_OUTLINE=%~5
set CFG_OUTLINE_COLOR=%~6

(
echo # AngelCode Bitmap Font Generator configuration file
echo # Generated by generate_outline.bat for PKODev
echo fileVersion=1
echo.
echo # Font settings
echo fontName=%CFG_FONT_NAME%
echo fontFile=%CFG_FONT_PATH%
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
echo # Padding for outline
echo paddingDown=%CFG_OUTLINE%
echo paddingUp=%CFG_OUTLINE%
echo paddingRight=%CFG_OUTLINE%
echo paddingLeft=%CFG_OUTLINE%
echo spacingHoriz=2
echo spacingVert=2
echo useFixedHeight=0
echo forceZero=0
echo widthPaddingFactor=0.00
echo.
echo # Output texture settings
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
echo # OUTLINE SETTINGS - This is the key setting!
echo # outlineThickness: pixels of outline around each glyph
echo outlineThickness=%CFG_OUTLINE%
echo.
echo # Character set - Basic ASCII + Extended Latin
echo chars=32-126,160-255,8211-8212,8216-8217,8220-8221,8226,8230,8364
echo.
echo # Icon settings
echo iconImages=
echo iconImages=
) > "%TEMP_CONFIG%"

goto :eof
