@echo off
set MCL_DIR=./mcl
if "%1"=="-s" (
  echo use static lib
  set LOCAL_CFLAGS=%BLS_CFLAGS% /DBLS_DONT_EXPORT /DMCL_DONT_EXPORT
) else if "%1"=="-d" (
  echo use dynamic lib
) else (
  echo "mk (-s|-d) <source file>"
  goto exit
)
set LOCAL_CFLAGS=%LOCAL_CFLAGS% -I %MCL_DIR%/include
set SRC=%2
set EXE=%SRC:.cpp=.exe%
set EXE=%EXE:.c=.exe%
set EXE=%EXE:test\=bin\%
set EXE=%EXE:sample\=bin\%
echo cl %LOCAL_CFLAGS% %2 /Fe:%EXE% /link %LDFLAGS%
cl %LOCAL_CFLAGS% %2 /Fe:%EXE% /link %LDFLAGS%

:exit
