ACLOCAL_AMFLAGS = -I m4
INCLUDES = $(SECP_INCLUDES)
lib_LTLIBRARIES = libsecp256k1.la
noinst_LTLIBRARIES = libsecp256k1_common.la
include_HEADERS = include/secp256k1.h
noinst_HEADERS =
noinst_HEADERS += src/group.h
noinst_HEADERS += src/group_impl.h
noinst_HEADERS += src/num_openssl.h
noinst_HEADERS += src/num_openssl_impl.h
noinst_HEADERS += src/num_gmp.h
noinst_HEADERS += src/num_gmp_impl.h
noinst_HEADERS += src/ecdsa.h
noinst_HEADERS += src/ecdsa_impl.h
noinst_HEADERS += src/ecmult.h
noinst_HEADERS += src/ecmult_impl.h
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
noinst_HEADERS += src/util_impl.h
noinst_HEADERS += src/field_gmp.h
noinst_HEADERS += src/field_gmp_impl.h
noinst_HEADERS += src/field.h
noinst_HEADERS += src/field_impl.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsecp256k1.pc

libsecp256k1_common_la_SOURCES =
if USE_ASM
libsecp256k1_common_la_SOURCES += src/field_5x52_asm.asm
endif

libsecp256k1_la_SOURCES = src/secp256k1.c
libsecp256k1_la_CPPFLAGS = -I$(top_srcdir)/include $(SECP_INCLUDES)
libsecp256k1_la_LIBADD = libsecp256k1_common.la $(SECP_LIBS)

noinst_PROGRAMS =
if USE_BENCHMARK
noinst_PROGRAMS += bench
bench_SOURCES = src/bench.c
bench_LDADD = libsecp256k1.la $(SECP_LIBS)
bench_LDFLAGS = -static
endif

if USE_TESTS
noinst_PROGRAMS += tests
tests_SOURCES = src/tests.c
tests_CPPFLAGS = -DVERIFY $(SECP_TEST_INCLUDES)
tests_LDADD = libsecp256k1_common.la $(SECP_LIBS) $(SECP_TEST_LIBS)
tests_LDFLAGS = -static
TESTS = tests
endif

EXTRA_DIST = autogen.sh nasm_lt.sh

#x86_64 only
if USE_ASM
.asm.lo:
	$(LIBTOOL) --mode=compile --tag YASM $(srcdir)/nasm_lt.sh $(YASM) -f $(YASM_BINFMT) $(YAFLAGS) -I$(srcdir) -I. $< -o $@
endif
