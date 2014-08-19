# Copyright 2014 Luke Dashjr
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the standard MIT license.  See COPYING for more details.

lib_LTLIBRARIES = libbase58.la
libbase58_la_SOURCES = base58.c
libbase58_la_LDFLAGS = -version-info $(LIBBASE58_SO_VERSION) -no-undefined

libbase58_includedir = $(includedir)
libbase58_include_HEADERS = libbase58.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbase58.pc

dist_noinst_SCRIPTS = autogen.sh
dist_doc_DATA = README

if USE_TOOL
bin_PROGRAMS = base58
base58_SOURCES = clitool.c
base58_CFLAGS = $(LIBGCRYPT_CFLAGS)
base58_LDADD = libbase58.la $(LIBGCRYPT_LIBS)

TESTS = \
	tests/decode.sh \
	tests/decode-b58c.sh \
	tests/decode-b58c-fail.sh \
	tests/decode-b58c-null.sh \
	tests/decode-b58c-toolong.sh \
	tests/decode-b58c-tooshort.sh \
	tests/decode-small.sh \
	tests/decode-zero.sh \
	tests/encode.sh \
	tests/encode-b58c.sh \
	tests/encode-fail.sh \
	tests/encode-small.sh
SH_LOG_COMPILER = /bin/sh
AM_TESTS_ENVIRONMENT = PATH='$(srcdir)':"$$PATH"; export PATH;
endif
TEST_EXTENSIONS = .sh
