noinst_PROGRAMS += test-libbson
TEST_PROGS = test-libbson


test_libbson_SOURCES = \
	tests/corpus-test.c \
	tests/corpus-test.h \
	tests/TestSuite.c \
	tests/TestSuite.h \
	tests/test-libbson.c \
	tests/test-atomic.c \
	tests/test-bson.c \
	tests/test-bson-corpus.c \
	tests/test-endian.c \
	tests/test-clock.c \
	tests/test-decimal128.c \
	tests/test-error.c \
	tests/test-iso8601.c \
	tests/test-iter.c \
	tests/test-json.c \
	tests/test-oid.c \
	tests/test-reader.c \
	tests/test-string.c \
	tests/test-utf8.c \
	tests/test-value.c \
	tests/test-version.c \
	tests/test-writer.c \
	tests/test-bcon-basic.c \
	tests/test-bcon-extract.c \
	tests/json-test.c \
	tests/json-test.h

test_libbson_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/bson \
	-I$(top_builddir)/src/bson \
	$(PTHREAD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DBSON_COMPILATION \
	-DJSONSL_PARSE_NAN \
	-DBINARY_DIR="\"$(top_srcdir)/tests/binary\"" \
	-DJSON_DIR="\"$(top_srcdir)/tests/json\""

test_libbson_LDFLAGS = $(COVERAGE_LDFLAGS)

test_libbson_LDADD = \
	$(PTHREAD_LIBS) \
	libbson-1.0.la \
	libbson.la


check: test


if OS_DARWIN
DEBUGGER = lldb --
else
DEBUGGER = gdb --args
endif

if OS_DARWIN
TEST_ARGS = --no-fork
else
TEST_ARGS = --threads --no-fork
endif


test: $(TEST_PROGS)
	@ for TEST_PROG in $(TEST_PROGS) ; do \
		$(LIBTOOL) --mode=execute $$VALGRIND ./$$TEST_PROG $(TEST_ARGS) -F test-results.json; \
	done

valgrind: $(TEST_PROGS)
	$(LIBTOOL) --mode=execute valgrind --error-exitcode=1 --leak-check=full ./test-libbson $(TEST_ARGS) --no-fork

debug: $(TEST_PROGS)
	$(LIBTOOL) --mode=execute $(DEBUGGER) ./test-libbson $(TEST_ARGS)

DISTCLEANFILES += \
	test.log \
	test-results.json

CLEANFILES += \
	tests/*.gcda \
	tests/*.gcno


EXTRA_DIST += \
	tests/bson-tests.h \
	$(wildcard tests/binary/*.bson) \
	$(wildcard tests/json//*.json) \
	$(wildcard tests/json/bson_corpus/*.json) \
	$(wildcard tests/json/type/*/*.json)
