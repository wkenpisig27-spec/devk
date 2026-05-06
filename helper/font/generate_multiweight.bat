@echo off
REM ============================================================================
REM BMFont Multi-Weight Generator for PKODev
REM ============================================================================
REM 
REM USAGE:
REM 1. Create subfolders in "input/" for each weight:
REM    input/regular/  - Regular weight font
REM    input/bold/     - Bold weight font
REM    input/light/    - Light weight font (optional)
REM    input/medium/   - Medium weight font (optional)
REM 
REM 2. Put ONE .ttf file in each subfolder
REM 3. Run this batch file
REM 4. Find generated fonts in "output/"
REM 5. Copy to client/font/
REM 
REM OUTPUT FILES:
REM   gamedefault_12.fnt       (from regular/)
REM   gamedefault_bold_12.fnt  (from bold/)
REM   gamedefault_light_12.fnt (from light/)
REM   etc...
REM ============================================================================

setlocal enabledelayedexpansion

REM === CONFIGURATION ===
set BMFONT_PATH=%~dp0bmfont64.exe
set INPUT_DIR=%~dp0input
set OUTPUT_DIR=%~dp0output
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
echo BMFont Multi-Weight Generator for PKODev
echo ============================================================================
echo.
echo Using BMFont: %BMFONT_PATH%
echo Input folder: %INPUT_DIR%
echo Output folder: %OUTPUT_DIR%
echo.

REM Check input folder structure
if not exist "%INPUT_DIR%" (
    mkdir "%INPUT_DIR%"
)

REM Clean output folder
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

echo Cleaning output folder...
del /q "%OUTPUT_DIR%\*.fnt" 2>nul
del /q "%OUTPUT_DIR%\*.png" 2>nul

set TOTAL_FONTS=0
set SUCCESS_COUNT=0

REM ============================================================================
REM Check for weight subfolders first (new multi-weight system)
REM ============================================================================
set HAS_SUBFOLDERS=0

for %%W in (regular bold light medium semibold thin black) do (
    if exist "%INPUT_DIR%\%%W" (
        set HAS_SUBFOLDERS=1
    )
)

if %HAS_SUBFOLDERS%==1 (
    echo Detected weight subfolders. Using multi-weight mode.
    echo.
    goto :MultiWeight
) else (
    echo No weight subfolders found. Using single-font mode.
    echo.
    echo TIP: For multi-weight support, create subfolders:
    echo      input\regular\   - Put regular weight .ttf here
    echo      input\bold\      - Put bold weight .ttf here
    echo.
    goto :SingleFont
)

REM ============================================================================
REM Single Font Mode (backwards compatible)
REM ============================================================================
:SingleFont

REM Find TTF files in input folder
set FONT_FILE=
for %%F in ("%INPUT_DIR%\*.ttf") do (
    set "FONT_FILE=%%~nxF"
    set "FONT_NAME=%%~nF"
    set "FONT_FULL_PATH=%%F"
)
for %%F in ("%INPUT_DIR%\*.otf") do (
    set "FONT_FILE=%%~nxF"
    set "FONT_NAME=%%~nF"
    set "FONT_FULL_PATH=%%F"
)

if "%FONT_FILE%"=="" (
    echo ERROR: No font files found!
    echo.
    echo Please either:
    echo   1. Put a .ttf file directly in input\
    echo   2. Or create weight subfolders: input\regular\, input\bold\, etc.
    echo.
    pause
    exit /b 1
)

echo Found font: %FONT_FILE%
echo.

REM Generate all sizes for single font
for %%S in (%FONT_SIZES%) do (
    set /a TOTAL_FONTS+=1
    echo [%%S pt] Generating gamedefault_%%S...
    call :CreateConfig "%%S" "%INPUT_DIR%\%FONT_FILE%" "%FONT_NAME%" "gamedefault_%%S"
    "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\gamedefault_%%S.fnt"
    if exist "%OUTPUT_DIR%\gamedefault_%%S.fnt" (
        echo         SUCCESS
        set /a SUCCESS_COUNT+=1
    ) else (
        echo         FAILED
    )
)

REM Generate default.fnt
set /a TOTAL_FONTS+=1
echo [12 pt] Generating default...
call :CreateConfig "12" "%INPUT_DIR%\%FONT_FILE%" "%FONT_NAME%" "default"
"%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\default.fnt"
if exist "%OUTPUT_DIR%\default.fnt" ( set /a SUCCESS_COUNT+=1 )

goto :Done

REM ============================================================================
REM Multi-Weight Mode
REM ============================================================================
:MultiWeight

REM Process each weight folder
for %%W in (regular bold light medium semibold thin black) do (
    if exist "%INPUT_DIR%\%%W" (
        echo.
        echo === Processing %%W weight ===
        
        REM Find font file in this weight folder
        set "WEIGHT_FONT="
        for %%F in ("%INPUT_DIR%\%%W\*.ttf") do (
            set "WEIGHT_FONT=%%F"
            set "WEIGHT_FONT_NAME=%%~nF"
        )
        for %%F in ("%INPUT_DIR%\%%W\*.otf") do (
            set "WEIGHT_FONT=%%F"
            set "WEIGHT_FONT_NAME=%%~nF"
        )
        
        if "!WEIGHT_FONT!"=="" (
            echo     WARNING: No font file in %%W folder, skipping...
        ) else (
            echo     Using: !WEIGHT_FONT_NAME!
            
            REM Generate all sizes for this weight
            for %%S in (%FONT_SIZES%) do (
                set /a TOTAL_FONTS+=1
                
                REM Regular weight uses standard naming, others get weight suffix
                if "%%W"=="regular" (
                    set "OUT_NAME=gamedefault_%%S"
                ) else (
                    set "OUT_NAME=gamedefault_%%W_%%S"
                )
                
                echo     [%%S pt] Generating !OUT_NAME!...
                call :CreateConfig "%%S" "!WEIGHT_FONT!" "!WEIGHT_FONT_NAME!" "!OUT_NAME!"
                "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\!OUT_NAME!.fnt"
                if exist "%OUTPUT_DIR%\!OUT_NAME!.fnt" (
                    echo             SUCCESS
                    set /a SUCCESS_COUNT+=1
                ) else (
                    echo             FAILED
                )
            )
        )
    )
)

REM Generate default.fnt from regular if exists
if exist "%INPUT_DIR%\regular" (
    for %%F in ("%INPUT_DIR%\regular\*.ttf") do (
        set "REG_FONT=%%F"
        set "REG_NAME=%%~nF"
    )
    for %%F in ("%INPUT_DIR%\regular\*.otf") do (
        set "REG_FONT=%%F"
        set "REG_NAME=%%~nF"
    )
    
    if not "!REG_FONT!"=="" (
        set /a TOTAL_FONTS+=1
        echo.
        echo [12 pt] Generating default.fnt from regular...
        call :CreateConfig "12" "!REG_FONT!" "!REG_NAME!" "default"
        "%BMFONT_PATH%" -c "%TEMP_CONFIG%" -o "%OUTPUT_DIR%\default.fnt"
        if exist "%OUTPUT_DIR%\default.fnt" ( set /a SUCCESS_COUNT+=1 )
    )
)

goto :Done

REM ============================================================================
REM Done
REM ============================================================================
:Done

REM Cleanup temp config
del "%TEMP_CONFIG%" 2>nul

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
echo 2. Update client\scripts\lua\font.clu to use new fonts:
echo.
echo    -- Regular fonts (dialog, system, hints)
echo    DEFAULT_FONT = UI_CreateFont( "gamedefault", 12, 12, 0 )
echo.
echo    -- Bold fonts (player names, titles)
echo    BOLD_FONT_14 = UI_CreateFont( "gamedefault_bold", 14, 14, 0 )
echo    BOLD_FONT_16 = UI_CreateFont( "gamedefault_bold", 16, 16, 0 )
echo.
pause
exit /b 0

REM ============================================================================
REM Function: CreateConfig
REM ============================================================================
:CreateConfig
set CFG_SIZE=%~1
set CFG_FONT_PATH=%~2
set CFG_FONT_NAME=%~3
set CFG_OUTPUT=%~4

(
echo # AngelCode Bitmap Font Generator configuration file
echo fileVersion=1
echo.
echo # font settings
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
