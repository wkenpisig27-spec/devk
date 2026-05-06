@echo off
title PKODev Resource Editor - Build
cd /d "%~dp0"

echo ============================================
echo  PKODev Resource Editor - Build EXE
echo ============================================
echo.

:: Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found. Install Python 3.10+ from https://python.org
    pause
    exit /b 1
)

:: Install / update build dependencies
echo [1/3] Installing build dependencies...
pip install -q -r requirements_win.txt
if errorlevel 1 (
    echo ERROR: pip install failed.
    pause
    exit /b 1
)

:: Clean previous build
echo [2/3] Cleaning previous build artifacts...
if exist dist\ResourceEditor rmdir /s /q dist\ResourceEditor
if exist build rmdir /s /q build

:: Run PyInstaller
echo [3/3] Building ResourceEditor.exe ...
echo.
pyinstaller ^
    --name "ResourceEditor" ^
    --windowed ^
    --onedir ^
    --noconfirm ^
    --add-data "templates;templates" ^
    --hidden-import "engineio.async_drivers.threading" ^
    --hidden-import "flask" ^
    --hidden-import "webview" ^
    --collect-all "webview" ^
    app_win.py

if errorlevel 1 (
    echo.
    echo ERROR: Build failed. Check the output above for details.
    pause
    exit /b 1
)

echo.
echo ============================================
echo  Build complete!
echo  Output: dist\ResourceEditor\ResourceEditor.exe
echo.
echo  To run from anywhere, copy the entire
echo  dist\ResourceEditor\ folder, keeping it at
echo  the same level relative to server\resource\
echo  (i.e. at helper\resource_editor\ or adjust
echo  RESOURCE_DIR in app.py if you move it).
echo ============================================
echo.
pause
