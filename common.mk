GCC_VER=$(shell $(PRE)$(CC) -dumpversion)
UNAME_S=$(shell uname -s)
ARCH?=$(shell uname -m)
NASM_ELF_OPT=-felf64
ifeq ($(UNAME_S),Linux)
  OS=Linux
endif
ifeq ($(findstring MINGW64,$(UNAME_S)),MINGW64)
  OS=mingw64
  CFLAGS+=-D__USE_MINGW_ANSI_STDIO=1
  NASM_ELF_OPT=-fwin64
endif
ifeq ($(findstring CYGWIN,$(UNAME_S)),CYGWIN)
  OS=cygwin
endif
ifeq ($(UNAME_S),Darwin)
  ifeq ($(ARCH),x86_64)
    OS=mac
  else
    OS=mac-m1
  endif
  LIB_SUF=dylib
  NASM_ELF_OPT=-fmacho64
else
  LIB_SUF=so
endif
ifeq ($(UNAME_S),OpenBSD)
  OS=openbsd
  CXX?=clang++
  CFLAGS+=-I/usr/local/include
  LDFLAGS+=-L/usr/local/lib
endif
ifeq ($(UNAME_S),FreeBSD)
  OS=freebsd
  CXX?=clang++
  CFLAGS+=-I/usr/local/include
  LDFLAGS+=-L/usr/local/lib
endif

ifneq ($(findstring $(ARCH),x86_64/amd64),)
  CPU=x86-64
  INTEL=1
  ifeq ($(findstring $(OS),mingw64/cygwin),)
    GCC_EXT=1
  endif
  BIT=64
  BIT_OPT=-m64
  #LOW_ASM_SRC=src/asm/low_x86-64.asm
  #ASM=nasm -felf64
endif
ifeq ($(ARCH),x86)
  CPU=x86
  INTEL=1
  BIT=32
  BIT_OPT=-m32
  #LOW_ASM_SRC=src/asm/low_x86.asm
endif
ifneq ($(findstring $(ARCH),armv7l/armv6l),)
  CPU=arm
  BIT=32
  #LOW_ASM_SRC=src/asm/low_arm.s
endif
#ifeq ($(ARCH),aarch64)
ifneq ($(findstring $(ARCH),aarch64/arm64),)
  CPU=aarch64
  BIT=64
endif
ifeq ($(findstring $(OS),mac/mac-m1/mingw64/openbsd),)
  LDFLAGS+=-lrt
endif
ifeq ($(ARCH),s390x)
  CPU=systemz
  BIT=64
endif

CP=cp -f
AR=ar r
MKDIR=mkdir -p
RM=rm -rf

ifeq ($(DEBUG),1)
  ifeq ($(GCC_EXT),1)
    CFLAGS+=-fsanitize=address
    LDFLAGS+=-fsanitize=address
  endif
else
  CFLAGS_OPT+=-fomit-frame-pointer -DNDEBUG -fno-stack-protector
  ifeq ($(CXX),clang++)
    CFLAGS_OPT+=-O3
  else
    ifeq ($(shell expr $(GCC_VER) \> 4.6.0),1)
      CFLAGS_OPT+=-O3
    else
      CFLAGS_OPT+=-O3
    endif
  endif
  ifeq ($(MARCH),)
    ifeq ($(INTEL),1)
#      CFLAGS_OPT+=-march=native
    endif
  else
    CFLAGS_OPT+=$(MARCH)
  endif
endif
CFLAGS_WARN=-Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wfloat-equal -Wpointer-arith -Wundef
CFLAGS+=-g3
INC_OPT=-I include -I test
CFLAGS+=$(CFLAGS_WARN) $(BIT_OPT) $(INC_OPT)
DEBUG=0
CFLAGS_OPT_USER?=$(CFLAGS_OPT)
ifeq ($(DEBUG),0)
CFLAGS+=$(CFLAGS_OPT_USER)
endif
CFLAGS+=$(CFLAGS_USER)
MCL_USE_GMP?=1
ifneq ($(OS),mac/mac-m1,)
  MCL_USE_GMP=0
endif
MCL_USE_OPENSSL?=0
ifeq ($(MCL_USE_GMP),0)
  CFLAGS+=-DMCL_USE_VINT
endif
ifneq ($(MCL_SIZEOF_UNIT),)
  CFLAGS+=-DMCL_SIZEOF_UNIT=$(MCL_SIZEOF_UNIT)
endif
ifeq ($(MCL_USE_OPENSSL),0)
  CFLAGS+=-DMCL_DONT_USE_OPENSSL
endif
ifeq ($(MCL_USE_GMP),1)
  GMP_LIB=-lgmp -lgmpxx
  ifeq ($(UNAME_S),Darwin)
    GMP_DIR?=/usr/local/opt/gmp
    CFLAGS+=-I$(GMP_DIR)/include
    LDFLAGS+=-L$(GMP_DIR)/lib
  endif
endif
ifeq ($(MCL_USE_OPENSSL),1)
  OPENSSL_LIB=-lcrypto
  ifeq ($(UNAME_S),Darwin)
    OPENSSL_DIR?=/usr/local/opt/openssl
    CFLAGS+=-I$(OPENSSL_DIR)/include
    LDFLAGS+=-L$(OPENSSL_DIR)/lib
  endif
endif
ifeq ($(MCL_STATIC_CODE),1)
  MCL_USE_XBYAK=0
  MCL_MAX_BIT_SIZE=384
  CFLAGS+=-DMCL_STATIC_CODE
endif
ifeq ($(MCL_USE_OMP),1)
  CFLAGS+=-DMCL_USE_OMP
  ifeq ($(OS),mac)
    CFLAGS+=-Xpreprocessor -fopenmp
    LDFLAGS+=-lomp
  else
    CFLAGS+=-fopenmp
    LDFLAGS+=-fopenmp
  endif
endif
LDFLAGS+=$(GMP_LIB) $(OPENSSL_LIB) $(BIT_OPT) $(LDFLAGS_USER)

CFLAGS+=-fPIC

