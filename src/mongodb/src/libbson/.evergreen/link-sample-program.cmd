REM Supported/used environment variables:
REM   LINK_STATIC              Whether to statically link to libbson

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
%TAR% xf ..\..\libbson.tar.gz -C . --strip-components=1

if "%LINK_STATIC%"=="1" (
  %CMAKE% -G "Visual Studio 14 2015 Win64" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DENABLE_TESTS=OFF .
) else (
  %CMAKE% -G "Visual Studio 14 2015 Win64" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DENABLE_TESTS=OFF -DENABLE_STATIC=OFF .
)

msbuild.exe /m ALL_BUILD.vcxproj
msbuild.exe INSTALL.vcxproj

echo off

rem Notice that the dll goes in "bin".
set DLL=%INSTALL_DIR%\bin\libbson-1.0.dll
if not exist %DLL% (
  echo %DLL% is missing!
  exit /B 1
)

rem Shim library around the DLL.
set SHIM=%INSTALL_DIR%\lib\bson-1.0.lib
if not exist %SHIM% (
  echo %SHIM% is missing!
  exit /B 1
) else (
  echo %SHIM% check ok
)

if not exist %INSTALL_DIR%\lib\pkgconfig\libbson-1.0.pc (
  echo libbson-1.0.pc missing!
  exit /B 1
) else (
  echo libbson-1.0.pc check ok
)
if not exist %INSTALL_DIR%\lib\cmake\libbson-1.0\libbson-1.0-config.cmake (
  echo libbson-1.0-config.cmake missing!
  exit /B 1
) else (
  echo libbson-1.0-config.cmake check ok
)
if not exist %INSTALL_DIR%\lib\cmake\libbson-1.0\libbson-1.0-config-version.cmake (
  echo libbson-1.0-config-version.cmake missing!
  exit /B 1
) else (
  echo libbson-1.0-config-version.cmake check ok
)

if "%LINK_STATIC%"=="1" (
  if not exist %INSTALL_DIR%\lib\bson-static-1.0.lib (
    echo bson-static-1.0.lib missing!
    exit /B 1
  ) else (
    echo bson-static-1.0.lib check ok
  )
  if not exist %INSTALL_DIR%\lib\pkgconfig\libbson-static-1.0.pc (
    echo libbson-static-1.0.pc missing!
    exit /B 1
  ) else (
    echo libbson-static-1.0.pc check ok
  )
  if not exist %INSTALL_DIR%\lib\cmake\libbson-static-1.0\libbson-static-1.0-config.cmake (
    echo libbson-static-1.0-config.cmake missing!
    exit /B 1
  ) else (
    echo libbson-static-1.0-config.cmake check ok
  )
  if not exist %INSTALL_DIR%\lib\cmake\libbson-static-1.0\libbson-static-1.0-config-version.cmake (
    echo libbson-static-1.0-config-version.cmake missing!
    exit /B 1
  ) else (
    echo libbson-static-1.0-config-version.cmake check ok
  )
) else (
  if exist %INSTALL_DIR%\lib\bson-static-1.0.lib (
    echo bson-static-1.0.lib should not be installed!
    exit /B 1
  ) else (
    echo bson-static-1.0.lib check ok
  )
  if exist %INSTALL_DIR%\lib\pkgconfig\libbson-static-1.0.pc (
    echo libbson-static-1.0.pc should not be installed!
    exit /B 1
  ) else (
    echo libbson-static-1.0.pc check ok
  )
  if exist %INSTALL_DIR%\lib\cmake\libbson-static-1.0\libbson-static-1.0-config.cmake (
    echo libbson-static-1.0-config.cmake should not be installed!
    exit /B 1
  ) else (
    echo libbson-static-1.0-config.cmake check ok
  )
  if exist %INSTALL_DIR%\lib\cmake\libbson-static-1.0\libbson-static-1.0-config-version.cmake (
    echo libbson-static-1.0-config-version.cmake should not be installed!
    exit /B 1
  ) else (
    echo libbson-static-1.0-config-version.cmake check ok
  )
)

echo on

cd %SRCROOT%

rem Test our CMake package config file with CMake's find_package command.
set EXAMPLE_DIR=%SRCROOT%\examples\cmake\find_package

if "%LINK_STATIC%"=="1" (
  set EXAMPLE_DIR="%EXAMPLE_DIR%_static"
)

cd %EXAMPLE_DIR%
%CMAKE% -G "Visual Studio 14 2015 Win64" -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake .
msbuild.exe ALL_BUILD.vcxproj

rem Yes, they should've named it "dependencies".
dumpbin.exe /dependents Debug\hello_bson.exe

Debug\hello_bson.exe
