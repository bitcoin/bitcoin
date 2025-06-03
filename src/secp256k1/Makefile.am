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
noinst_HEADERS += src/ecmult_compute_table.h
noinst_HEADERS += src/ecmult_compute_table_impl.h
noinst_HEADERS += src/ecmult_const.h
noinst_HEADERS += src/ecmult_const_impl.h
noinst_HEADERS += src/ecmult_gen.h
noinst_HEADERS += src/ecmult_gen_impl.h
noinst_HEADERS += src/ecmult_gen_compute_table.h
noinst_HEADERS += src/ecmult_gen_compute_table_impl.h
noinst_HEADERS += src/field_10x26.h
noinst_HEADERS += src/field_10x26_impl.h
noinst_HEADERS += src/field_5x52.h
noinst_HEADERS += src/field_5x52_impl.h
noinst_HEADERS += src/field_5x52_int128_impl.h
noinst_HEADERS += src/modinv32.h
noinst_HEADERS += src/modinv32_impl.h
noinst_HEADERS += src/modinv64.h
noinst_HEADERS += src/modinv64_impl.h
noinst_HEADERS += src/precomputed_ecmult.h
noinst_HEADERS += src/precomputed_ecmult_gen.h
noinst_HEADERS += src/assumptions.h
noinst_HEADERS += src/checkmem.h
noinst_HEADERS += src/testutil.h
noinst_HEADERS += src/util.h
noinst_HEADERS += src/util_local_visibility.h
noinst_HEADERS += src/int128.h
noinst_HEADERS += src/int128_impl.h
noinst_HEADERS += src/int128_native.h
noinst_HEADERS += src/int128_native_impl.h
noinst_HEADERS += src/int128_struct.h
noinst_HEADERS += src/int128_struct_impl.h
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
noinst_HEADERS += src/wycheproof/ecdsa_secp256k1_sha256_bitcoin_test.h
noinst_HEADERS += src/hsort.h
noinst_HEADERS += src/hsort_impl.h
noinst_HEADERS += contrib/lax_der_parsing.h
noinst_HEADERS += contrib/lax_der_parsing.c
noinst_HEADERS += contrib/lax_der_privatekey_parsing.h
noinst_HEADERS += contrib/lax_der_privatekey_parsing.c
noinst_HEADERS += examples/examples_util.h

PRECOMPUTED_LIB = libsecp256k1_precomputed.la
noinst_LTLIBRARIES = $(PRECOMPUTED_LIB)
libsecp256k1_precomputed_la_SOURCES =  src/precomputed_ecmult.c src/precomputed_ecmult_gen.c
# We need `-I$(top_srcdir)/src` in VPATH builds if libsecp256k1_precomputed_la_SOURCES have been recreated in the build tree.
# This helps users and packagers who insist on recreating the precomputed files (e.g., Gentoo).
libsecp256k1_precomputed_la_CPPFLAGS = -I$(top_srcdir)/src $(SECP_CONFIG_DEFINES)

if USE_EXTERNAL_ASM
COMMON_LIB = libsecp256k1_common.la
else
COMMON_LIB =
endif
noinst_LTLIBRARIES += $(COMMON_LIB)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsecp256k1.pc

if USE_EXTERNAL_ASM
if USE_ASM_ARM
libsecp256k1_common_la_SOURCES = src/asm/field_10x26_arm.s
endif
endif

libsecp256k1_la_SOURCES = src/secp256k1.c
libsecp256k1_la_CPPFLAGS = $(SECP_CONFIG_DEFINES)
libsecp256k1_la_LIBADD = $(COMMON_LIB) $(PRECOMPUTED_LIB)
libsecp256k1_la_LDFLAGS = -no-undefined -version-info $(LIB_VERSION_CURRENT):$(LIB_VERSION_REVISION):$(LIB_VERSION_AGE)

noinst_PROGRAMS =
if USE_BENCHMARK
noinst_PROGRAMS += bench bench_internal bench_ecmult
bench_SOURCES = src/bench.c
bench_LDADD = libsecp256k1.la
bench_CPPFLAGS = $(SECP_CONFIG_DEFINES)
bench_internal_SOURCES = src/bench_internal.c
bench_internal_LDADD = $(COMMON_LIB) $(PRECOMPUTED_LIB)
bench_internal_CPPFLAGS = $(SECP_CONFIG_DEFINES)
bench_ecmult_SOURCES = src/bench_ecmult.c
bench_ecmult_LDADD = $(COMMON_LIB) $(PRECOMPUTED_LIB)
bench_ecmult_CPPFLAGS = $(SECP_CONFIG_DEFINES)
endif

TESTS =
if USE_TESTS
TESTS += noverify_tests
noinst_PROGRAMS += noverify_tests
noverify_tests_SOURCES = src/tests.c
noverify_tests_CPPFLAGS = $(SECP_CONFIG_DEFINES)
noverify_tests_LDADD = $(COMMON_LIB) $(PRECOMPUTED_LIB)
noverify_tests_LDFLAGS = -static
if !ENABLE_COVERAGE
TESTS += tests
noinst_PROGRAMS += tests
tests_SOURCES  = $(noverify_tests_SOURCES)
tests_CPPFLAGS = $(noverify_tests_CPPFLAGS) -DVERIFY
tests_LDADD    = $(noverify_tests_LDADD)
tests_LDFLAGS  = $(noverify_tests_LDFLAGS)
endif
endif

if USE_CTIME_TESTS
noinst_PROGRAMS += ctime_tests
ctime_tests_SOURCES = src/ctime_tests.c
ctime_tests_LDADD = libsecp256k1.la
ctime_tests_CPPFLAGS = $(SECP_CONFIG_DEFINES)
endif

if USE_EXHAUSTIVE_TESTS
noinst_PROGRAMS += exhaustive_tests
exhaustive_tests_SOURCES = src/tests_exhaustive.c
exhaustive_tests_CPPFLAGS = $(SECP_CONFIG_DEFINES)
if !ENABLE_COVERAGE
exhaustive_tests_CPPFLAGS += -DVERIFY
endif
# Note: do not include $(PRECOMPUTED_LIB) in exhaustive_tests (it uses runtime-generated tables).
exhaustive_tests_LDADD = $(COMMON_LIB)
exhaustive_tests_LDFLAGS = -static
TESTS += exhaustive_tests
endif

if USE_EXAMPLES
noinst_PROGRAMS += ecdsa_example
ecdsa_example_SOURCES = examples/ecdsa.c
ecdsa_example_CPPFLAGS = -I$(top_srcdir)/include -DSECP256K1_STATIC
ecdsa_example_LDADD = libsecp256k1.la
ecdsa_example_LDFLAGS = -static
if BUILD_WINDOWS
ecdsa_example_LDFLAGS += -lbcrypt
endif
TESTS += ecdsa_example
if ENABLE_MODULE_ECDH
noinst_PROGRAMS += ecdh_example
ecdh_example_SOURCES = examples/ecdh.c
ecdh_example_CPPFLAGS = -I$(top_srcdir)/include -DSECP256K1_STATIC
ecdh_example_LDADD = libsecp256k1.la
ecdh_example_LDFLAGS = -static
if BUILD_WINDOWS
ecdh_example_LDFLAGS += -lbcrypt
endif
TESTS += ecdh_example
endif
if ENABLE_MODULE_SCHNORRSIG
noinst_PROGRAMS += schnorr_example
schnorr_example_SOURCES = examples/schnorr.c
schnorr_example_CPPFLAGS = -I$(top_srcdir)/include -DSECP256K1_STATIC
schnorr_example_LDADD = libsecp256k1.la
schnorr_example_LDFLAGS = -static
if BUILD_WINDOWS
schnorr_example_LDFLAGS += -lbcrypt
endif
TESTS += schnorr_example
endif
if ENABLE_MODULE_ELLSWIFT
noinst_PROGRAMS += ellswift_example
ellswift_example_SOURCES = examples/ellswift.c
ellswift_example_CPPFLAGS = -I$(top_srcdir)/include -DSECP256K1_STATIC
ellswift_example_LDADD = libsecp256k1.la
ellswift_example_LDFLAGS = -static
if BUILD_WINDOWS
ellswift_example_LDFLAGS += -lbcrypt
endif
TESTS += ellswift_example
endif
if ENABLE_MODULE_MUSIG
noinst_PROGRAMS += musig_example
musig_example_SOURCES = examples/musig.c
musig_example_CPPFLAGS = -I$(top_srcdir)/include -DSECP256K1_STATIC
musig_example_LDADD = libsecp256k1.la
musig_example_LDFLAGS = -static
if BUILD_WINDOWS
musig_example_LDFLAGS += -lbcrypt
endif
TESTS += musig_example
endif
endif

### Precomputed tables
EXTRA_PROGRAMS = precompute_ecmult precompute_ecmult_gen
CLEANFILES = $(EXTRA_PROGRAMS)

precompute_ecmult_SOURCES = src/precompute_ecmult.c
precompute_ecmult_CPPFLAGS = $(SECP_CONFIG_DEFINES) -DVERIFY
precompute_ecmult_LDADD = $(COMMON_LIB)

precompute_ecmult_gen_SOURCES = src/precompute_ecmult_gen.c
precompute_ecmult_gen_CPPFLAGS = $(SECP_CONFIG_DEFINES) -DVERIFY
precompute_ecmult_gen_LDADD = $(COMMON_LIB)

# See Automake manual, Section "Errors with distclean".
# We don't list any dependencies for the prebuilt files here because
# otherwise make's decision whether to rebuild them (even in the first
# build by a normal user) depends on mtimes, and thus is very fragile.
# This means that rebuilds of the prebuilt files always need to be
# forced by deleting them.
src/precomputed_ecmult.c:
	$(MAKE) $(AM_MAKEFLAGS) precompute_ecmult$(EXEEXT)
	./precompute_ecmult$(EXEEXT)
src/precomputed_ecmult_gen.c:
	$(MAKE) $(AM_MAKEFLAGS) precompute_ecmult_gen$(EXEEXT)
	./precompute_ecmult_gen$(EXEEXT)

PRECOMP = src/precomputed_ecmult_gen.c src/precomputed_ecmult.c
precomp: $(PRECOMP)

# Ensure the prebuilt files will be build first (only if they don't exist,
# e.g., after `make maintainer-clean`).
BUILT_SOURCES = $(PRECOMP)

.PHONY: clean-precomp
clean-precomp:
	rm -f $(PRECOMP)
maintainer-clean-local: clean-precomp

### Pregenerated test vectors
### (see the comments in the previous section for detailed rationale)
TESTVECTORS = src/wycheproof/ecdsa_secp256k1_sha256_bitcoin_test.h

if ENABLE_MODULE_ECDH
TESTVECTORS += src/wycheproof/ecdh_secp256k1_test.h
endif

src/wycheproof/ecdsa_secp256k1_sha256_bitcoin_test.h:
	mkdir -p $(@D)
	python3 $(top_srcdir)/tools/tests_wycheproof_generate_ecdsa.py $(top_srcdir)/src/wycheproof/ecdsa_secp256k1_sha256_bitcoin_test.json > $@

src/wycheproof/ecdh_secp256k1_test.h:
	mkdir -p $(@D)
	python3 $(top_srcdir)/tools/tests_wycheproof_generate_ecdh.py $(top_srcdir)/src/wycheproof/ecdh_secp256k1_test.json > $@

testvectors: $(TESTVECTORS)

BUILT_SOURCES += $(TESTVECTORS)

.PHONY: clean-testvectors
clean-testvectors:
	rm -f $(TESTVECTORS)
maintainer-clean-local: clean-testvectors

### Additional files to distribute
EXTRA_DIST = autogen.sh CHANGELOG.md SECURITY.md
EXTRA_DIST += doc/release-process.md doc/safegcd_implementation.md
EXTRA_DIST += doc/ellswift.md doc/musig.md
EXTRA_DIST += examples/EXAMPLES_COPYING
EXTRA_DIST += sage/gen_exhaustive_groups.sage
EXTRA_DIST += sage/gen_split_lambda_constants.sage
EXTRA_DIST += sage/group_prover.sage
EXTRA_DIST += sage/prove_group_implementations.sage
EXTRA_DIST += sage/secp256k1_params.sage
EXTRA_DIST += sage/weierstrass_prover.sage
EXTRA_DIST += src/wycheproof/WYCHEPROOF_COPYING
EXTRA_DIST += src/wycheproof/ecdsa_secp256k1_sha256_bitcoin_test.json
EXTRA_DIST += src/wycheproof/ecdh_secp256k1_test.json
EXTRA_DIST += tools/tests_wycheproof_generate_ecdsa.py
EXTRA_DIST += tools/tests_wycheproof_generate_ecdh.py

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

if ENABLE_MODULE_MUSIG
include src/modules/musig/Makefile.am.include
endif

if ENABLE_MODULE_ELLSWIFT
include src/modules/ellswift/Makefile.am.include
endif
