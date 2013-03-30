FLAGS_COMMON:=-Wall -Wno-unused -fPIC
FLAGS_PROD:=-DNDEBUG -O2 -march=native
FLAGS_DEBUG:=-DVERIFY -ggdb3 -O1
FLAGS_TEST:=-DVERIFY -ggdb3 -O2 -march=native

SECP256K1_FILES := src/num.h   src/field.h   src/field_5x52.h   src/group.h   src/ecmult.h   src/ecdsa.h   \
                   src/num.cpp src/field.cpp src/field_5x52.cpp src/group.cpp src/ecmult.cpp src/ecdsa.cpp


ifndef CONF
CONF := gmp
endif

OBJS := obj/secp256k1-$(CONF).o

default: all

ifeq ($(CONF), openssl)
FLAGS_CONF:=-DUSE_NUM_OPENSSL -DUSE_FIELD_INV_BUILTIN
LIBS:=-lcrypto
SECP256K1_FILES := $(SECP256K1_FILES) src/num_openssl.h src/num_openssl.cpp src/field_5x52_int128.cpp
else
ifeq ($(CONF), gmp)
FLAGS_CONF:=-DUSE_NUM_GMP
LIBS:=-lgmp
SECP256K1_FILES := $(SECP256K1_FILES) src/num_gmp.h src/num_gmp.cpp src/field_5x52_int128.cpp
else
ifeq ($(CONF), gmpasm)
FLAGS_CONF:=-DUSE_NUM_GMP -DUSE_FIELD_5X52_ASM
LIBS:=-lgmp obj/field_5x52_asm.o
OBJS:=$(OBJS) obj/field_5x52_asm.o
SECP256K1_FILES := $(SECP256K1_FILES) src/num_gmp.h src/num_gmp.cpp src/field_5x52_asm.cpp

obj/field_5x52_asm.o: src/field_5x52_asm.asm
	yasm -f elf64 -o obj/field_5x52_asm.o src/field_5x52_asm.asm
else
SECP256K1_FILES := $(SECP256K1_FILES) src/field_5x52_int128.cpp
endif
endif
endif


all: src/*.cpp src/*.asm src/*.h include/*.h
	+make CONF=openssl all-openssl
	+make CONF=gmp     all-gmp
	+make CONF=gmpasm  all-gmpasm

clean:
	+make CONF=openssl clean-openssl
	+make CONF=gmp     clean-gmp
	+make CONF=gmpasm  clean-gmpasm

bench-any: bench-$(CONF)
tests-any: tests-$(CONF)

all-$(CONF): bench-$(CONF) tests-$(CONF) libsecp256k1-$(CONF).a

clean-$(CONF):
	rm -f bench-$(CONF) tests-$(CONF) libsecp256k1-$(CONF).a obj/*

obj/secp256k1-$(CONF).o: $(SECP256K1_FILES) src/secp256k1.cpp include/secp256k1.h
	$(CXX) $(FLAGS_COMMON) $(FLAGS_PROD) $(FLAGS_CONF) src/secp256k1.cpp -c -o obj/secp256k1-$(CONF).o

bench-$(CONF): $(OBJS) src/bench.cpp
	$(CXX) $(FLAGS_COMMON) $(FLAGS_PROD) $(FLAGS_CONF) src/bench.cpp $(LIBS) -o bench-$(CONF)

tests-$(CONF): $(OBJS) src/tests.cpp
	$(CXX) $(FLAGS_COMMON) $(FLAGS_TEST) $(FLAGS_CONF) src/tests.cpp $(LIBS) -o tests-$(CONF)

libsecp256k1-$(CONF).a: $(OBJS)
	$(AR) -rs $@ $(OBJS)
