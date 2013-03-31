FLAGS_COMMON:=-Wall -Wno-unused
FLAGS_PROD:=-DNDEBUG -O2 -march=native
FLAGS_DEBUG:=-DVERIFY -ggdb3 -O1
FLAGS_TEST:=-DVERIFY -ggdb3 -O2 -march=native

SECP256K1_FILES := num.h   field.h   field_5x52.h   group.h   ecmult.h   ecdsa.h   \
                   num.cpp field.cpp field_5x52.cpp group.cpp ecmult.cpp ecdsa.cpp

ifndef CONF
CONF := gmp
endif

default: all

ifeq ($(CONF), openssl)
FLAGS_CONF:=-DUSE_NUM_OPENSSL -DUSE_FIELDINVERSE_BUILTIN
LIBS:=-lcrypto
SECP256K1_FILES := $(SECP256K1_FILES) num_openssl.h num_openssl.cpp field_5x52_int128.cpp
else
ifeq ($(CONF), gmp)
FLAGS_CONF:=-DUSE_NUM_GMP
LIBS:=-lgmp
SECP256K1_FILES := $(SECP256K1_FILES) num_gmp.h num_gmp.cpp field_5x52_int128.cpp
else
ifeq ($(CONF), gmpasm)
FLAGS_CONF:=-DUSE_NUM_GMP -DINLINE_ASM
LIBS:=-lgmp obj/field_5x52_asm.o
SECP256K1_FILES := $(SECP256K1_FILES) num_gmp.h num_gmp.cpp field_5x52_asm.cpp obj/field_5x52_asm.o

obj/field_5x52_asm.o: field_5x52_asm.asm
	yasm -f elf64 -o obj/field_5x52_asm.o field_5x52_asm.asm
else
SECP256K1_FILES := $(SECP256K1_FILES) field_5x52_int128.cpp
endif
endif
endif

all: *.cpp *.asm *.h
	+make CONF=openssl all-openssl
	+make CONF=gmp     all-gmp
	+make CONF=gmpasm  all-gmpasm

clean:
	+make CONF=openssl clean-openssl
	+make CONF=gmp     clean-gmp
	+make CONF=gmpasm  clean-gmpasm

bench-any: bench-$(CONF)
tests-any: tests-$(CONF)

all-$(CONF): bench-$(CONF) tests-$(CONF) obj/secp256k1-$(CONF).o

clean-$(CONF):
	rm -f bench-$(CONF) tests-$(CONF) obj/secp256k1-$(CONF).o

obj/secp256k1-$(CONF).o: $(SECP256K1_FILES)
	$(CXX) $(FLAGS_COMMON) $(FLAGS_PROD) $(FLAGS_CONF) secp256k1.cpp -c -o obj/secp256k1-$(CONF).o

bench-$(CONF): $(SECP256K1_FILES) bench.cpp
	$(CXX) $(FLAGS_COMMON) $(FLAGS_PROD) $(FLAGS_CONF) bench.cpp $(LIBS) -o bench-$(CONF)

tests-$(CONF): $(SECP256K1_FILES) tests.cpp
	$(CXX) $(FLAGS_COMMON) $(FLAGS_TEST) $(FLAGS_CONF) tests.cpp $(LIBS) -o tests-$(CONF)
