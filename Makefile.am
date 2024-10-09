# Copyright (c) 2013-2016 The Bitcoin Core developers
# Copyright (c) 2022 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING.MIT or http://www.opensource.org/licenses/mit-license.php.

print-%: FORCE
	@echo '$*'='$($*)'

ACLOCAL_AMFLAGS = -I build-aux/m4
.PHONY: deploy FORCE

AM_LDFLAGS    = $(HARDENED_LDFLAGS) $(CORE_LDFLAGS) $(GMP_LDFLAGS)
AM_CFLAGS     = $(DEBUG_FLAGS) $(HARDENED_FLAGS) $(CORE_FLAGS) $(WARN_FLAGS) $(NOWARN_FLAGS)
AM_CPPFLAGS   = $(CORE_CPPFLAGS) $(GMP_CPPFLAGS)
AM_CXXFLAGS   = $(DEBUG_FLAGS) $(HARDENED_FLAGS) $(CORE_FLAGS) $(WARN_FLAGS) $(NOWARN_FLAGS)
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)

EXTRA_LIBRARIES =

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

bin_PROGRAMS =
noinst_PROGRAMS =
EXTRA_DIST =

CLEANFILES =

include Makefile.bls.include

CLEANFILES += $(LIBRELIC) $(LIBDASHBLS) $(LIBMIMALLOC)

if USE_TESTS
CLEANFILES += $(DASHBLS_RUNTEST)
endif

if USE_BENCH
CLEANFILES += $(DASHBLS_RUNBENCH)
endif
