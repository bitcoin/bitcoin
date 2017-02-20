# Microsoft Developer Studio Project File - Name="bdb_brew" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=bdb_brew - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bdb_brew.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bdb_brew.mak" CFG="bdb_brew - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bdb_brew - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "bdb_brew - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bdb_brew - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release/bdb_brew"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release/bdb_brew"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "." /I ".." /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I ".." /I "$(BREWDIR)\inc" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# ADD BASE RSC /l 0xc09
# ADD RSC /l 0xc09
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "bdb_brew - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug/bdb_brew"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug/bdb_brew"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /I "." /I ".." /D "DIAGNOSTIC" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /X /I "." /I ".." /I "$(BREWDIR)\inc" /D "DIAGNOSTIC" /D "UNICODE" /D "_UNICODE" /D "_DEBUG" /D "AEE_SIMULATOR" /D "__NO_SYSTEM_INCLUDES" /FR /FD /c
# ADD BASE RSC /l 0xc09
# ADD RSC /l 0xc09
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "bdb_brew - Win32 Release"
# Name "bdb_brew - Win32 Debug"
# Begin Group "header_files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\brew_db.h
# End Source File
# Begin Source File

SOURCE=.\clib_port.h
# End Source File
# Begin Source File

SOURCE=.\db.h
# End Source File
# Begin Source File

SOURCE=.\db_config.h
# End Source File
# Begin Source File

SOURCE=.\db_int.h
# End Source File
# Begin Source File

SOURCE=.\errno.h
# End Source File
# End Group
# Begin Group "source_files"

# PROP Default_Filter ""
# Begin Source File
SOURCE=..\btree\bt_compact.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_compare.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_compress.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_conv.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_curadj.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_cursor.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_delete.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_method.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_open.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_put.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_rec.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_reclaim.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_recno.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_rsearch.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_search.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_split.c
# End Source File
# Begin Source File
SOURCE=..\btree\bt_stat.c
# End Source File
# Begin Source File
SOURCE=..\btree\btree_auto.c
# End Source File
# Begin Source File
SOURCE=..\clib\atol.c
# End Source File
# Begin Source File
SOURCE=..\clib\isalpha.c
# End Source File
# Begin Source File
SOURCE=..\clib\isdigit.c
# End Source File
# Begin Source File
SOURCE=..\clib\isprint.c
# End Source File
# Begin Source File
SOURCE=..\clib\isspace.c
# End Source File
# Begin Source File
SOURCE=..\clib\printf.c
# End Source File
# Begin Source File
SOURCE=..\clib\qsort.c
# End Source File
# Begin Source File
SOURCE=..\clib\rand.c
# End Source File
# Begin Source File
SOURCE=..\clib\strcasecmp.c
# End Source File
# Begin Source File
SOURCE=..\clib\strerror.c
# End Source File
# Begin Source File
SOURCE=..\clib\strncat.c
# End Source File
# Begin Source File
SOURCE=..\clib\strsep.c
# End Source File
# Begin Source File
SOURCE=..\clib\strtol.c
# End Source File
# Begin Source File
SOURCE=..\clib\time.c
# End Source File
# Begin Source File
SOURCE=..\common\crypto_stub.c
# End Source File
# Begin Source File
SOURCE=..\common\db_byteorder.c
# End Source File
# Begin Source File
SOURCE=..\common\db_compint.c
# End Source File
# Begin Source File
SOURCE=..\common\db_err.c
# End Source File
# Begin Source File
SOURCE=..\common\db_getlong.c
# End Source File
# Begin Source File
SOURCE=..\common\db_idspace.c
# End Source File
# Begin Source File
SOURCE=..\common\db_log2.c
# End Source File
# Begin Source File
SOURCE=..\common\db_shash.c
# End Source File
# Begin Source File
SOURCE=..\common\dbt.c
# End Source File
# Begin Source File
SOURCE=..\common\mkpath.c
# End Source File
# Begin Source File
SOURCE=..\common\zerofill.c
# End Source File
# Begin Source File
SOURCE=..\db\crdel_auto.c
# End Source File
# Begin Source File
SOURCE=..\db\crdel_rec.c
# End Source File
# Begin Source File
SOURCE=..\db\db.c
# End Source File
# Begin Source File
SOURCE=..\db\db_am.c
# End Source File
# Begin Source File
SOURCE=..\db\db_auto.c
# End Source File
# Begin Source File
SOURCE=..\db\db_cam.c
# End Source File
# Begin Source File
SOURCE=..\db\db_cds.c
# End Source File
# Begin Source File
SOURCE=..\db\db_conv.c
# End Source File
# Begin Source File
SOURCE=..\db\db_dispatch.c
# End Source File
# Begin Source File
SOURCE=..\db\db_dup.c
# End Source File
# Begin Source File
SOURCE=..\db\db_iface.c
# End Source File
# Begin Source File
SOURCE=..\db\db_join.c
# End Source File
# Begin Source File
SOURCE=..\db\db_meta.c
# End Source File
# Begin Source File
SOURCE=..\db\db_method.c
# End Source File
# Begin Source File
SOURCE=..\db\db_open.c
# End Source File
# Begin Source File
SOURCE=..\db\db_overflow.c
# End Source File
# Begin Source File
SOURCE=..\db\db_pr.c
# End Source File
# Begin Source File
SOURCE=..\db\db_rec.c
# End Source File
# Begin Source File
SOURCE=..\db\db_reclaim.c
# End Source File
# Begin Source File
SOURCE=..\db\db_remove.c
# End Source File
# Begin Source File
SOURCE=..\db\db_rename.c
# End Source File
# Begin Source File
SOURCE=..\db\db_ret.c
# End Source File
# Begin Source File
SOURCE=..\db\db_setid.c
# End Source File
# Begin Source File
SOURCE=..\db\db_setlsn.c
# End Source File
# Begin Source File
SOURCE=..\db\db_sort_multiple.c
# End Source File
# Begin Source File
SOURCE=..\db\db_stati.c
# End Source File
# Begin Source File
SOURCE=..\db\db_truncate.c
# End Source File
# Begin Source File
SOURCE=..\db\db_upg.c
# End Source File
# Begin Source File
SOURCE=..\db\db_vrfy_stub.c
# End Source File
# Begin Source File
SOURCE=..\dbreg\dbreg.c
# End Source File
# Begin Source File
SOURCE=..\dbreg\dbreg_auto.c
# End Source File
# Begin Source File
SOURCE=..\dbreg\dbreg_rec.c
# End Source File
# Begin Source File
SOURCE=..\dbreg\dbreg_stat.c
# End Source File
# Begin Source File
SOURCE=..\dbreg\dbreg_util.c
# End Source File
# Begin Source File
SOURCE=..\env\env_alloc.c
# End Source File
# Begin Source File
SOURCE=..\env\env_config.c
# End Source File
# Begin Source File
SOURCE=..\env\env_failchk.c
# End Source File
# Begin Source File
SOURCE=..\env\env_file.c
# End Source File
# Begin Source File
SOURCE=..\env\env_method.c
# End Source File
# Begin Source File
SOURCE=..\env\env_name.c
# End Source File
# Begin Source File
SOURCE=..\env\env_open.c
# End Source File
# Begin Source File
SOURCE=..\env\env_recover.c
# End Source File
# Begin Source File
SOURCE=..\env\env_region.c
# End Source File
# Begin Source File
SOURCE=..\env\env_register.c
# End Source File
# Begin Source File
SOURCE=..\env\env_sig.c
# End Source File
# Begin Source File
SOURCE=..\env\env_stat.c
# End Source File
# Begin Source File
SOURCE=..\fileops\fileops_auto.c
# End Source File
# Begin Source File
SOURCE=..\fileops\fop_basic.c
# End Source File
# Begin Source File
SOURCE=..\fileops\fop_rec.c
# End Source File
# Begin Source File
SOURCE=..\fileops\fop_util.c
# End Source File
# Begin Source File
SOURCE=..\hash\hash_func.c
# End Source File
# Begin Source File
SOURCE=..\hash\hash_stub.c
# End Source File
# Begin Source File
SOURCE=..\hmac\hmac.c
# End Source File
# Begin Source File
SOURCE=..\hmac\sha1.c
# End Source File
# Begin Source File
SOURCE=..\lock\lock_stub.c
# End Source File
# Begin Source File
SOURCE=..\log\log.c
# End Source File
# Begin Source File
SOURCE=..\log\log_archive.c
# End Source File
# Begin Source File
SOURCE=..\log\log_compare.c
# End Source File
# Begin Source File
SOURCE=..\log\log_debug.c
# End Source File
# Begin Source File
SOURCE=..\log\log_get.c
# End Source File
# Begin Source File
SOURCE=..\log\log_method.c
# End Source File
# Begin Source File
SOURCE=..\log\log_put.c
# End Source File
# Begin Source File
SOURCE=..\log\log_stat.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_alloc.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_bh.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_fget.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_fmethod.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_fopen.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_fput.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_fset.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_method.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_mvcc.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_region.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_register.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_resize.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_stat.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_sync.c
# End Source File
# Begin Source File
SOURCE=..\mp\mp_trickle.c
# End Source File
# Begin Source File
SOURCE=..\mutex\mut_stub.c
# End Source File
# Begin Source File
SOURCE=..\os\os_alloc.c
# End Source File
# Begin Source File
SOURCE=..\os\os_cpu.c
# End Source File
# Begin Source File
SOURCE=..\os\os_fid.c
# End Source File
# Begin Source File
SOURCE=..\os\os_flock.c
# End Source File
# Begin Source File
SOURCE=..\os\os_getenv.c
# End Source File
# Begin Source File
SOURCE=..\os\os_map.c
# End Source File
# Begin Source File
SOURCE=..\os\os_root.c
# End Source File
# Begin Source File
SOURCE=..\os\os_rpath.c
# End Source File
# Begin Source File
SOURCE=..\os\os_stack.c
# End Source File
# Begin Source File
SOURCE=..\os\os_tmpdir.c
# End Source File
# Begin Source File
SOURCE=..\os\os_uid.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\ctime.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\fclose.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\fgetc.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\fgets.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\fopen.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\fwrite.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\getcwd.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\globals.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\localtime.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_abort.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_abs.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_clock.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_config.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_dir.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_errno.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_handle.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_mkdir.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_open.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_pid.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_rename.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_rw.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_seek.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_stat.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_truncate.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_unlink.c
# End Source File
# Begin Source File
SOURCE=..\os_brew\os_yield.c
# End Source File
# Begin Source File
SOURCE=..\qam\qam_stub.c
# End Source File
# Begin Source File
SOURCE=..\rep\rep_stub.c
# End Source File
# Begin Source File
SOURCE=..\repmgr\repmgr_stub.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn_auto.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn_chkpt.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn_failchk.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn_method.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn_rec.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn_recover.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn_region.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn_stat.c
# End Source File
# Begin Source File
SOURCE=..\txn\txn_util.c
# End Source File
# End Group
# End Target
# End Project
