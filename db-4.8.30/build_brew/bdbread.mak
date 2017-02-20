#============================================================================
#  Name:
#    $(TARGET).MAK
#
#  Description:
#    Makefile to build the $(TARGET) downloadable module.
#
#   The following nmake targets are available in this makefile:
#
#     all           - make .elf and .mod image files (default)
#     clean         - delete object directory and image files
#     filename.o    - make object file
#
#   The above targets can be made with the following command:
#
#     nmake /f $(TARGET).mak [target]
#
#  Assumptions:
#    1. The environment variable ADSHOME is set to the root directory of the
#       arm tools.
#    2. The version of ADS is 1.2 or above.
#
#  Notes:
#    None.
#
#
#        Copyright © 2000-2003 QUALCOMM Incorporated.
#               All Rights Reserved.
#            QUALCOMM Proprietary/GTDR
#
#----------------------------------------------------------------------------
#============================================================================
BREW_HOME      =$(BREWDIR)
ARM_HOME       =$(ARMHOME)
TARGET         =D:\DB7588~1.BRE\BUILD_~1\bdb_brew
OBJS           =bdbread.o AEEModGen.o AEEAppGen.o bt_compact.o bt_compare.o bt_compress.o bt_conv.o bt_curadj.o bt_cursor.o bt_delete.o bt_method.o bt_open.o bt_put.o bt_rec.o bt_reclaim.o bt_recno.o bt_rsearch.o bt_search.o bt_split.o bt_stat.o btree_auto.o atol.o isalpha.o isdigit.o isprint.o isspace.o printf.o qsort.o rand.o strcasecmp.o strerror.o strncat.o strsep.o strtol.o time.o crypto_stub.o db_byteorder.o db_compint.o db_err.o db_getlong.o db_idspace.o db_log2.o db_shash.o dbt.o mkpath.o zerofill.o crdel_auto.o crdel_rec.o db.o db_am.o db_auto.o db_cam.o db_cds.o db_conv.o db_dispatch.o db_dup.o db_iface.o db_join.o db_meta.o db_method.o db_open.o db_overflow.o db_pr.o db_rec.o db_reclaim.o db_remove.o db_rename.o db_ret.o db_setid.o db_setlsn.o db_sort_multiple.o db_stati.o db_truncate.o db_upg.o db_vrfy_stub.o dbreg.o dbreg_auto.o dbreg_rec.o dbreg_stat.o dbreg_util.o env_alloc.o env_config.o env_failchk.o env_file.o env_method.o env_name.o env_open.o env_recover.o env_region.o env_register.o env_sig.o env_stat.o fileops_auto.o fop_basic.o fop_rec.o fop_util.o hash_func.o hash_stub.o hmac.o sha1.o lock_stub.o log.o log_archive.o log_compare.o log_debug.o log_get.o log_method.o log_put.o log_stat.o mp_alloc.o mp_bh.o mp_fget.o mp_fmethod.o mp_fopen.o mp_fput.o mp_fset.o mp_method.o mp_mvcc.o mp_region.o mp_register.o mp_resize.o mp_stat.o mp_sync.o mp_trickle.o mut_stub.o os_alloc.o os_cpu.o os_fid.o os_flock.o os_getenv.o os_map.o os_root.o os_rpath.o os_stack.o os_tmpdir.o os_uid.o ctime.o fclose.o fgetc.o fgets.o fopen.o fwrite.o getcwd.o globals.o localtime.o os_abort.o os_abs.o os_clock.o os_config.o os_dir.o os_errno.o os_handle.o os_mkdir.o os_open.o os_pid.o os_rename.o os_rw.o os_seek.o os_stat.o os_truncate.o os_unlink.o os_yield.o qam_stub.o rep_stub.o repmgr_stub.o txn.o txn_auto.o txn_chkpt.o txn_failchk.o txn_method.o txn_rec.o txn_recover.o txn_region.o txn_stat.o txn_util.o
APP_INCLUDES   =  -I ..\build_brew  -I ..

#-------------------------------------------------------------------------------
# Target file name and type definitions
#-------------------------------------------------------------------------------

EXETYPE    = elf                # Target image file format
MODULE     = mod                # Downloadable module extension

#-------------------------------------------------------------------------------
# Target compile time symbol definitions
#
# Tells the SDK source stuffs that we're building a dynamic app.
#-------------------------------------------------------------------------------

DYNAPP          = -DDYNAMIC_APP


#-------------------------------------------------------------------------------
# Software tool and environment definitions
#-------------------------------------------------------------------------------

AEESRCPATH = $(BREW_HOME)\src
AEEINCPATH = $(BREW_HOME)\inc

ARMBIN = $(ARM_HOME)\bin        # ARM ADS application directory
ARMINC = $(ARM_HOME)\include    # ARM ADS include file directory
ARMLIB = $(ARM_HOME)\lib        # ARM ADS library directory

ARMCC   = $(ARMBIN)\armcc       # ARM ADS ARM 32-bit inst. set ANSI C compiler
LD      = $(ARMBIN)\armlink     # ARM ADS linker
HEXTOOL = $(ARMBIN)\fromelf     # ARM ADS utility to create hex file from image

OBJ_CMD    = -o                 # Command line option to specify output filename

#-------------------------------------------------------------------------------
# Processor architecture options
#-------------------------------------------------------------------------------

CPU = -cpu ARM7TDMI             # ARM7TDMI target processor

#-------------------------------------------------------------------------------
# ARM Procedure Call Standard (APCS) options
#-------------------------------------------------------------------------------

ROPI     = ropi                 # Read-Only(code) Position independence
INTERWRK = interwork            # Allow ARM-Thumb interworking

APCS = -apcs /$(ROPI)/$(INTERWRK)/norwpi

#-------------------------------------------------------------------------------
# Additional compile time error checking options
#-------------------------------------------------------------------------------

CHK = -fa                       # Check for data flow anomolies

#-------------------------------------------------------------------------------
# Compiler output options
#-------------------------------------------------------------------------------

OUT = -c                        # Object file output only

#-------------------------------------------------------------------------------
# Compiler/assembler debug options
#-------------------------------------------------------------------------------

DBG = -g                        # Enable debug

#-------------------------------------------------------------------------------
# Compiler optimization options
#-------------------------------------------------------------------------------

OPT = -Ospace -O2               # Full compiler optimization for space

#-------------------------------------------------------------------------------
# Compiler code generation options
#-------------------------------------------------------------------------------

END = -littleend                # Compile for little endian memory architecture
ZA  = -zo                       # LDR may only access 32-bit aligned addresses

CODE = $(END) $(ZA)


#-------------------------------------------------------------------------------
# Include file search path options
#-------------------------------------------------------------------------------

INC = -I. -I$(AEEINCPATH) $(APP_INCLUDES)


#-------------------------------------------------------------------------------
# Compiler pragma emulation options
#-------------------------------------------------------------------------------


#-------------------------------------------------------------------------------
# Linker options
#-------------------------------------------------------------------------------

LINK_CMD = -o                    #Command line option to specify output file
                                 #on linking

ROPILINK = -ropi                 #Link image as Read-Only Position Independent

LINK_ORDER = -first AEEMod_Load

#-------------------------------------------------------------------------------
# HEXTOOL options
#-------------------------------------------------------------------------------

BINFORMAT = -bin


#-------------------------------------------------------------------------------
# Compiler flag definitions
#-------------------------------------------------------------------------------
NO_WARNING= -W

CFLAGS0 = $(OUT) $(DYNAPP) $(CPU) $(APCS) $(CODE) $(CHK) $(DBG)
CFLAGS  = $(NO_WARNING) $(CFLAGS0) $(OPT)

#-------------------------------------------------------------------------------
# Linker flag definitions
#-------------------------------------------------------------------------------

# the -entry flag is not really needed, but it keeps the linker from reporting
# warning L6305W (no entry point).  The address
LFLAGS = $(ROPILINK) -rwpi -entry 0x8000#

#----------------------------------------------------------------------------
# Default target
#----------------------------------------------------------------------------

all :  $(TARGET).$(MODULE)

#----------------------------------------------------------------------------
# Clean target
#----------------------------------------------------------------------------

# The object subdirectory, target image file, and target hex file are deleted.

clean :
        @echo ---------------------------------------------------------------
        @echo CLEAN
        -del /f $(OBJS)
        -del /f $(TARGET).$(EXETYPE)
        -del /f $(TARGET).$(MODULE)
        @echo ---------------------------------------------------------------

#============================================================================
#                           DEFAULT SUFFIX RULES
#============================================================================

# The following are the default suffix rules used to compile all objects that
# are not specifically included in one of the module specific rules defined
# in the next section.

# The following macros are used to specify the output object file, MSG_FILE
# symbol definition and input source file on the compile line in the rules
# defined below.

SRC_FILE = $(@F:.o=.c)                  # Input source file specification
OBJ_FILE = $(OBJ_CMD) $(@F)   # Output object file specification

.SUFFIXES :
.SUFFIXES : .o .dep .c

#--------------------------------------------------------------------------
# C code inference rules
#----------------------------------------------------------------------------

.c.o:
        @echo ---------------------------------------------------------------
        @echo OBJECT $(@F)
        $(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE) $(SRC_FILE)
        @echo ---------------------------------------------------------------

.c.mix:
        @echo ---------------------------------------------------------------
        @echo OBJECT $(@F)
        $(ARMCC) -S -fs $(CFLAGS) $(INC) $(OBJ_FILE) $<
        @echo ---------------------------------------------------------------


{$(AEESRCPATH)}.c.o:
        @echo ---------------------------------------------------------------
        @echo OBJECT $(@F)
        $(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE) $(AEESRCPATH)\$(SRC_FILE)
        @echo ---------------------------------------------------------------


#===============================================================================
#                           MODULE SPECIFIC RULES
#===============================================================================

APP_OBJS = $(OBJS)


#----------------------------------------------------------------------------
# Lib file targets
#----------------------------------------------------------------------------

$(TARGET).$(MODULE) : $(TARGET).$(EXETYPE)
        @echo ---------------------------------------------------------------
        @echo TARGET $@
        $(HEXTOOL)  $(TARGET).$(EXETYPE) $(BINFORMAT) $(TARGET).$(MODULE)

$(TARGET).$(EXETYPE) : $(APP_OBJS)
        @echo ---------------------------------------------------------------
        @echo TARGET $@
        $(LD) $(LINK_CMD) $(TARGET).$(EXETYPE) $(LFLAGS) $(APP_OBJS) $(LINK_ORDER)

#----------------------------------------------------------------------------
# Applet Specific Rules
#----------------------------------------------------------------------------


RULE1 = ..\clib
{$(RULE1)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE1)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE2 = ..\btree
{$(RULE2)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE2)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE3 = ..\db
{$(RULE3)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE3)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE4 = ..\common
{$(RULE4)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE4)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE5 = ..\os_brew
{$(RULE5)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE5)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE6 = ..\env
{$(RULE6)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE6)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE7 = ..\dbreg
{$(RULE7)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE7)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE8 = ..\fileops
{$(RULE8)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE8)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE9 = ..\hash
{$(RULE9)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE9)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE10 = ..\hmac
{$(RULE10)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE10)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE11 = ..\lock
{$(RULE11)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE11)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE12 = ..\log
{$(RULE12)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE12)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE13 = ..\mp
{$(RULE13)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE13)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE14 = ..\mutex
{$(RULE14)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE14)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE15 = ..\os
{$(RULE15)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE15)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE16 = ..\qam
{$(RULE16)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE16)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE17 = ..\rep
{$(RULE17)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE17)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE18 = ..\txn
{$(RULE18)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE18)\$(SRC_FILE)
	@echo ---------------------------------------------------------------


RULE19 = ..\xa
{$(RULE19)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE19)\$(SRC_FILE)
	@echo ---------------------------------------------------------------
RULE20 = ..\bdbread
{$(RULE20)}.c.o:
	@echo ---------------------------------------------------------------
	@echo OBJECT $(@F)
	$(ARMCC) $(CFLAGS) $(INC) $(OBJ_FILE)  $(RULE20)\$(SRC_FILE)
	@echo ---------------------------------------------------------------
	
# --------------------------------------------
# DEPENDENCY LIST, DO NOT EDIT BELOW THIS LINE
# --------------------------------------------

bdbread.o : ..\bdbread\bdbread.c
AEEModGen.o : ..\bdbread\AEEModGen.c
AEEAppGen.o : ..\bdbread\AEEAppGen.c
bt_compact.o:	..\btree\bt_compact.c
bt_compare.o:	..\btree\bt_compare.c
bt_compress.o:	..\btree\bt_compress.c
bt_conv.o:	..\btree\bt_conv.c
bt_curadj.o:	..\btree\bt_curadj.c
bt_cursor.o:	..\btree\bt_cursor.c
bt_delete.o:	..\btree\bt_delete.c
bt_method.o:	..\btree\bt_method.c
bt_open.o:	..\btree\bt_open.c
bt_put.o:	..\btree\bt_put.c
bt_rec.o:	..\btree\bt_rec.c
bt_reclaim.o:	..\btree\bt_reclaim.c
bt_recno.o:	..\btree\bt_recno.c
bt_rsearch.o:	..\btree\bt_rsearch.c
bt_search.o:	..\btree\bt_search.c
bt_split.o:	..\btree\bt_split.c
bt_stat.o:	..\btree\bt_stat.c
btree_auto.o:	..\btree\btree_auto.c
atol.o:	..\clib\atol.c
isalpha.o:	..\clib\isalpha.c
isdigit.o:	..\clib\isdigit.c
isprint.o:	..\clib\isprint.c
isspace.o:	..\clib\isspace.c
printf.o:	..\clib\printf.c
qsort.o:	..\clib\qsort.c
rand.o:	..\clib\rand.c
strcasecmp.o:	..\clib\strcasecmp.c
strerror.o:	..\clib\strerror.c
strncat.o:	..\clib\strncat.c
strsep.o:	..\clib\strsep.c
strtol.o:	..\clib\strtol.c
time.o:	..\clib\time.c
crypto_stub.o:	..\common\crypto_stub.c
db_byteorder.o:	..\common\db_byteorder.c
db_compint.o:	..\common\db_compint.c
db_err.o:	..\common\db_err.c
db_getlong.o:	..\common\db_getlong.c
db_idspace.o:	..\common\db_idspace.c
db_log2.o:	..\common\db_log2.c
db_shash.o:	..\common\db_shash.c
dbt.o:	..\common\dbt.c
mkpath.o:	..\common\mkpath.c
zerofill.o:	..\common\zerofill.c
crdel_auto.o:	..\db\crdel_auto.c
crdel_rec.o:	..\db\crdel_rec.c
db.o:	..\db\db.c
db_am.o:	..\db\db_am.c
db_auto.o:	..\db\db_auto.c
db_cam.o:	..\db\db_cam.c
db_cds.o:	..\db\db_cds.c
db_conv.o:	..\db\db_conv.c
db_dispatch.o:	..\db\db_dispatch.c
db_dup.o:	..\db\db_dup.c
db_iface.o:	..\db\db_iface.c
db_join.o:	..\db\db_join.c
db_meta.o:	..\db\db_meta.c
db_method.o:	..\db\db_method.c
db_open.o:	..\db\db_open.c
db_overflow.o:	..\db\db_overflow.c
db_pr.o:	..\db\db_pr.c
db_rec.o:	..\db\db_rec.c
db_reclaim.o:	..\db\db_reclaim.c
db_remove.o:	..\db\db_remove.c
db_rename.o:	..\db\db_rename.c
db_ret.o:	..\db\db_ret.c
db_setid.o:	..\db\db_setid.c
db_setlsn.o:	..\db\db_setlsn.c
db_sort_multiple.o:	..\db\db_sort_multiple.c
db_stati.o:	..\db\db_stati.c
db_truncate.o:	..\db\db_truncate.c
db_upg.o:	..\db\db_upg.c
db_vrfy_stub.o:	..\db\db_vrfy_stub.c
dbreg.o:	..\dbreg\dbreg.c
dbreg_auto.o:	..\dbreg\dbreg_auto.c
dbreg_rec.o:	..\dbreg\dbreg_rec.c
dbreg_stat.o:	..\dbreg\dbreg_stat.c
dbreg_util.o:	..\dbreg\dbreg_util.c
env_alloc.o:	..\env\env_alloc.c
env_config.o:	..\env\env_config.c
env_failchk.o:	..\env\env_failchk.c
env_file.o:	..\env\env_file.c
env_method.o:	..\env\env_method.c
env_name.o:	..\env\env_name.c
env_open.o:	..\env\env_open.c
env_recover.o:	..\env\env_recover.c
env_region.o:	..\env\env_region.c
env_register.o:	..\env\env_register.c
env_sig.o:	..\env\env_sig.c
env_stat.o:	..\env\env_stat.c
fileops_auto.o:	..\fileops\fileops_auto.c
fop_basic.o:	..\fileops\fop_basic.c
fop_rec.o:	..\fileops\fop_rec.c
fop_util.o:	..\fileops\fop_util.c
hash_func.o:	..\hash\hash_func.c
hash_stub.o:	..\hash\hash_stub.c
hmac.o:	..\hmac\hmac.c
sha1.o:	..\hmac\sha1.c
lock_stub.o:	..\lock\lock_stub.c
log.o:	..\log\log.c
log_archive.o:	..\log\log_archive.c
log_compare.o:	..\log\log_compare.c
log_debug.o:	..\log\log_debug.c
log_get.o:	..\log\log_get.c
log_method.o:	..\log\log_method.c
log_put.o:	..\log\log_put.c
log_stat.o:	..\log\log_stat.c
mp_alloc.o:	..\mp\mp_alloc.c
mp_bh.o:	..\mp\mp_bh.c
mp_fget.o:	..\mp\mp_fget.c
mp_fmethod.o:	..\mp\mp_fmethod.c
mp_fopen.o:	..\mp\mp_fopen.c
mp_fput.o:	..\mp\mp_fput.c
mp_fset.o:	..\mp\mp_fset.c
mp_method.o:	..\mp\mp_method.c
mp_mvcc.o:	..\mp\mp_mvcc.c
mp_region.o:	..\mp\mp_region.c
mp_register.o:	..\mp\mp_register.c
mp_resize.o:	..\mp\mp_resize.c
mp_stat.o:	..\mp\mp_stat.c
mp_sync.o:	..\mp\mp_sync.c
mp_trickle.o:	..\mp\mp_trickle.c
mut_stub.o:	..\mutex\mut_stub.c
os_alloc.o:	..\os\os_alloc.c
os_cpu.o:	..\os\os_cpu.c
os_fid.o:	..\os\os_fid.c
os_flock.o:	..\os\os_flock.c
os_getenv.o:	..\os\os_getenv.c
os_map.o:	..\os\os_map.c
os_root.o:	..\os\os_root.c
os_rpath.o:	..\os\os_rpath.c
os_stack.o:	..\os\os_stack.c
os_tmpdir.o:	..\os\os_tmpdir.c
os_uid.o:	..\os\os_uid.c
ctime.o:	..\os_brew\ctime.c
fclose.o:	..\os_brew\fclose.c
fgetc.o:	..\os_brew\fgetc.c
fgets.o:	..\os_brew\fgets.c
fopen.o:	..\os_brew\fopen.c
fwrite.o:	..\os_brew\fwrite.c
getcwd.o:	..\os_brew\getcwd.c
globals.o:	..\os_brew\globals.c
localtime.o:	..\os_brew\localtime.c
os_abort.o:	..\os_brew\os_abort.c
os_abs.o:	..\os_brew\os_abs.c
os_clock.o:	..\os_brew\os_clock.c
os_config.o:	..\os_brew\os_config.c
os_dir.o:	..\os_brew\os_dir.c
os_errno.o:	..\os_brew\os_errno.c
os_handle.o:	..\os_brew\os_handle.c
os_mkdir.o:	..\os_brew\os_mkdir.c
os_open.o:	..\os_brew\os_open.c
os_pid.o:	..\os_brew\os_pid.c
os_rename.o:	..\os_brew\os_rename.c
os_rw.o:	..\os_brew\os_rw.c
os_seek.o:	..\os_brew\os_seek.c
os_stat.o:	..\os_brew\os_stat.c
os_truncate.o:	..\os_brew\os_truncate.c
os_unlink.o:	..\os_brew\os_unlink.c
os_yield.o:	..\os_brew\os_yield.c
qam_stub.o:	..\qam\qam_stub.c
rep_stub.o:	..\rep\rep_stub.c
repmgr_stub.o:	..\repmgr\repmgr_stub.c
txn.o:	..\txn\txn.c
txn_auto.o:	..\txn\txn_auto.c
txn_chkpt.o:	..\txn\txn_chkpt.c
txn_failchk.o:	..\txn\txn_failchk.c
txn_method.o:	..\txn\txn_method.c
txn_rec.o:	..\txn\txn_rec.c
txn_recover.o:	..\txn\txn_recover.c
txn_region.o:	..\txn\txn_region.c
txn_stat.o:	..\txn\txn_stat.c
txn_util.o:	..\txn\txn_util.c
