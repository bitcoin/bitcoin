REM make-rdrand
@echo OFF

@cls

@del rdrand.obj rdrand-x86.obj rdrand-x64.obj rdrand-x86.lib rdrand-x64.lib /Q > nul

REM Visual Studio 2005
REM @set TOOLS32=C:\Program Files (x86)\Microsoft Visual Studio 8\VC\bin
REM @set TOOLS64=C:\Program Files (x86)\Microsoft Visual Studio 8\VC\bin\amd64

REM Visual Studio 2010
REM @set TOOLS32=C:\Program Files (x86)\Microsoft Visual Studio 10\VC\bin
REM @set TOOLS64=C:\Program Files (x86)\Microsoft Visual Studio 10\VC\bin\amd64

REM Visual Studio 2012
REM @set TOOLS32=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin
REM @set TOOLS64=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin\amd64

REM Visual Studio 2013
@set TOOLS32=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin
@set TOOLS64=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\amd64

@set MASM="%TOOLS32%\ml.exe"
@set MASM64="%TOOLS64%\ml64.exe"
@set DUMPBIN="%TOOLS32%\dumpbin.exe"
@set LIBTOOL="%TOOLS32%\lib.exe"

REM /W3 - Warning level
REM /Cx - Preserve case in external symbols
REM /Zi - Porgram Database information
@set ASFLAGS=/nologo /D_M_X86 /W3 /Cx /Zi /safeseh
@set ASFLAGS64=/nologo /D_M_X64 /W3 /Cx /Zi
@set LIBFLAGS=/nologo /SUBSYSTEM:CONSOLE

REM Use _M_X86 and _M_X64 becuase cl.exe uses them. It keeps preprocessor defines consistent.
echo ****************************************
echo Assembling rdrand.asm into rdrand-x86.obj
call %MASM% %ASFLAGS% /Fo rdrand-x86.obj /c rdrand.asm > nul
@IF NOT %ERRORLEVEL% EQU 0 (echo   Failed to assemble rdrand.asm with X86 && goto SCRIPT_FAILED)
echo Done...

echo ****************************************
echo Assembling rdrand.asm into rdrand-x64.obj
call %MASM64% %ASFLAGS64% /Fo rdrand-x64.obj /c rdrand.asm > nul
@IF NOT %ERRORLEVEL% EQU 0 (echo   Failed to assemble rdrand.asm with X64 && goto SCRIPT_FAILED)
echo Done...

echo ****************************************
echo Creating static library rdrand-x86.lib
call %LIBTOOL% %LIBFLAGS% /MACHINE:X86 /OUT:rdrand-x86.lib rdrand-x86.obj > nul
@IF NOT %ERRORLEVEL% EQU 0 (echo   Failed to create rdrand-x86.lib && goto SCRIPT_FAILED)
echo Done...

echo ****************************************
echo Creating static library rdrand-x64.lib
call %LIBTOOL% %LIBFLAGS% /MACHINE:X64 /OUT:rdrand-x64.lib rdrand-x64.obj > nul
@IF NOT %ERRORLEVEL% EQU 0 (echo   Failed to create rdrand-x64.lib && goto SCRIPT_FAILED)
echo Done...

goto SKIP_SYMBOL_DUMP_OBJ

echo ****************************************
echo Dumping symbols for rdrand-x86.obj
echo.
call %DUMPBIN% /SYMBOLS rdrand-x86.obj

echo ****************************************
echo Dumping symbols for rdrand-x64.obj
echo.
call %DUMPBIN% /SYMBOLS rdrand-x64.obj

:SKIP_SYMBOL_DUMP_OBJ

goto SKIP_SYMBOL_DUMP_LIB

echo ****************************************
echo Dumping symbols for rdrand-x86.lib
echo.
call %DUMPBIN% /SYMBOLS rdrand-x86.lib

echo ****************************************
echo Dumping symbols for rdrand-x64.lib
echo.
call %DUMPBIN% /SYMBOLS rdrand-x64.lib

:SKIP_SYMBOL_DUMP_LIB

goto SKIP_EXPORT_DUMP

echo ****************************************
echo Dumping exports for rdrand-x86.lib
echo.
call %DUMPBIN% /EXPORTS rdrand-x86.lib

echo ****************************************
echo Dumping exports for rdrand-x64.lib
echo.
call %DUMPBIN% /EXPORTS rdrand-x64.lib

:SKIP_EXPORT_DUMP

REM goto SKIP_DISASSEMBLY

echo ****************************************
echo Disassembling rdrand-x64.obj
echo.
call %DUMPBIN% /DISASM:NOBYTES rdrand-x64.obj

echo ****************************************
echo Disassembling rdrand-x86.obj
echo.
call %DUMPBIN% /DISASM:NOBYTES rdrand-x86.obj

:SKIP_DISASSEMBLY

:SCRIPT_FAILED
