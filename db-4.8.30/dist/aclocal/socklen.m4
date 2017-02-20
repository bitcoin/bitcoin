dnl Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Albert Chin, Windows fixes from Simon Josefsson.

dnl Check for socklen_t: historically on BSD it is an int, and in
dnl POSIX 1g it is a type of its own, but some platforms use different
dnl types for the argument to getsockopt, getpeername, etc.  So we
dnl have to test to find something that will work.

dnl On mingw32, socklen_t is in ws2tcpip.h ('int'), so we try to find
dnl it there first.  That file is included by gnulib's socket_.h, which
dnl all users of this module should include.  Cygwin must not include
dnl ws2tcpip.h.

dnl Windows fixes removed for Berkeley DB. Functions renamed, basic check
dnl remains the same though.
dnl !!!
dnl The original version had fixes for MinGW -- if you need those, go back
dnl and look at the original code.

AC_DEFUN([AM_SOCKLEN_T],[
   AC_CHECK_TYPE([socklen_t], ,
     [AC_MSG_CHECKING([for socklen_t equivalent])
      AC_CACHE_VAL([db_cv_socklen_t_equiv],
	[# Systems have either "struct sockaddr *" or
	 # "void *" as the second argument to getpeername
	 db_cv_socklen_t_equiv=
	 for arg2 in "struct sockaddr" void; do
	   for t in int size_t "unsigned int" "long int" "unsigned long int"; do
	     AC_TRY_COMPILE([$db_includes
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
		int getpeername (int, $arg2 *, $t *);],
	       [$t len;
		getpeername (0, 0, &len);],
	       [db_cv_socklen_t_equiv="$t"])
	     test "$db_cv_socklen_t_equiv" != "" && break
	   done
	   test "$db_cv_socklen_t_equiv" != "" && break
	 done
      ])
      if test "$db_cv_socklen_t_equiv" = ""; then
	AC_MSG_ERROR([Cannot find a type to use in place of socklen_t])
      fi
      AC_MSG_RESULT([$db_cv_socklen_t_equiv])
      AC_DEFINE_UNQUOTED([socklen_t], [$db_cv_socklen_t_equiv],
	[type to use in place of socklen_t if not defined])],
	[$db_includes
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif])])
