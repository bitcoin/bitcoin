REM Supported/used environment variables:
REM   LINK_STATIC              Whether to statically link to libmongoc
REM   ENABLE_SSL               Enable SSL with Microsoft Secure Channel

rem Ensure Cygwin executables like sh.exe are not in PATH
rem set PATH=C:\Windows\system32;C:\Windows

echo on
echo

set TAR=C:\cygwin\bin\tar
set CMAKE=C:\cmake\bin\cmake

set SRCROOT=%CD%
set BUILD_DIR=%CD%\build-dir
rmdir /S /Q %BUILD_DIR%
mkdir %BUILD_DIR%

set INSTALL_DIR=%CD%\install-dir
rmdir /S /Q %INSTALL_DIR%
mkdir %INSTALL_DIR%

set PATH=%PATH%;"c:\Program Files (x86)\MSBuild\14.0\Bin"
set PATH=%PATH%;"c:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin"
set PATH=%PATH%;%INSTALL_DIR%\bin

cd %BUILD_DIR%
%TAR% xf ..\..\mongoc.tar.gz -C . --strip-components=1

rem Build libbson
cd src\libbson
%CMAKE% -G "Visual Studio 14 2015 Win64" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% .
msbuild.exe /m ALL_BUILD.vcxproj
msbuild.exe INSTALL.vcxproj

cd ..\..
rem Build libmongoc
if "%ENABLE_SSL%"=="1" (
  %CMAKE% -G "Visual Studio 14 2015 Win64" -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DENABLE_SSL=WINDOWS .
) else (
  %CMAKE% -G "Visual Studio 14 2015 Win64" -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DENABLE_SSL=OFF .
)

msbuild.exe /m ALL_BUILD.vcxproj
msbuild.exe INSTALL.vcxproj

call ..\.evergreen\check-installed-files.bat
if errorlevel 1 (
   exit /B %errorlevel%
)

rem Shim library around the DLL.
set SHIM=%INSTALL_DIR%\lib\mongoc-1.0.lib
if not exist %SHIM% (
  echo %SHIM% is missing!
  exit /B 1
) else (
  echo %SHIM% check ok
)

if not exist %INSTALL_DIR%\lib\mongoc-static-1.0.lib (
  echo mongoc-static-1.0.lib missing!
  exit /B 1
) else (
  echo mongoc-static-1.0.lib check ok
)

cd %SRCROOT%

rem Test our CMake package config file with CMake's find_package command.
set EXAMPLE_DIR=%SRCROOT%\examples\cmake\find_package

if "%LINK_STATIC%"=="1" (
  set EXAMPLE_DIR="%EXAMPLE_DIR%_static"
)

cd %EXAMPLE_DIR%

if "%ENABLE_SSL%"=="1" (
  cp ..\..\..\tests\x509gen\client.pem .
  cp ..\..\..\tests\x509gen\ca.pem .
  set MONGODB_EXAMPLE_URI="mongodb://localhost/?ssl=true&sslclientcertificatekeyfile=client.pem&sslcertificateauthorityfile=ca.pem&sslallowinvalidhostnames=true"
)

%CMAKE% -G "Visual Studio 14 2015 Win64" -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake .
msbuild.exe ALL_BUILD.vcxproj

rem Yes, they should've named it "dependencies".
dumpbin.exe /dependents Debug\hello_mongoc.exe

Debug\hello_mongoc.exe %MONGODB_EXAMPLE_URI%
