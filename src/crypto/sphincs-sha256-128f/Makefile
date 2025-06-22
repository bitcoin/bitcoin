CC = /usr/bin/gcc
CFLAGS = -Wall -Wextra -Wpedantic -O3
LDLIBS += -lcrypto
HASH_C = hash_sha256.c

SOURCES =          $(HASH_C) hash_address.c rng.c wots.c utils.c fors.c sign.c
HEADERS = params.h hash.h    hash_address.h rng.h wots.h utils.h fors.h api.h

ifeq ($(HASH_C),hash_shake256.c)
	SOURCES += fips202.c
	HEADERS += fips202.h
endif
ifeq ($(HASH_C),hash_haraka.c)
	SOURCES += haraka.c
	HEADERS += haraka.h
endif

DET_SOURCES = $(SOURCES:rng.%=rng.%)
DET_HEADERS = $(HEADERS:rng.%=rng.%)

TESTS = test/wots \
		test/fors \
		test/spx \

.PHONY: clean test

default: PQCgenKAT_sign

all: PQCgenKAT_sign tests

tests: $(TESTS)

test: $(TESTS:=.exec)

PQCgenKAT_sign: PQCgenKAT_sign.c $(DET_SOURCES) $(DET_HEADERS)
	$(CC) $(CFLAGS) -o $@ $(DET_SOURCES) $< $(LDLIBS)

test/%: test/%.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $< $(LDLIBS)

test/%.exec: test/%
	@$<

clean:
	-$(RM) $(TESTS)
	-$(RM) PQCgenKAT_sign
	-$(RM) PQCsignKAT_*.rsp
	-$(RM) PQCsignKAT_*.req
