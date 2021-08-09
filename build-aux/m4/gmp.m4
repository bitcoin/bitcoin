# Tests for GNU GMP (or any compatible replacement).

dnl Copyright (C) 2008-2020 Free Software Foundation, Inc.

dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by James Youngman.

dnl Check for libgmp.  We avoid use of AC_CHECK_LIBS because we don't want to
dnl add this to $LIBS for all targets.
AC_DEFUN([cu_GMP],
[
  LIB_GMP=
  AC_SUBST([LIB_GMP])

  AC_ARG_WITH([gmp],
    AS_HELP_STRING([--without-gmp],
      [do not use the GNU MP library for arbitrary precision
       calculation (default: use it if available)]),
    [cu_use_gmp=$withval],
    [cu_use_gmp=auto])

  if test $cu_use_gmp != no; then
    dnl It was noticed on one MacOS X 10.5.8 system at least
    dnl that the libs were available but the header wasn't
    HAVE_GMP=0
    AC_CHECK_HEADERS_ONCE([gmp.h])
    if test $ac_cv_header_gmp_h = yes; then
      cu_saved_libs=$LIBS
      AC_SEARCH_LIBS([__gmpz_init], [gmp],
        [test "$ac_cv_search___gmpz_init" = "none required" ||
           LIB_GMP=$ac_cv_search___gmpz_init
         AC_DEFINE([HAVE_GMP], [1],
           [Define if you have GNU libgmp (or replacement)])
         HAVE_GMP=1
         # This only available in GMP >= 5
         AC_CHECK_DECLS([mpz_inits], [], [], [[#include <gmp.h>]])
        ])
      LIBS=$cu_saved_libs
    fi
    if test $HAVE_GMP != 1; then
     AC_MSG_WARN([libgmp development library was not found or not usable.])
     AC_MSG_WARN([AC_PACKAGE_NAME will be built without GMP support.])
    fi
  fi
])
