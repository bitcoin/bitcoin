AC_INIT([minisketch], [0.0.1], [http://github.com/bitcoin-core/minisketch/])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PREREQ(2.60)
AC_CONFIG_SRCDIR([src/minisketch.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AM_INIT_AUTOMAKE([subdir-objects foreign])

LT_INIT
LT_LANG([C++])
AC_LANG([C++])

AC_PATH_PROG(CCACHE,ccache)

AC_ARG_ENABLE([ccache],
  [AS_HELP_STRING([--disable-ccache],
  [do not use ccache for building (default is to use if found)])],
  [use_ccache=$enableval],
  [use_ccache=auto])

AC_ARG_ENABLE(tests,
    AS_HELP_STRING([--enable-tests],[compile tests (default is yes)]),
    [use_tests=$enableval],
    [use_tests=yes])

AC_ARG_ENABLE(benchmark,
    AS_HELP_STRING([--enable-benchmark],[compile benchmark (default is no)]),
    [use_benchmark=$enableval],
    [use_benchmark=no])

m4_define([SUPPORTED_FIELDS], [2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64])

AC_MSG_CHECKING([which field sizes to build])
AC_ARG_ENABLE([fields], AS_HELP_STRING([--enable-fields=LIST], [Comma-separated list of field sizes to build. Default=all. Available sizes:] m4_translit(m4_defn([SUPPORTED_FIELDS]), [,], [ ])), [], [enable_fields=SUPPORTED_FIELDS])
have_disabled_fields=no
have_enabled_fields=no
m4_foreach([FIELD], [SUPPORTED_FIELDS], [
    case ",$enable_fields," in
    *,FIELD,*)
        have_enabled_fields=yes
        ;;
    *)
        AC_DEFINE(DISABLE_FIELD_[]FIELD, [1],
        [Define to 1 to remove support for field size] FIELD [.])
        have_disabled_fields=yes
        ;;
    esac
])
AC_MSG_RESULT([$enable_fields])
if test "x$have_enabled_fields" = xno; then
    AC_MSG_ERROR([No field sizes are enabled.])
fi

AX_CHECK_COMPILE_FLAG([-Werror],[CXXFLAG_WERROR="-Werror"],[CXXFLAG_WERROR=""])

AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])
enable_clmul=
AX_CHECK_COMPILE_FLAG([-mpclmul],[[enable_clmul=yes]],,[[$CXXFLAG_WERROR]],[AC_LANG_PROGRAM([
  #include <stdint.h>
  #include <x86intrin.h>
], [
  __m128i a = _mm_cvtsi64_si128((uint64_t)7);
  __m128i b = _mm_clmulepi64_si128(a, a, 37);
  __m128i c = _mm_srli_epi64(b, 41);
  __m128i d = _mm_xor_si128(b, c);
  uint64_t e = _mm_cvtsi128_si64(d);
  return e == 0;
])])
if test x$enable_clmul = xyes; then
  CLMUL_CXXFLAGS="-mpclmul"
  AC_DEFINE(HAVE_CLMUL, 1, [Define this symbol if clmul instructions can be used])
fi


AC_MSG_CHECKING(for working clz builtins)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [
  unsigned a = __builtin_clz(1);
  unsigned long b = __builtin_clzl(1);
  unsigned long long c = __builtin_clzll(1);
  ])],
  [
    AC_DEFINE(HAVE_CLZ, 1, [Define this symbol if clz builtins are present and working])
    AC_MSG_RESULT(yes)
  ],[
    AC_MSG_RESULT(no)
  ]
)

AX_CHECK_LINK_FLAG([[-Wl,--exclude-libs,ALL]],[LDFLAGS="-Wl,--exclude-libs,ALL $LDFLAGS"])

case $host in
  *mingw*)
     dnl -static is interpreted by libtool, where it has a different meaning.
     dnl In libtool-speak, it's -all-static.
     AX_CHECK_LINK_FLAG([[-static]],[LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static"])
     ;;
  *)
     AX_CHECK_LINK_FLAG([[-static]],[LIBTOOL_APP_LDFLAGS="-static"])
     ;;
esac

AX_CHECK_COMPILE_FLAG([-Wall],[WARN_CXXFLAGS="$WARN_CXXFLAGS -Wall"],,[[$CXXFLAG_WERROR]])
AX_CHECK_COMPILE_FLAG([-Wundef], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wundef"], [], [$CXXFLAG_WERROR])
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden],[CXXFLAGS="$CXXFLAGS -fvisibility=hidden"],[],[$CXXFLAG_WERROR])

if test "x$use_ccache" != "xno"; then
  AC_MSG_CHECKING(if ccache should be used)
  if test x$CCACHE = x; then
    if test "x$use_ccache" = "xyes"; then
      AC_MSG_ERROR([ccache not found.]);
    else
      use_ccache=no
    fi
  else
    use_ccache=yes
    CXX="$ac_cv_path_CCACHE $CXX"
  fi
  AC_MSG_RESULT($use_ccache)
fi

VERIFY_DEFINES=-DMINISKETCH_VERIFY
RELEASE_DEFINES=

AC_CONFIG_FILES([
    Makefile
])

AC_SUBST(CLMUL_CXXFLAGS)
AC_SUBST(WARN_CXXFLAGS)
AC_SUBST(NOWARN_CXXFLAGS)
AC_SUBST(VERIFY_DEFINES)
AC_SUBST(RELEASE_DEFINES)
AC_SUBST(LIBTOOL_APP_LDFLAGS)
AM_CONDITIONAL([ENABLE_CLMUL],[test x$enable_clmul = xyes])
AM_CONDITIONAL([USE_BENCHMARK], [test x"$use_benchmark" = x"yes"])
AM_CONDITIONAL([USE_TESTS], [test x"$use_tests" != x"no"])
AC_OUTPUT

echo
echo "Build Options:"
echo "  with benchmarks         = $use_benchmark"
echo "  with tests              = $use_tests"
echo "  enable clmul fields     = $enable_clmul"
echo "  CXX                     = $CXX"
echo "  CXXFLAGS                = $CXXFLAGS"
echo "  CPPFLAGS                = $CPPFLAGS"
echo "  LDFLAGS                 = $LDFLAGS"
if test "$have_disabled_fields" = "yes"; then
echo
echo "Only compiling in support for field sizes: $enable_fields"
echo "WARNING: this means the library will lack support for other field sizes entirely"
fi
