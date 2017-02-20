# $Id$

# Check the sizes we know about, and see if any of them match what's needed.
#
# Prefer ints to anything else, because read, write and others historically
# returned an int.
AC_DEFUN(AM_SEARCH_USIZES, [
	case "$3" in
	"$ac_cv_sizeof_unsigned_int")
		$1="typedef unsigned int $2;";;
	"$ac_cv_sizeof_unsigned_char")
		$1="typedef unsigned char $2;";;
	"$ac_cv_sizeof_unsigned_short")
		$1="typedef unsigned short $2;";;
	"$ac_cv_sizeof_unsigned_long")
		$1="typedef unsigned long $2;";;
	"$ac_cv_sizeof_unsigned_long_long")
		$1="typedef unsigned long long $2;";;
	*)
		if test "$4" != "notfatal"; then
			AC_MSG_ERROR([No unsigned $3-byte integral type])
		fi;;
	esac])
AC_DEFUN(AM_SEARCH_SSIZES, [
	case "$3" in
	"$ac_cv_sizeof_int")
		$1="typedef int $2;";;
	"$ac_cv_sizeof_char")
		$1="typedef char $2;";;
	"$ac_cv_sizeof_short")
		$1="typedef short $2;";;
	"$ac_cv_sizeof_long")
		$1="typedef long $2;";;
	"$ac_cv_sizeof_long_long")
		$1="typedef long long $2;";;
	*)
		if test "$4" != "notfatal"; then
			AC_MSG_ERROR([No signed $3-byte integral type])
		fi;;
	esac])

# Check for the standard system types.
AC_DEFUN(AM_TYPES, [

# db.h includes <sys/types.h> and <stdio.h>, not the other default includes
# autoconf usually includes.  For that reason, we specify a set of includes
# for all type checking tests. [#5060]
#
# C99 says types should be in <stdint.h>; include <stdint.h> if it exists.
#
# Some systems have types in <stddef.h>; include <stddef.h> if it exists.
#
# IBM's OS/390 and z/OS releases have types in <inttypes.h> not also found
# in <sys/types.h>; include <inttypes.h> if it exists.
db_includes="#include <sys/types.h>"
AC_SUBST(inttypes_h_decl)
AC_CHECK_HEADER(inttypes.h, [
	db_includes="$db_includes
#include <inttypes.h>"
	inttypes_h_decl="#include <inttypes.h>"])

# IRIX has stdint.h that is only available when using c99 (i.e. __c99
# is defined). Problem with having it in a public header is that a c++
# compiler cannot #include <db.h> if db.h #includes stdint.h, so we
# need to check that stdint.h is available for all cases.  Also the
# IRIX compiler does not exit with a non-zero exit code when it sees
# #error, so we actually need to use the header for the compiler to fail.
AC_SUBST(stdint_h_decl)
AC_MSG_CHECKING(for stdint.h)
AC_COMPILE_IFELSE([#include <stdint.h>
  int main() {
  uint_least8_t x=0;
  return x;
  }],[AC_MSG_RESULT(yes)
if test "$db_cv_cxx" = "yes"; then
  AC_MSG_CHECKING([if stdint.h can be used by C++])
  AC_LANG_PUSH(C++)
  AC_COMPILE_IFELSE([#include <stdint.h>
    int main() {
    uint_least8_t x=0;
    return x;
  }],[AC_MSG_RESULT(yes)
    stdint_h_decl="#include <stdint.h>"
    db_includes="$db_includes
#include <stdint.h>"
],[AC_MSG_RESULT(no)
    stdint_h_decl="#ifndef __cplusplus
#include <stdint.h>
#endif"
	db_includes="$db_includes
#ifndef __cplusplus
#include <stdint.h>
#endif"
])
    AC_LANG_POP
else
    stdint_h_decl="#include <stdint.h>"
    db_includes="$db_includes
#include <stdint.h>"
fi],[AC_MSG_RESULT(no)])

AC_SUBST(stddef_h_decl)
AC_CHECK_HEADER(stddef.h, [
	db_includes="$db_includes
#include <stddef.h>"
	stddef_h_decl="#include <stddef.h>"])
AC_SUBST(unistd_h_decl)
AC_CHECK_HEADER(unistd.h, [
	db_includes="$db_includes
#include <unistd.h>"
	unistd_h_decl="#include <unistd.h>"])
db_includes="$db_includes
#include <stdio.h>"

# We need to know the sizes of various objects on this system.
AC_CHECK_SIZEOF(char,, $db_includes)
AC_CHECK_SIZEOF(unsigned char,, $db_includes)
AC_CHECK_SIZEOF(short,, $db_includes)
AC_CHECK_SIZEOF(unsigned short,, $db_includes)
AC_CHECK_SIZEOF(int,, $db_includes)
AC_CHECK_SIZEOF(unsigned int,, $db_includes)
AC_CHECK_SIZEOF(long,, $db_includes)
AC_CHECK_SIZEOF(unsigned long,, $db_includes)
AC_CHECK_SIZEOF(long long,, $db_includes)
AC_CHECK_SIZEOF(unsigned long long,, $db_includes)
AC_CHECK_SIZEOF(char *,, $db_includes)

# We look for u_char, u_short, u_int, u_long -- if we can't find them,
# we create our own.
AC_SUBST(u_char_decl)
AC_CHECK_TYPE(u_char,,
    [u_char_decl="typedef unsigned char u_char;"], $db_includes)

AC_SUBST(u_short_decl)
AC_CHECK_TYPE(u_short,,
    [u_short_decl="typedef unsigned short u_short;"], $db_includes)

AC_SUBST(u_int_decl)
AC_CHECK_TYPE(u_int,,
    [u_int_decl="typedef unsigned int u_int;"], $db_includes)

AC_SUBST(u_long_decl)
AC_CHECK_TYPE(u_long,,
    [u_long_decl="typedef unsigned long u_long;"], $db_includes)

# We look for fixed-size variants of u_char, u_short, u_int, u_long as well.
AC_SUBST(u_int8_decl)
AC_CHECK_TYPE(u_int8_t,,
    [AM_SEARCH_USIZES(u_int8_decl, u_int8_t, 1)], $db_includes)

AC_SUBST(u_int16_decl)
AC_CHECK_TYPE(u_int16_t,,
    [AM_SEARCH_USIZES(u_int16_decl, u_int16_t, 2)], $db_includes)

AC_SUBST(int16_decl)
AC_CHECK_TYPE(int16_t,,
    [AM_SEARCH_SSIZES(int16_decl, int16_t, 2)], $db_includes)

AC_SUBST(u_int32_decl)
AC_CHECK_TYPE(u_int32_t,,
    [AM_SEARCH_USIZES(u_int32_decl, u_int32_t, 4)], $db_includes)

AC_SUBST(int32_decl)
AC_CHECK_TYPE(int32_t,,
    [AM_SEARCH_SSIZES(int32_decl, int32_t, 4)], $db_includes)

AC_SUBST(u_int64_decl)
AC_CHECK_TYPE(u_int64_t,,
    [AM_SEARCH_USIZES(u_int64_decl, u_int64_t, 8, notfatal)], $db_includes)

AC_SUBST(int64_decl)
AC_CHECK_TYPE(int64_t,,
    [AM_SEARCH_SSIZES(int64_decl, int64_t, 8, notfatal)], $db_includes)

# No currently autoconf'd systems lack FILE, off_t pid_t, size_t, time_t.
#
# We require them, we don't try to substitute our own if we can't find them.
AC_SUBST(FILE_t_decl)
AC_CHECK_TYPE(FILE *,, AC_MSG_ERROR([No FILE type.]), $db_includes)
AC_SUBST(off_t_decl)
AC_CHECK_TYPE(off_t,, AC_MSG_ERROR([No off_t type.]), $db_includes)
AC_SUBST(pid_t_decl)
AC_CHECK_TYPE(pid_t,, AC_MSG_ERROR([No pid_t type.]), $db_includes)
AC_SUBST(size_t_decl)
AC_CHECK_TYPE(size_t,, AC_MSG_ERROR([No size_t type.]), $db_includes)
AC_SUBST(time_t_decl)
AC_CHECK_TYPE(time_t,, AC_MSG_ERROR([No time_t type.]), $db_includes)

# Check for ssize_t -- if none exists, find a signed integral type that's
# the same size as a size_t.
AC_CHECK_SIZEOF(size_t,, $db_includes)
AC_SUBST(ssize_t_decl)
AC_CHECK_TYPE(ssize_t,,
    [AM_SEARCH_SSIZES(ssize_t_decl, ssize_t, $ac_cv_sizeof_size_t)],
    $db_includes)

# Check for uintmax_t -- if none exists, find the largest unsigned integral
# type available.
AC_SUBST(uintmax_t_decl)
AC_CHECK_TYPE(uintmax_t,, [AC_CHECK_TYPE(unsigned long long,
    [uintmax_t_decl="typedef unsigned long long uintmax_t;"],
    [uintmax_t_decl="typedef unsigned long uintmax_t;"], $db_includes)])

# Check for uintptr_t -- if none exists, find an integral type which is
# the same size as a pointer.
AC_SUBST(uintptr_t_decl)
AC_CHECK_TYPE(uintptr_t,,
    [AM_SEARCH_USIZES(uintptr_t_decl, uintptr_t, $ac_cv_sizeof_char_p)])

AM_SOCKLEN_T
])
