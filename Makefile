$(shell CC=$(CC) YASM=$(YASM) ./configure)
include config.mk

FILES := src/*.h src/impl/*.h

JAVA_FILES := src/java/org_bitcoin_NativeSecp256k1.h src/java/org_bitcoin_NativeSecp256k1.c

OBJS :=

ifeq ($(USE_ASM), 1)
    OBJS := $(OBJS) obj/field_5x52_asm.o
endif

default: tests libsecp256k1.a libsecp256k1.so
	./tests

clean:
	rm -rf obj/*.o bench tests *.a *.so config.mk

obj/field_5x52_asm.o: src/field_5x52_asm.asm
	$(YASM) -f elf64 -o obj/field_5x52_asm.o src/field_5x52_asm.asm

obj/secp256k1.o: $(FILES) src/secp256k1.c include/secp256k1.h
	$(CC) -fPIC -std=c99 $(CFLAGS) $(CFLAGS_EXTRA) -DNDEBUG -O2 src/secp256k1.c -c -o obj/secp256k1.o

bench: $(FILES) src/bench.c $(OBJS)
	$(CC) -fPIC -std=c99 $(CFLAGS) $(CFLAGS_EXTRA) -DNDEBUG -O2 src/bench.c $(OBJS) $(LDFLAGS_EXTRA) -o bench

tests: $(FILES) src/tests.c $(OBJS)
	$(CC) -std=c99 $(CFLAGS) $(CFLAGS_EXTRA) -DVERIFY -fstack-protector-all -O2 -ggdb3 src/tests.c $(OBJS) $(LDFLAGS_EXTRA) -o tests

coverage: $(FILES) src/tests.c $(OBJS)
	rm -rf tests.gcno tests.gcda tests_cov
	$(CC) -std=c99 $(CFLAGS) $(CFLAGS_EXTRA) -DVERIFY --coverage -O0 -g src/tests.c $(OBJS) $(LDFLAGS_EXTRA) -o tests_cov
	rm -rf lcov
	mkdir -p lcov
	cd lcov; lcov --directory ../ --zerocounters
	cd lcov; ../tests_cov
	cd lcov; lcov --directory ../ --capture --output-file secp256k1.info
	cd lcov; genhtml -o . secp256k1.info

libsecp256k1.a: obj/secp256k1.o $(OBJS)
	$(AR) -rs $@ $(OBJS) obj/secp256k1.o

libsecp256k1.so: obj/secp256k1.o $(OBJS)
	$(CC) -std=c99 $(LDFLAGS_EXTRA) $(OBJS) obj/secp256k1.o -shared -o libsecp256k1.so

libjavasecp256k1.so: $(OBJS) obj/secp256k1.o $(JAVA_FILES)
	$(CC) -fPIC -std=c99 $(CFLAGS) $(CFLAGS_EXTRA) -DNDEBUG -O2 -I. src/java/org_bitcoin_NativeSecp256k1.c $(LDFLAGS_EXTRA) $(OBJS) obj/secp256k1.o -shared -o libjavasecp256k1.so
