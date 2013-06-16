$(shell CC=$(CC) YASM=$(YASM) ./configure)
include config.mk

FILES := src/*.h src/impl/*.h

JAVA_FILES := src/java/org_bitcoin_NativeSecp256k1.h src/java/org_bitcoin_NativeSecp256k1.c

OBJS :=

ifeq ($(USE_ASM), 1)
    OBJS := $(OBJS) obj/field_5x$(HAVE_LIMB)_asm.o
endif
STD="gnu99"

default: tests libsecp256k1.a libsecp256k1.so

clean:
	rm -rf obj/*.o bench tests *.a *.so config.mk

obj/field_5x52_asm.o: src/field_5x52_asm.asm
	$(YASM) -f elf64 -o obj/field_5x52_asm.o src/field_5x52_asm.asm

obj/field_5x64_asm.o: src/field_5x64_asm.asm
	$(YASM) -f elf64 -o obj/field_5x64_asm.o src/field_5x64_asm.asm

obj/secp256k1.o: $(FILES) src/secp256k1.c include/secp256k1.h
	$(CC) -fPIC -std=$(STD) $(CFLAGS) $(CFLAGS_EXTRA) -DNDEBUG -$(OPTLEVEL) src/secp256k1.c -c -o obj/secp256k1.o

bench: $(FILES) src/bench.c $(OBJS)
	$(CC) -fPIC -std=$(STD) $(CFLAGS) $(CFLAGS_EXTRA) $(CFLAGS_TEST_EXTRA) -DNDEBUG -$(OPTLEVEL) src/bench.c $(OBJS) $(LDFLAGS_EXTRA) $(LDFLAGS_TEST_EXTRA) -o bench

tests: $(FILES) src/tests.c $(OBJS)
	$(CC) -std=$(STD) $(CFLAGS) $(CFLAGS_EXTRA) $(CFLAGS_TEST_EXTRA) -DVERIFY -fstack-protector-all -$(OPTLEVEL) -ggdb3 src/tests.c $(OBJS) $(LDFLAGS_EXTRA) $(LDFLAGS_TEST_EXTRA) -o tests

coverage: $(FILES) src/tests.c $(OBJS)
	rm -rf tests.gcno tests.gcda tests_cov
	$(CC) -std=$(STD) $(CFLAGS) $(CFLAGS_EXTRA) $(CFLAGS_TEST_EXTRA) -DVERIFY --coverage -$(OPTLEVEL) -g src/tests.c $(OBJS) $(LDFLAGS_EXTRA) $(LDFLAGS_TEST_EXTRA) -o tests_cov
	rm -rf lcov
	mkdir -p lcov
	cd lcov; lcov --directory ../ --zerocounters
	cd lcov; ../tests_cov
	cd lcov; lcov --directory ../ --capture --output-file secp256k1.info
	cd lcov; genhtml -o . secp256k1.info

libsecp256k1.a: obj/secp256k1.o $(OBJS)
	$(AR) -rs $@ $(OBJS) obj/secp256k1.o

libsecp256k1.so: obj/secp256k1.o $(OBJS)
	$(CC) -std=$(STD) $(LDFLAGS_EXTRA) $(OBJS) obj/secp256k1.o -shared -o libsecp256k1.so

libjavasecp256k1.so: $(OBJS) obj/secp256k1.o $(JAVA_FILES)
	$(CC) -fPIC -std=$(STD) $(CFLAGS) $(CFLAGS_EXTRA) -DNDEBUG -$(OPTLEVEL) -I. src/java/org_bitcoin_NativeSecp256k1.c $(LDFLAGS_EXTRA) $(OBJS) obj/secp256k1.o -shared -o libjavasecp256k1.so
