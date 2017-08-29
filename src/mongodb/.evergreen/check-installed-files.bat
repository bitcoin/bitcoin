rem Validations shared by link-sample-program-msvc.bat and
rem link-sample-program-mingw.bat

rem Notice that the dll goes in "bin".
set DLL=%INSTALL_DIR%\bin\libmongoc-1.0.dll
if not exist %DLL% (
  echo %DLL% is missing!
  exit /B 1
)
if not exist %INSTALL_DIR%\lib\pkgconfig\libmongoc-1.0.pc (
  echo libmongoc-1.0.pc missing!
  exit /B 1
) else (
  echo libmongoc-1.0.pc check ok
)
if not exist %INSTALL_DIR%\lib\cmake\libmongoc-1.0\libmongoc-1.0-config.cmake (
  echo libmongoc-1.0-config.cmake missing!
  exit /B 1
) else (
  echo libmongoc-1.0-config.cmake check ok
)
if not exist %INSTALL_DIR%\lib\cmake\libmongoc-1.0\libmongoc-1.0-config-version.cmake (
  echo libmongoc-1.0-config-version.cmake missing!
  exit /B 1
) else (
  echo libmongoc-1.0-config-version.cmake check ok
)
if not exist %INSTALL_DIR%\lib\pkgconfig\libmongoc-static-1.0.pc (
  echo libmongoc-static-1.0.pc missing!
  exit /B 1
) else (
  echo libmongoc-static-1.0.pc check ok
)
if not exist %INSTALL_DIR%\lib\cmake\libmongoc-static-1.0\libmongoc-static-1.0-config.cmake (
  echo libmongoc-static-1.0-config.cmake missing!
  exit /B 1
) else (
  echo libmongoc-static-1.0-config.cmake check ok
)
if not exist %INSTALL_DIR%\lib\cmake\libmongoc-static-1.0\libmongoc-static-1.0-config-version.cmake (
  echo libmongoc-static-1.0-config-version.cmake missing!
  exit /B 1
) else (
  echo libmongoc-static-1.0-config-version.cmake check ok
)
