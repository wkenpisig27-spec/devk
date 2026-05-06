@echo off
REM ============================================================================
REM BMFont Simple Generator for PKODev
REM ============================================================================
REM 
REM USAGE:
REM 1. Put your .ttf files in "input/" with the name you want:
REM    input/gamedefaultsm.ttf   -> output/gamedefaultsm.fnt (12pt)
REM    input/gamedefaultmid.ttf  -> output/gamedefaultmid.fnt (14pt)
REM    input/gamedefaultbig.ttf  -> output/gamedefaultbig.fnt (20pt)
REM    input/gamedefaulthuge.ttf -> output/gamedefaulthuge.fnt (28pt)
REM 
REM 2. Run this batch file
REM 3. Find generated fonts in "output/"
REM 4. Copy to client/font/
REM 
REM SIZE is auto-detected from filename:
REM   *micro*              = 8pt
REM   *tiny*               = 10pt
REM   *sm* or *small*      = 12pt
REM   *mid* or *medium*    = 14pt
REM   *normal* or *base*   = 16pt (default)
REM   *large* or *lg*      = 18pt
REM   *big*                = 20pt
REM   *xl* or *xlarge*     = 24pt
REM   *huge*               = 28pt
REM   *giant* or *xxl*     = 32pt
REM   *massive* or *xxxl*  = 36pt
REM   *title*              = 40pt
REM   *splash* or *banner* = 48pt
REM   (no match)           = 16pt
REM ============================================================================

setlocal enabledelayedexpansion

REM === CONFIGURATION ===
set BMFONT_PATH=%~dp0bmfont64.exe
set INPUT_DIR=%~dp0input
set OUTPUT_DIR=%~dp0output
set TEMP_CONFIG=%~dp0temp_font.bmfc

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
echo BMFont Simple Generator for PKODev
echo ============================================================================
echo.
echo Using BMFont: %BMFONT_PATH%
echo Input folder: %INPUT_DIR%
echo Output folder: %OUTPUT_DIR%
echo.

REM Check input folder
if not exist "%INPUT_DIR%" (
    mkdir "%INPUT_DIR%"
)

REM Check output folder
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

REM Clean output folder
echo Cleaning output folder...
del /q "%OUTPUT_DIR%\*.fnt" 2>nul
del /q "%OUTPUT_DIR%\*.png" 2>nul

set TOTAL_FONTS=0
set SUCCESS_COUNT=0

REM ============================================================================
REM Process each font file - filename determines font name AND size
REM ============================================================================
echo.
echo Processing font files...
echo.

for %%F in ("%INPUT_DIR%\*.ttf") do (
    set /a TOTAL_FONTS+=1
    set "FONT_NAME=%%~nF"
    set "FONT_PATH=%%F"
    
    REM Auto-detect size from filename (check most specific patterns first)
    set "FONT_SIZE=16"
    echo !FONT_NAME! | findstr /i "micro" >nul && set "FONT_SIZE=8"
    echo !FONT_NAME! | findstr /i "tiny" >nul && set "FONT_SIZE=10"
    echo !FONT_NAME! | findstr /i "sm small" >nul && set "FONT_SIZE=12"
    echo !FONT_NAME! | findstr /i "mid medium name" >nul && set "FONT_SIZE=14"
    echo !FONT_NAME! | findstr /i "normal base" >nul && set "FONT_SIZE=16"
    echo !FONT_NAME! | findstr /i "large lg" >nul && set "FONT_SIZE=18"
    echo !FONT_NAME! | findstr /i "big" >nul && set "FONT_SIZE=20"
    echo !FONT_NAME! | findstr /i "xlarge xl" >nul && set "FONT_SIZE=24"
    echo !FONT_NAME! | findstr /i "huge" >nul && set "FONT_SIZE=28"
    echo !FONT_NAME! | findstr /i "giant xxl" >nul && set "FONT_SIZE=32"
    echo !FONT_NAME! | findstr /i "massive xxxl" >nul && set "FONT_SIZE=36"
    echo !FONT_NAME! | findstr /i "title" >nul && set "FONT_SIZE=40"
    echo !FONT_NAME! | findstr /i "splash banner" >nul && set "FONT_SIZE=48"
    
    echo [!FONT_NAME!] Generating at !FONT_SIZE!pt...
    
    REM Create config for this font
    call :CreateConfig "!FONT_PATH!" "!FONT_NAME!" "!FONT_SIZE!"
    
    REM Run BMFont
    "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\!FONT_NAME!.fnt"
    
    if exist "%OUTPUT_DIR%\!FONT_NAME!.fnt" (
        echo     SUCCESS: !FONT_NAME!.fnt ^(!FONT_SIZE!pt^)
        set /a SUCCESS_COUNT+=1
    ) else (
        echo     FAILED: !FONT_NAME!
    )
)

for %%F in ("%INPUT_DIR%\*.otf") do (
    set /a TOTAL_FONTS+=1
    set "FONT_NAME=%%~nF"
    set "FONT_PATH=%%F"
    
    REM Auto-detect size from filename (check most specific patterns first)
    set "FONT_SIZE=16"
    echo !FONT_NAME! | findstr /i "micro" >nul && set "FONT_SIZE=8"
    echo !FONT_NAME! | findstr /i "tiny" >nul && set "FONT_SIZE=10"
    echo !FONT_NAME! | findstr /i "sm small" >nul && set "FONT_SIZE=12"
    echo !FONT_NAME! | findstr /i "mid medium name" >nul && set "FONT_SIZE=14"
    echo !FONT_NAME! | findstr /i "normal base" >nul && set "FONT_SIZE=16"
    echo !FONT_NAME! | findstr /i "large lg" >nul && set "FONT_SIZE=18"
    echo !FONT_NAME! | findstr /i "big" >nul && set "FONT_SIZE=20"
    echo !FONT_NAME! | findstr /i "xlarge xl" >nul && set "FONT_SIZE=24"
    echo !FONT_NAME! | findstr /i "huge" >nul && set "FONT_SIZE=28"
    echo !FONT_NAME! | findstr /i "giant xxl" >nul && set "FONT_SIZE=32"
    echo !FONT_NAME! | findstr /i "massive xxxl" >nul && set "FONT_SIZE=36"
    echo !FONT_NAME! | findstr /i "title" >nul && set "FONT_SIZE=40"
    echo !FONT_NAME! | findstr /i "splash banner" >nul && set "FONT_SIZE=48"
    
    echo [!FONT_NAME!] Generating at !FONT_SIZE!pt...
    
    call :CreateConfig "!FONT_PATH!" "!FONT_NAME!" "!FONT_SIZE!"
    "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\!FONT_NAME!.fnt"
    
    if exist "%OUTPUT_DIR%\!FONT_NAME!.fnt" (
        echo     SUCCESS: !FONT_NAME!.fnt ^(!FONT_SIZE!pt^)
        set /a SUCCESS_COUNT+=1
    ) else (
        echo     FAILED: !FONT_NAME!
    )
)

REM Cleanup temp config
del "%TEMP_CONFIG%" 2>nul

if %TOTAL_FONTS%==0 (
    echo.
    echo ERROR: No font files found in input folder!
    echo.
    echo Please copy your .ttf files to: %INPUT_DIR%
    echo.
    echo IMPORTANT: The filename determines the SIZE:
    echo.
    echo   gamedefaultsm.ttf   -^> 12pt (small - dialogs, chat)
    echo   gamedefaultmid.ttf  -^> 14pt (medium - buttons, menus)
    echo   gamedefaultbig.ttf  -^> 20pt (big - titles, names)
    echo   gamedefaulthuge.ttf -^> 28pt (huge - splash)
    echo.
    echo You can use the SAME source TTF file for all - just copy and rename!
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================================================
echo Generation Complete!
echo ============================================================================
echo.
echo Generated: %SUCCESS_COUNT% / %TOTAL_FONTS% font files
echo.
echo Output location: %OUTPUT_DIR%
echo.

REM List generated files
echo Generated files:
for %%F in ("%OUTPUT_DIR%\*.fnt") do (
    echo   - %%~nxF
)

echo.
echo ============================================================================
echo NEXT STEPS:
echo ============================================================================
echo.
echo 1. Copy ALL files from output\ to client\font\
echo    Or run: copy_to_client.bat
echo.
echo 2. Rebuild and test the client!
echo.

pause
exit /b 0

REM ============================================================================
REM Function: CreateConfig
REM Parameters: %1=fontPath, %2=fontName, %3=fontSize
REM ============================================================================
:CreateConfig
set CFG_FONT_PATH=%~1
set CFG_FONT_NAME=%~2
set CFG_FONT_SIZE=%~3

REM Auto-detect if font should have outline (for black/bold fonts used in names)
set CFG_OUTLINE=0
set CFG_ISBOLD=0
echo %CFG_FONT_NAME% | findstr /i "black" >nul && set "CFG_OUTLINE=1" && set "CFG_ISBOLD=1"
echo %CFG_FONT_NAME% | findstr /i "semibold" >nul && set "CFG_ISBOLD=1"
echo %CFG_FONT_NAME% | findstr /i "bold" >nul && set "CFG_ISBOLD=1"

(
echo # AngelCode Bitmap Font Generator configuration file
echo fileVersion=1
echo.
echo # font settings
echo fontName=%CFG_FONT_NAME%
echo fontFile=%CFG_FONT_PATH%
echo charSet=0
echo fontSize=-%CFG_FONT_SIZE%
echo aa=1
echo scaleH=100
echo useSmoothing=1
echo isBold=%CFG_ISBOLD%
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
echo outlineThickness=%CFG_OUTLINE%
echo.
echo # selected chars - Basic ASCII + Extended Latin
echo chars=32-126,160-255,8211-8212,8216-8217,8220-8221,8226,8230,8364
echo.
echo # icon settings
echo iconImages=
echo iconImages=
) > "%TEMP_CONFIG%"

goto :eof
