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
  CFLAGS="-g"
fi

AM_PROG_CC_C_O

AC_PROG_CC_C89
if test x"$ac_cv_prog_cc_c89" = x"no"; then
  AC_MSG_ERROR([c89 compiler support required])
fi
AM_PROG_AS

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
    AS_HELP_STRING([--enable-benchmark],[compile benchmark (default is yes)]),
    [use_benchmark=$enableval],
    [use_benchmark=yes])

AC_ARG_ENABLE(coverage,
    AS_HELP_STRING([--enable-coverage],[enable compiler flags to support kcov coverage analysis]),
    [enable_coverage=$enableval],
    [enable_coverage=no])

AC_ARG_ENABLE(tests,
    AS_HELP_STRING([--enable-tests],[compile tests (default is yes)]),
    [use_tests=$enableval],
    [use_tests=yes])

AC_ARG_ENABLE(openssl_tests,
    AS_HELP_STRING([--enable-openssl-tests],[enable OpenSSL tests, if OpenSSL is available (default is auto)]),
    [enable_openssl_tests=$enableval],
    [enable_openssl_tests=auto])

AC_ARG_ENABLE(experimental,
    AS_HELP_STRING([--enable-experimental],[allow experimental configure options (default is no)]),
    [use_experimental=$enableval],
    [use_experimental=no])

AC_ARG_ENABLE(exhaustive_tests,
    AS_HELP_STRING([--enable-exhaustive-tests],[compile exhaustive tests (default is yes)]),
    [use_exhaustive_tests=$enableval],
    [use_exhaustive_tests=yes])

AC_ARG_ENABLE(endomorphism,
    AS_HELP_STRING([--enable-endomorphism],[enable endomorphism (default is no)]),
    [use_endomorphism=$enableval],
    [use_endomorphism=no])

AC_ARG_ENABLE(ecmult_static_precomputation,
    AS_HELP_STRING([--enable-ecmult-static-precomputation],[enable precomputed ecmult table for signing (default is yes)]),
    [use_ecmult_static_precomputation=$enableval],
    [use_ecmult_static_precomputation=auto])

AC_ARG_ENABLE(module_ecdh,
    AS_HELP_STRING([--enable-module-ecdh],[enable ECDH shared secret computation (experimental)]),
    [enable_module_ecdh=$enableval],
    [enable_module_ecdh=no])

AC_ARG_ENABLE(module_schnorrsig,
    AS_HELP_STRING([--enable-module-schnorrsig],[enable schnorrsig module (experimental)]),
    [enable_module_schnorrsig=$enableval],
    [enable_module_schnorrsig=no])

AC_ARG_ENABLE(module_recovery,
    AS_HELP_STRING([--enable-module-recovery],[enable ECDSA pubkey recovery module (default is no)]),
    [enable_module_recovery=$enableval],
    [enable_module_recovery=no])

AC_ARG_ENABLE(module_extrakeys,
    AS_HELP_STRING([--enable-module-extrakeys],[enable extrakeys module (experimental)]),
    [enable_module_extrakeys=$enableval],
    [enable_module_extrakeys=no])

AC_ARG_ENABLE(module_generator,
    AS_HELP_STRING([--enable-module-generator],[enable NUMS generator module (default is no)]),
    [enable_module_generator=$enableval],
    [enable_module_generator=no])

AC_ARG_ENABLE(module_commitment,
    AS_HELP_STRING([--enable-module-commitment],[enable Pedersen commitments module (default is no)]),
    [enable_module_commitment=$enableval],
    [enable_module_commitment=no])

AC_ARG_ENABLE(module_rangeproof,
    AS_HELP_STRING([--enable-module-rangeproof],[enable zero-knowledge range proofs module (default is no)]),
    [enable_module_rangeproof=$enableval],
    [enable_module_rangeproof=no])

AC_ARG_ENABLE(module_bulletproof,
    AS_HELP_STRING([--enable-module-bulletproof],[enable Pedersen / zero-knowledge bulletproofs module (default is no)]),
    [enable_module_bulletproof=$enableval],
    [enable_module_bulletproof=no])

AC_ARG_ENABLE(module_aggsig,
    AS_HELP_STRING([--enable-module-aggsig],[enable Grin aggsig modules]),
    [enable_module_aggsig=$enableval],
    [enable_module_aggsig=no])

AC_ARG_ENABLE(module_whitelist,
    AS_HELP_STRING([--enable-module-whitelist],[enable key whitelisting module (default is no)]),
    [enable_module_whitelist=$enableval],
    [enable_module_whitelist=no])

AC_ARG_ENABLE(jni,
    AS_HELP_STRING([--enable-jni],[enable libsecp256k1_jni (default is no)]),
    [use_jni=$enableval],
    [use_jni=no])

AC_ARG_ENABLE(module_surjectionproof,
    AS_HELP_STRING([--enable-module-surjectionproof],[enable surjection proof module (default is no)]),
    [enable_module_surjectionproof=$enableval],
    [enable_module_surjectionproof=no])

AC_ARG_WITH([field], [AS_HELP_STRING([--with-field=64bit|32bit|auto],
[Specify Field Implementation. Default is auto])],[req_field=$withval], [req_field=auto])

AC_ARG_WITH([bignum], [AS_HELP_STRING([--with-bignum=gmp|no|auto],
[Specify Bignum Implementation. Default is auto])],[req_bignum=$withval], [req_bignum=auto])

AC_ARG_WITH([scalar], [AS_HELP_STRING([--with-scalar=64bit|32bit|auto],
[Specify scalar implementation. Default is auto])],[req_scalar=$withval], [req_scalar=auto])

AC_ARG_WITH([asm], [AS_HELP_STRING([--with-asm=x86_64|arm|no|auto]
[Specify assembly optimizations to use. Default is auto (experimental: arm)])],[req_asm=$withval], [req_asm=auto])

AC_CHECK_TYPES([__int128])

AC_MSG_CHECKING([for __builtin_expect])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() {__builtin_expect(0,0);}]])],
    [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_EXPECT,1,[Define this symbol if __builtin_expect is available]) ],
    [ AC_MSG_RESULT([no])
    ])

if test x"$enable_coverage" = x"yes"; then
    AC_DEFINE(COVERAGE, 1, [Define this symbol to compile out all VERIFY code])
    CFLAGS="$CFLAGS -O0 --coverage"
    LDFLAGS="--coverage"
else
    CFLAGS="$CFLAGS -O3"
fi

AC_MSG_CHECKING([for __builtin_popcount])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() {__builtin_popcount(0);}]])],
    [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_POPCOUNT,1,[Define this symbol if __builtin_popcount is available]) ],
    [ AC_MSG_RESULT([no])
    ])

AC_MSG_CHECKING([for __builtin_popcountl])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() {__builtin_popcountl(0);}]])],
    [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_POPCOUNTL,1,[Define this symbol if __builtin_popcountl is available]) ],
    [ AC_MSG_RESULT([no])
    ])

AC_MSG_CHECKING([for __builtin_ctzl])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() {__builtin_ctzl(0);}]])],
    [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_CTZL,1,[Define this symbol if __builtin_ctzl is available]) ],
    [ AC_MSG_RESULT([no])
    ])


if test x"$use_ecmult_static_precomputation" != x"no"; then
  save_cross_compiling=$cross_compiling
  cross_compiling=no
  TEMP_CC="$CC"
  CC="$CC_FOR_BUILD"
  AC_MSG_CHECKING([native compiler: ${CC_FOR_BUILD}])
  AC_RUN_IFELSE(
    [AC_LANG_PROGRAM([], [return 0])],
    [working_native_cc=yes],
    [working_native_cc=no],[dnl])
  CC="$TEMP_CC"
  cross_compiling=$save_cross_compiling

  if test x"$working_native_cc" = x"no"; then
    set_precomp=no
    if test x"$use_ecmult_static_precomputation" = x"yes";  then
      AC_MSG_ERROR([${CC_FOR_BUILD} does not produce working binaries. Please set CC_FOR_BUILD])
    else
      AC_MSG_RESULT([${CC_FOR_BUILD} does not produce working binaries. Please set CC_FOR_BUILD])
    fi
  else
    AC_MSG_RESULT([ok])
    set_precomp=yes
  fi
else
  set_precomp=no
fi

AC_MSG_CHECKING([for  __builtin_clzll])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() { __builtin_clzll(1);}]])],
    [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_CLZLL,1,[Define this symbol if  __builtin_clzll is available]) ],
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
  arm)
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
use_external_asm=no

case $set_asm in
x86_64)
  AC_DEFINE(USE_ASM_X86_64, 1, [Define this symbol to enable x86_64 assembly optimizations])
  ;;
arm)
  use_external_asm=yes
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
    if test x"$enable_openssl_tests" != x"no"; then
      AC_DEFINE(ENABLE_OPENSSL_TESTS, 1, [Define this symbol if OpenSSL EC functions are available])
      SECP_TEST_INCLUDES="$SSL_CFLAGS $CRYPTO_CFLAGS"
      SECP_TEST_LIBS="$CRYPTO_LIBS"

      case $host in
      *mingw*)
        SECP_TEST_LIBS="$SECP_TEST_LIBS -lgdi32"
        ;;
      esac
    fi
  else
    if test x"$enable_openssl_tests" = x"yes"; then
      AC_MSG_ERROR([OpenSSL tests requested but OpenSSL with EC support is not available])
    fi
  fi
else
  if test x"$enable_openssl_tests" = x"yes"; then
    AC_MSG_ERROR([OpenSSL tests requested but tests are not enabled])
  fi
fi

if test x"$use_jni" != x"no"; then
  AX_JNI_INCLUDE_DIR
  have_jni_dependencies=yes
  if test x"$enable_module_ecdh" = x"no"; then
    have_jni_dependencies=no
  fi
  if test "x$JNI_INCLUDE_DIRS" = "x"; then
    have_jni_dependencies=no
  fi
  if test "x$have_jni_dependencies" = "xno"; then
    if test x"$use_jni" = x"yes"; then
      AC_MSG_ERROR([jni support explicitly requested but headers/dependencies were not found. Enable ECDH and try again.])
    fi
    AC_MSG_WARN([jni headers/dependencies not found. jni support disabled])
    use_jni=no
  else
    use_jni=yes
    for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS; do
      JNI_INCLUDES="$JNI_INCLUDES -I$JNI_INCLUDE_DIR"
    done
  fi
fi

if test x"$set_bignum" = x"gmp"; then
  SECP_LIBS="$SECP_LIBS $GMP_LIBS"
  SECP_INCLUDES="$SECP_INCLUDES $GMP_CPPFLAGS"
fi

if test x"$use_endomorphism" = x"yes"; then
  AC_DEFINE(USE_ENDOMORPHISM, 1, [Define this symbol to use endomorphism optimization])
fi

if test x"$set_precomp" = x"yes"; then
  AC_DEFINE(USE_ECMULT_STATIC_PRECOMPUTATION, 1, [Define this symbol to use a statically generated ecmult table])
fi

if test x"$enable_module_ecdh" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_ECDH, 1, [Define this symbol to enable the ECDH module])
fi

if test x"$enable_module_schnorrsig" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_SCHNORRSIG, 1, [Define this symbol to enable the schnorrsig module])
fi

if test x"$enable_module_recovery" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_RECOVERY, 1, [Define this symbol to enable the ECDSA pubkey recovery module])
fi

if test x"$enable_module_schnorrsig" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_SCHNORRSIG, 1, [Define this symbol to enable the schnorrsig module])
  enable_module_extrakeys=yes
fi

# Test if extrakeys is set after the schnorrsig module to allow the schnorrsig
# module to set enable_module_extrakeys=yes
if test x"$enable_module_extrakeys" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_EXTRAKEYS, 1, [Define this symbol to enable the extrakeys module])
fi

if test x"$enable_module_generator" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_GENERATOR, 1, [Define this symbol to enable the NUMS generator module])
fi

if test x"$enable_module_commitment" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_COMMITMENT, 1, [Define this symbol to enable the Pedersen commitment module])
fi

if test x"$enable_module_rangeproof" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_RANGEPROOF, 1, [Define this symbol to enable the zero knowledge range proof module])
fi

if test x"$enable_module_bulletproof" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_BULLETPROOF, 1, [Define this symbol to enable the Pedersen / zero knowledge bulletproof module])
fi

if test x"$enable_module_aggsig" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_AGGSIG, 1, [Define this symbol to enable the Grin Aggsig module])
fi

if test x"$enable_module_whitelist" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_WHITELIST, 1, [Define this symbol to enable the key whitelisting module])
fi

if test x"$enable_module_surjectionproof" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_SURJECTIONPROOF, 1, [Define this symbol to enable the surjection proof module])
fi

AC_C_BIGENDIAN()

if test x"$use_external_asm" = x"yes"; then
  AC_DEFINE(USE_EXTERNAL_ASM, 1, [Define this symbol if an external (non-inline) assembly implementation is used])
fi

AC_MSG_NOTICE([Using static precomputation: $set_precomp])
AC_MSG_NOTICE([Using assembly optimizations: $set_asm])
AC_MSG_NOTICE([Using field implementation: $set_field])
AC_MSG_NOTICE([Using bignum implementation: $set_bignum])
AC_MSG_NOTICE([Using scalar implementation: $set_scalar])
AC_MSG_NOTICE([Using endomorphism optimizations: $use_endomorphism])
AC_MSG_NOTICE([Building benchmarks: $use_benchmark])
AC_MSG_NOTICE([Building for coverage analysis: $enable_coverage])
AC_MSG_NOTICE([Building ECDH module: $enable_module_ecdh])
AC_MSG_NOTICE([Building ECDSA pubkey recovery module: $enable_module_recovery])
AC_MSG_NOTICE([Using jni: $use_jni])

if test x"$enable_experimental" = x"yes"; then
  AC_MSG_NOTICE([******])
  AC_MSG_NOTICE([WARNING: experimental build])
  AC_MSG_NOTICE([Experimental features do not have stable APIs or properties, and may not be safe for production use.])
  AC_MSG_NOTICE([Building ECDH module: $enable_module_ecdh])
  AC_MSG_NOTICE([Building NUMS generator module: $enable_module_generator])
  AC_MSG_NOTICE([Building Pedersen commitment module: $enable_module_commitment])
  AC_MSG_NOTICE([Building range proof module: $enable_module_rangeproof])
  AC_MSG_NOTICE([Building bulletproof module: $enable_module_bulletproof])
  AC_MSG_NOTICE([Building aggsig module: $enable_module_aggsig])
  AC_MSG_NOTICE([Building key whitelisting module: $enable_module_whitelist])
  AC_MSG_NOTICE([Building surjection proof module: $enable_module_surjectionproof])
  AC_MSG_NOTICE([Building schnorrsig module: $enable_module_schnorrsig])
  AC_MSG_NOTICE([******])

  if test x"$enable_module_generator" != x"yes"; then
    if test x"$enable_module_commitment" = x"yes"; then
      AC_MSG_ERROR([Commitment module requires the generator module. Use --enable-module-generator to allow.])
    fi
    if test x"$enable_module_bulletproof" = x"yes"; then
      AC_MSG_ERROR([Bulletproof module requires the generator module. Use --enable-module-generator to allow.])
    fi
  fi

  if test x"$enable_module_commitment" != x"yes"; then
    if test x"$enable_module_rangeproof" = x"yes"; then
      AC_MSG_ERROR([Rangeproof module requires the commitment module. Use --enable-module-commitment to allow.])
    fi
    if test x"$enable_module_bulletproof" = x"yes"; then
      AC_MSG_ERROR([Bulletproof module requires the commitment module. Use --enable-module-commitment to allow.])
    fi
  fi

  if test x"$enable_module_rangeproof" != x"yes"; then
    if test x"$enable_module_whitelist" = x"yes"; then
      AC_MSG_ERROR([Whitelist module requires the rangeproof module. Use --enable-module-rangeproof to allow.])
    fi
    if test x"$enable_module_surjectionproof" = x"yes"; then
      AC_MSG_ERROR([Surjection proof module requires the rangeproof module. Use --enable-module-rangeproof to allow.])
    fi
  fi
else
  if test x"$enable_module_extrakeys" = x"yes"; then
    AC_MSG_ERROR([extrakeys module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_ecdh" = x"yes"; then
    AC_MSG_ERROR([ECDH module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_schnorrsig" = x"yes"; then
    AC_MSG_ERROR([schnorrsig module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$set_asm" = x"arm"; then
    AC_MSG_ERROR([ARM assembly optimization is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_generator" = x"yes"; then
    AC_MSG_ERROR([NUMS generator module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_commitment" = x"yes"; then
    AC_MSG_ERROR([Pedersen commitment module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_rangeproof" = x"yes"; then
    AC_MSG_ERROR([Range proof module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_bulletproof" = x"yes"; then
    AC_MSG_ERROR([Bulletproof module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_agggsig" = x"yes"; then
    AC_MSG_ERROR([Aggsig module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_whitelist" = x"yes"; then
    AC_MSG_ERROR([Key whitelisting module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_surjectionproof" = x"yes"; then
    AC_MSG_ERROR([Surjection proof module is experimental. Use --enable-experimental to allow.])
  fi
fi

AC_CONFIG_HEADERS([src/libsecp256k1-config.h])
AC_CONFIG_FILES([Makefile libsecp256k1.pc])
AC_SUBST(JNI_INCLUDES)
AC_SUBST(SECP_INCLUDES)
AC_SUBST(SECP_LIBS)
AC_SUBST(SECP_TEST_LIBS)
AC_SUBST(SECP_TEST_INCLUDES)
AM_CONDITIONAL([ENABLE_COVERAGE], [test x"$enable_coverage" = x"yes"])
AM_CONDITIONAL([USE_TESTS], [test x"$use_tests" != x"no"])
AM_CONDITIONAL([USE_EXHAUSTIVE_TESTS], [test x"$use_exhaustive_tests" != x"no"])
AM_CONDITIONAL([USE_BENCHMARK], [test x"$use_benchmark" = x"yes"])
AM_CONDITIONAL([USE_ECMULT_STATIC_PRECOMPUTATION], [test x"$set_precomp" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_ECDH], [test x"$enable_module_ecdh" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_SCHNORRSIG], [test x"$enable_module_schnorrsig" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_RECOVERY], [test x"$enable_module_recovery" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_EXTRAKEYS], [test x"$enable_module_extrakeys" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_GENERATOR], [test x"$enable_module_generator" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_COMMITMENT], [test x"$enable_module_commitment" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_RANGEPROOF], [test x"$enable_module_rangeproof" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_BULLETPROOF], [test x"$enable_module_bulletproof" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_AGGSIG], [test x"$enable_module_aggsig" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_WHITELIST], [test x"$enable_module_whitelist" = x"yes"])
AM_CONDITIONAL([USE_JNI], [test x"$use_jni" == x"yes"])
AM_CONDITIONAL([USE_EXTERNAL_ASM], [test x"$use_external_asm" = x"yes"])
AM_CONDITIONAL([USE_ASM_ARM], [test x"$set_asm" = x"arm"])
AM_CONDITIONAL([ENABLE_MODULE_SURJECTIONPROOF], [test x"$enable_module_surjectionproof" = x"yes"])

dnl make sure nothing new is exported so that we don't break the cache
PKGCONFIG_PATH_TEMP="$PKG_CONFIG_PATH"
unset PKG_CONFIG_PATH
PKG_CONFIG_PATH="$PKGCONFIG_PATH_TEMP"

AC_OUTPUT
