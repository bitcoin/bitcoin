# Microsoft Developer Studio Project File - Name="test_micro" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=test_micro - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "test_micro.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "test_micro.mak" CFG="test_micro - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "test_micro - Win32 Release x86" (based on "Win32 (x86) Console Application")
!MESSAGE "test_micro - Win32 Debug x86" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "test_micro - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Win32/Release"
# PROP BASE Intermediate_Dir "Win32/Release/test_micro"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32/Release"
# PROP Intermediate_Dir "Win32/Release/test_micro"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 libdb48.lib  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libcmt" /libpath:"$(OUTDIR)"

!ELSEIF  "$(CFG)" == "test_micro - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32/Debug"
# PROP BASE Intermediate_Dir "Win32/Debug/test_micro"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32/Debug"
# PROP Intermediate_Dir "Win32/Debug/test_micro"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE"  /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libdb48d.lib  kernel32.lib user32.lib advapi32.lib shell32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /nodefaultlib:"libcmtd" /fixed:no /libpath:"$(OUTDIR)"

!ENDIF 

# Begin Target

# Name "test_micro - Win32 Release x86"
# Name "test_micro - Win32 Debug x86"
# Begin Source File

SOURCE=..\common\util_arg.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_curalloc.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_curwalk.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_del.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_get.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_inmem.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_latch.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_load.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_open.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_put.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_recover.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_txn.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_txn_write.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_uname.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_util.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\b_workload.c
# End Source File
# Begin Source File

SOURCE=..\test_micro\source\test_micro.c
# End Source File
# Begin Source File

SOURCE=..\clib\getopt.c
# End Source File
# End Target
# End Project
