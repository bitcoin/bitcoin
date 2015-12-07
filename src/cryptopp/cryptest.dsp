# Microsoft Developer Studio Project File - Name="cryptest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=cryptest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cryptest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cryptest.mak" CFG="cryptest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cryptest - Win32 DLL-Import Release" (based on "Win32 (x86) Console Application")
!MESSAGE "cryptest - Win32 DLL-Import Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "cryptest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "cryptest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF "$(CFG)" == "cryptest - Win32 DLL-Import Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cryptest___Win32_FIPS_140_Release"
# PROP BASE Intermediate_Dir "cryptest___Win32_FIPS_140_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "CT_DLL_Import_Release"
# PROP Intermediate_Dir "CT_DLL_Import_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /Gz /MT /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /Zm200 /c
# ADD CPP /nologo /G5 /Gz /MT /W3 /GR /GX /Zi /O1 /Ob2 /D "NDEBUG" /D "CRYPTOPP_IMPORTS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /Zm400 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib advapi32.lib Ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /OPT:NOWIN98
# ADD LINK32 Ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /out:"DLL_Release/cryptest.exe" /libpath:"DLL_Release" /OPT:NOWIN98 /OPT:REF /OPT:ICF
# SUBTRACT LINK32 /pdb:none /incremental:yes
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=echo This configuration requires cryptopp.dll.	echo You can build it yourself using the cryptdll project, or	echo obtain a pre-built, FIPS 140-2 validated DLL. If you build it yourself	echo the resulting DLL will not be considered FIPS validated	echo unless it undergoes FIPS validation.
# End Special Build Tool

!ELSEIF "$(CFG)" == "cryptest - Win32 DLL-Import Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cryptest___Win32_FIPS_140_Debug"
# PROP BASE Intermediate_Dir "cryptest___Win32_FIPS_140_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CT_DLL_Import_Debug"
# PROP Intermediate_Dir "CT_DLL_Import_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /Zm200 /c
# ADD CPP /nologo /G5 /Gz /MTd /W3 /GR /GX /Zi /Oi /D "_DEBUG" /D "CRYPTOPP_IMPORTS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /Zm400 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib advapi32.lib Ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /OPT:NOWIN98
# ADD LINK32 Ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /out:"DLL_Debug/cryptest.exe" /pdbtype:sept /libpath:"DLL_Debug" /OPT:NOWIN98
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=echo This configuration requires cryptopp.dll.	echo You can build it yourself using the cryptdll project, or	echo obtain a pre-built, FIPS 140-2 validated DLL. If you build it yourself	echo the resulting DLL will not be considered FIPS validated	echo unless it undergoes FIPS validation.
# End Special Build Tool

!ELSEIF "$(CFG)" == "cryptest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cryptes0"
# PROP BASE Intermediate_Dir "cryptes0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "CTRelease"
# PROP Intermediate_Dir "CTRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /Zi /O1 /Ob2 /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "WIN32" /YX /FD /Zm400 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 advapi32.lib Ws2_32.lib /nologo /subsystem:console /map /debug /machine:I386 /OPT:NOWIN98 /OPT:REF /OPT:ICF
# SUBTRACT LINK32 /pdb:none

!ELSEIF "$(CFG)" == "cryptest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cryptes1"
# PROP BASE Intermediate_Dir "cryptes1"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CTDebug"
# PROP Intermediate_Dir "CTDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /GR /GX /Zi /Oi /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "WIN32" /YX /FD /Zm400 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 advapi32.lib Ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /OPT:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "cryptest - Win32 DLL-Import Release"
# Name "cryptest - Win32 DLL-Import Debug"
# Name "cryptest - Win32 Release"
# Name "cryptest - Win32 Debug"
# Begin Group "Source Code"

# PROP Default_Filter ".cpp;.h"
# Begin Source File

SOURCE=.\adhoc.cpp
# End Source File
# Begin Source File

SOURCE=.\bench.cpp
# End Source File
# Begin Source File

SOURCE=.\bench.h
# End Source File
# Begin Source File

SOURCE=.\bench2.cpp
# End Source File
# Begin Source File

SOURCE=.\datatest.cpp
# End Source File
# Begin Source File

SOURCE=.\dlltest.cpp
# End Source File
# Begin Source File

SOURCE=.\fipsalgt.cpp
# End Source File
# Begin Source File

SOURCE=.\regtest.cpp
# End Source File
# Begin Source File

SOURCE=.\test.cpp
# End Source File
# Begin Source File

SOURCE=.\validat1.cpp
# End Source File
# Begin Source File

SOURCE=.\validat2.cpp
# End Source File
# Begin Source File

SOURCE=.\validat3.cpp
# End Source File
# Begin Source File

SOURCE=.\validate.h
# End Source File
# End Group
# End Target
# End Project
