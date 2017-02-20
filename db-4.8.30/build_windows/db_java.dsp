# Microsoft Developer Studio Project File - Name="db_java" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=db_java - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "db_java.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "db_java.mak" CFG="db_java - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "db_java - Win32 Release x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db_java - Win32 Debug x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "db_java - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Win32/Release"
# PROP BASE Intermediate_Dir "Win32/Release/db_java"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32/Release"
# PROP Intermediate_Dir "Win32/Release/db_java"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob2 /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb48.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libdb48.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /machine:I386 /out:"Win32/Release/libdb_java48.dll" /libpath:"$(OUTDIR)"
# Begin Custom Build - Compiling java files using javac
ProjDir=.

"force_compilation.txt" : "$(INTDIR)"
	echo compiling Berkeley DB classes
	mkdir "$(INTDIR)\classes"
	javac -g -d "$(INTDIR)\classes" -classpath "$(INTDIR)\classes" ..\java\src\com\sleepycat\db\*.java ..\java\src\com\sleepycat\db\internal\*.java ..\java\src\com\sleepycat\asm\*.java ..\java\src\com\sleepycat\bind\*.java ..\java\src\com\sleepycat\bind\serial\*.java ..\java\src\com\sleepycat\bind\tuple\*.java ..\java\src\com\sleepycat\collections\*.java ..\java\src\com\sleepycat\compat\*.java ..\java\src\com\sleepycat\persist\*.java ..\java\src\com\sleepycat\persist\evolve\*.java ..\java\src\com\sleepycat\persist\impl\*.java ..\java\src\com\sleepycat\persist\model\*.java ..\java\src\com\sleepycat\persist\raw\*.java ..\java\src\com\sleepycat\util\*.java ..\java\src\com\sleepycat\util\keyrange\*.java
	echo compiling examples
	mkdir "$(INTDIR)\classes.ex"
	javac -g -d "$(INTDIR)\classes.ex" -classpath "$(INTDIR)\classes;$(INTDIR)\classes.ex" ..\examples_java\src\db\*.java ..\examples_java\src\db\GettingStarted\*.java ..\examples_java\src\db\repquote\*.java ..\examples_java\src\collections\access\*.java ..\examples_java\src\collections\hello\*.java ..\examples_java\src\collections\ship\basic\*.java ..\examples_java\src\collections\ship\entity\*.java ..\examples_java\src\collections\ship\tuple\*.java ..\examples_java\src\collections\ship\sentity\*.java ..\examples_java\src\collections\ship\marshal\*.java ..\examples_java\src\collections\ship\factory\*.java ..\examples_java\src\persist\*.java ..\examples_java\src\persist\gettingStarted\*.java ..\examples_java\src\persist\txn\*.java
	echo creating jar files
	jar cf "$(OUTDIR)/db.jar" -C "$(INTDIR)\classes" .
	jar cf "$(OUTDIR)/dbexamples.jar" -C "$(INTDIR)\classes.ex" .
	echo Java build finished
# End Custom Build

!ELSEIF  "$(CFG)" == "db_java - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32/Debug"
# PROP BASE Intermediate_Dir "Win32/Debug/db_java"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32/Debug"
# PROP Intermediate_Dir "Win32/Debug/db_java"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "DB_CREATE_DLL" /D "DIAGNOSTIC" /D "CONFIG_TEST" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libdb48d.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libdb48d.lib kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /base:"0x13000000" /subsystem:windows /dll /export:__db_assert /pdb:none /debug /machine:I386 /out:"Win32/Debug/libdb_java48d.dll" /fixed:no /libpath:"$(OUTDIR)"
# Begin Custom Build - Compiling java files using javac
ProjDir=.

"force_compilation.txt" : "$(INTDIR)"
	echo compiling Berkeley DB classes
	mkdir "$(INTDIR)\classes"
	javac -g -d "$(INTDIR)\classes" -classpath "$(INTDIR)\classes" ..\java\src\com\sleepycat\db\*.java ..\java\src\com\sleepycat\db\internal\*.java ..\java\src\com\sleepycat\asm\*.java ..\java\src\com\sleepycat\bind\*.java ..\java\src\com\sleepycat\bind\serial\*.java ..\java\src\com\sleepycat\bind\tuple\*.java ..\java\src\com\sleepycat\collections\*.java ..\java\src\com\sleepycat\compat\*.java ..\java\src\com\sleepycat\persist\*.java ..\java\src\com\sleepycat\persist\evolve\*.java ..\java\src\com\sleepycat\persist\impl\*.java ..\java\src\com\sleepycat\persist\model\*.java ..\java\src\com\sleepycat\persist\raw\*.java ..\java\src\com\sleepycat\util\*.java ..\java\src\com\sleepycat\util\keyrange\*.java
	echo compiling examples
	mkdir "$(INTDIR)\classes.ex"
	javac -g -d "$(INTDIR)\classes.ex" -classpath "$(INTDIR)\classes;$(INTDIR)\classes.ex" ..\examples_java\src\db\*.java ..\examples_java\src\db\GettingStarted\*.java ..\examples_java\src\db\repquote\*.java ..\examples_java\src\collections\access\*.java ..\examples_java\src\collections\hello\*.java ..\examples_java\src\collections\ship\basic\*.java ..\examples_java\src\collections\ship\entity\*.java ..\examples_java\src\collections\ship\tuple\*.java ..\examples_java\src\collections\ship\sentity\*.java ..\examples_java\src\collections\ship\marshal\*.java ..\examples_java\src\collections\ship\factory\*.java ..\examples_java\src\persist\*.java ..\examples_java\src\persist\gettingStarted\*.java ..\examples_java\src\persist\txn\*.java
	echo creating jar files
	jar cf "$(OUTDIR)/db.jar" -C "$(INTDIR)\classes" .
	jar cf "$(OUTDIR)/dbexamples.jar" -C "$(INTDIR)\classes.ex" .
	echo Java build finished
# End Custom Build

!ENDIF 

# Begin Target

# Name "db_java - Win32 Release x86"
# Name "db_java - Win32 Debug x86"
# Begin Source File

SOURCE=..\libdb_java\db_java_wrap.c
# End Source File

# End Target
# End Project
