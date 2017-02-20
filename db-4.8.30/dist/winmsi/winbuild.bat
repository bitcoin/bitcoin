@echo off
::	$Id$
::	Helper script to build Berkeley DB libraries and executables
::	using MSDEV
::

cd build_windows

:: One of these calls should find the desired batch file

call :TryBat "c:\Program Files\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat" && goto BATFOUND0

call :TryBat "c:\Program Files\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat" && goto BATFOUND1

call :TryBat "c:\Program Files\Microsoft Visual Studio .NET\Common7\Tools\vsvars32.bat" && goto BATFOUND2

call :TryBat "c:\Program Files\Microsoft Visual Studio.NET\Common7\Tools\vsvars32.bat" && goto BATFOUND3

goto BATNOTFOUND

:BATFOUND0
echo Using Visual Studio 2005
goto BATFOUND

:BATFOUND1
echo Using Visual Studio .NET 2003
goto BATFOUND

:BATFOUND2
echo Using Visual Studio .NET
echo *********** CHECK: Make sure the binaries are built with the same system libraries that are shipped.
goto BATFOUND

:BATFOUND3
echo Using Visual Studio.NET
echo *********** CHECK: Make sure the binaries are built with the same system libraries that are shipped.
goto BATFOUND

:BATFOUND

::intenv is used to set environment variables but this isn't used anymore
::devenv /useenv /build Release /project instenv ..\instenv\instenv.sln >> ..\winbld.out 2>&1
::if not %errorlevel% == 0 goto ERROR

echo Building Berkeley DB
devenv /useenv /build "Debug" Berkeley_DB.sln >> ..\winbld.out 2>&1
if not %errorlevel% == 0 goto ERROR
devenv /useenv /build "Release" Berkeley_DB.sln >> ..\winbld.out 2>&1
if not %errorlevel% == 0 goto ERROR
devenv /useenv /build "Debug" /project db_java Berkeley_DB.sln >> ..\winbld.out 2>&1
if not %errorlevel% == 0 goto ERROR
devenv /useenv /build "Release" /project db_java Berkeley_DB.sln >> ..\winbld.out 2>&1
if not %errorlevel% == 0 goto ERROR
devenv /useenv /build "Debug" /project db_tcl Berkeley_DB.sln >> ..\winbld.out 2>&1
if not %errorlevel% == 0 goto ERROR
devenv /useenv /build "Release" /project db_tcl Berkeley_DB.sln >> ..\winbld.out 2>&1
if not %errorlevel% == 0 goto ERROR
echo Building Berkeley DB CSharp API
devenv /useenv /build "Debug" BDB_dotNet.sln >> ..\winbld.out 2>&1
if not %errorlevel% == 0 goto ERROR
devenv /useenv /build "Release" BDB_dotNet.sln >> ..\winbld.out 2>&1
if not %errorlevel% == 0 goto ERROR


goto END


:ERROR
echo *********** ERROR: during win_build.bat *************
echo *********** ERROR: during win_build.bat *************  >> ..\winbld.err
exit 1
goto END

:NSTOP
echo *********** ERROR: win_build.bat stop requested *************
echo *********** ERROR: win_built.bat stop requested *************  >> ..\winbld.err
exit 2
goto END

:BATNOTFOUND
echo *********** ERROR: VC Config batch file not found *************
echo *********** ERROR: VC Config batch file not found *************  >> ..\winbld.err
exit 3
goto END

:: TryBat(BATPATH)
:: If the BATPATH exists, use it and return 0,
:: otherwise, return 1.

:TryBat
:: Filename = %1
if not exist %1 exit /b 1
call %1
exit /b 0
goto :EOF

:END
