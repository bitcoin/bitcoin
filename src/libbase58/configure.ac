dnl * Copyright 2012-2014 Luke Dashjr
dnl *
dnl * This program is free software; you can redistribute it and/or modify it
dnl * under the terms of the standard MIT license.  See COPYING for more details.

AC_INIT(
	[libbase58],
	[0.1],
	[luke_libbase58@dashjr.org],
	[libbase58])
AC_CONFIG_AUX_DIR([.])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.12 -Wall dist-xz foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC_C99
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT([])

# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
AC_SUBST([LIBBASE58_SO_VERSION], [0:0:0])

AC_CONFIG_FILES([Makefile
	libbase58.pc:libbase58.pc.in
])

AC_CHECK_LIB([ws2_32], [strchr])

AC_ARG_ENABLE([tool],
	[AC_HELP_STRING([--disable-tool],[Compile command line base58 tool (default enabled)])],
	[use_tool=$enableval],
	[use_tool=auto])
if test x$use_tool != xno; then
	AM_PATH_LIBGCRYPT([],[
		use_tool=yes
	],[
		if test x$use_tool = xyes; then
			AC_MSG_ERROR([libgcrypt not found; use --disable-tool])
		fi
		use_tool=no
	])
fi
AM_CONDITIONAL([USE_TOOL], [test x$use_tool = xyes])

AC_OUTPUT
