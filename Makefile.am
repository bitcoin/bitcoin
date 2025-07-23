include sources.mk
ACLOCAL_AMFLAGS = -I build-aux/m4
.PHONY: gen FORCE
.INTERMEDIATE: $(GENBIN)

include_HEADERS = $(UNIVALUE_DIST_HEADERS_INT)
noinst_HEADERS = $(UNIVALUE_LIB_HEADERS_INT)

lib_LTLIBRARIES = libunivalue.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pc/libunivalue.pc

libunivalue_la_SOURCES = $(UNIVALUE_LIB_SOURCES_INT)

libunivalue_la_LDFLAGS = \
	-version-info $(LIBUNIVALUE_CURRENT):$(LIBUNIVALUE_REVISION):$(LIBUNIVALUE_AGE) \
	-no-undefined
libunivalue_la_CXXFLAGS = -I$(top_srcdir)/include

TESTS = test/object test/unitester test/no_nul

GENBIN = gen/gen$(BUILD_EXEEXT)
GEN_SRCS = gen/gen.cpp

$(GENBIN): $(GEN_SRCS)
	@echo Building $@
	$(AM_V_at)c++ -I$(top_srcdir)/include -o $@ $<

gen: $(GENBIN) FORCE
	@echo Updating lib/univalue_escapes.h
	$(AM_V_at)$(GENBIN) > lib/univalue_escapes.h

noinst_PROGRAMS = $(TESTS) test/test_json

test_unitester_SOURCES = $(UNIVALUE_TEST_UNITESTER_INT)
test_unitester_LDADD = libunivalue.la
test_unitester_CXXFLAGS = -I$(top_srcdir)/include -DJSON_TEST_SRC=\"$(srcdir)/$(UNIVALUE_TEST_DATA_DIR_INT)\"
test_unitester_LDFLAGS = -static $(LIBTOOL_APP_LDFLAGS)

test_test_json_SOURCES = $(UNIVALUE_TEST_JSON_INT)
test_test_json_LDADD = libunivalue.la
test_test_json_CXXFLAGS = -I$(top_srcdir)/include
test_test_json_LDFLAGS = -static $(LIBTOOL_APP_LDFLAGS)

test_no_nul_SOURCES = $(UNIVALUE_TEST_NO_NUL_INT)
test_no_nul_LDADD = libunivalue.la
test_no_nul_CXXFLAGS = -I$(top_srcdir)/include
test_no_nul_LDFLAGS = -static $(LIBTOOL_APP_LDFLAGS)

test_object_SOURCES = $(UNIVALUE_TEST_OBJECT_INT)
test_object_LDADD = libunivalue.la
test_object_CXXFLAGS = -I$(top_srcdir)/include
test_object_LDFLAGS = -static $(LIBTOOL_APP_LDFLAGS)

TEST_FILES = $(UNIVALUE_TEST_FILES_INT)

EXTRA_DIST=$(UNIVALUE_TEST_FILES_INT) $(GEN_SRCS)
