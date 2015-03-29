@ECHO ON
cd C:\MyProjects\Deps\qt-everywhere-opensource-src-5.3.2-64
if %errorlevel% NEQ 0 goto ERRORCLEANUP
SET OLDPATH=%PATH%
set VisualStudioVersion=11.0
REM first change the debug compiler options
perl -pi.bak -e "s#QMAKE_CFLAGS_RELEASE    = -O2 -MD#QMAKE_CFLAGS_RELEASE    = -O2 -MT#g;" qtbase\mkspecs\win32-msvc2012\qmake.conf
perl -pi.bak -e "s#QMAKE_CFLAGS_DEBUG      = -Zi -MDd#QMAKE_CFLAGS_DEBUG      = -Z7 -MTd#g;" qtbase\mkspecs\win32-msvc2012\qmake.conf
REM Now do 64 bit by setting environment to MSVC 64 bit
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86_amd64
call configure -debug-and-release -openssl-linked -opensource -confirm-license -platform win32-msvc2012 -nomake examples -nomake tests -static -I \MyProjects\Deps\openssl-1.0.2\inc32 -L \MyProjects\Deps\openssl-1.0.2\out64.dbg -L \MyProjects\Deps\openssl-1.0.2\out64 -l gdi32 -no-opengl -qt-zlib -qt-libpng -qt-libjpeg
nmake
REM put back the path
set PATH=%OLDPATH%
echo All finished!
pause
goto EOF
:ERRORCLEANUP
echo Something went wrong, please check the directories in this batch file!
pause
:EOF