m4_define([snappy_major], [1])
m4_define([snappy_minor], [1])
m4_define([snappy_patchlevel], [3])

# Libtool shared library interface versions (current:revision:age)
# Update this value for every release!  (A:B:C will map to foo.so.(A-C).C.B)
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
m4_define([snappy_ltversion], [4:0:3])

AC_INIT([snappy], [snappy_major.snappy_minor.snappy_patchlevel])
AC_CONFIG_MACRO_DIR([m4])

# These are flags passed to automake (though they look like gcc flags!)
AM_INIT_AUTOMAKE([-Wall])

LT_INIT
AC_SUBST([LIBTOOL_DEPS])
AC_PROG_CXX
AC_LANG([C++])
AC_C_BIGENDIAN
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_HEADERS([stdint.h stddef.h sys/mman.h sys/resource.h windows.h byteswap.h sys/byteswap.h sys/endian.h sys/time.h])

# Don't use AC_FUNC_MMAP, as it checks for mappings of already-mapped memory,
# which we don't need (and does not exist on Windows).
AC_CHECK_FUNC([mmap])

GTEST_LIB_CHECK([], [true], [true # Ignore; we can live without it.])

AC_ARG_WITH([gflags],
  [AS_HELP_STRING(
    [--with-gflags],
    [use Google Flags package to enhance the unit test @<:@default=check@:>@])],
    [],
    [with_gflags=check])

if test "x$with_gflags" != "xno"; then
  PKG_CHECK_MODULES(
    [gflags],
    [libgflags],
    [AC_DEFINE([HAVE_GFLAGS], [1], [Use the gflags package for command-line parsing.])],
    [if test "x$with_gflags" != "xcheck"; then
      AC_MSG_FAILURE([--with-gflags was given, but test for gflags failed])
    fi])
fi

# See if we have __builtin_expect.
# TODO: Use AC_CACHE.
AC_MSG_CHECKING([if the compiler supports __builtin_expect])
 
AC_TRY_COMPILE(, [
    return __builtin_expect(1, 1) ? 1 : 0
], [
    snappy_have_builtin_expect=yes
    AC_MSG_RESULT([yes])
], [
    snappy_have_builtin_expect=no
    AC_MSG_RESULT([no])
])
if test x$snappy_have_builtin_expect = xyes ; then
    AC_DEFINE([HAVE_BUILTIN_EXPECT], [1], [Define to 1 if the compiler supports __builtin_expect.])
fi

# See if we have working count-trailing-zeros intrinsics.
# TODO: Use AC_CACHE.
AC_MSG_CHECKING([if the compiler supports __builtin_ctzll])

AC_TRY_COMPILE(, [
    return (__builtin_ctzll(0x100000000LL) == 32) ? 1 : 0
], [
    snappy_have_builtin_ctz=yes
    AC_MSG_RESULT([yes])
], [
    snappy_have_builtin_ctz=no
    AC_MSG_RESULT([no])
])
if test x$snappy_have_builtin_ctz = xyes ; then
    AC_DEFINE([HAVE_BUILTIN_CTZ], [1], [Define to 1 if the compiler supports __builtin_ctz and friends.])
fi

# Other compression libraries; the unit test can use these for comparison
# if they are available. If they are not found, just ignore.
UNITTEST_LIBS=""
AC_DEFUN([CHECK_EXT_COMPRESSION_LIB], [
  AH_CHECK_LIB([$1])
  AC_CHECK_LIB(
    [$1],
    [$2],
    [
      AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_LIB$1))
      UNITTEST_LIBS="-l$1 $UNITTEST_LIBS"
    ],
    [true]
  )
])
CHECK_EXT_COMPRESSION_LIB([z], [zlibVersion])
CHECK_EXT_COMPRESSION_LIB([lzo2], [lzo1x_1_15_compress])
CHECK_EXT_COMPRESSION_LIB([lzf], [lzf_compress])
CHECK_EXT_COMPRESSION_LIB([fastlz], [fastlz_compress])
CHECK_EXT_COMPRESSION_LIB([quicklz], [qlz_compress])
AC_SUBST([UNITTEST_LIBS])

# These are used by snappy-stubs-public.h.in.
if test "$ac_cv_header_stdint_h" = "yes"; then
    AC_SUBST([ac_cv_have_stdint_h], [1])
else
    AC_SUBST([ac_cv_have_stdint_h], [0])
fi
if test "$ac_cv_header_stddef_h" = "yes"; then
    AC_SUBST([ac_cv_have_stddef_h], [1])
else
    AC_SUBST([ac_cv_have_stddef_h], [0])
fi
if test "$ac_cv_header_sys_uio_h" = "yes"; then
    AC_SUBST([ac_cv_have_sys_uio_h], [1])
else
    AC_SUBST([ac_cv_have_sys_uio_h], [0])
fi

# Export the version to snappy-stubs-public.h.
SNAPPY_MAJOR="snappy_major"
SNAPPY_MINOR="snappy_minor"
SNAPPY_PATCHLEVEL="snappy_patchlevel"

AC_SUBST([SNAPPY_MAJOR])
AC_SUBST([SNAPPY_MINOR])
AC_SUBST([SNAPPY_PATCHLEVEL])
AC_SUBST([SNAPPY_LTVERSION], snappy_ltversion)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile snappy-stubs-public.h])
AC_OUTPUT
