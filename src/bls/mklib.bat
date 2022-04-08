@echo off
set MCL_DIR=./mcl
call setvar.bat
set MAKE_DLL=0
set BLS_ETH=0
if "%1"=="dll" (
  set MAKE_DLL=1
  shift
)
if "%1"=="eth" (
  echo make ETH mode
  set BLS_CFLAGS=%BLS_CFLAGS% -DBLS_ETH=1
  shift
)

set BLS_CFLAGS=%BLS_CFLAGS% /DMCL_NO_AUTOLINK

if %MAKE_DLL%==1 (
  echo make dynamic library DLL
  cl /c %BLS_CFLAGS% /Foobj/bls_c256.obj src/bls_c256.cpp /DBLS_NO_AUTOLINK
  cl /c %BLS_CFLAGS% /Foobj/bls_c384.obj src/bls_c384.cpp /DBLS_NO_AUTOLINK
  cl /c %BLS_CFLAGS% /Foobj/bls_c384_256.obj src/bls_c384_256.cpp /DBLS_NO_AUTOLINK
  cl /c %BLS_CFLAGS% /Foobj/fp.obj %MCL_DIR%/src/fp.cpp
  link /nologo /DLL /OUT:bin\bls256.dll obj\bls_c256.obj obj\fp.obj %LDFLAGS% /implib:lib\bls256.lib
  link /nologo /DLL /OUT:bin\bls384.dll obj\bls_c384.obj obj\fp.obj %LDFLAGS% /implib:lib\bls384.lib
  link /nologo /DLL /OUT:bin\bls384_256.dll obj\bls_c384_256.obj obj\fp.obj %LDFLAGS% /implib:lib\bls384_256.lib
) else (
  echo make static library LIB
  cl /c %BLS_CFLAGS% /Foobj/bls_c256.obj src/bls_c256.cpp
  cl /c %BLS_CFLAGS% /Foobj/bls_c384.obj src/bls_c384.cpp
  cl /c %BLS_CFLAGS% /Foobj/bls_c384_256.obj src/bls_c384_256.cpp
  cl /c %BLS_CFLAGS% /Foobj/fp.obj %MCL_DIR%/src/fp.cpp /DMCLBN_DONT_EXPORT /DMCLBN_FORCE_EXPORT
  lib /OUT:lib/bls256.lib /nodefaultlib obj/bls_c256.obj obj/fp.obj %LDFLAGS%
  lib /OUT:lib/bls384.lib /nodefaultlib obj/bls_c384.obj obj/fp.obj %LDFLAGS%
  lib /OUT:lib/bls384_256.lib /nodefaultlib obj/bls_c384_256.obj obj/fp.obj %LDFLAGS%
)
