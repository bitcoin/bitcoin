FLAGS_COMMON:=-Wall -Wno-unused -fPIC -std=c99
FLAGS_PROD:=-DNDEBUG -O2 -march=native
FLAGS_DEBUG:=-DVERIFY -ggdb3 -O1
FLAGS_TEST:=-DVERIFY -ggdb3 -O2 -march=native

SECP256K1_FILES := src/num.h   src/field.h   src/field_5x52.h   src/group.h   src/ecmult.h   src/ecdsa.h   \
                   src/num.c src/field.c src/field_5x52.c src/group.c src/ecmult.c src/ecdsa.c

JAVA_FILES := src/java/org_bitcoin_NativeSecp256k1.h src/java/org_bitcoin_NativeSecp256k1.c

ifndef CONF
CONF := gmp
endif

OBJS := obj/secp256k1-$(CONF).o

default: all

ifeq ($(CONF), openssl)
FLAGS_CONF:=-DUSE_NUM_OPENSSL -DUSE_FIELD_INV_BUILTIN
LIBS:=-lcrypto
SECP256K1_FILES := $(SECP256K1_FILES) src/num_openssl.h src/num_openssl.c src/field_5x52_int128.c
else
ifeq ($(CONF), gmp)
FLAGS_CONF:=-DUSE_NUM_GMP
LIBS:=-lgmp
SECP256K1_FILES := $(SECP256K1_FILES) src/num_gmp.h src/num_gmp.c src/field_5x52_int128.c
else
ifeq ($(CONF), gmpasm)
FLAGS_CONF:=-DUSE_NUM_GMP -DUSE_FIELD_5X52_ASM
LIBS:=-lgmp obj/field_5x52_asm.o
OBJS:=$(OBJS) obj/field_5x52_asm.o
SECP256K1_FILES := $(SECP256K1_FILES) src/num_gmp.h src/num_gmp.c src/field_5x52_asm.c

obj/field_5x52_asm.o: src/field_5x52_asm.asm
	yasm -f elf64 -o obj/field_5x52_asm.o src/field_5x52_asm.asm
else
SECP256K1_FILES := $(SECP256K1_FILES) src/field_5x52_int128.c
endif
endif
endif


all: src/*.c src/*.asm src/*.h include/*.h
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
	rm -f bench-$(CONF) tests-$(CONF) libsecp256k1-$(CONF).a libjavasecp256k1-$(CONF).so obj/*

obj/secp256k1-$(CONF).o: $(SECP256K1_FILES) src/secp256k1.c include/secp256k1.h
	$(CC) $(FLAGS_COMMON) $(FLAGS_PROD) $(FLAGS_CONF) src/secp256k1.c -c -o obj/secp256k1-$(CONF).o

bench-$(CONF): $(OBJS) src/bench.c
	$(CC) $(FLAGS_COMMON) $(FLAGS_PROD) $(FLAGS_CONF) src/bench.c $(LIBS) -o bench-$(CONF)

tests-$(CONF): $(OBJS) src/tests.c
	$(CC) $(FLAGS_COMMON) $(FLAGS_TEST) $(FLAGS_CONF) src/tests.c $(LIBS) -o tests-$(CONF)

libsecp256k1-$(CONF).a: $(OBJS)
	$(AR) -rs $@ $(OBJS)

libjavasecp256k1-$(CONF).so: $(OBJS) $(JAVA_FILES)
	$(CC) $(FLAGS_COMMON) $(FLAGS_PROD) $(FLAGS_CONF) -I. src/java/org_bitcoin_NativeSecp256k1.c $(LIBS) $(OBJS) -shared -o libjavasecp256k1-$(CONF).so

java: libjavasecp256k1-$(CONF).so
