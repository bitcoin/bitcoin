@ECHO ON
cd c:\MyProjects\Deps\miniupnpc
if %errorlevel% NEQ 0 goto ERRORCLEANUP
copy miniupnpcstrings.h.in miniupnpcstrings.h
cd C:\MyProjects\Deps\miniupnpc\msvc
if %errorlevel% NEQ 0 goto ERRORCLEANUP
set VisualStudioVersion=11.0
copy C:\MyProjects\Deps\build-helpers\miniupnpc.vcxproj . /Y
if %errorlevel% NEQ 0 goto ERRORCLEANUP
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin\vcvars32.bat"
msbuild.exe  miniupnpc.vcxproj /t:clean /p:Configuration="Release" /p:Platform="Win32"
msbuild.exe  miniupnpc.vcxproj /t:clean /p:Configuration="Debug" /p:Platform="Win32"
msbuild.exe  miniupnpc.vcxproj /p:Configuration="Release" /p:Platform="Win32"
msbuild.exe  miniupnpc.vcxproj /p:Configuration="Debug" /p:Platform="Win32"
REM
REM now clean and build 64 bit
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86_amd64
msbuild.exe  miniupnpc.vcxproj /t:clean /p:Configuration="Release" /p:Platform="x64"
msbuild.exe  miniupnpc.vcxproj /t:clean /p:Configuration="Debug" /p:Platform="x64"
msbuild.exe  miniupnpc.vcxproj /p:Configuration="Release" /p:Platform="x64"
msbuild.exe  miniupnpc.vcxproj /p:Configuration="Debug" /p:Platform="x64"
echo All finished!
pause
goto EOF
:ERRORCLEANUP
echo Something went wrong, please check the directories in this batch file!
pause
:EOF