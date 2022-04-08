@echo off
call setvar.bat
if "%1"=="dll" (
  echo make dynamic library DLL
) else (
  echo make static library LIB
)
rem nasm -f win64 -D_WIN64 src\asm\low_x86-64.asm
rem lib /OUT:lib\mcl.lib /nodefaultlib fp.obj src\asm\low_x86-64.obj

if "%1"=="dll" (
  set CFLAGS=%CFLAGS% /DMCL_NO_AUTOLINK /DMCLBN_NO_AUTOLINK
)

echo cl /c %CFLAGS% src\fp.cpp /Foobj\fp.obj
     cl /c %CFLAGS% src\fp.cpp /Foobj\fp.obj
echo lib /nologo /OUT:lib\mcl.lib /nodefaultlib obj\fp.obj
     lib /nologo /OUT:lib\mcl.lib /nodefaultlib obj\fp.obj

if "%1"=="dll" (
  echo cl /c %CFLAGS% src\bn_c256.cpp /Foobj\bn_c256.obj
     cl /c %CFLAGS% src\bn_c256.cpp /Foobj\bn_c256.obj
  echo link /nologo /DLL /OUT:bin\mclbn256.dll obj\bn_c256.obj obj\fp.obj %LDFLAGS% /implib:lib\mclbn256.lib
     link /nologo /DLL /OUT:bin\mclbn256.dll obj\bn_c256.obj obj\fp.obj %LDFLAGS% /implib:lib\mclbn256.lib

  echo cl /c %CFLAGS% src\bn_c384_256.cpp /Foobj\bn_c384_256.obj
     cl /c %CFLAGS% src\bn_c384_256.cpp /Foobj\bn_c384_256.obj
  echo link /nologo /DLL /OUT:bin\mclbn384_256.dll obj\bn_c384_256.obj obj\fp.obj %LDFLAGS% /implib:lib\mclbn384_256.lib
     link /nologo /DLL /OUT:bin\mclbn384_256.dll obj\bn_c384_256.obj obj\fp.obj %LDFLAGS% /implib:lib\mclbn384_256.lib

  echo cl /c %CFLAGS% src\she_c384_256.cpp /Foobj\she_c384_256.obj /DMCL_NO_AUTOLINK
     cl /c %CFLAGS% src\she_c384_256.cpp /Foobj\she_c384_256.obj /DMCL_NO_AUTOLINK
  echo link /nologo /DLL /OUT:bin\mclshe384_256.dll obj\she_c384_256.obj obj\fp.obj %LDFLAGS% /implib:lib\mclshe_c384_256.lib
     link /nologo /DLL /OUT:bin\mclshe384_256.dll obj\she_c384_256.obj obj\fp.obj %LDFLAGS% /implib:lib\mclshe_c384_256.lib
) else (
  echo cl /c %CFLAGS% src\bn_c256.cpp /Foobj\bn_c256.obj
     cl /c %CFLAGS% src\bn_c256.cpp /Foobj\bn_c256.obj
     lib /nologo /OUT:lib\mclbn256.lib /nodefaultlib obj\bn_c256.obj lib\mcl.lib

  echo cl /c %CFLAGS% src\bn_c384_256.cpp /Foobj\bn_c384_256.obj
     cl /c %CFLAGS% src\bn_c384_256.cpp /Foobj\bn_c384_256.obj
     lib /nologo /OUT:lib\mclbn384_256.lib /nodefaultlib obj\bn_c384_256.obj lib\mcl.lib
)
