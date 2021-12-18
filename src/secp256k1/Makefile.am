.PHONY: clean-precomp precomp

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
noinst_HEADERS += src/ecmult_gen_prec.h
noinst_HEADERS += src/ecmult_gen_prec_impl.h
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
noinst_HEADERS += src/basic-config.h
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
libsecp256k1_la_LDFLAGS = -no-undefined

if VALGRIND_ENABLED
libsecp256k1_la_CPPFLAGS += -DVALGRIND
endif

noinst_PROGRAMS =
if USE_BENCHMARK
noinst_PROGRAMS += bench bench_internal bench_ecmult
bench_SOURCES = src/bench.c
bench_LDADD = libsecp256k1.la $(SECP_LIBS) $(SECP_TEST_LIBS) $(COMMON_LIB)
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
exhaustive_tests_CPPFLAGS = $(SECP_INCLUDES)
if !ENABLE_COVERAGE
exhaustive_tests_CPPFLAGS += -DVERIFY
endif
exhaustive_tests_LDADD = $(SECP_LIBS) $(COMMON_LIB)
exhaustive_tests_LDFLAGS = -static
TESTS += exhaustive_tests
endif

### Precomputed tables
EXTRA_PROGRAMS = gen_ecmult_static_pre_g gen_ecmult_gen_static_prec_table
CLEANFILES = $(EXTRA_PROGRAMS)

gen_ecmult_static_pre_g_SOURCES = src/gen_ecmult_static_pre_g.c
gen_ecmult_static_pre_g_CPPFLAGS = $(SECP_INCLUDES)
gen_ecmult_static_pre_g_LDADD = $(SECP_LIBS) $(COMMON_LIB)

gen_ecmult_gen_static_prec_table_SOURCES = src/gen_ecmult_gen_static_prec_table.c
gen_ecmult_gen_static_prec_table_CPPFLAGS = $(SECP_INCLUDES)
gen_ecmult_gen_static_prec_table_LDADD = $(SECP_LIBS) $(COMMON_LIB)

# See Automake manual, Section "Errors with distclean".
# We don't list any dependencies for the prebuilt files here because
# otherwise make's decision whether to rebuild them (even in the first
# build by a normal user) depends on mtimes, and thus is very fragile.
# This means that rebuilds of the prebuilt files always need to be
# forced by deleting them, e.g., by invoking `make clean-precomp`.
src/ecmult_static_pre_g.h:
	$(MAKE) $(AM_MAKEFLAGS) gen_ecmult_static_pre_g$(EXEEXT)
	./gen_ecmult_static_pre_g$(EXEEXT)
src/ecmult_gen_static_prec_table.h:
	$(MAKE) $(AM_MAKEFLAGS) gen_ecmult_gen_static_prec_table$(EXEEXT)
	./gen_ecmult_gen_static_prec_table$(EXEEXT)

PRECOMP = src/ecmult_gen_static_prec_table.h src/ecmult_static_pre_g.h
noinst_HEADERS += $(PRECOMP)
precomp: $(PRECOMP)

# Ensure the prebuilt files will be build first (only if they don't exist,
# e.g., after `make maintainer-clean`).
BUILT_SOURCES = $(PRECOMP)

maintainer-clean-local: clean-precomp

clean-precomp:
	rm -f $(PRECOMP)

EXTRA_DIST = autogen.sh SECURITY.md

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
