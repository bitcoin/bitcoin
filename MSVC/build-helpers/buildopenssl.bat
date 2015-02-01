@ECHO ON
SET OLDPATH=%PATH%
cd C:\MyProjects\Deps\openssl-1.0.2
if %errorlevel% NEQ 0 goto ERRORCLEANUP
REM first change the debug compiler options
perl -pi.bak -e "s#/Zi#/Z7#g;" util/pl/VC-32.pl
perl -pi.bak -e "s#-Zi#-Z7#g;" configure
REM Now do 64 bit by setting environment to MSVC 64 bit
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86_amd64
REM compile debug mode first
call perl Configure no-asm debug-VC-WIN64A 
call ms\do_win64a
nmake -f ms\nt.mak clean
nmake -f ms\nt.mak 
REM Now do release mode
call perl Configure no-asm VC-WIN64A 
call ms\do_win64a
nmake -f ms\nt.mak clean
nmake -f ms\nt.mak 
REM clean up and move files
set PATH=%OLDPATH%
md out64
md out64.dbg
move /Y out32\* out64\
move /Y out32.dbg\* out64.dbg\
REM
REM Now do 32 bit by setting environment to MSVC 32 bit
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin\vcvars32.bat"
REM debug 32 first
call perl Configure debug-VC-WIN32 
call ms\do_nasm
nmake -f ms\nt.mak clean
nmake -f ms\nt.mak 
REM now do release mode
call perl Configure VC-WIN32 
call ms\do_nasm
nmake -f ms\nt.mak clean
nmake -f ms\nt.mak 
REM put back the path
set PATH=%OLDPATH%
echo All finished!
pause
goto EOF
:ERRORCLEANUP
echo Something went wrong, please check the directories in this batch file!
pause
:EOF
