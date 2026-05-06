@echo off
echo.
echo ================================================================================
echo   PKO Shader Compiler - Compiles HLSL to Obfuscated VSH
echo ================================================================================
echo.
echo Starting compilation...
echo.

luajit compile_shaders.lua

echo.
echo ================================================================================
echo   Done! Press any key to exit...
echo ================================================================================
pause > nul
