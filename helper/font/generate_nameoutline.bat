@echo off
REM Simple script to generate nameoutline font

setlocal

set BMFONT_PATH=%~dp0bmfont64.exe
set INPUT_DIR=%~dp0input
set OUTPUT_DIR=%~dp0output
set CONFIG_FILE=%~dp0temp_nameoutline.bmfc

echo ============================================
echo Generating nameoutline.fnt with pre-baked outline
echo ============================================
echo.

REM Create the BMFont config file
echo # BMFont config for nameoutline > "%CONFIG_FILE%"
echo fileVersion=1 >> "%CONFIG_FILE%"
echo fontName=Metropolis-Black >> "%CONFIG_FILE%"
echo fontFile=%INPUT_DIR%\gamedefaultmidblack.ttf >> "%CONFIG_FILE%"
echo charSet=0 >> "%CONFIG_FILE%"
echo fontSize=-14 >> "%CONFIG_FILE%"
echo aa=1 >> "%CONFIG_FILE%"
echo scaleH=100 >> "%CONFIG_FILE%"
echo useSmoothing=1 >> "%CONFIG_FILE%"
echo isBold=0 >> "%CONFIG_FILE%"
echo isItalic=0 >> "%CONFIG_FILE%"
echo useUnicode=1 >> "%CONFIG_FILE%"
echo disableBoxChars=1 >> "%CONFIG_FILE%"
echo outputInvalidCharGlyph=0 >> "%CONFIG_FILE%"
echo dontIncludeKerningPairs=0 >> "%CONFIG_FILE%"
echo useHinting=1 >> "%CONFIG_FILE%"
echo renderFromOutline=1 >> "%CONFIG_FILE%"
echo useClearType=0 >> "%CONFIG_FILE%"
echo autoFitNumPages=0 >> "%CONFIG_FILE%"
echo autoFitFontSizeMin=0 >> "%CONFIG_FILE%"
echo autoFitFontSizeMax=0 >> "%CONFIG_FILE%"
echo paddingDown=1 >> "%CONFIG_FILE%"
echo paddingUp=1 >> "%CONFIG_FILE%"
echo paddingRight=1 >> "%CONFIG_FILE%"
echo paddingLeft=1 >> "%CONFIG_FILE%"
echo spacingHoriz=2 >> "%CONFIG_FILE%"
echo spacingVert=2 >> "%CONFIG_FILE%"
echo useFixedHeight=0 >> "%CONFIG_FILE%"
echo forceZero=0 >> "%CONFIG_FILE%"
echo widthPaddingFactor=0.00 >> "%CONFIG_FILE%"
echo outWidth=512 >> "%CONFIG_FILE%"
echo outHeight=512 >> "%CONFIG_FILE%"
echo outBitDepth=32 >> "%CONFIG_FILE%"
echo fontDescFormat=0 >> "%CONFIG_FILE%"
echo fourChnlPacked=0 >> "%CONFIG_FILE%"
echo textureFormat=png >> "%CONFIG_FILE%"
echo textureCompression=0 >> "%CONFIG_FILE%"
echo alphaChnl=0 >> "%CONFIG_FILE%"
echo redChnl=4 >> "%CONFIG_FILE%"
echo greenChnl=4 >> "%CONFIG_FILE%"
echo blueChnl=4 >> "%CONFIG_FILE%"
echo invA=0 >> "%CONFIG_FILE%"
echo invR=0 >> "%CONFIG_FILE%"
echo invG=0 >> "%CONFIG_FILE%"
echo invB=0 >> "%CONFIG_FILE%"
echo outlineThickness=1 >> "%CONFIG_FILE%"
echo chars=32-126,160-255,8211-8212,8216-8217,8220-8221,8226,8230,8364 >> "%CONFIG_FILE%"

echo Running BMFont...
"%BMFONT_PATH%" -c "%CONFIG_FILE%" -o "%OUTPUT_DIR%\nameoutline.fnt"

if exist "%OUTPUT_DIR%\nameoutline.fnt" (
    echo.
    echo SUCCESS! Generated:
    echo   - %OUTPUT_DIR%\nameoutline.fnt
    echo   - %OUTPUT_DIR%\nameoutline_0.png
    echo.
    echo Now copying to client/font/...
    copy /y "%OUTPUT_DIR%\nameoutline.fnt" "%~dp0..\..\client\font\"
    copy /y "%OUTPUT_DIR%\nameoutline_0.png" "%~dp0..\..\client\font\"
    echo.
    echo Done! Update font.clu:
    echo   NAME_FONT = "nameoutline"
    echo   NAME_FONT_HAS_OUTLINE = true
) else (
    echo.
    echo FAILED - Check if BMFont is installed correctly
)

del "%CONFIG_FILE%" 2>nul
echo.
pause
