noinst_LTLIBRARIES += libbson.la

headerdir = $(prefix)/include/libbson-@BSON_API_VERSION@
header_DATA = \
	$(INST_H_FILES) \
	src/bson/bson-config.h

INST_H_FILES = \
	src/bson/bcon.h \
	src/bson/bson.h \
	src/bson/bson-atomic.h \
	src/bson/bson-clock.h \
	src/bson/bson-compat.h \
	src/bson/bson-context.h \
	src/bson/bson-decimal128.h \
	src/bson/bson-endian.h \
	src/bson/bson-error.h \
	src/bson/bson-iter.h \
	src/bson/bson-json.h \
	src/bson/bson-keys.h \
	src/bson/bson-macros.h \
	src/bson/bson-md5.h \
	src/bson/bson-memory.h \
	src/bson/bson-oid.h \
	src/bson/bson-reader.h \
	src/bson/bson-string.h \
	src/bson/bson-types.h \
	src/bson/bson-utf8.h \
	src/bson/bson-value.h \
	src/bson/bson-version.h \
	src/bson/bson-version-functions.h \
	src/bson/bson-writer.h

if OS_WIN32
INST_H_FILES += src/bson/bson-stdint-win32.h
else
INST_H_FILES += src/bson/bson-stdint.h
endif

NOINST_H_FILES = \
	src/bson/b64_ntop.h \
	src/bson/b64_pton.h \
	src/bson/bson-private.h \
	src/bson/bson-iso8601-private.h \
	src/bson/bson-context-private.h \
	src/bson/bson-thread-private.h \
	src/bson/bson-timegm-private.h


libbson_la_CPPFLAGS = \
	-DBSON_COMPILATION \
	-DJSONSL_PARSE_NAN \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/bson \
	-I$(top_srcdir)/src/jsonsl \
	-I$(top_builddir)/src/bson

libbson_la_CFLAGS = \
	$(MAINTAINER_CFLAGS) \
	$(OPTIMIZE_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	$(PTHREAD_CFLAGS) \
	-fvisibility=hidden

libbson_la_SOURCES = \
	$(INST_H_FILES) \
	$(NOINST_H_FILES) \
	src/bson/bcon.c \
	src/bson/bson.c \
	src/bson/bson-atomic.c \
	src/bson/bson-clock.c \
	src/bson/bson-context.c \
	src/bson/bson-decimal128.c \
	src/bson/bson-error.c \
	src/bson/bson-iter.c \
	src/bson/bson-iso8601.c \
	src/bson/bson-json.c \
	src/bson/bson-keys.c \
	src/bson/bson-md5.c \
	src/bson/bson-memory.c \
	src/bson/bson-oid.c \
	src/bson/bson-reader.c \
	src/bson/bson-string.c \
	src/bson/bson-timegm.c \
	src/bson/bson-utf8.c \
	src/bson/bson-value.c \
	src/bson/bson-version-functions.c \
	src/bson/bson-writer.c

libbson_la_LIBADD =

EXTRA_DIST += \
	src/bson/bson-config.h.in \
	src/bson/bson-version.h.in

DISTCLEANFILES += \
	src/bson/bson-stdint.h
