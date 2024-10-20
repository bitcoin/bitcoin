ACLOCAL_AMFLAGS = -I build-aux/m4
AM_CXXFLAGS = $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS)

include sources.mk

include_HEADERS = $(MINISKETCH_DIST_HEADERS_INT)
noinst_HEADERS = $(MINISKETCH_LIB_HEADERS_INT) $(MINISKETCH_FIELD_GENERIC_HEADERS_INT) $(MINISKETCH_FIELD_CLMUL_HEADERS_INT)

LIBMINISKETCH = libminisketch.la
LIBMINISKETCH_FIELD_GENERIC = libminisketch_field_generic.la
if ENABLE_CLMUL
LIBMINISKETCH_FIELD_CLMUL = libminisketch_field_clmul.la
endif
if USE_TESTS
LIBMINISKETCH_VERIFY=libminisketch_verify.la
LIBMINISKETCH_FIELD_GENERIC_VERIFY=libminisketch_field_generic_verify.la
if ENABLE_CLMUL
LIBMINISKETCH_FIELD_CLMUL_VERIFY=libminisketch_field_clmul_verify.la
endif
endif

lib_LTLIBRARIES =
lib_LTLIBRARIES +=  $(LIBMINISKETCH)

noinst_LTLIBRARIES =
noinst_LTLIBRARIES += $(LIBMINISKETCH_FIELD_GENERIC)
noinst_LTLIBRARIES += $(LIBMINISKETCH_FIELD_GENERIC_VERIFY)
noinst_LTLIBRARIES += $(LIBMINISKETCH_FIELD_CLMUL)
noinst_LTLIBRARIES += $(LIBMINISKETCH_FIELD_CLMUL_VERIFY)
noinst_LTLIBRARIES += $(LIBMINISKETCH_VERIFY)

# Release libs
libminisketch_field_generic_la_SOURCES = $(MINISKETCH_FIELD_GENERIC_SOURCES_INT)
libminisketch_field_generic_la_CPPFLAGS = $(AM_CPPFLAGS) $(RELEASE_DEFINES)

libminisketch_field_clmul_la_SOURCES = $(MINISKETCH_FIELD_CLMUL_SOURCES_INT)
libminisketch_field_clmul_la_CPPFLAGS = $(AM_CPPFLAGS) $(RELEASE_DEFINES)
libminisketch_field_clmul_la_CXXFLAGS = $(AM_CXXFLAGS) $(CLMUL_CXXFLAGS)

libminisketch_la_SOURCES = $(MINISKETCH_LIB_SOURCES_INT)
libminisketch_la_CPPFLAGS = $(AM_CPPFLAGS) $(RELEASE_DEFINES)
libminisketch_la_LIBADD = $(LIBMINISKETCH_FIELD_CLMUL) $(LIBMINISKETCH_FIELD_GENERIC)

# Libs with extra verification checks
libminisketch_field_generic_verify_la_SOURCES = $(MINISKETCH_FIELD_GENERIC_SOURCES_INT)
libminisketch_field_generic_verify_la_CPPFLAGS = $(AM_CPPFLAGS) $(VERIFY_DEFINES)

libminisketch_field_clmul_verify_la_SOURCES = $(MINISKETCH_FIELD_CLMUL_SOURCES_INT)
libminisketch_field_clmul_verify_la_CPPFLAGS = $(AM_CPPFLAGS) $(VERIFY_DEFINES)
libminisketch_field_clmul_verify_la_CXXFLAGS = $(AM_CXXFLAGS) $(CLMUL_CXXFLAGS)

libminisketch_verify_la_SOURCES = $(MINISKETCH_LIB_SOURCES_INT)
libminisketch_verify_la_CPPFLAGS = $(AM_CPPFLAGS) $(VERIFY_DEFINES)
libminisketch_verify_la_LIBADD = $(LIBMINISKETCH_FIELD_CLMUL_VERIFY) $(LIBMINISKETCH_FIELD_GENERIC_VERIFY)

noinst_PROGRAMS =
if USE_BENCHMARK
noinst_PROGRAMS += bench
endif
if USE_TESTS
noinst_PROGRAMS += test test-verify
TESTS = test test-verify
endif

bench_SOURCES = $(MINISKETCH_BENCH_SOURCES_INT)
bench_CPPFLAGS = $(AM_CPPFLAGS) $(RELEASE_DEFINES)
bench_LDADD = $(LIBMINISKETCH)
bench_LDFLAGS = $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

test_SOURCES = $(MINISKETCH_TEST_SOURCES_INT)
test_CPPFLAGS = $(AM_CPPFLAGS) $(RELEASE_DEFINES)
test_LDADD = $(LIBMINISKETCH)
test_LDFLAGS = $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

test_verify_SOURCES = $(MINISKETCH_TEST_SOURCES_INT)
test_verify_CPPFLAGS = $(AM_CPPFLAGS) $(VERIFY_DEFINES)
test_verify_LDADD = $(LIBMINISKETCH_VERIFY)
test_verify_LDFLAGS = $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

EXTRA_DIST=
EXTRA_DIST += LICENSE
EXTRA_DIST += README.md
EXTRA_DIST += doc/example.c
EXTRA_DIST += doc/gen_params.sage
EXTRA_DIST += doc/math.md
EXTRA_DIST += doc/moduli.md
EXTRA_DIST += doc/plot_bits.png
EXTRA_DIST += doc/plot_capacity.png
EXTRA_DIST += doc/plot_diff.png
EXTRA_DIST += doc/plot_size.png
EXTRA_DIST += doc/protocoltips.md
EXTRA_DIST += tests/pyminisketch.py
