# Microsoft Developer Studio Project File - Name="cryptdll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cryptdll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cryptdll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cryptdll.mak" CFG="cryptdll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cryptdll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cryptdll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF "$(CFG)" == "cryptdll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cryptdll___Win32_Release"
# PROP BASE Intermediate_Dir "cryptdll___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DLL_Release"
# PROP Intermediate_Dir "DLL_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CRYPTDLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G5 /MT /W3 /GR /GX /Zi /O1 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CRYPTOPP_EXPORTS" /D CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2=1 /D "USE_PRECOMPILED_HEADERS" /Yu"pch.h" /FD /Zm200 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib advapi32.lib /nologo /dll /machine:I386
# ADD LINK32 advapi32.lib /nologo /base:"0x42900000" /dll /map /debug /machine:I386 /out:"DLL_Release/cryptopp.dll" /opt:ref
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
OutDir=.\DLL_Release
TargetPath=.\DLL_Release\cryptopp.dll
InputPath=.\DLL_Release\cryptopp.dll
SOURCE="$(InputPath)"

"$(OutDir)\cryptopp.mac.done" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	CTRelease\cryptest mac_dll $(TargetPath) 
	echo mac done > $(OutDir)\cryptopp.mac.done 
	
# End Custom Build

!ELSEIF "$(CFG)" == "cryptdll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cryptdll___Win32_Debug"
# PROP BASE Intermediate_Dir "cryptdll___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DLL_Debug"
# PROP Intermediate_Dir "DLL_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CRYPTDLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GR /GX /Zi /Oi /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CRYPTOPP_EXPORTS" /D CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2=1 /D "USE_PRECOMPILED_HEADERS" /Yu"pch.h" /FD /GZ /Zm200 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib advapi32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 advapi32.lib /nologo /base:"0x42900000" /dll /incremental:no /debug /machine:I386 /out:"DLL_Debug/cryptopp.dll" /opt:ref
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
OutDir=.\DLL_Debug
TargetPath=.\DLL_Debug\cryptopp.dll
InputPath=.\DLL_Debug\cryptopp.dll
SOURCE="$(InputPath)"

"$(OutDir)\cryptopp.mac.done" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	CTDebug\cryptest mac_dll $(TargetPath) 
	echo mac done > $(OutDir)\cryptopp.mac.done 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "cryptdll - Win32 Release"
# Name "cryptdll - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\algebra.cpp
# End Source File
# Begin Source File

SOURCE=.\algparam.cpp
# End Source File
# Begin Source File

SOURCE=.\asn.cpp
# End Source File
# Begin Source File

SOURCE=.\authenc.cpp
# End Source File
# Begin Source File

SOURCE=.\basecode.cpp
# End Source File
# Begin Source File

SOURCE=.\cbcmac.cpp
# End Source File
# Begin Source File

SOURCE=.\ccm.cpp
# End Source File
# Begin Source File

SOURCE=.\channels.cpp
# End Source File
# Begin Source File

SOURCE=.\cmac.cpp
# End Source File
# Begin Source File

SOURCE=.\cpu.cpp
# End Source File
# Begin Source File

SOURCE=.\cryptlib.cpp
# End Source File
# Begin Source File

SOURCE=.\des.cpp
# End Source File
# Begin Source File

SOURCE=.\dessp.cpp
# End Source File
# Begin Source File

SOURCE=.\dh.cpp
# End Source File
# Begin Source File

SOURCE=.\dll.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\dsa.cpp
# End Source File
# Begin Source File

SOURCE=.\ec2n.cpp
# End Source File
# Begin Source File

SOURCE=.\eccrypto.cpp
# End Source File
# Begin Source File

SOURCE=.\ecp.cpp
# End Source File
# Begin Source File

SOURCE=.\emsa2.cpp
# End Source File
# Begin Source File

SOURCE=.\eprecomp.cpp
# End Source File
# Begin Source File

SOURCE=.\files.cpp
# End Source File
# Begin Source File

SOURCE=.\filters.cpp
# End Source File
# Begin Source File

SOURCE=.\fips140.cpp
# End Source File
# Begin Source File

SOURCE=.\fipstest.cpp
# End Source File
# Begin Source File

SOURCE=.\gcm.cpp
# End Source File
# Begin Source File

SOURCE=.\gf2n.cpp
# End Source File
# Begin Source File

SOURCE=.\gfpcrypt.cpp
# End Source File
# Begin Source File

SOURCE=.\hex.cpp
# End Source File
# Begin Source File

SOURCE=.\hmac.cpp
# End Source File
# Begin Source File

SOURCE=.\hrtimer.cpp
# End Source File
# Begin Source File

SOURCE=.\integer.cpp
# End Source File
# Begin Source File

SOURCE=.\iterhash.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\misc.cpp
# End Source File
# Begin Source File

SOURCE=.\modes.cpp
# End Source File
# Begin Source File

SOURCE=.\mqueue.cpp
# End Source File
# Begin Source File

SOURCE=.\nbtheory.cpp
# End Source File
# Begin Source File

SOURCE=.\oaep.cpp
# End Source File
# Begin Source File

SOURCE=.\osrng.cpp
# End Source File
# Begin Source File

SOURCE=.\pch.cpp
# ADD CPP /Yc"pch.h"
# End Source File
# Begin Source File

SOURCE=.\pkcspad.cpp
# End Source File
# Begin Source File

SOURCE=.\pssr.cpp
# End Source File
# Begin Source File

SOURCE=.\pubkey.cpp
# End Source File
# Begin Source File

SOURCE=.\queue.cpp
# End Source File
# Begin Source File

SOURCE=.\randpool.cpp
# End Source File
# Begin Source File

SOURCE=.\rdtables.cpp
# End Source File
# Begin Source File

SOURCE=.\rijndael.cpp
# End Source File
# Begin Source File

SOURCE=.\rng.cpp
# End Source File
# Begin Source File

SOURCE=.\rsa.cpp
# End Source File
# Begin Source File

SOURCE=.\rw.cpp
# End Source File
# Begin Source File

SOURCE=.\sha.cpp
# End Source File
# Begin Source File

SOURCE=.\sha3.cpp
# End Source File
# Begin Source File

SOURCE=.\simple.cpp
# End Source File
# Begin Source File

SOURCE=.\skipjack.cpp
# End Source File
# Begin Source File

SOURCE=.\strciphr.cpp
# End Source File
# Begin Source File

SOURCE=.\trdlocal.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\aes.h
# End Source File
# Begin Source File

SOURCE=.\algebra.h
# End Source File
# Begin Source File

SOURCE=.\algparam.h
# End Source File
# Begin Source File

SOURCE=.\argnames.h
# End Source File
# Begin Source File

SOURCE=.\asn.h
# End Source File
# Begin Source File

SOURCE=.\authenc.h
# End Source File
# Begin Source File

SOURCE=.\basecode.h
# End Source File
# Begin Source File

SOURCE=.\cbcmac.h
# End Source File
# Begin Source File

SOURCE=.\ccm.h
# End Source File
# Begin Source File

SOURCE=.\channels.h
# End Source File
# Begin Source File

SOURCE=.\cmac.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\cryptlib.h
# End Source File
# Begin Source File

SOURCE=.\des.h
# End Source File
# Begin Source File

SOURCE=.\dh.h
# End Source File
# Begin Source File

SOURCE=.\dll.h
# End Source File
# Begin Source File

SOURCE=.\dsa.h
# End Source File
# Begin Source File

SOURCE=.\ec2n.h
# End Source File
# Begin Source File

SOURCE=.\eccrypto.h
# End Source File
# Begin Source File

SOURCE=.\ecp.h
# End Source File
# Begin Source File

SOURCE=.\eprecomp.h
# End Source File
# Begin Source File

SOURCE=.\files.h
# End Source File
# Begin Source File

SOURCE=.\filters.h
# End Source File
# Begin Source File

SOURCE=.\fips140.h
# End Source File
# Begin Source File

SOURCE=.\fltrimpl.h
# End Source File
# Begin Source File

SOURCE=.\gcm.h
# End Source File
# Begin Source File

SOURCE=.\gf2n.h
# End Source File
# Begin Source File

SOURCE=.\gfpcrypt.h
# End Source File
# Begin Source File

SOURCE=.\hex.h
# End Source File
# Begin Source File

SOURCE=.\hkdf.h
# End Source File
# Begin Source File

SOURCE=.\hmac.h
# End Source File
# Begin Source File

SOURCE=.\integer.h
# End Source File
# Begin Source File

SOURCE=.\iterhash.h
# End Source File
# Begin Source File

SOURCE=.\mdc.h
# End Source File
# Begin Source File

SOURCE=.\misc.h
# End Source File
# Begin Source File

SOURCE=.\modarith.h
# End Source File
# Begin Source File

SOURCE=.\modes.h
# End Source File
# Begin Source File

SOURCE=.\modexppc.h
# End Source File
# Begin Source File

SOURCE=.\mqueue.h
# End Source File
# Begin Source File

SOURCE=.\mqv.h
# End Source File
# Begin Source File

SOURCE=.\nbtheory.h
# End Source File
# Begin Source File

SOURCE=.\oaep.h
# End Source File
# Begin Source File

SOURCE=.\oids.h
# End Source File
# Begin Source File

SOURCE=.\osrng.h
# End Source File
# Begin Source File

SOURCE=.\pch.h
# End Source File
# Begin Source File

SOURCE=.\pkcspad.h
# End Source File
# Begin Source File

SOURCE=.\pubkey.h
# End Source File
# Begin Source File

SOURCE=.\queue.h
# End Source File
# Begin Source File

SOURCE=.\randpool.h
# End Source File
# Begin Source File

SOURCE=.\rijndael.h
# End Source File
# Begin Source File

SOURCE=.\rng.h
# End Source File
# Begin Source File

SOURCE=.\rsa.h
# End Source File
# Begin Source File

SOURCE=.\secblock.h
# End Source File
# Begin Source File

SOURCE=.\seckey.h
# End Source File
# Begin Source File

SOURCE=.\sha.h
# End Source File
# Begin Source File

SOURCE=.\sha3.h
# End Source File
# Begin Source File

SOURCE=.\simple.h
# End Source File
# Begin Source File

SOURCE=.\skipjack.h
# End Source File
# Begin Source File

SOURCE=.\smartptr.h
# End Source File
# Begin Source File

SOURCE=.\stdcpp.h
# End Source File
# Begin Source File

SOURCE=.\strciphr.h
# End Source File
# Begin Source File

SOURCE=.\trdlocal.h
# End Source File
# Begin Source File

SOURCE=.\words.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\cryptopp.rc
# End Source File
# End Target
# End Project
