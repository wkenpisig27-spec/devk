@echo off
echo Stopping PKO Dev Website services...
echo.

echo Stopping nginx...
cd C:\nginx
nginx.exe -s stop
timeout /t 2 /nobreak >nul
echo [OK] nginx stopped
echo.

echo Stopping PHP-CGI...
taskkill /F /IM php-cgi.exe >nul 2>&1
timeout /t 1 /nobreak >nul
echo [OK] PHP-CGI stopped
echo.

echo ========================================
echo  All services stopped
echo ========================================
echo.
pause
