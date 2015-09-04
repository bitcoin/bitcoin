ACLOCAL_AMFLAGS = -I build-aux/m4

lib_LTLIBRARIES = libsecp256k1.la
include_HEADERS = include/secp256k1.h
noinst_HEADERS =
noinst_HEADERS += src/scalar.h
noinst_HEADERS += src/scalar_4x64.h
noinst_HEADERS += src/scalar_8x32.h
noinst_HEADERS += src/scalar_impl.h
noinst_HEADERS += src/scalar_4x64_impl.h
noinst_HEADERS += src/scalar_8x32_impl.h
noinst_HEADERS += src/group.h
noinst_HEADERS += src/group_impl.h
noinst_HEADERS += src/num_gmp.h
noinst_HEADERS += src/num_gmp_impl.h
noinst_HEADERS += src/ecdsa.h
noinst_HEADERS += src/ecdsa_impl.h
noinst_HEADERS += src/eckey.h
noinst_HEADERS += src/eckey_impl.h
noinst_HEADERS += src/ecmult.h
noinst_HEADERS += src/ecmult_impl.h
noinst_HEADERS += src/ecmult_const.h
noinst_HEADERS += src/ecmult_const_impl.h
noinst_HEADERS += src/ecmult_gen.h
noinst_HEADERS += src/ecmult_gen_impl.h
noinst_HEADERS += src/num.h
noinst_HEADERS += src/num_impl.h
noinst_HEADERS += src/field_10x26.h
noinst_HEADERS += src/field_10x26_impl.h
noinst_HEADERS += src/field_5x52.h
noinst_HEADERS += src/field_5x52_impl.h
noinst_HEADERS += src/field_5x52_int128_impl.h
noinst_HEADERS += src/field_5x52_asm_impl.h
noinst_HEADERS += src/java/org_bitcoin_NativeSecp256k1.h
noinst_HEADERS += src/util.h
noinst_HEADERS += src/testrand.h
noinst_HEADERS += src/testrand_impl.h
noinst_HEADERS += src/hash.h
noinst_HEADERS += src/hash_impl.h
noinst_HEADERS += src/field.h
noinst_HEADERS += src/field_impl.h
noinst_HEADERS += src/bench.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsecp256k1.pc

libsecp256k1_la_SOURCES = src/secp256k1.c
libsecp256k1_la_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src $(SECP_INCLUDES)
libsecp256k1_la_LIBADD = $(SECP_LIBS)


noinst_PROGRAMS =
if USE_BENCHMARK
noinst_PROGRAMS += bench_verify bench_sign bench_internal
bench_verify_SOURCES = src/bench_verify.c
bench_verify_LDADD = libsecp256k1.la $(SECP_LIBS)
bench_verify_LDFLAGS = -static
bench_sign_SOURCES = src/bench_sign.c
bench_sign_LDADD = libsecp256k1.la $(SECP_LIBS)
bench_sign_LDFLAGS = -static
bench_internal_SOURCES = src/bench_internal.c
bench_internal_LDADD = $(SECP_LIBS)
bench_internal_LDFLAGS = -static
bench_internal_CPPFLAGS = $(SECP_INCLUDES)
endif

if USE_TESTS
noinst_PROGRAMS += tests
tests_SOURCES = src/tests.c
tests_CPPFLAGS = -DVERIFY -I$(top_srcdir)/src $(SECP_INCLUDES) $(SECP_TEST_INCLUDES)
tests_LDADD = $(SECP_LIBS) $(SECP_TEST_LIBS)
tests_LDFLAGS = -static
TESTS = tests
endif

if USE_ECMULT_STATIC_PRECOMPUTATION
CPPFLAGS_FOR_BUILD +=-I$(top_srcdir)/
CFLAGS_FOR_BUILD += -Wall -Wextra -Wno-unused-function

gen_context_OBJECTS = gen_context.o
gen_context_BIN = gen_context$(BUILD_EXEEXT)
gen_%.o: src/gen_%.c
	$(CC_FOR_BUILD) $(CPPFLAGS_FOR_BUILD) $(CFLAGS_FOR_BUILD) -c $< -o $@

$(gen_context_BIN): $(gen_context_OBJECTS)
	$(CC_FOR_BUILD) $^ -o $@

$(libsecp256k1_la_OBJECTS): src/ecmult_static_context.h
$(tests_OBJECTS): src/ecmult_static_context.h
$(bench_internal_OBJECTS): src/ecmult_static_context.h

src/ecmult_static_context.h: $(gen_context_BIN)
	./$(gen_context_BIN)

CLEANFILES = $(gen_context_BIN) src/ecmult_static_context.h
endif

EXTRA_DIST = autogen.sh src/gen_context.c src/basic-config.h

if ENABLE_MODULE_ECDH
include src/modules/ecdh/Makefile.am.include
endif

if ENABLE_MODULE_SCHNORR
include src/modules/schnorr/Makefile.am.include
endif

if ENABLE_MODULE_RECOVERY
include src/modules/recovery/Makefile.am.include
endif
