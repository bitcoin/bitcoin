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
LIST OF OBJECT FILES
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
