ACLOCAL_AMFLAGS = -I build-aux/m4

# AM_CFLAGS will be automatically prepended to CFLAGS by Automake when compiling some foo
# which does not have an explicit foo_CFLAGS variable set.
AM_CFLAGS = $(SECP_CFLAGS)

lib_LTLIBRARIES = libsecp256k1.la
include_HEADERS = include/secp256k1.h
include_HEADERS += include/secp256k1_preallocated.h
noinst_HEADERS =
noinst_HEADERS += src/scalar.h
noinst_HEADERS += src/scalar_4x64.h
noinst_HEADERS += src/scalar_8x32.h
noinst_HEADERS += src/scalar_low.h
noinst_HEADERS += src/scalar_impl.h
noinst_HEADERS += src/scalar_4x64_impl.h
noinst_HEADERS += src/scalar_8x32_impl.h
noinst_HEADERS += src/scalar_low_impl.h
noinst_HEADERS += src/group.h
noinst_HEADERS += src/group_impl.h
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
noinst_HEADERS += src/field_10x26.h
noinst_HEADERS += src/field_10x26_impl.h
noinst_HEADERS += src/field_5x52.h
noinst_HEADERS += src/field_5x52_impl.h
noinst_HEADERS += src/field_5x52_int128_impl.h
noinst_HEADERS += src/field_5x52_asm_impl.h
noinst_HEADERS += src/modinv32.h
noinst_HEADERS += src/modinv32_impl.h
noinst_HEADERS += src/modinv64.h
noinst_HEADERS += src/modinv64_impl.h
noinst_HEADERS += src/assumptions.h
noinst_HEADERS += src/util.h
noinst_HEADERS += src/scratch.h
noinst_HEADERS += src/scratch_impl.h
noinst_HEADERS += src/selftest.h
noinst_HEADERS += src/testrand.h
noinst_HEADERS += src/testrand_impl.h
noinst_HEADERS += src/hash.h
noinst_HEADERS += src/hash_impl.h
noinst_HEADERS += src/field.h
noinst_HEADERS += src/field_impl.h
noinst_HEADERS += src/bench.h
noinst_HEADERS += contrib/lax_der_parsing.h
noinst_HEADERS += contrib/lax_der_parsing.c
noinst_HEADERS += contrib/lax_der_privatekey_parsing.h
noinst_HEADERS += contrib/lax_der_privatekey_parsing.c

if USE_EXTERNAL_ASM
COMMON_LIB = libsecp256k1_common.la
noinst_LTLIBRARIES = $(COMMON_LIB)
else
COMMON_LIB =
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsecp256k1.pc

if USE_EXTERNAL_ASM
if USE_ASM_ARM
libsecp256k1_common_la_SOURCES = src/asm/field_10x26_arm.s
endif
endif

libsecp256k1_la_SOURCES = src/secp256k1.c
libsecp256k1_la_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src $(SECP_INCLUDES)
libsecp256k1_la_LIBADD = $(SECP_LIBS) $(COMMON_LIB)

if VALGRIND_ENABLED
libsecp256k1_la_CPPFLAGS += -DVALGRIND
endif

noinst_PROGRAMS =
if USE_BENCHMARK
noinst_PROGRAMS += bench_verify bench_sign bench_internal bench_ecmult
bench_verify_SOURCES = src/bench_verify.c
bench_verify_LDADD = libsecp256k1.la $(SECP_LIBS) $(SECP_TEST_LIBS) $(COMMON_LIB)
# SECP_TEST_INCLUDES are only used here for CRYPTO_CPPFLAGS
bench_verify_CPPFLAGS = $(SECP_TEST_INCLUDES)
bench_sign_SOURCES = src/bench_sign.c
bench_sign_LDADD = libsecp256k1.la $(SECP_LIBS) $(SECP_TEST_LIBS) $(COMMON_LIB)
bench_internal_SOURCES = src/bench_internal.c
bench_internal_LDADD = $(SECP_LIBS) $(COMMON_LIB)
bench_internal_CPPFLAGS = $(SECP_INCLUDES)
bench_ecmult_SOURCES = src/bench_ecmult.c
bench_ecmult_LDADD = $(SECP_LIBS) $(COMMON_LIB)
bench_ecmult_CPPFLAGS = $(SECP_INCLUDES)
endif

TESTS =
if USE_TESTS
noinst_PROGRAMS += tests
tests_SOURCES = src/tests.c
tests_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/include $(SECP_INCLUDES) $(SECP_TEST_INCLUDES)
if VALGRIND_ENABLED
tests_CPPFLAGS += -DVALGRIND
noinst_PROGRAMS += valgrind_ctime_test
valgrind_ctime_test_SOURCES = src/valgrind_ctime_test.c
valgrind_ctime_test_LDADD = libsecp256k1.la $(SECP_LIBS) $(COMMON_LIB)
endif
if !ENABLE_COVERAGE
tests_CPPFLAGS += -DVERIFY
endif
tests_LDADD = $(SECP_LIBS) $(SECP_TEST_LIBS) $(COMMON_LIB)
tests_LDFLAGS = -static
TESTS += tests
endif

if USE_EXHAUSTIVE_TESTS
noinst_PROGRAMS += exhaustive_tests
exhaustive_tests_SOURCES = src/tests_exhaustive.c
exhaustive_tests_CPPFLAGS = -I$(top_srcdir)/src $(SECP_INCLUDES)
if !ENABLE_COVERAGE
exhaustive_tests_CPPFLAGS += -DVERIFY
endif
exhaustive_tests_LDADD = $(SECP_LIBS) $(COMMON_LIB)
exhaustive_tests_LDFLAGS = -static
TESTS += exhaustive_tests
endif

if USE_ECMULT_STATIC_PRECOMPUTATION
CPPFLAGS_FOR_BUILD +=-I$(top_srcdir) -I$(builddir)/src

gen_context_OBJECTS = gen_context.o
gen_context_BIN = gen_context$(BUILD_EXEEXT)
gen_%.o: src/gen_%.c src/libsecp256k1-config.h
	$(CC_FOR_BUILD) $(DEFS) $(CPPFLAGS_FOR_BUILD) $(SECP_CFLAGS_FOR_BUILD) $(CFLAGS_FOR_BUILD) -c $< -o $@

$(gen_context_BIN): $(gen_context_OBJECTS)
	$(CC_FOR_BUILD) $(SECP_CFLAGS_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) $^ -o $@

$(libsecp256k1_la_OBJECTS): src/ecmult_static_context.h
$(tests_OBJECTS): src/ecmult_static_context.h
$(bench_internal_OBJECTS): src/ecmult_static_context.h
$(bench_ecmult_OBJECTS): src/ecmult_static_context.h

src/ecmult_static_context.h: $(gen_context_BIN)
	./$(gen_context_BIN)

CLEANFILES = $(gen_context_BIN) src/ecmult_static_context.h
endif

EXTRA_DIST = autogen.sh src/gen_context.c src/basic-config.h

if ENABLE_MODULE_ECDH
include src/modules/ecdh/Makefile.am.include
endif

if ENABLE_MODULE_RECOVERY
include src/modules/recovery/Makefile.am.include
endif

if ENABLE_MODULE_EXTRAKEYS
include src/modules/extrakeys/Makefile.am.include
endif

if ENABLE_MODULE_SCHNORRSIG
include src/modules/schnorrsig/Makefile.am.include
endif
