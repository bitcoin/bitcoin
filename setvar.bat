@echo off
call %MCL_DIR%\setvar.bat
set BLS_CFLAGS=%CFLAGS% /I %MCL_DIR%\include /I ./
set BLS_LDFLAGS=%LDFLAGS%
echo BLS_CFLAGS=%BLS_CFLAGS%
echo BLS_LDFLAGS=%BLS_LDFLAGS%
