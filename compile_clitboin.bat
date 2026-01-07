@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo Clitboin Compilation Helper
echo ==========================================

REM Attempt to find VsDevCmd.bat
set "VS_DEV_CMD="

REM Check common locations for Visual Studio 2022
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" (
    set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
    set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
)

REM Check common locations for Visual Studio 2019 (fallback)
if not defined VS_DEV_CMD (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
        set "VS_DEV_CMD=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
    )
)

if not defined VS_DEV_CMD (
    echo ERROR: Could not find Visual Studio Developer Command Prompt (VsDevCmd.bat).
    echo Please verify you have Visual Studio 2022 installed with "Desktop development with C++".
    echo.
    echo You may need to compile manually by opening "Developer PowerShell for VS 2022" and running:
    echo   cmake -B build --preset vs2022-static
    echo   cmake --build build --config Release
    exit /b 1
)

echo Found VS Dev Cmd: !VS_DEV_CMD!
echo Setting up build environment...
call "!VS_DEV_CMD!"

echo.
echo Configuring Clitboin build...
cmake -B build --preset vs2022-static
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed.
    exit /b 1
)

echo.
echo Compiling Clitboin (Release)...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo ERROR: Compilation failed.
    exit /b 1
)

echo.
echo ==========================================
echo Compilation Successful!
echo You can now run 'launch_clitboin.ps1'
echo ==========================================
