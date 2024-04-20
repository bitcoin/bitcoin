@echo off

:: Remove main.gson
%~dp0GeometrisExplorer_BG96.exe -d: /ufs/main.gson
if ERRORLEVEL 1 (
    echo Error: GeometrisExplorer_BG96.exe -d: /ufs/main.gson command failed
    exit 
) else (
    echo Success: Current script deleted from device
)

:: Copy new main.gson into device
%~dp0GeometrisExplorer_BG96.exe -f: %~dp0main.gson -p: /ufs
if ERRORLEVEL 1 (
    echo Error: GeometrisExplorer_BG96.exe -f: main.gson -p: /ufs command failed
    exit
) else (
    echo Success: Current script downloaded to device
)