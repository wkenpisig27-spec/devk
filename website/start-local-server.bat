@echo off
title PKO Dev Website - Local Server
color 0A
echo ========================================
echo  PKO Dev Website - Quick Start
echo ========================================
echo.

REM Check if PHP is installed
if not exist "C:\php\php.exe" (
    echo [ERROR] PHP is not installed at C:\php\
    echo.
    echo Please follow SETUP_LOCAL.md to install PHP
    echo.
    pause
    exit /b 1
)

echo [OK] PHP found: 
C:\php\php.exe --version | findstr /R "^PHP"
echo.

REM Check if nginx is running
tasklist /FI "IMAGENAME eq nginx.exe" 2>NUL | find /I /N "nginx.exe">NUL
if %ERRORLEVEL% EQU 0 (
    echo [INFO] nginx is already running
) else (
    echo [INFO] Starting nginx...
    cd C:\nginx
    start "" nginx.exe
    timeout /t 2 /nobreak >nul
    echo [OK] nginx started
)
echo.

REM Check if PHP-CGI is running
netstat -ano | find ":9000" >nul
if %ERRORLEVEL% EQU 0 (
    echo [INFO] PHP-CGI is already running on port 9000
) else (
    echo [INFO] Starting PHP-CGI...
    start "PHP-CGI" C:\php\start-php-cgi.bat
    timeout /t 2 /nobreak >nul
    echo [OK] PHP-CGI started
)
echo.

echo ========================================
echo  Website Ready!
echo ========================================
echo.
echo  Visit: http://localhost:8080/
echo.
echo  Pages:
echo    - Home:        http://localhost:8080/
echo    - Login:       http://localhost:8080/login.php
echo    - Register:    http://localhost:8080/register.php
echo    - Leaderboard: http://localhost:8080/leaderboard.php
echo.
echo  Test PHP:      http://localhost:8080/test.php
echo.
echo ========================================
echo.
echo Press any key to open website in browser...
pause >nul

start http://localhost:8080/

echo.
echo Services are running in background windows.
echo To stop: Close PHP-CGI window and run "C:\nginx\nginx.exe -s stop"
echo.
pause
