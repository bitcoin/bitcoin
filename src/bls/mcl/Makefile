include common.mk
LIB_DIR=lib
OBJ_DIR=obj
EXE_DIR=bin
SRC_SRC=fp.cpp bn_c256.cpp bn_c384.cpp bn_c384_256.cpp bn_c512.cpp she_c256.cpp
TEST_SRC=fp_test.cpp ec_test.cpp fp_util_test.cpp window_method_test.cpp elgamal_test.cpp fp_tower_test.cpp gmp_test.cpp bn_test.cpp bn384_test.cpp glv_test.cpp paillier_test.cpp she_test.cpp vint_test.cpp bn512_test.cpp conversion_test.cpp
TEST_SRC+=bn_c256_test.cpp bn_c384_test.cpp bn_c384_256_test.cpp bn_c512_test.cpp
TEST_SRC+=she_c256_test.cpp she_c384_test.cpp she_c384_256_test.cpp
TEST_SRC+=aggregate_sig_test.cpp array_test.cpp
TEST_SRC+=bls12_test.cpp
TEST_SRC+=mapto_wb19_test.cpp
TEST_SRC+=modp_test.cpp
TEST_SRC+=ecdsa_test.cpp ecdsa_c_test.cpp
TEST_SRC+=mul_test.cpp
LIB_OBJ=$(OBJ_DIR)/fp.o
ifeq ($(MCL_STATIC_CODE),1)
  LIB_OBJ+=obj/static_code.o
  TEST_SRC=bls12_test.cpp
endif
ifeq ($(CPU),x86-64)
  MCL_USE_XBYAK?=1
  TEST_SRC+=mont_fp_test.cpp sq_test.cpp
  ifeq ($(USE_LOW_ASM),1)
    TEST_SRC+=low_test.cpp
  endif
  ifeq ($(MCL_USE_XBYAK),1)
    TEST_SRC+=fp_generator_test.cpp
  endif
endif
SAMPLE_SRC=bench.cpp ecdh.cpp random.cpp rawbench.cpp vote.cpp pairing.cpp large.cpp tri-dh.cpp bls_sig.cpp pairing_c.c she_smpl.cpp mt_test.cpp

ifneq ($(MCL_MAX_BIT_SIZE),)
  CFLAGS+=-DMCL_MAX_BIT_SIZE=$(MCL_MAX_BIT_SIZE)
endif
ifeq ($(MCL_USE_XBYAK),0)
  CFLAGS+=-DMCL_DONT_USE_XBYAK
endif
ifeq ($(MCL_USE_PROF),1)
  CFLAGS+=-DMCL_USE_PROF
endif
ifeq ($(MCL_USE_PROF),2)
  CFLAGS+=-DMCL_USE_PROF -DXBYAK_USE_VTUNE -I /opt/intel/vtune_amplifier/include/
  LDFLAGS+=-L /opt/intel/vtune_amplifier/lib64 -ljitprofiling -ldl
endif
##################################################################
MCL_LIB=$(LIB_DIR)/libmcl.a
MCL_SNAME=mcl
BN256_SNAME=mclbn256
BN384_SNAME=mclbn384
BN384_256_SNAME=mclbn384_256
BN512_SNAME=mclbn512
SHE256_SNAME=mclshe256
SHE384_SNAME=mclshe384
SHE384_256_SNAME=mclshe384_256
MCL_SLIB=$(LIB_DIR)/lib$(MCL_SNAME).$(LIB_SUF)
BN256_LIB=$(LIB_DIR)/libmclbn256.a
BN256_SLIB=$(LIB_DIR)/lib$(BN256_SNAME).$(LIB_SUF)
BN384_LIB=$(LIB_DIR)/libmclbn384.a
BN384_SLIB=$(LIB_DIR)/lib$(BN384_SNAME).$(LIB_SUF)
BN384_256_LIB=$(LIB_DIR)/libmclbn384_256.a
BN384_256_SLIB=$(LIB_DIR)/lib$(BN384_256_SNAME).$(LIB_SUF)
BN512_LIB=$(LIB_DIR)/libmclbn512.a
BN512_SLIB=$(LIB_DIR)/lib$(BN512_SNAME).$(LIB_SUF)
SHE256_LIB=$(LIB_DIR)/libmclshe256.a
SHE256_SLIB=$(LIB_DIR)/lib$(SHE256_SNAME).$(LIB_SUF)
SHE384_LIB=$(LIB_DIR)/libmclshe384.a
SHE384_SLIB=$(LIB_DIR)/lib$(SHE384_SNAME).$(LIB_SUF)
SHE384_256_LIB=$(LIB_DIR)/libmclshe384_256.a
SHE384_256_SLIB=$(LIB_DIR)/lib$(SHE384_256_SNAME).$(LIB_SUF)
SHE_LIB_ALL=$(SHE256_LIB) $(SHE256_SLIB) $(SHE384_LIB) $(SHE384_SLIB) $(SHE384_256_LIB) $(SHE384_256_SLIB)
all: $(MCL_LIB) $(MCL_SLIB) $(BN256_LIB) $(BN256_SLIB) $(BN384_LIB) $(BN384_SLIB) $(BN384_256_LIB) $(BN384_256_SLIB) $(BN512_LIB) $(BN512_SLIB) $(SHE_LIB_ALL)
ECDSA_LIB=$(LIB_DIR)/libmclecdsa.a

#LLVM_VER=-3.8
LLVM_LLC=llc$(LLVM_VER)
LLVM_OPT=opt$(LLVM_VER)
LLVM_OPT_VERSION=$(shell $(LLVM_OPT) --version 2>/dev/null | awk '/version/ { split($$3,a,"."); print a[1]}')
GEN_EXE=src/gen
GEN_EXE_OPT=-u $(BIT)
# incompatibility between llvm 3.4 and the later version
ifneq ($(LLVM_OPT_VERSION),)
ifeq ($(shell expr $(LLVM_OPT_VERSION) \>= 9),1)
  GEN_EXE_OPT+=-ver 0x90
endif
endif
ifeq ($(OS),mac)
  ASM_SRC_PATH_NAME=src/asm/$(CPU)mac
else
  ASM_SRC_PATH_NAME=src/asm/$(CPU)
endif
ifneq ($(CPU),)
  ASM_SRC=$(ASM_SRC_PATH_NAME).s
endif
ASM_OBJ=$(OBJ_DIR)/$(CPU).o
ifeq ($(OS),mac-m1)
  ASM_SRC=src/base64.ll
  ASM_OBJ=$(OBJ_DIR)/base64.o
endif
BN256_OBJ=$(OBJ_DIR)/bn_c256.o
BN384_OBJ=$(OBJ_DIR)/bn_c384.o
BN384_256_OBJ=$(OBJ_DIR)/bn_c384_256.o
BN512_OBJ=$(OBJ_DIR)/bn_c512.o
SHE256_OBJ=$(OBJ_DIR)/she_c256.o
SHE384_OBJ=$(OBJ_DIR)/she_c384.o
SHE384_256_OBJ=$(OBJ_DIR)/she_c384_256.o
FUNC_LIST=src/func.list
ifeq ($(findstring $(OS),mingw64/cygwin),)
  MCL_USE_LLVM?=1
else
  MCL_USE_LLVM=0
endif
ifeq ($(MCL_USE_LLVM),1)
  CFLAGS+=-DMCL_USE_LLVM=1
  LIB_OBJ+=$(ASM_OBJ)
  # special case for intel with bmi2
  ifeq ($(INTEL),1)
    ifneq ($(MCL_STATIC_CODE),1)
      LIB_OBJ+=$(OBJ_DIR)/$(CPU).bmi2.o
    endif
  endif
endif
LLVM_SRC=src/base$(BIT).ll

# CPU is used for llvm
# see $(LLVM_LLC) --version
LLVM_FLAGS=-march=$(CPU) -relocation-model=pic #-misched=ilpmax
LLVM_FLAGS+=-pre-RA-sched=list-ilp -max-sched-reorder=128 -mattr=-sse

#HAS_BMI2=$(shell cat "/proc/cpuinfo" | grep bmi2 >/dev/null && echo "1")
#ifeq ($(HAS_BMI2),1)
#  LLVM_FLAGS+=-mattr=bmi2
#endif

ifeq ($(USE_LOW_ASM),1)
  LOW_ASM_OBJ=$(LOW_ASM_SRC:.asm=.o)
  LIB_OBJ+=$(LOW_ASM_OBJ)
endif

ifeq ($(UPDATE_ASM),1)
  ASM_SRC_DEP=$(LLVM_SRC)
  ASM_BMI2_SRC_DEP=src/base$(BIT).bmi2.ll
else
  ASM_SRC_DEP=
  ASM_BMI2_SRC_DEP=
endif

ifneq ($(findstring $(OS),mac/mac-m1/mingw64),)
  BN256_SLIB_LDFLAGS+=-l$(MCL_SNAME) -L./lib
  BN384_SLIB_LDFLAGS+=-l$(MCL_SNAME) -L./lib
  BN384_256_SLIB_LDFLAGS+=-l$(MCL_SNAME) -L./lib
  BN512_SLIB_LDFLAGS+=-l$(MCL_SNAME) -L./lib
  SHE256_SLIB_LDFLAGS+=-l$(MCL_SNAME) -L./lib
  SHE384_SLIB_LDFLAGS+=-l$(MCL_SNAME) -L./lib
  SHE384_256_SLIB_LDFLAGS+=-l$(MCL_SNAME) -L./lib
endif
ifeq ($(OS),mingw64)
  MCL_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(MCL_SNAME).a
  BN256_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(BN256_SNAME).a
  BN384_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(BN384_SNAME).a
  BN384_256_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(BN384_256_SNAME).a
  BN512_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(BN512_SNAME).a
  SHE256_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(SHE256_SNAME).a
  SHE384_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(SHE384_SNAME).a
  SHE384_256_SLIB_LDFLAGS+=-Wl,--out-implib,$(LIB_DIR)/lib$(SHE384_256_SNAME).a
endif

$(MCL_LIB): $(LIB_OBJ)
	$(AR) $@ $(LIB_OBJ)

$(MCL_SLIB): $(LIB_OBJ)
	$(PRE)$(CXX) -o $@ $(LIB_OBJ) -shared $(LDFLAGS) $(MCL_SLIB_LDFLAGS)

$(BN256_LIB): $(BN256_OBJ)
	$(AR) $@ $(BN256_OBJ)

$(SHE256_LIB): $(SHE256_OBJ)
	$(AR) $@ $(SHE256_OBJ)

$(SHE384_LIB): $(SHE384_OBJ)
	$(AR) $@ $(SHE384_OBJ)

$(SHE384_256_LIB): $(SHE384_256_OBJ)
	$(AR) $@ $(SHE384_256_OBJ)

$(SHE256_SLIB): $(SHE256_OBJ) $(MCL_LIB)
	$(PRE)$(CXX) -o $@ $(SHE256_OBJ) $(MCL_LIB) -shared $(LDFLAGS) $(SHE256_SLIB_LDFLAGS)

$(SHE384_SLIB): $(SHE384_OBJ) $(MCL_LIB)
	$(PRE)$(CXX) -o $@ $(SHE384_OBJ) $(MCL_LIB) -shared $(LDFLAGS) $(SHE384_SLIB_LDFLAGS)

$(SHE384_256_SLIB): $(SHE384_256_OBJ) $(MCL_LIB)
	$(PRE)$(CXX) -o $@ $(SHE384_256_OBJ) $(MCL_LIB) -shared $(LDFLAGS) $(SHE384_256_SLIB_LDFLAGS)

$(BN256_SLIB): $(BN256_OBJ) $(MCL_SLIB)
	$(PRE)$(CXX) -o $@ $(BN256_OBJ) -shared $(LDFLAGS) $(BN256_SLIB_LDFLAGS)

$(BN384_LIB): $(BN384_OBJ)
	$(AR) $@ $(BN384_OBJ)

$(BN384_256_LIB): $(BN384_256_OBJ)
	$(AR) $@ $(BN384_256_OBJ)

$(BN512_LIB): $(BN512_OBJ)
	$(AR) $@ $(BN512_OBJ)

$(BN384_SLIB): $(BN384_OBJ) $(MCL_SLIB)
	$(PRE)$(CXX) -o $@ $(BN384_OBJ) -shared $(LDFLAGS) $(BN384_SLIB_LDFLAGS)

$(BN384_256_SLIB): $(BN384_256_OBJ) $(MCL_SLIB)
	$(PRE)$(CXX) -o $@ $(BN384_256_OBJ) -shared $(LDFLAGS) $(BN384_256_SLIB_LDFLAGS)

$(BN512_SLIB): $(BN512_OBJ) $(MCL_SLIB)
	$(PRE)$(CXX) -o $@ $(BN512_OBJ) -shared $(LDFLAGS) $(BN512_SLIB_LDFLAGS)

ECDSA_OBJ=$(OBJ_DIR)/ecdsa_c.o
$(ECDSA_LIB): $(ECDSA_OBJ)
	$(AR) $@ $(ECDSA_OBJ)

$(ASM_OBJ): $(ASM_SRC)
	$(PRE)$(CXX) -c $< -o $@ $(CFLAGS)

$(ASM_SRC): $(ASM_SRC_DEP)
	$(LLVM_OPT) -O3 -o - $< -march=$(CPU) | $(LLVM_LLC) -O3 -o $@ $(LLVM_FLAGS)

$(LLVM_SRC): $(GEN_EXE) $(FUNC_LIST)
	$(GEN_EXE) $(GEN_EXE_OPT) -f $(FUNC_LIST) > $@

$(ASM_SRC_PATH_NAME).bmi2.s: $(ASM_BMI2_SRC_DEP)
	$(LLVM_OPT) -O3 -o - $< -march=$(CPU) | $(LLVM_LLC) -O3 -o $@ $(LLVM_FLAGS) -mattr=bmi2

$(OBJ_DIR)/$(CPU).bmi2.o: $(ASM_SRC_PATH_NAME).bmi2.s
	$(PRE)$(CXX) -c $< -o $@ $(CFLAGS)

src/base$(BIT).bmi2.ll: $(GEN_EXE)
	$(GEN_EXE) $(GEN_EXE_OPT) -f $(FUNC_LIST) -s bmi2 > $@

src/base64m.ll: $(GEN_EXE)
	$(GEN_EXE) $(GEN_EXE_OPT) -wasm > $@

$(FUNC_LIST): $(LOW_ASM_SRC)
ifeq ($(USE_LOW_ASM),1)
	$(shell awk '/global/ { print $$2}' $(LOW_ASM_SRC) > $(FUNC_LIST))
	$(shell awk '/proc/ { print $$2}' $(LOW_ASM_SRC) >> $(FUNC_LIST))
else
	$(shell touch $(FUNC_LIST))
endif

$(GEN_EXE): src/gen.cpp src/llvm_gen.hpp
	$(CXX) -o $@ $< $(CFLAGS) -DMCL_USE_VINT -DMCL_VINT_FIXED_BUFFER

src/dump_code: src/dump_code.cpp src/fp.cpp src/fp_generator.hpp
	$(CXX) -o $@ src/dump_code.cpp src/fp.cpp -g -I include -DMCL_DUMP_JIT -DMCL_MAX_BIT_SIZE=384 -DMCL_DONT_USE_OPENSSL -DMCL_USE_VINT -DMCL_SIZEOF_UNIT=8 -DMCL_VINT_FIXED_BUFFER

src/static_code.asm: src/dump_code
	$< > $@

obj/static_code.o: src/static_code.asm
	nasm $(NASM_ELF_OPT) -o $@ $<

bin/static_code_test.exe: test/static_code_test.cpp src/fp.cpp obj/static_code.o
	$(CXX) -o $@ -O3 $^ -g -DMCL_DONT_USE_XBYAK -DMCL_STATIC_CODE -DMCL_MAX_BIT_SIZE=384 -DMCL_DONT_USE_OPENSSL -DMCL_USE_VINT -DMCL_SIZEOF_UNIT=8 -DMCL_VINT_FIXED_BUFFER -I include -Wall -Wextra
 
asm: $(LLVM_SRC)
	$(LLVM_OPT) -O3 -o - $(LLVM_SRC) | $(LLVM_LLC) -O3 $(LLVM_FLAGS) -x86-asm-syntax=intel

$(LOW_ASM_OBJ): $(LOW_ASM_SRC)
	$(ASM) $<

# set PATH for mingw, set LD_LIBRARY_PATH is for other env
COMMON_LIB_PATH="../../../lib"
PATH_VAL=$$PATH:$(COMMON_LIB_PATH) LD_LIBRARY_PATH=$(COMMON_LIB_PATH) DYLD_LIBRARY_PATH=$(COMMON_LIB_PATH) CGO_CFLAGS="-I$(shell pwd)/include" CGO_LDFLAGS="-L../../../lib"
test_go256: $(MCL_SLIB) $(BN256_SLIB)
	cd ffi/go/mcl && env PATH=$(PATH_VAL) go test -tags bn256 .

test_go384: $(MCL_SLIB) $(BN384_SLIB)
	cd ffi/go/mcl && env PATH=$(PATH_VAL) go test -tags bn384 .

test_go384_256: $(MCL_SLIB) $(BN384_256_SLIB)
	cd ffi/go/mcl && env PATH=$(PATH_VAL) go test -tags bn384_256 .

test_go:
	$(MAKE) test_go256
	$(MAKE) test_go384
	$(MAKE) test_go384_256

test_python_she: $(SHE256_SLIB)
	cd ffi/python && env LD_LIBRARY_PATH="../../lib" DYLD_LIBRARY_PATH="../../lib" PATH=$$PATH:"../../lib" python3 she.py
test_python:
	$(MAKE) test_python_she

test_java:
	$(MAKE) -C ffi/java test

##################################################################

VPATH=test sample src

.SUFFIXES: .cpp .d .exe .c .o

$(OBJ_DIR)/%.o: %.cpp
	$(PRE)$(CXX) $(CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

$(OBJ_DIR)/%.o: %.c
	$(PRE)$(CC) $(CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

$(EXE_DIR)/%.exe: $(OBJ_DIR)/%.o $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(MCL_LIB) $(LDFLAGS)

$(EXE_DIR)/bn_c256_test.exe: $(OBJ_DIR)/bn_c256_test.o $(BN256_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(BN256_LIB) $(MCL_LIB) $(LDFLAGS)

$(EXE_DIR)/bn_c384_test.exe: $(OBJ_DIR)/bn_c384_test.o $(BN384_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(BN384_LIB) $(MCL_LIB) $(LDFLAGS)

$(EXE_DIR)/bn_c384_256_test.exe: $(OBJ_DIR)/bn_c384_256_test.o $(BN384_256_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(BN384_256_LIB) $(MCL_LIB) $(LDFLAGS)

$(EXE_DIR)/bn_c512_test.exe: $(OBJ_DIR)/bn_c512_test.o $(BN512_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(BN512_LIB) $(MCL_LIB) $(LDFLAGS)

$(EXE_DIR)/pairing_c.exe: $(OBJ_DIR)/pairing_c.o $(BN384_256_LIB) $(MCL_LIB)
	$(PRE)$(CC) $< -o $@ $(BN384_256_LIB) $(MCL_LIB) $(LDFLAGS) -lstdc++

$(EXE_DIR)/she_c256_test.exe: $(OBJ_DIR)/she_c256_test.o $(SHE256_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(SHE256_LIB) $(MCL_LIB) $(LDFLAGS)

$(EXE_DIR)/she_c384_test.exe: $(OBJ_DIR)/she_c384_test.o $(SHE384_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(SHE384_LIB) $(MCL_LIB) $(LDFLAGS)

$(EXE_DIR)/she_c384_256_test.exe: $(OBJ_DIR)/she_c384_256_test.o $(SHE384_256_LIB) $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(SHE384_256_LIB) $(MCL_LIB) $(LDFLAGS)

$(OBJ_DIR)/modp_test.o: test/modp_test.cpp
	$(PRE)$(CXX) -c $< -o $@ -MMD -MP -MF $(@:.o=.d) -DMCL_USE_VINT -DMCL_MAX_BIT_SIZE=384 -DMCL_VINT_64BIT_PORTABLE -DMCL_SIZEOF_UNIT=8 -DMCL_VINT_FIXED_BUFFER -I./include -O2 $(CFLAGS_WARN)

$(EXE_DIR)/modp_test.exe: $(OBJ_DIR)/modp_test.o
	$(PRE)$(CXX) $< -o $@

$(EXE_DIR)/ecdsa_c_test.exe: $(OBJ_DIR)/ecdsa_c_test.o $(ECDSA_LIB) $(MCL_LIB) src/ecdsa_c.cpp include/mcl/ecdsa.hpp include/mcl/ecdsa.h
	$(PRE)$(CXX) $< -o $@ $(ECDSA_LIB) $(MCL_LIB) $(LDFLAGS)

SAMPLE_EXE=$(addprefix $(EXE_DIR)/,$(addsuffix .exe,$(basename $(SAMPLE_SRC))))
sample: $(SAMPLE_EXE) $(MCL_LIB)

TEST_EXE=$(addprefix $(EXE_DIR)/,$(TEST_SRC:.cpp=.exe))
test_ci: $(TEST_EXE)
	@sh -ec 'for i in $(TEST_EXE); do echo $$i; env LSAN_OPTIONS=verbosity=0:log_threads=1 $$i; done'
test: $(TEST_EXE)
	@echo test $(TEST_EXE)
	@sh -ec 'for i in $(TEST_EXE); do $$i|grep "ctest:name"; done' > result.txt
	@grep -v "ng=0, exception=0" result.txt; if [ $$? -eq 1 ]; then echo "all unit tests succeed"; else exit 1; fi

EMCC_OPT=-I./include -I./src -Wall -Wextra
EMCC_OPT+=-O3 -DNDEBUG -DMCLSHE_WIN_SIZE=8
EMCC_OPT+=-s WASM=1 -s NO_EXIT_RUNTIME=1 -s NODEJS_CATCH_EXIT=0 -s NODEJS_CATCH_REJECTION=0  -s MODULARIZE=1 #-s ASSERTIONS=1
EMCC_OPT+=-DCYBOZU_MINIMUM_EXCEPTION
EMCC_OPT+=-s ABORTING_MALLOC=0
SHE_C_DEP=src/fp.cpp src/she_c_impl.hpp include/mcl/she.hpp include/mcl/fp.hpp include/mcl/op.hpp include/mcl/she.h Makefile
MCL_C_DEP=src/fp.cpp include/mcl/impl/bn_c_impl.hpp include/mcl/bn.hpp include/mcl/fp.hpp include/mcl/op.hpp include/mcl/bn.h Makefile
ifeq ($(MCL_USE_LLVM),2)
  EMCC_OPT+=src/base64m.ll -DMCL_USE_LLVM
  SHE_C_DEP+=src/base64m.ll
endif

# test
bin/emu:
	$(CXX) -g -o $@ src/fp.cpp src/bn_c256.cpp test/bn_c256_test.cpp -DMCL_DONT_USE_XBYAK -DMCL_DONT_USE_OPENSSL -DMCL_USE_VINT -DMCL_SIZEOF_UNIT=8 -DMCL_VINT_64BIT_PORTABLE -DMCL_VINT_FIXED_BUFFER -DMCL_MAX_BIT_SIZE=256 -I./include
bin/pairing_c_min.exe: sample/pairing_c.c include/mcl/vint.hpp src/fp.cpp include/mcl/bn.hpp
	$(CXX) -std=c++03 -O3 -g -fno-threadsafe-statics -fno-exceptions -fno-rtti -o $@ sample/pairing_c.c src/fp.cpp src/bn_c384_256.cpp -I./include -DXBYAK_NO_EXCEPTION -DMCL_DONT_USE_OPENSSL -DMCL_USE_VINT -DMCL_SIZEOF_UNIT=8 -DMCL_VINT_FIXED_BUFFER -DMCL_MAX_BIT_SIZE=384 -DMCL_VINT_64BIT_PORTABLE -DCYBOZU_DONT_USE_STRING -DCYBOZU_DONT_USE_EXCEPTION -DNDEBUG # -DMCL_DONT_USE_CSPRNG
bin/ecdsa-emu:
	$(CXX) -g -o $@ src/fp.cpp test/ecdsa_test.cpp -DMCL_SIZEOF_UNIT=4 -D__EMSCRIPTEN__ -DMCL_MAX_BIT_SIZE=256 -I./include

bin/llvm_test64.exe: test/llvm_test.cpp src/base64.ll
	clang++$(LLVM_VER) -o $@ -Ofast -DNDEBUG -Wall -Wextra -I ./include test/llvm_test.cpp src/base64.ll

bin/llvm_test32.exe: test/llvm_test.cpp src/base32.ll
	clang++$(LLVM_VER) -o $@ -Ofast -DNDEBUG -Wall -Wextra -I ./include test/llvm_test.cpp src/base32.ll -m32

make_tbl:
	$(MAKE) ../bls/src/qcoeff-bn254.hpp

../bls/src/qcoeff-bn254.hpp:  $(MCL_LIB) misc/precompute.cpp
	$(CXX) -o misc/precompute misc/precompute.cpp $(CFLAGS) $(MCL_LIB) $(LDFLAGS)
	./misc/precompute > ../bls/src/qcoeff-bn254.hpp

update_xbyak:
	cp -a ../xbyak/xbyak/xbyak.h ../xbyak/xbyak/xbyak_util.h ../xbyak/xbyak/xbyak_mnemonic.h src/xbyak/

update_cybozulib:
	cp -a $(addprefix ../cybozulib/,$(wildcard include/cybozu/*.hpp)) include/cybozu/

clean:
	$(RM) $(LIB_DIR)/*.a $(LIB_DIR)/*.$(LIB_SUF) $(OBJ_DIR)/*.o $(OBJ_DIR)/*.obj $(OBJ_DIR)/*.d $(EXE_DIR)/*.exe $(GEN_EXE) $(ASM_OBJ) $(LIB_OBJ) $(BN256_OBJ) $(BN384_OBJ) $(BN512_OBJ) $(FUNC_LIST) lib/*.a src/static_code.asm src/dump_code

ALL_SRC=$(SRC_SRC) $(TEST_SRC) $(SAMPLE_SRC)
DEPEND_FILE=$(addprefix $(OBJ_DIR)/, $(addsuffix .d,$(basename $(ALL_SRC))))
-include $(DEPEND_FILE)

PREFIX?=/usr/local
install: lib/libmcl.a lib/libmcl.$(LIB_SUF)
	$(MKDIR) $(PREFIX)/include/mcl
	cp -a include/mcl $(PREFIX)/include/
	cp -a include/cybozu $(PREFIX)/include/
	$(MKDIR) $(PREFIX)/lib
	cp -a lib/libmcl.a lib/libmcl.$(LIB_SUF) $(PREFIX)/lib/

.PHONY: test she-wasm bin/emu

# don't remove these files automatically
.SECONDARY: $(addprefix $(OBJ_DIR)/, $(ALL_SRC:.cpp=.o))

