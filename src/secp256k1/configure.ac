AC_PREREQ([2.60])
AC_INIT([libsecp256k1],[0.1])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AC_CANONICAL_HOST
AH_TOP([#ifndef LIBSECP256K1_CONFIG_H])
AH_TOP([#define LIBSECP256K1_CONFIG_H])
AH_BOTTOM([#endif /*LIBSECP256K1_CONFIG_H*/])
AM_INIT_AUTOMAKE([foreign subdir-objects])
LT_INIT

dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

PKG_PROG_PKG_CONFIG

AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)
AC_PATH_TOOL(STRIP, strip)
AX_PROG_CC_FOR_BUILD

if test "x$CFLAGS" = "x"; then
  CFLAGS="-O3 -g"
fi

AM_PROG_CC_C_O

AC_PROG_CC_C89
if test x"$ac_cv_prog_cc_c89" = x"no"; then
  AC_MSG_ERROR([c89 compiler support required])
fi

case $host_os in
  *darwin*)
     if  test x$cross_compiling != xyes; then
       AC_PATH_PROG([BREW],brew,)
       if test x$BREW != x; then
         dnl These Homebrew packages may be keg-only, meaning that they won't be found
         dnl in expected paths because they may conflict with system files. Ask
         dnl Homebrew where each one is located, then adjust paths accordingly.

         openssl_prefix=`$BREW --prefix openssl 2>/dev/null`
         gmp_prefix=`$BREW --prefix gmp 2>/dev/null`
         if test x$openssl_prefix != x; then
           PKG_CONFIG_PATH="$openssl_prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
           export PKG_CONFIG_PATH
         fi
         if test x$gmp_prefix != x; then
           GMP_CPPFLAGS="-I$gmp_prefix/include"
           GMP_LIBS="-L$gmp_prefix/lib"
         fi
       else
         AC_PATH_PROG([PORT],port,)
         dnl if homebrew isn't installed and macports is, add the macports default paths
         dnl as a last resort.
         if test x$PORT != x; then
           CPPFLAGS="$CPPFLAGS -isystem /opt/local/include"
           LDFLAGS="$LDFLAGS -L/opt/local/lib"
         fi
       fi
     fi
   ;;
esac

CFLAGS="$CFLAGS -W"

warn_CFLAGS="-std=c89 -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings"
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $warn_CFLAGS"
AC_MSG_CHECKING([if ${CC} supports ${warn_CFLAGS}])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden"
AC_MSG_CHECKING([if ${CC} supports -fvisibility=hidden])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

AC_ARG_ENABLE(benchmark,
    AS_HELP_STRING([--enable-benchmark],[compile benchmark (default is no)]),
    [use_benchmark=$enableval],
    [use_benchmark=no])

AC_ARG_ENABLE(tests,
    AS_HELP_STRING([--enable-tests],[compile tests (default is yes)]),
    [use_tests=$enableval],
    [use_tests=yes])

AC_ARG_ENABLE(endomorphism,
    AS_HELP_STRING([--enable-endomorphism],[enable endomorphism (default is no)]),
    [use_endomorphism=$enableval],
    [use_endomorphism=no])
    
AC_ARG_ENABLE(ecmult_static_precomputation,
    AS_HELP_STRING([--enable-ecmult-static-precomputation],[enable precomputed ecmult table for signing (default is yes)]),
    [use_ecmult_static_precomputation=$enableval],
    [use_ecmult_static_precomputation=yes])

AC_ARG_ENABLE(module_ecdh,
    AS_HELP_STRING([--enable-module-ecdh],[enable ECDH shared secret computation (default is no)]),
    [enable_module_ecdh=$enableval],
    [enable_module_ecdh=no])

AC_ARG_ENABLE(module_schnorr,
    AS_HELP_STRING([--enable-module-schnorr],[enable Schnorr signature module (default is no)]),
    [enable_module_schnorr=$enableval],
    [enable_module_schnorr=no])

AC_ARG_ENABLE(module_recovery,
    AS_HELP_STRING([--enable-module-recovery],[enable ECDSA pubkey recovery module (default is no)]),
    [enable_module_recovery=$enableval],
    [enable_module_recovery=no])

AC_ARG_WITH([field], [AS_HELP_STRING([--with-field=64bit|32bit|auto],
[Specify Field Implementation. Default is auto])],[req_field=$withval], [req_field=auto])

AC_ARG_WITH([bignum], [AS_HELP_STRING([--with-bignum=gmp|no|auto],
[Specify Bignum Implementation. Default is auto])],[req_bignum=$withval], [req_bignum=auto])

AC_ARG_WITH([scalar], [AS_HELP_STRING([--with-scalar=64bit|32bit|auto],
[Specify scalar implementation. Default is auto])],[req_scalar=$withval], [req_scalar=auto])

AC_ARG_WITH([asm], [AS_HELP_STRING([--with-asm=x86_64|no|auto]
[Specify assembly optimizations to use. Default is auto])],[req_asm=$withval], [req_asm=auto])

AC_CHECK_TYPES([__int128])

AC_MSG_CHECKING([for __builtin_expect])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() {__builtin_expect(0,0);}]])],
    [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_EXPECT,1,[Define this symbol if __builtin_expect is available]) ],
    [ AC_MSG_RESULT([no])
    ])

if test x"$req_asm" = x"auto"; then
  SECP_64BIT_ASM_CHECK
  if test x"$has_64bit_asm" = x"yes"; then
    set_asm=x86_64
  fi
  if test x"$set_asm" = x; then
    set_asm=no
  fi
else
  set_asm=$req_asm
  case $set_asm in
  x86_64)
    SECP_64BIT_ASM_CHECK
    if test x"$has_64bit_asm" != x"yes"; then
      AC_MSG_ERROR([x86_64 assembly optimization requested but not available])
    fi
    ;;
  no)
    ;;
  *)
    AC_MSG_ERROR([invalid assembly optimization selection])
    ;;
  esac
fi

if test x"$req_field" = x"auto"; then
  if test x"set_asm" = x"x86_64"; then
    set_field=64bit
  fi
  if test x"$set_field" = x; then
    SECP_INT128_CHECK
    if test x"$has_int128" = x"yes"; then
      set_field=64bit
    fi
  fi
  if test x"$set_field" = x; then
    set_field=32bit
  fi
else
  set_field=$req_field
  case $set_field in
  64bit)
    if test x"$set_asm" != x"x86_64"; then
      SECP_INT128_CHECK
      if test x"$has_int128" != x"yes"; then
        AC_MSG_ERROR([64bit field explicitly requested but neither __int128 support or x86_64 assembly available])
      fi
    fi
    ;;
  32bit)
    ;;
  *)
    AC_MSG_ERROR([invalid field implementation selection])
    ;;
  esac
fi

if test x"$req_scalar" = x"auto"; then
  SECP_INT128_CHECK
  if test x"$has_int128" = x"yes"; then
    set_scalar=64bit
  fi
  if test x"$set_scalar" = x; then
    set_scalar=32bit
  fi
else
  set_scalar=$req_scalar
  case $set_scalar in
  64bit)
    SECP_INT128_CHECK
    if test x"$has_int128" != x"yes"; then
      AC_MSG_ERROR([64bit scalar explicitly requested but __int128 support not available])
    fi
    ;;
  32bit)
    ;;
  *)
    AC_MSG_ERROR([invalid scalar implementation selected])
    ;;
  esac
fi

if test x"$req_bignum" = x"auto"; then
  SECP_GMP_CHECK
  if test x"$has_gmp" = x"yes"; then
    set_bignum=gmp
  fi

  if test x"$set_bignum" = x; then
    set_bignum=no
  fi
else
  set_bignum=$req_bignum
  case $set_bignum in
  gmp)
    SECP_GMP_CHECK
    if test x"$has_gmp" != x"yes"; then
      AC_MSG_ERROR([gmp bignum explicitly requested but libgmp not available])
    fi
    ;;
  no)
    ;;
  *)
    AC_MSG_ERROR([invalid bignum implementation selection])
    ;;
  esac
fi

# select assembly optimization
case $set_asm in
x86_64)
  AC_DEFINE(USE_ASM_X86_64, 1, [Define this symbol to enable x86_64 assembly optimizations])
  ;;
no)
  ;;
*)
  AC_MSG_ERROR([invalid assembly optimizations])
  ;;
esac

# select field implementation
case $set_field in
64bit)
  AC_DEFINE(USE_FIELD_5X52, 1, [Define this symbol to use the FIELD_5X52 implementation])
  ;;
32bit)
  AC_DEFINE(USE_FIELD_10X26, 1, [Define this symbol to use the FIELD_10X26 implementation])
  ;;
*)
  AC_MSG_ERROR([invalid field implementation])
  ;;
esac

# select bignum implementation
case $set_bignum in
gmp)
  AC_DEFINE(HAVE_LIBGMP, 1, [Define this symbol if libgmp is installed])
  AC_DEFINE(USE_NUM_GMP, 1, [Define this symbol to use the gmp implementation for num])
  AC_DEFINE(USE_FIELD_INV_NUM, 1, [Define this symbol to use the num-based field inverse implementation])
  AC_DEFINE(USE_SCALAR_INV_NUM, 1, [Define this symbol to use the num-based scalar inverse implementation])
  ;;
no)
  AC_DEFINE(USE_NUM_NONE, 1, [Define this symbol to use no num implementation])
  AC_DEFINE(USE_FIELD_INV_BUILTIN, 1, [Define this symbol to use the native field inverse implementation])
  AC_DEFINE(USE_SCALAR_INV_BUILTIN, 1, [Define this symbol to use the native scalar inverse implementation])
  ;;
*)
  AC_MSG_ERROR([invalid bignum implementation])
  ;;
esac

#select scalar implementation
case $set_scalar in
64bit)
  AC_DEFINE(USE_SCALAR_4X64, 1, [Define this symbol to use the 4x64 scalar implementation])
  ;;
32bit)
  AC_DEFINE(USE_SCALAR_8X32, 1, [Define this symbol to use the 8x32 scalar implementation])
  ;;
*)
  AC_MSG_ERROR([invalid scalar implementation])
  ;;
esac

if test x"$use_tests" = x"yes"; then
  SECP_OPENSSL_CHECK
  if test x"$has_openssl_ec" = x"yes"; then
    AC_DEFINE(ENABLE_OPENSSL_TESTS, 1, [Define this symbol if OpenSSL EC functions are available])
    SECP_TEST_INCLUDES="$SSL_CFLAGS $CRYPTO_CFLAGS"
    SECP_TEST_LIBS="$CRYPTO_LIBS"

    case $host in
    *mingw*)
      SECP_TEST_LIBS="$SECP_TEST_LIBS -lgdi32"
      ;;
    esac

  fi
fi

if test x"$set_bignum" = x"gmp"; then
  SECP_LIBS="$SECP_LIBS $GMP_LIBS"
  SECP_INCLUDES="$SECP_INCLUDES $GMP_CPPFLAGS"
fi

if test x"$use_endomorphism" = x"yes"; then
  AC_DEFINE(USE_ENDOMORPHISM, 1, [Define this symbol to use endomorphism optimization])
fi

if test x"$use_ecmult_static_precomputation" = x"yes"; then
  AC_DEFINE(USE_ECMULT_STATIC_PRECOMPUTATION, 1, [Define this symbol to use a statically generated ecmult table])
fi

if test x"$enable_module_ecdh" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_ECDH, 1, [Define this symbol to enable the ECDH module])
fi

if test x"$enable_module_schnorr" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_SCHNORR, 1, [Define this symbol to enable the Schnorr signature module])
fi

if test x"$enable_module_recovery" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_RECOVERY, 1, [Define this symbol to enable the ECDSA pubkey recovery module])
fi

AC_C_BIGENDIAN()

AC_MSG_NOTICE([Using assembly optimizations: $set_asm])
AC_MSG_NOTICE([Using field implementation: $set_field])
AC_MSG_NOTICE([Using bignum implementation: $set_bignum])
AC_MSG_NOTICE([Using scalar implementation: $set_scalar])
AC_MSG_NOTICE([Using endomorphism optimizations: $use_endomorphism])
AC_MSG_NOTICE([Building ECDH module: $enable_module_ecdh])

AC_MSG_NOTICE([Building Schnorr signatures module: $enable_module_schnorr])
AC_MSG_NOTICE([Building ECDSA pubkey recovery module: $enable_module_recovery])

AC_CONFIG_HEADERS([src/libsecp256k1-config.h])
AC_CONFIG_FILES([Makefile libsecp256k1.pc])
AC_SUBST(SECP_INCLUDES)
AC_SUBST(SECP_LIBS)
AC_SUBST(SECP_TEST_LIBS)
AC_SUBST(SECP_TEST_INCLUDES)
AM_CONDITIONAL([USE_TESTS], [test x"$use_tests" != x"no"])
AM_CONDITIONAL([USE_BENCHMARK], [test x"$use_benchmark" = x"yes"])
AM_CONDITIONAL([USE_ECMULT_STATIC_PRECOMPUTATION], [test x"$use_ecmult_static_precomputation" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_ECDH], [test x"$enable_module_ecdh" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_SCHNORR], [test x"$enable_module_schnorr" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_RECOVERY], [test x"$enable_module_recovery" = x"yes"])

dnl make sure nothing new is exported so that we don't break the cache
PKGCONFIG_PATH_TEMP="$PKG_CONFIG_PATH"
unset PKG_CONFIG_PATH
PKG_CONFIG_PATH="$PKGCONFIG_PATH_TEMP"

AC_OUTPUT
