dnl ==============================================================
dnl Process this file with autoconf to produce a configure script.
dnl ==============================================================

dnl require autoconf 2.60 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.60])

AC_INIT([Bitcoin],[0.4.0],[info@bitcoin.org],[bitcoin])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST

dnl backwards compatibility for autoconf >= 2.64
dnl PACKAGE_URL should be the fifth argument of AC_INIT
m4_define([AC_PACKAGE_URL], [http://bitcoin.org])
AC_DEFINE(PACKAGE_URL, "AC_PACKAGE_URL", [Package URL])
AC_SUBST(PACKAGE_URL, AC_PACKAGE_URL)

dnl ==============================================================
dnl Setup for automake
dnl ==============================================================

AM_INIT_AUTOMAKE([dist-bzip2 subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Libtool
AC_PROG_LIBTOOL

dnl Advanced network address translating functions
AC_CHECK_FUNCS(inet_ntop)

dnl Checks for libraries
AC_CHECK_LIB(crypto, MD5_Init, CRYPTO_LIBS=-lcrypto)
AX_BOOST_BASE
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_THREAD

dnl Support BSD4.4 "sa_len" extension when calculating sockaddrs arrays
AC_CHECK_MEMBERS(struct sockaddr.sa_len, , , [#include <sys/types.h>
#include <sys/socket.h>])

dnl Check if it is available the RFC2292 IPv4 extension and in_port_t.
AC_CHECK_TYPES([struct in_pktinfo, in_port_t], , , [#include <sys/types.h>
#include <netinet/in.h>])

AC_CHECK_HEADERS([stdio.h stdlib.h unistd.h strings.h sys/types.h sys/stat.h])


dnl ==============================================================
dnl compile with full warnings and debugging symbols
dnl ==============================================================
AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug],[compile with debug symbols (no)]),
    [enable_debug=$enableval],
    [enable_debug=no])

if test x$enable_debug = xyes; then
    GLOBAL_CFLAGS="$GLOBAL_CFLAGS -Wall -g -ggdb"
fi


# declare the flags gathered so far
CFLAGS="$GLOBAL_CFLAGS $CFLAGS"
AC_SUBST(CFLAGS)
CXXFLAGS="$GLOBAL_CFLAGS $CXXFLAGS"
AC_SUBST(CXXFLAGS)

AC_CONFIG_FILES([
Makefile
src/Makefile
src/cryptopp/Makefile
])
AC_OUTPUT


