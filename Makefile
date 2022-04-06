ifeq ($(findstring MINGW64,$(shell uname -s)),MINGW64)
  # cgo accepts not '/c/path' but 'c:/path'
  PWD=$(shell pwd|sed s'@^/\([a-z]\)/@\1:/@')
else
  PWD=$(shell pwd)
endif
MCL_DIR?=$(PWD)/mcl
include $(MCL_DIR)/common.mk
LIB_DIR=lib
OBJ_DIR=obj
EXE_DIR=bin
CFLAGS += -std=c++11
LDFLAGS += -lpthread

SRC_SRC=bls_c256.cpp bls_c384.cpp bls_c384_256.cpp bls_c512.cpp
TEST_SRC=bls256_test.cpp bls384_test.cpp bls384_256_test.cpp bls_c256_test.cpp bls_c384_test.cpp bls_c384_256_test.cpp bls_c512_test.cpp
SAMPLE_SRC=bls_smpl.cpp bls12_381_smpl.cpp

CFLAGS+=-I$(MCL_DIR)/include
ifneq ($(MCL_MAX_BIT_SIZE),)
  CFLAGS+=-DMCL_MAX_BIT_SIZE=$(MCL_MAX_BIT_SIZE)
endif
ifeq ($(BLS_ETH),1)
  CFLAGS+=-DBLS_ETH
endif

BLS256_LIB=$(LIB_DIR)/libbls256.a
BLS384_LIB=$(LIB_DIR)/libbls384.a
BLS512_LIB=$(LIB_DIR)/libbls512.a
BLS384_256_LIB=$(LIB_DIR)/libbls384_256.a
BLS256_SNAME=bls256
BLS384_SNAME=bls384
BLS512_SNAME=bls512
BLS384_256_SNAME=bls384_256
BLS256_SLIB=$(LIB_DIR)/lib$(BLS256_SNAME).$(LIB_SUF)
BLS384_SLIB=$(LIB_DIR)/lib$(BLS384_SNAME).$(LIB_SUF)
BLS512_SLIB=$(LIB_DIR)/lib$(BLS512_SNAME).$(LIB_SUF)
BLS384_256_SLIB=$(LIB_DIR)/lib$(BLS384_256_SNAME).$(LIB_SUF)
all: $(BLS256_LIB) $(BLS256_SLIB) $(BLS384_LIB) $(BLS384_SLIB) $(BLS384_256_LIB) $(BLS384_256_SLIB) $(BLS512_LIB) $(BLS512_SLIB)

MCL_LIB=$(MCL_DIR)/lib/libmcl.a

$(MCL_LIB):
	$(MAKE) -C $(MCL_DIR)

$(BLS256_LIB): $(OBJ_DIR)/bls_c256.o
	$(AR) $@ $<
$(BLS384_LIB): $(OBJ_DIR)/bls_c384.o
	$(AR) $@ $<
$(BLS512_LIB): $(OBJ_DIR)/bls_c512.o
	$(AR) $@ $<
$(BLS384_256_LIB): $(OBJ_DIR)/bls_c384_256.o
	$(AR) $@ $<

ifneq ($(findstring $(OS),mac/mingw64),)
  COMMON_LIB=$(GMP_LIB) $(OPENSSL_LIB) -lstdc++
  BLS256_SLIB_LDFLAGS+=$(COMMON_LIB)
  BLS384_SLIB_LDFLAGS+=$(COMMON_LIB)
  BLS512_SLIB_LDFLAGS+=$(COMMON_LIB)
  BLS384_256_SLIB_LDFLAGS+=$(COMMON_LIB)
endif
ifeq ($(OS),mingw64)
  CFLAGS+=-I$(MCL_DIR)
  BLS256_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(BLS256_SNAME).a
  BLS384_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(BLS384_SNAME).a
  BLS512_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(BLS512_SNAME).a
  BLS384_256_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(BLS384_256_SNAME).a
endif
$(BLS256_SLIB): $(OBJ_DIR)/bls_c256.o $(MCL_LIB)
	$(PRE)$(CXX) -shared -o $@ $< -L$(MCL_DIR)/lib -lmcl $(LDFLAGS) $(BLS256_SLIB_LDFLAGS)
$(BLS384_SLIB): $(OBJ_DIR)/bls_c384.o $(MCL_LIB)
	$(PRE)$(CXX) -shared -o $@ $< -L$(MCL_DIR)/lib -lmcl $(LDFLAGS) $(BLS384_SLIB_LDFLAGS)
$(BLS512_SLIB): $(OBJ_DIR)/bls_c512.o $(MCL_LIB)
	$(PRE)$(CXX) -shared -o $@ $< -L$(MCL_DIR)/lib -lmcl $(LDFLAGS) $(BLS512_SLIB_LDFLAGS)
$(BLS384_256_SLIB): $(OBJ_DIR)/bls_c384_256.o $(MCL_LIB)
	$(PRE)$(CXX) -shared -o $@ $< -L$(MCL_DIR)/lib -lmcl $(LDFLAGS) $(BLS384_256_SLIB_LDFLAGS)

VPATH=test sample src

.SUFFIXES: .cpp .d .exe

$(OBJ_DIR)/%.o: %.cpp
	$(PRE)$(CXX) $(CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

$(EXE_DIR)/%384_256_test.exe: $(OBJ_DIR)/%384_256_test.o $(BLS384_256_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(BLS384_256_LIB) -L$(MCL_DIR)/lib -lmcl $(LDFLAGS)

$(EXE_DIR)/%384_test.exe: $(OBJ_DIR)/%384_test.o $(BLS384_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(BLS384_LIB) -L$(MCL_DIR)/lib -lmcl $(LDFLAGS)

$(EXE_DIR)/%512_test.exe: $(OBJ_DIR)/%512_test.o $(BLS512_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(BLS512_LIB) -L$(MCL_DIR)/lib -lmcl $(LDFLAGS)

$(EXE_DIR)/%256_test.exe: $(OBJ_DIR)/%256_test.o $(BLS256_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(BLS256_LIB) -L$(MCL_DIR)/lib -lmcl $(LDFLAGS)

# sample exe links libbls384_256.a
$(EXE_DIR)/%.exe: $(OBJ_DIR)/%.o $(BLS384_256_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(BLS384_256_LIB) -L$(MCL_DIR)/lib -lmcl $(LDFLAGS)
ifeq ($(OS),mac)
	install_name_tool bin/bls_smpl.exe -change lib/libmcl.dylib $(MCL_DIR)/lib/libmcl.dylib
endif

SAMPLE_EXE=$(addprefix $(EXE_DIR)/,$(SAMPLE_SRC:.cpp=.exe))
sample: $(SAMPLE_EXE)

TEST_EXE=$(addprefix $(EXE_DIR)/,$(TEST_SRC:.cpp=.exe))
ifeq ($(OS),mac)
  LIBPATH_KEY=DYLD_LIBRARY_PATH
else
  LIBPATH_KEY=LD_LIBRARY_PATH
endif
test_ci: $(TEST_EXE)
#	@sh -ec 'for i in $(TEST_EXE); do echo $$i; env PATH=$$PATH:$(MCL_DIR)/lib $(LIBPATH_KEY)=$(MCL_DIR)/lib LSAN_OPTIONS=verbosity=1 log_threads=1 $$i; done'
	@sh -ec 'for i in $(TEST_EXE); do echo $$i; env PATH=$$PATH:$(MCL_DIR)/lib $(LIBPATH_KEY)=$(MCL_DIR)/lib $$i; done'
	$(MAKE) sample_test

test: $(TEST_EXE)
	@echo test $(TEST_EXE)
	@sh -ec 'for i in $(TEST_EXE); do env PATH=$$PATH:$(MCL_DIR)/lib $(LIBPATH_KEY)=$(MCL_DIR)/lib $$i|grep "ctest:name"; done' > result.txt
	@grep -v "ng=0, exception=0" result.txt; if [ $$? -eq 1 ]; then echo "all unit tests succeed"; else exit 1; fi
	$(MAKE) sample_test

sample_test: $(EXE_DIR)/bls_smpl.exe
	env PATH=$$PATH:$(MCL_DIR)/lib $(LIBPATH_KEY)=$(MCL_DIR)/lib python bls_smpl.py

# PATH is for mingw, LD_LIBRARY_PATH is for linux, DYLD_LIBRARY_PATH is for mac
COMMON_LIB_PATH="../../../lib:../../../$(MCL_DIR)/lib"
PATH_VAL=$$PATH:$(COMMON_LIB_PATH) LD_LIBRARY_PATH=$(COMMON_LIB_PATH) DYLD_LIBRARY_PATH=$(COMMON_LIB_PATH) CGO_LDFLAGS="-L../../../lib -L$(OPENSSL_DIR)/lib" CGO_CFLAGS="-I$(PWD)/include -I$(MCL_DIR)/include"
test_go256: ffi/go/bls/bls.go ffi/go/bls/bls_test.go $(BLS256_SLIB)
	cd ffi/go/bls && env PATH=$(PATH_VAL) go test -tags bn256 .
test_go384: ffi/go/bls/bls.go ffi/go/bls/bls_test.go $(BLS384_SLIB)
	cd ffi/go/bls && env PATH=$(PATH_VAL) go test -tags bn384 .
test_go384_256: ffi/go/bls/bls.go ffi/go/bls/bls_test.go $(BLS384_256_SLIB)
	cd ffi/go/bls && env PATH=$(PATH_VAL) go test -tags bn384_256 .

test_eth: bin/bls_c384_256_test.exe
	bin/bls_c384_256_test.exe

test_go:
	$(MAKE) test_go256
	$(MAKE) test_go384
	$(MAKE) test_go384_256

EMCC_OPT=-I./include -I./src -I$(MCL_DIR)/include -I./ -Wall -Wextra
EMCC_OPT+=-O3 -DNDEBUG
EMCC_OPT+=-s WASM=1 -s NO_EXIT_RUNTIME=1 -s NODEJS_CATCH_EXIT=0 -s NODEJS_CATCH_REJECTION=0 #-s ASSERTIONS=1
EMCC_OPT+=-s MODULARIZE=1
EMCC_OPT+=-s EXPORT_NAME='blsCreateModule'
EMCC_OPT+=-s STRICT_JS=1
EMCC_OPT+=-s SINGLE_FILE=1
EMCC_OPT+=-s MINIFY_HTML=0
EMCC_OPT+=--minify 0
EMCC_OPT+=-DCYBOZU_MINIMUM_EXCEPTION
EMCC_OPT+=-s ABORTING_MALLOC=0
JS_DEP=src/bls_c384.cpp $(MCL_DIR)/src/fp.cpp Makefile

../bls-wasm/bls_c.js: $(JS_DEP)
	emcc -o $@ src/bls_c384.cpp $(MCL_DIR)/src/fp.cpp $(EMCC_OPT) -DMCL_MAX_BIT_SIZE=384 -DMCL_USE_WEB_CRYPTO_API -s DISABLE_EXCEPTION_CATCHING=1 -DCYBOZU_DONT_USE_EXCEPTION -DCYBOZU_DONT_USE_STRING -DMCL_DONT_USE_CSPRNG -fno-exceptions -MD -MP -MF obj/bls_c384.d

bls-wasm:
	$(MAKE) ../bls-wasm/bls_c.js

../bls-eth-wasm/bls_c.js: src/bls_c384_256.cpp $(MCL_DIR)/src/fp.cpp Makefile
	emcc -o $@ src/bls_c384_256.cpp $(MCL_DIR)/src/fp.cpp $(EMCC_OPT) -DMCL_MAX_BIT_SIZE=384 -DBLS_ETH -DMCL_USE_WEB_CRYPTO_API -s DISABLE_EXCEPTION_CATCHING=1 -DCYBOZU_DONT_USE_EXCEPTION -DCYBOZU_DONT_USE_STRING -DMCL_DONT_USE_CSPRNG -fno-exceptions -MD -MP -MF obj/bls_c384_256.d
bls-eth-wasm:
	$(MAKE) ../bls-eth-wasm/bls_c.js

#CLANG_WASM_OPT= -fno-builtin  --target=wasm32-unknown-unknown-wasm -Wno-unused-parameter -ffreestanding -fno-exceptions -fvisibility=hidden -Wall -Wextra -fno-threadsafe-statics -nodefaultlibs -nostdlib -fno-use-cxa-atexit -fno-unwind-tables -fno-rtti -nostdinc++ -DLLONG_MIN=-0x8000000000000000LL

BASE_CFLAGS=-O3 -g -DNDEBUG -I ./include -I $(MCL_DIR)/include -I ./src -fPIC -DMCL_MAX_BIT_SIZE=384 -DMCL_SIZEOF_UNIT=8 -DMCL_LLVM_BMI2=0 -DCYBOZU_DONT_USE_EXCEPTION -DMCL_DONT_USE_CSPRNG -DCYBOZU_DONT_USE_STRING -DCYBOZU_MINIMUM_EXCEPTION -Wno-unused-parameter -Wall -Wextra -fno-threadsafe-statics -fno-use-cxa-atexit -fno-unwind-tables -fno-builtin -fvisibility=hidden -fno-rtti -fno-stack-protector -fno-exceptions -nostdinc++
WASM_OUT_DIR=../bls-wasm/
WASM_SRC_BASENAME=bls_c384
ifeq ($(BLS_ETH),1)
  BASE_CFLAGS+=-DBLS_ETH
  WASM_OUT_DIR=../bls-eth-wasm/
  WASM_SRC_BASENAME=bls_c384_256
endif
CLANG_WASM_OPT=$(BASE_CFLAGS) --target=wasm32-unknown-unknown-wasm -ffreestanding -nostdlib -I /usr/include -DMCL_USE_LLVM=1
# apt install liblld-10-dev
bls-wasm-by-clang: $(MCL_DIR)/src/base64m.ll
	$(CXX) -x c -c -o $(OBJ_DIR)/mylib.o src/mylib.c $(CLANG_WASM_OPT) -Wstrict-prototypes
	$(CXX) -c -o $(OBJ_DIR)/base64m.o $(MCL_DIR)/src/base64m.ll $(CLANG_WASM_OPT) -std=c++03
	$(CXX) -c -o $(OBJ_DIR)/$(WASM_SRC_BASENAME).o src/$(WASM_SRC_BASENAME).cpp $(CLANG_WASM_OPT) -std=c++03
	$(CXX) -c -o $(OBJ_DIR)/fp.o $(MCL_DIR)/src/fp.cpp $(CLANG_WASM_OPT) -std=c++03
	wasm-ld-10 --no-entry --export-dynamic -o $(WASM_OUT_DIR)/bls.wasm $(OBJ_DIR)/$(WASM_SRC_BASENAME).o $(OBJ_DIR)/fp.o $(OBJ_DIR)/mylib.o $(OBJ_DIR)/base64m.o #-z stack-size=524288

bin/minsample: sample/minsample.c
	$(CXX) -o bin/minsample sample/minsample.c src/mylib.c src/$(WASM_SRC_BASENAME).cpp $(MCL_DIR)/src/fp.cpp $(BASE_CFLAGS) -std=c++03 -DMCL_DONT_USE_XBYAK -DMCL_USE_VINT -DMCL_VINT_FIXED_BUFFER -DMCL_DONT_USE_OPENSSL

$(MCL_DIR)/src/base64.ll:
	$(MAKE) -C $(MCL_DIR) src/base64.ll

$(MCL_DIR)/src/base64m.ll:
	$(MAKE) -C $(MCL_DIR) src/base64m.ll

MIN_CFLAGS=-std=c++03 -O3 -DNDEBUG -fPIC -DMCL_DONT_USE_OPENSSL -DMCL_USE_VINT -DMCL_SIZEOF_UNIT=8 -DMCL_VINT_FIXED_BUFFER -DMCL_MAX_BIT_SIZE=384 -DCYBOZU_DONT_USE_EXCEPTION -DCYBOZU_DONT_USE_STRING -I./include -I $(MCL_DIR)/include
ifneq ($(MIN_WITH_XBYAK),1)
  MIN_CFLAGS+=-DMCL_DONT_USE_XBYAK -fno-exceptions -fno-rtti -fno-threadsafe-statics -nodefaultlibs -nostdlib -fno-use-cxa-atexit -fno-unwind-tables -nostdinc++
endif
ifeq ($(BLS_ETH),1)
  MIN_CFLAGS+=-DBLS_ETH
endif
minimized_static:
	$(CXX) -c -o $(OBJ_DIR)/fp.o $(MCL_DIR)/src/fp.cpp $(MIN_CFLAGS)
	$(CXX) -c -o $(OBJ_DIR)/bls_c384_256.o src/bls_c384_256.cpp $(MIN_CFLAGS)
	$(AR) $(LIB_DIR)/libbls384_256.a $(OBJ_DIR)/bls_c384_256.o $(OBJ_DIR)/fp.o


clean:
	$(RM) $(OBJ_DIR)/*.d $(OBJ_DIR)/*.o $(EXE_DIR)/*.exe $(GEN_EXE) $(ASM_SRC) $(ASM_OBJ) $(LLVM_SRC) $(BLS256_LIB) $(BLS256_SLIB) $(BLS384_LIB) $(BLS384_SLIB) $(BLS384_256_LIB) $(BLS384_256_SLIB) $(BLS512_LIB) $(BLS512_SLIB)

ALL_SRC=$(SRC_SRC) $(TEST_SRC) $(SAMPLE_SRC)
DEPEND_FILE=$(addprefix $(OBJ_DIR)/, $(ALL_SRC:.cpp=.d))
-include $(DEPEND_FILE)

PREFIX?=/usr/local
prefix?=$(PREFIX)
includedir?=$(prefix)/include
libdir?=$(prefix)/lib
INSTALL?=install
INSTALL_DATA?=$(INSTALL) -m 644
install: lib/libbls256.a lib/libbls256.$(LIB_SUF) lib/libbls384.a lib/libbls384.$(LIB_SUF) lib/libbls384_256.a lib/libbls384_256.$(LIB_SUF)
	$(MKDIR) $(DESTDIR)$(includedir)/bls $(DESTDIR)$(libdir)
	$(INSTALL_DATA) include/bls/*.h* $(DESTDIR)$(includedir)/bls
	$(INSTALL_DATA) lib/libbls*.a $(DESTDIR)$(libdir)
	$(INSTALL) -m 755 lib/libbls*.$(LIB_SUF) $(DESTDIR)$(libdir)

.PHONY: test bls-wasm ios

# don't remove these files automatically
.SECONDARY: $(addprefix $(OBJ_DIR)/, $(ALL_SRC:.cpp=.o))

