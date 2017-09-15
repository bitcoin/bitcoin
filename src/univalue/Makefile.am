ACLOCAL_AMFLAGS = -I build-aux/m4
.PHONY: gen
.INTERMEDIATE: $(GENBIN)

include_HEADERS = include/univalue.h
noinst_HEADERS = lib/univalue_escapes.h lib/univalue_utffilter.h

lib_LTLIBRARIES = libunivalue.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pc/libunivalue.pc

libunivalue_la_SOURCES = \
	lib/univalue.cpp \
	lib/univalue_read.cpp \
	lib/univalue_write.cpp

libunivalue_la_LDFLAGS = \
	-version-info $(LIBUNIVALUE_CURRENT):$(LIBUNIVALUE_REVISION):$(LIBUNIVALUE_AGE) \
	-no-undefined
libunivalue_la_CXXFLAGS = -I$(top_srcdir)/include

TESTS = test/unitester

GENBIN = gen/gen$(BUILD_EXEEXT)
GEN_SRCS = gen/gen.cpp

$(GENBIN): $(GEN_SRCS)
	@echo Building $@
	$(AM_V_at)c++ -I$(top_srcdir)/include -o $@ $<

gen: lib/univalue_escapes.h $(GENBIN)
	@echo Updating $<
	$(AM_V_at)$(GENBIN) > lib/univalue_escapes.h

noinst_PROGRAMS = $(TESTS)

TEST_DATA_DIR=test

test_unitester_SOURCES = test/unitester.cpp
test_unitester_LDADD = libunivalue.la
test_unitester_CXXFLAGS = -I$(top_srcdir)/include -DJSON_TEST_SRC=\"$(srcdir)/$(TEST_DATA_DIR)\"
test_unitester_LDFLAGS = -static $(LIBTOOL_APP_LDFLAGS)

TEST_FILES = \
	$(TEST_DATA_DIR)/fail10.json \
	$(TEST_DATA_DIR)/fail11.json \
	$(TEST_DATA_DIR)/fail12.json \
	$(TEST_DATA_DIR)/fail13.json \
	$(TEST_DATA_DIR)/fail14.json \
	$(TEST_DATA_DIR)/fail15.json \
	$(TEST_DATA_DIR)/fail16.json \
	$(TEST_DATA_DIR)/fail17.json \
	$(TEST_DATA_DIR)/fail18.json \
	$(TEST_DATA_DIR)/fail19.json \
	$(TEST_DATA_DIR)/fail1.json \
	$(TEST_DATA_DIR)/fail20.json \
	$(TEST_DATA_DIR)/fail21.json \
	$(TEST_DATA_DIR)/fail22.json \
	$(TEST_DATA_DIR)/fail23.json \
	$(TEST_DATA_DIR)/fail24.json \
	$(TEST_DATA_DIR)/fail25.json \
	$(TEST_DATA_DIR)/fail26.json \
	$(TEST_DATA_DIR)/fail27.json \
	$(TEST_DATA_DIR)/fail28.json \
	$(TEST_DATA_DIR)/fail29.json \
	$(TEST_DATA_DIR)/fail2.json \
	$(TEST_DATA_DIR)/fail30.json \
	$(TEST_DATA_DIR)/fail31.json \
	$(TEST_DATA_DIR)/fail32.json \
	$(TEST_DATA_DIR)/fail33.json \
	$(TEST_DATA_DIR)/fail34.json \
	$(TEST_DATA_DIR)/fail35.json \
	$(TEST_DATA_DIR)/fail36.json \
	$(TEST_DATA_DIR)/fail37.json \
	$(TEST_DATA_DIR)/fail38.json \
	$(TEST_DATA_DIR)/fail39.json \
	$(TEST_DATA_DIR)/fail40.json \
	$(TEST_DATA_DIR)/fail41.json \
	$(TEST_DATA_DIR)/fail3.json \
	$(TEST_DATA_DIR)/fail4.json \
	$(TEST_DATA_DIR)/fail5.json \
	$(TEST_DATA_DIR)/fail6.json \
	$(TEST_DATA_DIR)/fail7.json \
	$(TEST_DATA_DIR)/fail8.json \
	$(TEST_DATA_DIR)/fail9.json \
	$(TEST_DATA_DIR)/pass1.json \
	$(TEST_DATA_DIR)/pass2.json \
	$(TEST_DATA_DIR)/pass3.json \
	$(TEST_DATA_DIR)/round1.json \
	$(TEST_DATA_DIR)/round2.json

EXTRA_DIST=$(TEST_FILES) $(GEN_SRCS)
