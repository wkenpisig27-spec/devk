@echo off
title PKODev Resource Editor
cd /d "%~dp0"

echo Checking Python...
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python is not installed or not in PATH.
    echo Please install Python 3.10+ from https://python.org
    pause
    exit /b 1
)

echo Installing/checking dependencies...
pip install -q -r requirements_win.txt

echo.
echo Starting PKODev Resource Editor (native window)...
python app_win.py
