# Check for strnlen()
dnl AC_CHECK_FUNC isn't properly respecting _XOPEN_SOURCE for strnlen for unknown reason
AC_SUBST(BSON_HAVE_STRNLEN, 0)
AC_CACHE_CHECK([for strnlen],
  bson_cv_have_strnlen,
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <string.h>
int strnlen () { return 0; }
]])],
    [bson_cv_have_strnlen=no],
    [bson_cv_have_strnlen=yes])])
if test "$bson_cv_have_strnlen" = yes; then
    AC_SUBST(BSON_HAVE_STRNLEN, 1)
fi

# Check for reallocf() (BSD/Darwin)
AC_SUBST(BSON_HAVE_REALLOCF, 0)
AC_CACHE_CHECK([for reallocf],
  bson_cv_have_reallocf,
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
int reallocf () { return 0; }
]])],
    [bson_cv_have_reallocf=no],
    [bson_cv_have_reallocf=yes])])
if test "$bson_cv_have_reallocf" = yes; then
    AC_SUBST(BSON_HAVE_REALLOCF, 1)
fi

# Check for syscall()
AC_SUBST(BSON_HAVE_SYSCALL_TID, 0)
AC_CACHE_CHECK([for syscall],
  bson_cv_have_syscall_tid,
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <unistd.h>
#include <sys/syscall.h>
int syscall () { return 0; }
]])],
    [bson_cv_have_syscall_tid=no],
    [bson_cv_have_syscall_tid=yes])])
if test "$bson_cv_have_syscall_tid" = yes -a "$os_darwin" != "yes"; then
   AC_CACHE_CHECK([for SYS_gettid],
     bson_cv_have_sys_gettid_tid,
     [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <unistd.h>
#include <sys/syscall.h>
int gettid () { return SYS_gettid; }
   ]])],
       [bson_cv_have_sys_gettid_tid=yes],
       [bson_cv_have_sys_gettid_tid=no])])
   if test "$bson_cv_have_sys_gettid_tid" = yes; then
       AC_SUBST(BSON_HAVE_SYSCALL_TID, 1)
   fi
fi

# Check for snprintf()
AC_SUBST(BSON_HAVE_SNPRINTF, 0)
AC_CHECK_FUNC(snprintf, [AC_SUBST(BSON_HAVE_SNPRINTF, 1)])

# Check for struct timespec
AC_SUBST(BSON_HAVE_TIMESPEC, 0)
AC_CHECK_TYPE([struct timespec], [AC_SUBST(BSON_HAVE_TIMESPEC, 1)], [], [#include <time.h>])

# Check for clock_gettime and if it needs -lrt
AC_SUBST(BSON_HAVE_CLOCK_GETTIME, 0)
AC_SEARCH_LIBS([clock_gettime], [rt], [AC_SUBST(BSON_HAVE_CLOCK_GETTIME, 1)])
# Check if math functions need -lm
AC_SEARCH_LIBS([floor], [m])

# Check for gmtime_r()
AC_SUBST(BSON_HAVE_GMTIME_R, 0)
AC_CHECK_FUNC(gmtime_r, [AC_SUBST(BSON_HAVE_GMTIME_R, 1)])

# Check for pthreads. We might need to make this better to handle mingw,
# but I actually think it is okay to just check for it even though we will
# use win32 primatives.
AX_PTHREAD([],
           [AC_MSG_ERROR([libbson requires pthreads on non-Windows platforms.])])


# The following is borrowed from the guile configure script.
#
# On past versions of Solaris, believe 8 through 10 at least, you
# had to write "pthread_once_t foo = { PTHREAD_ONCE_INIT };".
# This is contrary to POSIX:
# http://www.opengroup.org/onlinepubs/000095399/functions/pthread_once.html
# Check here if this style is required.
#
# glibc (2.3.6 at least) works both with or without braces, so the
# test checks whether it works without.
#
AC_SUBST(BSON_PTHREAD_ONCE_INIT_NEEDS_BRACES, 0)
AC_CACHE_CHECK([whether PTHREAD_ONCE_INIT needs braces],
  bson_cv_need_braces_on_pthread_once_init,
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <pthread.h>
     pthread_once_t foo = PTHREAD_ONCE_INIT;]])],
    [bson_cv_need_braces_on_pthread_once_init=no],
    [bson_cv_need_braces_on_pthread_once_init=yes])])
if test "$bson_cv_need_braces_on_pthread_once_init" = yes; then
    AC_SUBST(BSON_PTHREAD_ONCE_INIT_NEEDS_BRACES, 1)
fi

# Solaris needs to link against socket libs.
# This is only used in our streaming bson examples
if test "$os_solaris" = "yes"; then
    SOCKET_CFLAGS="$CFLAGS -D__EXTENSIONS__"
    SOCKET_CFLAGS="$CFLAGS -D_XOPEN_SOURCE=1"
    SOCKET_CFLAGS="$CFLAGS -D_XOPEN_SOURCE_EXTENDED=1"
    SOCKET_LDFLAGS="$LDFLAGS -lsocket -lnsl"
    AC_SUBST(SOCKET_CFLAGS)
    AC_SUBST(SOCKET_LDFLAGS)
fi
