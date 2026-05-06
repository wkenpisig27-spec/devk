@echo off
REM Build script for PKO License Tools
REM Requires Visual Studio Developer Command Prompt or MSVC in PATH

echo ============================================
echo Building PKO License Tools
echo ============================================
echo.

REM Check if cl.exe is available
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: cl.exe not found!
    echo Please run this from Visual Studio Developer Command Prompt
    echo or add MSVC to your PATH.
    echo.
    echo Alternative: Open Visual Studio Developer Command Prompt and run:
    echo   cd /d "%~dp0"
    echo   build.bat
    pause
    exit /b 1
)

echo Building LicenseGenerator.exe (Console)...
cl /nologo /EHsc /O2 /Fe:LicenseGenerator.exe LicenseGenerator.cpp /link crypt32.lib bcrypt.lib
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to build LicenseGenerator.exe
    pause
    exit /b 1
)
echo   Done!
echo.

echo Building LicenseGeneratorGUI.exe (GUI)...
cl /nologo /EHsc /O2 /Fe:LicenseGeneratorGUI.exe LicenseGeneratorGUI.cpp /link crypt32.lib bcrypt.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib shell32.lib /SUBSYSTEM:WINDOWS
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to build LicenseGeneratorGUI.exe
    pause
    exit /b 1
)
echo   Done!
echo.

echo Building GetHWID.exe...
cl /nologo /EHsc /O2 /Fe:GetHWID.exe GetHWID.cpp /link crypt32.lib iphlpapi.lib
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to build GetHWID.exe
    pause
    exit /b 1
)
echo   Done!
echo.

REM Cleanup object files
del *.obj 2>nul

echo ============================================
echo Build complete!
echo ============================================
echo.
echo Created:
echo   - LicenseGenerator.exe    (Console version - KEEP PRIVATE!)
echo   - LicenseGeneratorGUI.exe (GUI version - KEEP PRIVATE!)
echo   - GetHWID.exe             (Give to customers)
echo.
echo IMPORTANT: Change the secret key before distributing!
echo See README.md for details.
echo.
pause
