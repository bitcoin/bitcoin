@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo Clitboin Compilation Helper
echo ==========================================

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_DEV_CMD="

REM Strategy 1: Use vswhere to find any VS installation
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do (
        set "VS_INSTALL_DIR=%%i"
    )
)

if defined VS_INSTALL_DIR (
    if exist "!VS_INSTALL_DIR!\Common7\Tools\VsDevCmd.bat" (
        set "VS_DEV_CMD=!VS_INSTALL_DIR!\Common7\Tools\VsDevCmd.bat"
        goto :Found
    )
)

REM Strategy 2: Check standard paths (fallback)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    goto :Found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" (
    set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
    goto :Found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
    set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
    goto :Found
)

:NotFound
echo ERROR: Could not find Visual Studio Developer Command Prompt (VsDevCmd.bat).
echo Please verify you have Visual Studio 2022 installed.
exit /b 1

:Found
echo Found VS Dev Cmd: "!VS_DEV_CMD!"
echo Setting up build environment...
call "!VS_DEV_CMD!"

echo.
echo Configuring Clitboin build...
cmake -B build --preset vs2022-static
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed.
    echo Ensure "Desktop development with C++" workload is installed.
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
