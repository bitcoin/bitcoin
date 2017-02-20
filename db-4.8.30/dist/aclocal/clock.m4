# $Id$

# Configure clocks and timers.
AC_DEFUN(AC_TIMERS, [

AC_CHECK_FUNCS(gettimeofday localtime time strftime)

# AIX 4.3 will link applications with calls to clock_gettime, but the
# calls always fail.
case "$host_os" in
aix4.3.*)
	;;
*)
	AC_CHECK_FUNCS(clock_gettime);;
esac

# clock_gettime -- monotonic clocks.
#	Check to see if we can get a monotonic clock.  We actually try and
#	run the program if possible, because we don't trust the #define's
#	existence to mean the clock really exists.
AC_CACHE_CHECK([for clock_gettime monotonic clock], db_cv_clock_monotonic, [
AC_TRY_RUN([
#include <sys/time.h>
main() {
	struct timespec t;
	return (clock_gettime(CLOCK_MONOTONIC, &t) != 0);
}], db_cv_clock_monotonic=yes, db_cv_clock_monotonic=no,
AC_TRY_LINK([
#include <sys/time.h>], [
struct timespec t;
clock_gettime(CLOCK_MONOTONIC, &t);
], db_cv_clock_monotonic=yes, db_cv_clock_monotonic=no))
])
if test "$db_cv_clock_monotonic" = "yes"; then
	AC_DEFINE(HAVE_CLOCK_MONOTONIC)
	AH_TEMPLATE(HAVE_CLOCK_MONOTONIC,
	    [Define to 1 if clock_gettime supports CLOCK_MONOTONIC.])
fi

# ctime_r --
#
# There are two versions of ctime_r, one of which takes a buffer length as a
# third argument, and one which only takes two arguments.  (There is also a
# difference in return values and the type of the 3rd argument, but we handle
# those problems in the code itself.)
AC_CHECK_FUNCS(ctime_r)
if test "$ac_cv_func_ctime_r" = "yes"; then
AC_CACHE_CHECK([for 2 or 3 argument version of ctime_r], db_cv_ctime_r_3arg, [
AC_TRY_LINK([
#include <time.h>], [
	ctime_r(NULL, NULL, 100);
],  [db_cv_ctime_r_3arg="3-argument"], [db_cv_ctime_r_3arg="2-argument"])])
fi
if test "$db_cv_ctime_r_3arg" = "3-argument"; then
	AC_DEFINE(HAVE_CTIME_R_3ARG)
	AH_TEMPLATE(HAVE_CTIME_R_3ARG,
	    [Define to 1 if ctime_r takes a buffer length as a third argument.])
fi
])
