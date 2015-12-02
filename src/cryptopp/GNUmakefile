# Base CXXFLAGS used if the user did not specify them
CXXFLAGS ?= -DNDEBUG -g2 -O2

# -fPIC is supported, please report failures with steps to reproduce
# If PIC is required but results in a crash, then use -DCRYPTOPP_DISABLE_ASM
# CXXFLAGS += -fPIC

# Add the following options reduce code size, but breaks link
#   or makes link very slow on some systems
# CXXFLAGS += -ffunction-sections -fdata-sections
#   On OS X, you need to use "LDFLAGS += -Wl,-dead_strip"
# LDFLAGS += -Wl,--gc-sections

AR ?= ar
ARFLAGS ?= -cr	# ar needs the dash on OpenBSD
RANLIB ?= ranlib
CP ?= cp
CHMOD ?= chmod
MKDIR ?= mkdir
EGREP ?= egrep

UNAME := $(shell uname)
IS_X86 := $(shell uname -m | $(EGREP) -i -c "i.86|x86|i86|amd64")
IS_X86_64 := $(shell uname -m | $(EGREP) -i -c "(_64|d64)")
IS_AARCH64 := $(shell uname -m | $(EGREP) -i -c "aarch64")

IS_SUN := $(shell uname | $(EGREP) -i -c "SunOS")
IS_LINUX := $(shell $(CXX) -dumpmachine 2>&1 | $(EGREP) -i -c "Linux")
IS_MINGW := $(shell $(CXX) -dumpmachine 2>&1 | $(EGREP) -i -c "MinGW")
IS_CYGWIN := $(shell $(CXX) -dumpmachine 2>&1 | $(EGREP) -i -c "Cygwin")
IS_DARWIN := $(shell $(CXX) -dumpmachine 2>&1 | $(EGREP) -i -c "Darwin")

SUN_COMPILER := $(shell $(CXX) -V 2>&1 | $(EGREP) -i -c "CC: Sun")
GCC_COMPILER := $(shell $(CXX) --version 2>&1 | $(EGREP) -i -c "(gcc|g\+\+)")
CLANG_COMPILER := $(shell $(CXX) --version 2>&1 | $(EGREP) -i -c "clang")
INTEL_COMPILER := $(shell $(CXX) --version 2>&1 | $(EGREP) -c "\(ICC\)")
MACPORTS_COMPILER := $(shell $(CXX) --version 2>&1 | $(EGREP) -i -c "macports")

# Default prefix for make install
ifeq ($(PREFIX),)
PREFIX = /usr
endif

ifeq ($(CXX),gcc)	# for some reason CXX is gcc on cygwin 1.1.4
CXX := g++
endif

# We honor ARFLAGS, but the "v" often option used by default causes a noisy make
ifeq ($(ARFLAGS),rv)
ARFLAGS = r
endif

ifeq ($(IS_X86),1)

IS_GCC_29 := $(shell $(CXX) -v 2>&1 | $(EGREP) -i -c gcc-9[0-9][0-9])
GCC42_OR_LATER := $(shell $(CXX) -v 2>&1 | $(EGREP) -i -c "gcc version (4\.[2-9]|[5-9]\.)")
GCC46_OR_LATER := $(shell $(CXX) -v 2>&1 | $(EGREP) -i -c "gcc version (4\.[6-9]|[5-9]\.)")
GCC48_OR_LATER := $(shell $(CXX) -v 2>&1 | $(EGREP) -i -c "gcc version (4\.[8-9]|[5-9]\.)")
GCC49_OR_LATER := $(shell $(CXX) -v 2>&1 | $(EGREP) -i -c "gcc version (4\.9|[5-9]\.)")

ICC111_OR_LATER := $(shell $(CXX) --version 2>&1 | $(EGREP) -c "\(ICC\) ([2-9][0-9]|1[2-9]|11\.[1-9])")
GAS210_OR_LATER := $(shell $(CXX) -xc -c /dev/null -Wa,-v -o/dev/null 2>&1 | $(EGREP) -c "GNU assembler version (2\.[1-9][0-9]|[3-9])")
GAS217_OR_LATER := $(shell $(CXX) -xc -c /dev/null -Wa,-v -o/dev/null 2>&1 | $(EGREP) -c "GNU assembler version (2\.1[7-9]|2\.[2-9]|[3-9])")
GAS219_OR_LATER := $(shell $(CXX) -xc -c /dev/null -Wa,-v -o/dev/null 2>&1 | $(EGREP) -c "GNU assembler version (2\.19|2\.[2-9]|[3-9])")

# Add -fPIC for x86_64, but not X32, Cygwin or MinGW
ifneq ($(IS_X86_64),0)
 IS_X32 := $(shell $(CXX) -dM -E - < /dev/null 2>&1 | $(EGREP) -c "ILP32")
 ifeq ($(IS_X32)$(IS_CYGWIN)$(IS_MINGW),000)
 ifeq ($(findstring -fPIC,$(CXXFLAGS)),)
   CXXFLAGS += -fPIC
 endif
 endif
endif

# Guard use of -march=native
ifeq ($(GCC_COMPILER),0)
   CXXFLAGS += -march=native
else ifneq ($(GCC42_OR_LATER),0)
   CXXFLAGS += -march=native
else
  # GCC 3.3 and "unknown option -march="
  # GCC 4.1 compiler crash with -march=native.
  ifneq ($(IS_X86_64),0)
    CXXFLAGS += -m64
  else
    CXXFLAGS += -m32
  endif # X86/X32/X64
endif

# Aligned access required at -O3 for GCC due to vectorization (circa 08/2008). Expect other compilers to do the same.
UNALIGNED_ACCESS := $(shell $(EGREP) -c "^[[:space:]]*//[[:space:]]*\#[[:space:]]*define[[:space:]]*CRYPTOPP_NO_UNALIGNED_DATA_ACCESS" config.h)
ifeq ($(findstring -O3,$(CXXFLAGS)),-O3)
ifneq ($(UNALIGNED_ACCESS),0)
ifeq ($(GCC46_OR_LATER),1)
ifeq ($(findstring -DCRYPTOPP_NO_UNALIGNED_DATA_ACCESS,$(CXXFLAGS)),)
CXXFLAGS += -DCRYPTOPP_NO_UNALIGNED_DATA_ACCESS
endif # CRYPTOPP_NO_UNALIGNED_DATA_ACCESS
endif # GCC 4.6
endif # UNALIGNED_ACCESS
endif # Vectorization

ifneq ($(INTEL_COMPILER),0)
CXXFLAGS += -wd68 -wd186 -wd279 -wd327 -wd161 -wd3180
ifeq ($(ICC111_OR_LATER),0)
# "internal error: backend signals" occurs on some x86 inline assembly with ICC 9 and some x64 inline assembly with ICC 11.0
# if you want to use Crypto++'s assembly code with ICC, try enabling it on individual files
CXXFLAGS += -DCRYPTOPP_DISABLE_ASM
endif
endif

ifeq ($(GCC_COMPILER)$(GAS210_OR_LATER),10)	# .intel_syntax wasn't supported until GNU assembler 2.10
CXXFLAGS += -DCRYPTOPP_DISABLE_ASM
else
ifeq ($(GCC_COMPILER)$(GAS217_OR_LATER),10)
CXXFLAGS += -DCRYPTOPP_DISABLE_SSSE3
else
ifeq ($(GCC_COMPILER)$(GAS219_OR_LATER),10)
CXXFLAGS += -DCRYPTOPP_DISABLE_AESNI
endif
endif

ifneq ($(IS_SUN),0)
CXXFLAGS += -Wa,--divide	# allow use of "/" operator
endif
endif

endif	# IS_X86

ifeq ($(UNAME),)	# for DJGPP, where uname doesn't exist
CXXFLAGS += -mbnu210
else ifneq ($(findstring -save-temps,$(CXXFLAGS)),-save-temps)
CXXFLAGS += -pipe
endif

ifneq ($(IS_MINGW),0)
LDLIBS += -lws2_32
endif

ifeq ($(IS_LINUX),1)
LDFLAGS += -pthread
ifeq ($(findstring -fopenmp,$(CXXFLAGS)),-fopenmp)
ifeq ($(findstring -lgomp,$(LDLIBS)),)
LDLIBS += -lgomp
endif # LDLIBS
endif # OpenMP
ifneq ($(IS_X86_64),0)
M32OR64 = -m64
endif
endif # IS_LINUX

# And add it for ARM64, too
ifneq ($(IS_AARCH64),0)
 ifeq ($(findstring -fPIC,$(CXXFLAGS)),)
   CXXFLAGS += -fPIC
 endif
endif

ifneq ($(IS_DARWIN),0)
AR = libtool
ARFLAGS = -static -o
CXX ?= c++
ifeq ($(IS_GCC_29),1)
CXXFLAGS += -fno-coalesce-templates -fno-coalesce-static-vtables
LDLIBS += -lstdc++
LDFLAGS += -flat_namespace -undefined suppress -m
endif
endif

ifneq ($(IS_SUN),0)
LDLIBS += -lnsl -lsocket
M32OR64 = -m$(shell isainfo -b)
endif

ifneq ($(SUN_COMPILER),0)	# override flags for CC Sun C++ compiler
CXXFLAGS ?= -DNDEBUG -O -g0 -native -template=no%extdef $(M32OR64)
LDFLAGS =
AR = $(CXX)
ARFLAGS = -xar -o
RANLIB = true
SUN_CC10_BUGGY := $(shell $(CXX) -V 2>&1 | $(EGREP) -c "CC: Sun .* 5\.10 .* (2009|2010/0[1-4])")
ifneq ($(SUN_CC10_BUGGY),0)
# -DCRYPTOPP_INCLUDE_VECTOR_CC is needed for Sun Studio 12u1 Sun C++ 5.10 SunOS_i386 128229-02 2009/09/21 and was fixed in May 2010
# remove it if you get "already had a body defined" errors in vector.cc
CXXFLAGS += -DCRYPTOPP_INCLUDE_VECTOR_CC
endif
endif

# Undefined Behavior Sanitizer (UBsan) testing. There's no sense in
#   allowing unaligned data access. There will too many findings.
ifeq ($(findstring ubsan,$(MAKECMDGOALS)),ubsan)
ifeq ($(findstring -fsanitize=undefined,$(CXXFLAGS)),)
CXXFLAGS += -fsanitize=undefined
endif # CXXFLAGS
ifeq ($(findstring -DCRYPTOPP_NO_UNALIGNED_DATA_ACCESS,$(CXXFLAGS)),)
CXXFLAGS += -DCRYPTOPP_NO_UNALIGNED_DATA_ACCESS
endif # CXXFLAGS
endif # UBsan

# Address Sanitizer (Asan) testing
ifeq ($(findstring asan,$(MAKECMDGOALS)),asan)
ifeq ($(findstring -fsanitize=address,$(CXXFLAGS)),)
CXXFLAGS += -fsanitize=address
endif # CXXFLAGS
endif # Asan

# LD gold linker testing
ifeq ($(findstring ld.gold,$(LD)),ld.gold)
ifeq ($(findstring -Wl,-fuse-ld=gold,$(CXXFLAGS)),)
ELF_FORMAT := $(shell file `which ld.gold` 2>&1 | cut -d":" -f 2 | $(EGREP) -i -c "elf")
ifneq ($(ELF_FORMAT),0)
GOLD_OPTION = -Wl,-fuse-ld=gold
endif # ELF/ELF64
endif # CXXFLAGS
endif # Gold

# Aligned access testing
ifneq ($(filter align aligned,$(MAKECMDGOALS)),)
ifeq ($(findstring -DCRYPTOPP_NO_UNALIGNED_DATA_ACCESS,$(CXXFLAGS)),)
CXXFLAGS += -DCRYPTOPP_NO_UNALIGNED_DATA_ACCESS
endif # # CXXFLAGS
endif # Aligned access

# Debug testing on GNU systems
ifneq ($(filter -DDEBUG -DDEBUG=1,$(CXXFLAGS)),)
USING_GLIBCXX := $(shell $(CXX) -x c++ $(CXXFLAGS) -E adhoc.cpp.proto 2>&1 | $(EGREP) -i -c "__GLIBCXX__")
ifneq ($(USING_GLIBCXX),0)
ifeq ($(findstring -D_GLIBCXX_DEBUG,$(CXXFLAGS)),)
CXXFLAGS += -D_GLIBCXX_DEBUG
endif # CXXFLAGS
ifeq ($(findstring -D_GLIBCXX_CONCEPT_CHECKS,$(CXXFLAGS)),)
CXXFLAGS += -D_GLIBCXX_CONCEPT_CHECKS
endif # CXXFLAGS
endif # USING_GLIBCXX
endif # GNU Debug build

# List cryptlib.cpp first and cpu.o second in an attempt to tame C++ static initialization problems. The issue
#  spills into POD data types, so cpu.cpp is the second candidate for explicit initialization order.
SRCS := cryptlib.cpp cpu.cpp $(filter-out cryptlib.cpp cpu.cpp pch.cpp simple.cpp winpipes.cpp cryptlib_bds.cpp,$(wildcard *.cpp))

# No need for CPU or RDRAND on non-X86 systems. X32 is represented with X64.
ifeq ($(IS_X86)$(IS_X86_64),00)
  SRCS := $(filter-out cpu.cpp rdrand.cpp, $(SRCS))
endif

ifneq ($(IS_MINGW),0)
SRCS += winpipes.cpp
endif

# List of objects with crytlib.o and cpu.o at the first and second index position
OBJS := $(SRCS:.cpp=.o)

# test.o needs to be after bench.o for cygwin 1.1.4 (possible ld bug?)
TESTOBJS := bench.o bench2.o test.o validat1.o validat2.o validat3.o adhoc.o datatest.o regtest.o fipsalgt.o dlltest.o
LIBOBJS := $(filter-out $(TESTOBJS),$(OBJS))

# List cryptlib.cpp first in an attempt to tame C++ static initialization problems
DLLSRCS := cryptlib.cpp algebra.cpp algparam.cpp asn.cpp basecode.cpp cbcmac.cpp channels.cpp des.cpp dessp.cpp dh.cpp dll.cpp dsa.cpp ec2n.cpp eccrypto.cpp ecp.cpp eprecomp.cpp files.cpp filters.cpp fips140.cpp fipstest.cpp gf2n.cpp gfpcrypt.cpp hex.cpp hmac.cpp integer.cpp iterhash.cpp misc.cpp modes.cpp modexppc.cpp mqueue.cpp nbtheory.cpp oaep.cpp osrng.cpp pch.cpp pkcspad.cpp pubkey.cpp queue.cpp randpool.cpp rdtables.cpp rijndael.cpp rng.cpp rsa.cpp sha.cpp simple.cpp skipjack.cpp strciphr.cpp trdlocal.cpp
DLLOBJS := $(DLLSRCS:.cpp=.export.o)

# Import lib testing
LIBIMPORTOBJS := $(LIBOBJS:.o=.import.o)
TESTIMPORTOBJS := $(TESTOBJS:.o=.import.o)
DLLTESTOBJS := dlltest.dllonly.o

# For Shared Objects, Diff, Dist/Zip rules
LIB_VER := $(shell $(EGREP) "define CRYPTOPP_VERSION" config.h | cut -d" " -f 3)
LIB_MAJOR := $(shell echo $(LIB_VER) | cut -c 1)
LIB_MINOR := $(shell echo $(LIB_VER) | cut -c 2)
LIB_PATCH := $(shell echo $(LIB_VER) | cut -c 3)

ifeq ($(strip $(LIB_PATCH)),)
LIB_PATCH := 0
endif

all: cryptest.exe

ifneq ($(IS_DARWIN),0)
static: libcryptopp.a
shared dynamic dylib: libcryptopp.dylib
else
static: libcryptopp.a
shared dynamic: libcryptopp.so
endif

.PHONY: deps
deps GNUmakefile.deps:
	$(CXX) $(CXXFLAGS) -MM *.cpp > GNUmakefile.deps

.PHONY: asan ubsan align aligned
asan ubsan align aligned: libcryptopp.a cryptest.exe

.PHONY: test check
test check: cryptest.exe
	./cryptest.exe v

# Directory we want (can't specify on Doygen command line)
DOCUMENT_DIRECTORY := ref$(LIB_VER)
# Directory Doxygen uses (specified in Doygen config file)
ifeq ($(wildcard Doxyfile),Doxyfile)
DOXYGEN_DIRECTORY := $(strip $(shell $(EGREP) "OUTPUT_DIRECTORY" Doxyfile | grep -v "\#" | cut -d "=" -f 2))
endif
# Default directory (missing in config file)
ifeq ($(strip $(DOXYGEN_DIRECTORY)),)
DOXYGEN_DIRECTORY := html-docs
endif

.PHONY: docs html
docs html:
	-$(RM) -r $(DOXYGEN_DIRECTORY)/ $(DOCUMENT_DIRECTORY)/ html-docs/
	doxygen Doxyfile -d CRYPTOPP_DOXYGEN_PROCESSING
	mv $(DOXYGEN_DIRECTORY)/ $(DOCUMENT_DIRECTORY)/
	-$(RM) CryptoPPRef.zip
	zip -9 CryptoPPRef.zip -x ".*" -x "*/.*" -r $(DOCUMENT_DIRECTORY)/

.PHONY: clean
clean:
	-$(RM) libcryptopp.a libcryptopp.so libcryptopp.dylib cryptopp.dll libcryptopp.dll.a libcryptopp.import.a
	-$(RM) adhoc.cpp.o adhoc.cpp.proto.o $(LIBOBJS) $(TESTOBJS) $(DLLOBJS) $(LIBIMPORTOBJS) $(TESTIMPORTOBJS) $(DLLTESTOBJS) *.stackdump core-*
	-$(RM) cryptest.exe dlltest.exe cryptest.import.exe ct rdrand-???.o
ifneq ($(wildcard *.exe.dSYM),)
	-$(RM) -r *.exe.dSYM/
endif
ifneq ($(wildcard $(DOCUMENT_DIRECTORY)/),)
	-$(RM) -r $(DOCUMENT_DIRECTORY)/
endif
ifneq ($(wildcard cov-int/),)
	-$(RM) -r cov-int/
endif

.PHONY: distclean
distclean: clean
	-$(RM) adhoc.cpp adhoc.cpp.copied GNUmakefile.deps benchmarks.html cryptest.txt cryptest-*.txt *.o *.ii *.s
ifneq ($(wildcard cryptopp$(LIB_VER)\.*),)
	-$(RM) cryptopp$(LIB_VER)\.*
endif
ifneq ($(wildcard $(DOC_DIRECTORY)),)
	-$(RM) -r $(DOC_DIRECTORY)
endif
ifneq ($(wildcard CryptoPPRef.zip),)
	-$(RM) CryptoPPRef.zip
endif

.PHONY: install
install:
	$(MKDIR) -p $(PREFIX)/include/cryptopp $(PREFIX)/lib $(PREFIX)/bin
	-$(CP) *.h $(PREFIX)/include/cryptopp
	-$(CHMOD) 755 $(PREFIX)/include/cryptopp
	-$(CHMOD) 644 $(PREFIX)/include/cryptopp/*.h
	-$(CP) libcryptopp.a $(PREFIX)/lib
	-$(CHMOD) 644 $(PREFIX)/lib/libcryptopp.a
	-$(CP) cryptest.exe $(PREFIX)/bin
	-$(CHMOD) 755 $(PREFIX)/bin/cryptest.exe
ifneq ($(IS_DARWIN),0)
	-$(CP) libcryptopp.dylib $(PREFIX)/lib
	-$(CHMOD) 755 $(PREFIX)/lib/libcryptopp.dylib
else
	-$(CP) libcryptopp.so $(PREFIX)/lib
	-$(CHMOD) 755 $(PREFIX)/lib/libcryptopp.so
endif

.PHONY: remove uninstall
remove uninstall:
	-$(RM) -r $(PREFIX)/include/cryptopp
	-$(RM) $(PREFIX)/lib/libcryptopp.a
	-$(RM) $(PREFIX)/bin/cryptest.exe
ifneq ($(IS_DARWIN),0)
	-$(RM) $(PREFIX)/lib/libcryptopp.dylib
else
	-$(RM) $(PREFIX)/lib/libcryptopp.so
endif

libcryptopp.a: public_service | $(LIBOBJS)
	$(AR) $(ARFLAGS) $@ $(LIBOBJS)
	$(RANLIB) $@

libcryptopp.so: public_service | $(LIBOBJS)
	$(CXX) -shared -o $@ $(CXXFLAGS) $(GOLD_OPTION) $(LIBOBJS) $(LDLIBS)

libcryptopp.dylib: $(LIBOBJS)
	$(CXX) -dynamiclib -o $@ $(CXXFLAGS) -install_name "$@" -current_version "$(LIB_MAJOR).$(LIB_MINOR).$(LIB_PATCH)" -compatibility_version "$(LIB_MAJOR).$(LIB_MINOR)" $(LIBOBJS)

cryptest.exe: public_service | libcryptopp.a $(TESTOBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(TESTOBJS) ./libcryptopp.a $(LDFLAGS) $(GOLD_OPTION) $(LDLIBS)

nolib: $(OBJS)		# makes it faster to test changes
	$(CXX) -o ct $(CXXFLAGS) $(OBJS) $(LDFLAGS) $(LDLIBS)

dll: cryptest.import.exe dlltest.exe

cryptopp.dll: $(DLLOBJS)
	$(CXX) -shared -o $@ $(CXXFLAGS) $(DLLOBJS) $(LDFLAGS) $(LDLIBS) -Wl,--out-implib=libcryptopp.dll.a

libcryptopp.import.a: $(LIBIMPORTOBJS)
	$(AR) $(ARFLAGS) $@ $(LIBIMPORTOBJS)
	$(RANLIB) $@

cryptest.import.exe: cryptopp.dll libcryptopp.import.a $(TESTIMPORTOBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(TESTIMPORTOBJS) -L. -lcryptopp.dll -lcryptopp.import $(LDFLAGS) $(LDLIBS)

dlltest.exe: cryptopp.dll $(DLLTESTOBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(DLLTESTOBJS) -L. -lcryptopp.dll $(LDFLAGS) $(LDLIBS)

# This recipe requires a previous "svn co -r 541 http://svn.code.sf.net/p/cryptopp/code/trunk/c5"
.PHONY: diff
diff:
	-$(RM) cryptopp$(LIB_VER).diff
	-svn diff -r 541 > cryptopp$(LIB_VER).diff

# This recipe prepares the distro files
TEXT_FILES := *.h *.cpp adhoc.cpp.proto License.txt Readme.txt Install.txt Filelist.txt config.recommend Doxyfile cryptest* cryptlib* dlltest* cryptdll* *.sln *.vcproj *.dsw *.dsp cryptopp.rc TestVectors/*.txt TestData/*.dat
EXEC_FILES := GNUmakefile GNUmakefile-cross cryptest.sh rdrand-nasm.sh TestData/ TestVectors/

ifeq ($(wildcard Filelist.txt),Filelist.txt)
DIST_FILES := $(shell cat Filelist.txt)
endif

.PHONY: convert
convert:
	chmod 0700 TestVectors/ TestData/
	chmod 0600 $(TEXT_FILES) *.zip
	chmod 0700 $(EXEC_FILES)
	chmod u+x *.cmd *.sh
	unix2dos --keepdate --quiet $(TEXT_FILES) *.asm *.cmd
	dos2unix --keepdate --quiet GNUmakefile GNUmakefile-cross *.S *.sh
ifneq ($(IS_DARWIN),0)
	xattr -c *
endif

.PHONY: zip dist
zip dist: | distclean convert diff
	zip -q -9 cryptopp$(LIB_VER).zip $(DIST_FILES)

.PHONY: bench benchmark benchmarks
bench benchmark benchmarks: cryptest.exe
	rm -f benchmarks.html
	echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\" \"http://www.w3.org/TR/REC-html40/loose.dtd\">" >> benchmarks.html
	echo "<HTML>" >> benchmarks.html
	echo "<HEAD>" >> benchmarks.html
	echo "<TITLE>Speed Comparison of Popular Crypto Algorithms</TITLE>" >> benchmarks.html
	echo "</HEAD>" >> benchmarks.html
	echo "<BODY>" >> benchmarks.html
	echo "<H1><a href=\"http://www.cryptopp.com\">Crypto++</a>" $(LIB_MAJOR).$(LIB_MINOR).$(LIB_REVISION) "Benchmarks</H1>" >> benchmarks.html
	echo "<P>Here are speed benchmarks for some commonly used cryptographic algorithms.</P>"  >> benchmarks.html
	./cryptest.exe b 3 2.4 >> benchmarks.html
	echo "</BODY>" >> benchmarks.html
	echo "</HTML>" >> benchmarks.html

adhoc.cpp: adhoc.cpp.proto
ifeq ($(wildcard adhoc.cpp),)
	cp adhoc.cpp.proto adhoc.cpp
else
	touch adhoc.cpp
endif

# Include dependencies, if present. You must issue `make deps` to create them.
ifeq ($(wildcard GNUmakefile.deps),GNUmakefile.deps)
-include GNUmakefile.deps
endif # Dependencies

# MacPorts/GCC issue with init_priority. Apple/GCC and Fink/GCC are fine; limit to MacPorts.
#   Also see http://lists.macosforge.org/pipermail/macports-users/2015-September/039223.html
ifeq ($(GCC_COMPILER)$(MACPORTS_COMPILER),11)
ifeq ($(findstring -DMACPORTS_GCC_COMPILER,$(CXXFLAGS)),)
cryptlib.o:
	$(CXX) $(CXXFLAGS) -DMACPORTS_GCC_COMPILER=1 -c cryptlib.cpp
cpu.o:
	$(CXX) $(CXXFLAGS) -DMACPORTS_GCC_COMPILER=1 -c cpu.cpp
endif
endif

%.dllonly.o : %.cpp
	$(CXX) $(CXXFLAGS) -DCRYPTOPP_DLL_ONLY -c $< -o $@

%.import.o : %.cpp
	$(CXX) $(CXXFLAGS) -DCRYPTOPP_IMPORTS -c $< -o $@

%.export.o : %.cpp
	$(CXX) $(CXXFLAGS) -DCRYPTOPP_EXPORTS -c $< -o $@

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Warn of potential configurations issues. They will go away after 5.6.3.
UNALIGNED_ACCESS := $(shell $(EGREP) -c "^[[:space:]]*//[[:space:]]*\#[[:space:]]*define[[:space:]]*CRYPTOPP_NO_UNALIGNED_DATA_ACCESS" config.h)
NO_INIT_PRIORITY := $(shell $(EGREP) -c "^[[:space:]]*//[[:space:]]*\#[[:space:]]*define[[:space:]]*CRYPTOPP_INIT_PRIORITY" config.h)
COMPATIBILITY_562 := $(shell $(EGREP) -c "^[[:space:]]*\#[[:space:]]*define[[:space:]]*CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562" config.h)
.PHONY: public_service
public_service:
ifneq ($(UNALIGNED_ACCESS),0)
	$(info WARNING: CRYPTOPP_NO_UNALIGNED_DATA_ACCESS is not defined in config.h.)
endif
ifneq ($(NO_INIT_PRIORITY),0)
	$(info WARNING: CRYPTOPP_INIT_PRIORITY is not defined in config.h.)
endif
ifneq ($(COMPATIBILITY_562),0)
	$(info WARNING: CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562 is defined in config.h.)
endif
ifneq ($(UNALIGNED_ACCESS)$(NO_INIT_PRIORITY)$(COMPATIBILITY_562),000)
	$(info WARNING: You should make these changes in config.h, and not CXXFLAGS.)
	$(info WARNING: You can 'mv config.recommend config.h', but it breaks versioning.)
	$(info WARNING: See http://cryptopp.com/wiki/config.h for more details.)
	$(info )
endif
