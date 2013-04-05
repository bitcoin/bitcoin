FLAGS_COMMON:=-Wall -Wno-unused -fPIC -std=c99
FLAGS_PROD:=-DNDEBUG -O2 -march=native
FLAGS_DEBUG:=-DVERIFY -ggdb3 -O1
FLAGS_TEST:=-DVERIFY -ggdb3 -O2 -march=native

SECP256K1_FILES := src/num.h src/field.h src/field_5x52.h src/group.h src/ecmult.h src/ecdsa.h \
                   src/impl/*.h

JAVA_FILES := src/java/org_bitcoin_NativeSecp256k1.h src/java/org_bitcoin_NativeSecp256k1.c

ifndef CONF
CONF := gmp
endif

OBJS :=

default: all

ifeq ($(CONF), gmpgmp)
FLAGS_COMMON := $(FLAGS_COMMON) -DUSE_NUM_GMP -DUSE_FIELD_GMP
LIBS := -lgmp
SECP256K1_FILES := $(SECP256K1_FILES) src/num_gmp.h src/field_gmp.h
else
ifeq ($(CONF), gmp32)
FLAGS_COMMON := $(FLAGS_COMMON) -DUSE_NUM_GMP -DUSE_FIELD_10X26
LIBS := -lgmp
SECP256K1_FILES := $(SECP256K1_FILES) src/num_gmp.h src/field_10x26.h
else
ifeq ($(CONF), openssl)
FLAGS_COMMON := $(FLAGS_COMMON) -DUSE_NUM_OPENSSL -DUSE_FIELD_INV_BUILTIN
LIBS := -lcrypto
SECP256K1_FILES := $(SECP256K1_FILES) src/num_openssl.h src/field_5x52.h
else
ifeq ($(CONF), gmp)
FLAGS_COMMON := $(FLAGS_COMMON) -DUSE_NUM_GMP
LIBS := -lgmp
SECP256K1_FILES := $(SECP256K1_FILES) src/num_gmp.h src/field_5x52.h
else
ifeq ($(CONF), gmpasm)
FLAGS_COMMON := $(FLAGS_COMMON) -DUSE_NUM_GMP -DUSE_FIELD_5X52_ASM
LIBS := -lgmp
OBJS := $(OBJS) obj/field_5x52_asm.o
SECP256K1_FILES := $(SECP256K1_FILES) src/num_gmp.h src/field_5x52.h

obj/field_5x52_asm.o: src/field_5x52_asm.asm
	yasm -f elf64 -o obj/field_5x52_asm.o src/field_5x52_asm.asm
else
error
endif
endif
endif
endif
endif

all: src/*.c src/*.asm src/*.h include/*.h
	+make CONF=openssl all-openssl
	+make CONF=gmp     all-gmp
	+make CONF=gmpgmp  all-gmpgmp
	+make CONF=gmp32   all-gmp32
	+make CONF=gmpasm  all-gmpasm

clean:
	+make CONF=openssl clean-openssl
	+make CONF=gmp     clean-gmp
	+make CONF=gmp32   clean-gmp32
	+make CONF=gmpasm  clean-gmpasm

bench-any: bench-$(CONF)
tests-any: tests-$(CONF)

all-$(CONF): bench-$(CONF) tests-$(CONF) libsecp256k1-$(CONF).a

clean-$(CONF):
	rm -f bench-$(CONF) tests-$(CONF) libsecp256k1-$(CONF).a libjavasecp256k1-$(CONF).so obj/*

obj/secp256k1-$(CONF).o: $(SECP256K1_FILES) src/secp256k1.c include/secp256k1.h
	$(CC) $(FLAGS_COMMON) $(FLAGS_PROD) src/secp256k1.c -c -o obj/secp256k1-$(CONF).o

bench-$(CONF): $(OBJS) $(SECP256K1_FILES) src/bench.c
	$(CC) $(FLAGS_COMMON) $(FLAGS_PROD) src/bench.c $(LIBS) $(OBJS) -o bench-$(CONF)

tests-$(CONF): $(OBJS) $(SECP256K1_FILES) src/tests.c
	$(CC) $(FLAGS_COMMON) $(FLAGS_TEST) src/tests.c $(LIBS) $(OBJS) -o tests-$(CONF)

libsecp256k1-$(CONF).a: $(OBJS) obj/secp256k1-$(CONF).o
	$(AR) -rs $@ $(OBJS) obj/secp256k1-$(CONF).o

libjavasecp256k1-$(CONF).so: $(OBJS) obj/secp256k1-$(CONF).o $(JAVA_FILES)
	$(CC) $(FLAGS_COMMON) $(FLAGS_PROD) -I. src/java/org_bitcoin_NativeSecp256k1.c $(LIBS) $(OBJS) obj/secp256k1-$(CONF).o -shared -o libjavasecp256k1-$(CONF).so

java: libjavasecp256k1-$(CONF).so
