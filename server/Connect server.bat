@echo off
echo Starting PKO Server Components...
echo.

echo Starting Account Server...
START "AccountServer" AccountServer.exe AccountServer.cfg
timeout /t 3 /nobreak >nul

echo Starting Group Server...
START "GroupServer" GroupServer.exe GroupServer.cfg
timeout /t 5 /nobreak >nul

echo Starting Gate Server...
START "GateServer" GateServer.exe GateServer.cfg
timeout /t 3 /nobreak >nul

echo Starting Game Server...
START "GameServer" GameServer.exe GameServer.cfg

echo.
echo All server components started!
echo Wait for GateServer Connect.log: "Successfully logged into GroupServer!" before logging in.
pause
