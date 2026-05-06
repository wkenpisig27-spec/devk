@echo off
echo Starting PKO Server Components...
echo.

echo Starting Account Server...
START "AccountServer" AccountServer.exe AccountServer.cfg

echo Starting Gate Server...
START "GateServer" GateServer.exe GateServer.cfg

echo Starting Group Server...
START "GroupServer" GroupServer.exe GroupServer.cfg

echo Starting Game Servers...
START "GameServer00" GameServer.exe GameServer.cfg

echo.
echo All server components started successfully!
pause