@ECHO ON
cd C:\MyProjects\Deps\boost_1_57_0
if %errorlevel% NEQ 0 goto ERRORCLEANUP
md stage\lib\x64
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86_amd64
call bootstrap.bat
bjam --toolset=msvc-11.0 address-model=64 -a link=static runtime-link=static cxxflags="-Zc:wchar_t- 
move /Y stage\lib\* stage\lib\x64\
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin\vcvars32.bat"
call bootstrap.bat
bjam --toolset=msvc-11.0 -a link=static runtime-link=static cxxflags="-Zc:wchar_t-  
echo All finished!
pause
goto EOF
:ERRORCLEANUP
echo Something went wrong, please check the directories in this batch file!
pause
:EOF

