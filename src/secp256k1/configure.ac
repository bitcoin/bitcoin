AC_PREREQ([2.60])
AC_INIT([libsecp256k1],[0.1])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AC_CANONICAL_HOST
AH_TOP([#ifndef LIBSECP256K1_CONFIG_H])
AH_TOP([#define LIBSECP256K1_CONFIG_H])
AH_BOTTOM([#endif /*LIBSECP256K1_CONFIG_H*/])
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Set -g if CFLAGS are not already set, which matches the default autoconf
# behavior (see PROG_CC in the Autoconf manual) with the exception that we don't
# set -O2 here because we set it in any case (see further down).
: ${CFLAGS="-g"}
LT_INIT

# Make the compilation flags quiet unless V=1 is used.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

PKG_PROG_PKG_CONFIG

AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)
AC_PATH_TOOL(STRIP, strip)

# Save definition of AC_PROG_CC because AM_PROG_CC_C_O in automake<=1.13 will
# redefine AC_PROG_CC to exit with an error, which avoids the user calling it
# accidently and screwing up the effect of AM_PROG_CC_C_O. However, we'll need
# AC_PROG_CC later on in AX_PROG_CC_FOR_BUILD, where its usage is fine, and
# we'll carefully make sure not to call AC_PROG_CC anywhere else.
m4_copy([AC_PROG_CC], [saved_AC_PROG_CC])
AM_PROG_CC_C_O
# Restore AC_PROG_CC
m4_rename_force([saved_AC_PROG_CC], [AC_PROG_CC])

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
         # These Homebrew packages may be keg-only, meaning that they won't be found
         # in expected paths because they may conflict with system files. Ask
         # Homebrew where each one is located, then adjust paths accordingly.
         openssl_prefix=`$BREW --prefix openssl 2>/dev/null`
         valgrind_prefix=`$BREW --prefix valgrind 2>/dev/null`
         if test x$openssl_prefix != x; then
           PKG_CONFIG_PATH="$openssl_prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
           export PKG_CONFIG_PATH
           CRYPTO_CPPFLAGS="-I$openssl_prefix/include"
         fi
         if test x$valgrind_prefix != x; then
           VALGRIND_CPPFLAGS="-I$valgrind_prefix/include"
         fi
       else
         AC_PATH_PROG([PORT],port,)
         # If homebrew isn't installed and macports is, add the macports default paths
         # as a last resort.
         if test x$PORT != x; then
           CPPFLAGS="$CPPFLAGS -isystem /opt/local/include"
           LDFLAGS="$LDFLAGS -L/opt/local/lib"
         fi
       fi
     fi
   ;;
esac

CFLAGS="-W $CFLAGS"

warn_CFLAGS="-std=c89 -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wundef -Wno-unused-function -Wno-long-long -Wno-overlength-strings"
saved_CFLAGS="$CFLAGS"
CFLAGS="$warn_CFLAGS $CFLAGS"
AC_MSG_CHECKING([if ${CC} supports ${warn_CFLAGS}])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

saved_CFLAGS="$CFLAGS"
CFLAGS="-Wconditional-uninitialized $CFLAGS"
AC_MSG_CHECKING([if ${CC} supports -Wconditional-uninitialized])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

saved_CFLAGS="$CFLAGS"
CFLAGS="-fvisibility=hidden $CFLAGS"
AC_MSG_CHECKING([if ${CC} supports -fvisibility=hidden])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

###
### Define config arguments
###

AC_ARG_ENABLE(benchmark,
    AS_HELP_STRING([--enable-benchmark],[compile benchmark [default=yes]]),
    [use_benchmark=$enableval],
    [use_benchmark=yes])

AC_ARG_ENABLE(coverage,
    AS_HELP_STRING([--enable-coverage],[enable compiler flags to support kcov coverage analysis [default=no]]),
    [enable_coverage=$enableval],
    [enable_coverage=no])

AC_ARG_ENABLE(tests,
    AS_HELP_STRING([--enable-tests],[compile tests [default=yes]]),
    [use_tests=$enableval],
    [use_tests=yes])

AC_ARG_ENABLE(openssl_tests,
    AS_HELP_STRING([--enable-openssl-tests],[enable OpenSSL tests [default=auto]]),
    [enable_openssl_tests=$enableval],
    [enable_openssl_tests=auto])

AC_ARG_ENABLE(experimental,
    AS_HELP_STRING([--enable-experimental],[allow experimental configure options [default=no]]),
    [use_experimental=$enableval],
    [use_experimental=no])

AC_ARG_ENABLE(exhaustive_tests,
    AS_HELP_STRING([--enable-exhaustive-tests],[compile exhaustive tests [default=yes]]),
    [use_exhaustive_tests=$enableval],
    [use_exhaustive_tests=yes])

AC_ARG_ENABLE(ecmult_static_precomputation,
    AS_HELP_STRING([--enable-ecmult-static-precomputation],[enable precomputed ecmult table for signing [default=auto]]),
    [use_ecmult_static_precomputation=$enableval],
    [use_ecmult_static_precomputation=auto])

AC_ARG_ENABLE(module_ecdh,
    AS_HELP_STRING([--enable-module-ecdh],[enable ECDH shared secret computation]),
    [enable_module_ecdh=$enableval],
    [enable_module_ecdh=no])

AC_ARG_ENABLE(module_recovery,
    AS_HELP_STRING([--enable-module-recovery],[enable ECDSA pubkey recovery module [default=no]]),
    [enable_module_recovery=$enableval],
    [enable_module_recovery=no])

AC_ARG_ENABLE(module_extrakeys,
    AS_HELP_STRING([--enable-module-extrakeys],[enable extrakeys module (experimental)]),
    [enable_module_extrakeys=$enableval],
    [enable_module_extrakeys=no])

AC_ARG_ENABLE(module_schnorrsig,
    AS_HELP_STRING([--enable-module-schnorrsig],[enable schnorrsig module (experimental)]),
    [enable_module_schnorrsig=$enableval],
    [enable_module_schnorrsig=no])

AC_ARG_ENABLE(external_default_callbacks,
    AS_HELP_STRING([--enable-external-default-callbacks],[enable external default callback functions [default=no]]),
    [use_external_default_callbacks=$enableval],
    [use_external_default_callbacks=no])

# Test-only override of the (autodetected by the C code) "widemul" setting.
# Legal values are int64 (for [u]int64_t), int128 (for [unsigned] __int128), and auto (the default).
AC_ARG_WITH([test-override-wide-multiply], [] ,[set_widemul=$withval], [set_widemul=auto])

AC_ARG_WITH([asm], [AS_HELP_STRING([--with-asm=x86_64|arm|no|auto],
[assembly optimizations to use (experimental: arm) [default=auto]])],[req_asm=$withval], [req_asm=auto])

AC_ARG_WITH([ecmult-window], [AS_HELP_STRING([--with-ecmult-window=SIZE|auto],
[window size for ecmult precomputation for verification, specified as integer in range [2..24].]
[Larger values result in possibly better performance at the cost of an exponentially larger precomputed table.]
[The table will store 2^(SIZE-1) * 64 bytes of data but can be larger in memory due to platform-specific padding and alignment.]
["auto" is a reasonable setting for desktop machines (currently 15). [default=auto]]
)],
[req_ecmult_window=$withval], [req_ecmult_window=auto])

AC_ARG_WITH([ecmult-gen-precision], [AS_HELP_STRING([--with-ecmult-gen-precision=2|4|8|auto],
[Precision bits to tune the precomputed table size for signing.]
[The size of the table is 32kB for 2 bits, 64kB for 4 bits, 512kB for 8 bits of precision.]
[A larger table size usually results in possible faster signing.]
["auto" is a reasonable setting for desktop machines (currently 4). [default=auto]]
)],
[req_ecmult_gen_precision=$withval], [req_ecmult_gen_precision=auto])

AC_ARG_WITH([valgrind], [AS_HELP_STRING([--with-valgrind=yes|no|auto],
[Build with extra checks for running inside Valgrind [default=auto]]
)],
[req_valgrind=$withval], [req_valgrind=auto])

###
### Handle config options (except for modules)
###

if test x"$req_valgrind" = x"no"; then
  enable_valgrind=no
else
  SECP_VALGRIND_CHECK
  if test x"$has_valgrind" != x"yes"; then
    if test x"$req_valgrind" = x"yes"; then
      AC_MSG_ERROR([Valgrind support explicitly requested but valgrind/memcheck.h header not available])
    fi
    enable_valgrind=no
  else
    enable_valgrind=yes
  fi
fi
AM_CONDITIONAL([VALGRIND_ENABLED],[test "$enable_valgrind" = "yes"])

if test x"$enable_coverage" = x"yes"; then
    AC_DEFINE(COVERAGE, 1, [Define this symbol to compile out all VERIFY code])
    CFLAGS="-O0 --coverage $CFLAGS"
    LDFLAGS="--coverage $LDFLAGS"
else
    CFLAGS="-O2 $CFLAGS"
fi

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

# Select assembly optimization
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

if test x"$use_external_asm" = x"yes"; then
  AC_DEFINE(USE_EXTERNAL_ASM, 1, [Define this symbol if an external (non-inline) assembly implementation is used])
fi


# Select wide multiplication implementation
case $set_widemul in
int128)
  AC_DEFINE(USE_FORCE_WIDEMUL_INT128, 1, [Define this symbol to force the use of the (unsigned) __int128 based wide multiplication implementation])
  ;;
int64)
  AC_DEFINE(USE_FORCE_WIDEMUL_INT64, 1, [Define this symbol to force the use of the (u)int64_t based wide multiplication implementation])
  ;;
auto)
  ;;
*)
  AC_MSG_ERROR([invalid wide multiplication implementation])
  ;;
esac

# Set ecmult window size
if test x"$req_ecmult_window" = x"auto"; then
  set_ecmult_window=15
else
  set_ecmult_window=$req_ecmult_window
fi

error_window_size=['window size for ecmult precomputation not an integer in range [2..24] or "auto"']
case $set_ecmult_window in
''|*[[!0-9]]*)
  # no valid integer
  AC_MSG_ERROR($error_window_size)
  ;;
*)
  if test "$set_ecmult_window" -lt 2 -o "$set_ecmult_window" -gt 24 ; then
    # not in range
    AC_MSG_ERROR($error_window_size)
  fi
  AC_DEFINE_UNQUOTED(ECMULT_WINDOW_SIZE, $set_ecmult_window, [Set window size for ecmult precomputation])
  ;;
esac

# Set ecmult gen precision
if test x"$req_ecmult_gen_precision" = x"auto"; then
  set_ecmult_gen_precision=4
else
  set_ecmult_gen_precision=$req_ecmult_gen_precision
fi

case $set_ecmult_gen_precision in
2|4|8)
  AC_DEFINE_UNQUOTED(ECMULT_GEN_PREC_BITS, $set_ecmult_gen_precision, [Set ecmult gen precision bits])
  ;;
*)
  AC_MSG_ERROR(['ecmult gen precision not 2, 4, 8 or "auto"'])
  ;;
esac

if test x"$use_tests" = x"yes"; then
  SECP_OPENSSL_CHECK
  if test x"$enable_openssl_tests" != x"no" && test x"$has_openssl_ec" = x"yes"; then
      enable_openssl_tests=yes
      AC_DEFINE(ENABLE_OPENSSL_TESTS, 1, [Define this symbol if OpenSSL EC functions are available])
      SECP_TEST_INCLUDES="$SSL_CFLAGS $CRYPTO_CFLAGS $CRYPTO_CPPFLAGS"
      SECP_TEST_LIBS="$CRYPTO_LIBS"

      case $host in
      *mingw*)
        SECP_TEST_LIBS="$SECP_TEST_LIBS -lgdi32"
        ;;
      esac
  else
    if test x"$enable_openssl_tests" = x"yes"; then
      AC_MSG_ERROR([OpenSSL tests requested but OpenSSL with EC support is not available])
    fi
    enable_openssl_tests=no
  fi
else
  if test x"$enable_openssl_tests" = x"yes"; then
    AC_MSG_ERROR([OpenSSL tests requested but tests are not enabled])
  fi
  enable_openssl_tests=no
fi

if test x"$enable_valgrind" = x"yes"; then
  SECP_INCLUDES="$SECP_INCLUDES $VALGRIND_CPPFLAGS"
fi

# Handle static precomputation (after everything which modifies CFLAGS and friends)
if test x"$use_ecmult_static_precomputation" != x"no"; then
  if test x"$cross_compiling" = x"no"; then
    set_precomp=yes
    if test x"${CC_FOR_BUILD+x}${CFLAGS_FOR_BUILD+x}${CPPFLAGS_FOR_BUILD+x}${LDFLAGS_FOR_BUILD+x}" != x; then
      AC_MSG_WARN([CC_FOR_BUILD, CFLAGS_FOR_BUILD, CPPFLAGS_FOR_BUILD, and/or LDFLAGS_FOR_BUILD is set but ignored because we are not cross-compiling.])
    fi
    # If we're not cross-compiling, simply use the same compiler for building the static precompation code.
    CC_FOR_BUILD="$CC"
    CFLAGS_FOR_BUILD="$CFLAGS"
    CPPFLAGS_FOR_BUILD="$CPPFLAGS"
    LDFLAGS_FOR_BUILD="$LDFLAGS"
  else
    AX_PROG_CC_FOR_BUILD

    # Temporarily switch to an environment for the native compiler
    save_cross_compiling=$cross_compiling
    cross_compiling=no
    SAVE_CC="$CC"
    CC="$CC_FOR_BUILD"
    SAVE_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS_FOR_BUILD"
    SAVE_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS_FOR_BUILD"
    SAVE_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS_FOR_BUILD"

    warn_CFLAGS_FOR_BUILD="-Wall -Wextra -Wno-unused-function"
    saved_CFLAGS="$CFLAGS"
    CFLAGS="$warn_CFLAGS_FOR_BUILD $CFLAGS"
    AC_MSG_CHECKING([if native ${CC_FOR_BUILD} supports ${warn_CFLAGS_FOR_BUILD}])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
        [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_RESULT([no])
          CFLAGS="$saved_CFLAGS"
        ])

    AC_MSG_CHECKING([for working native compiler: ${CC_FOR_BUILD}])
    AC_RUN_IFELSE(
      [AC_LANG_PROGRAM([], [])],
      [working_native_cc=yes],
      [working_native_cc=no],[:])

    CFLAGS_FOR_BUILD="$CFLAGS"

    # Restore the environment
    cross_compiling=$save_cross_compiling
    CC="$SAVE_CC"
    CFLAGS="$SAVE_CFLAGS"
    CPPFLAGS="$SAVE_CPPFLAGS"
    LDFLAGS="$SAVE_LDFLAGS"

    if test x"$working_native_cc" = x"no"; then
      AC_MSG_RESULT([no])
      set_precomp=no
      m4_define([please_set_for_build], [Please set CC_FOR_BUILD, CFLAGS_FOR_BUILD, CPPFLAGS_FOR_BUILD, and/or LDFLAGS_FOR_BUILD.])
      if test x"$use_ecmult_static_precomputation" = x"yes";  then
        AC_MSG_ERROR([native compiler ${CC_FOR_BUILD} does not produce working binaries. please_set_for_build])
      else
        AC_MSG_WARN([Disabling statically generated ecmult table because the native compiler ${CC_FOR_BUILD} does not produce working binaries. please_set_for_build])
      fi
    else
      AC_MSG_RESULT([yes])
      set_precomp=yes
    fi
  fi

  AC_SUBST(CC_FOR_BUILD)
  AC_SUBST(CFLAGS_FOR_BUILD)
  AC_SUBST(CPPFLAGS_FOR_BUILD)
  AC_SUBST(LDFLAGS_FOR_BUILD)
else
  set_precomp=no
fi

if test x"$set_precomp" = x"yes"; then
  AC_DEFINE(USE_ECMULT_STATIC_PRECOMPUTATION, 1, [Define this symbol to use a statically generated ecmult table])
fi

###
### Handle module options
###

if test x"$enable_module_ecdh" = x"yes"; then
  AC_DEFINE(ENABLE_MODULE_ECDH, 1, [Define this symbol to enable the ECDH module])
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

if test x"$use_external_default_callbacks" = x"yes"; then
  AC_DEFINE(USE_EXTERNAL_DEFAULT_CALLBACKS, 1, [Define this symbol if an external implementation of the default callbacks is used])
fi

###
### Check for --enable-experimental if necessary
###

if test x"$enable_experimental" = x"yes"; then
  AC_MSG_NOTICE([******])
  AC_MSG_NOTICE([WARNING: experimental build])
  AC_MSG_NOTICE([Experimental features do not have stable APIs or properties, and may not be safe for production use.])
  AC_MSG_NOTICE([Building extrakeys module: $enable_module_extrakeys])
  AC_MSG_NOTICE([Building schnorrsig module: $enable_module_schnorrsig])
  AC_MSG_NOTICE([******])
else
  if test x"$enable_module_extrakeys" = x"yes"; then
    AC_MSG_ERROR([extrakeys module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$enable_module_schnorrsig" = x"yes"; then
    AC_MSG_ERROR([schnorrsig module is experimental. Use --enable-experimental to allow.])
  fi
  if test x"$set_asm" = x"arm"; then
    AC_MSG_ERROR([ARM assembly optimization is experimental. Use --enable-experimental to allow.])
  fi
fi

###
### Generate output
###

AC_CONFIG_HEADERS([src/libsecp256k1-config.h])
AC_CONFIG_FILES([Makefile libsecp256k1.pc])
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
AM_CONDITIONAL([ENABLE_MODULE_RECOVERY], [test x"$enable_module_recovery" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_EXTRAKEYS], [test x"$enable_module_extrakeys" = x"yes"])
AM_CONDITIONAL([ENABLE_MODULE_SCHNORRSIG], [test x"$enable_module_schnorrsig" = x"yes"])
AM_CONDITIONAL([USE_EXTERNAL_ASM], [test x"$use_external_asm" = x"yes"])
AM_CONDITIONAL([USE_ASM_ARM], [test x"$set_asm" = x"arm"])

# Make sure nothing new is exported so that we don't break the cache.
PKGCONFIG_PATH_TEMP="$PKG_CONFIG_PATH"
unset PKG_CONFIG_PATH
PKG_CONFIG_PATH="$PKGCONFIG_PATH_TEMP"

AC_OUTPUT

echo
echo "Build Options:"
echo "  with ecmult precomp     = $set_precomp"
echo "  with external callbacks = $use_external_default_callbacks"
echo "  with benchmarks         = $use_benchmark"
echo "  with tests              = $use_tests"
echo "  with openssl tests      = $enable_openssl_tests"
echo "  with coverage           = $enable_coverage"
echo "  module ecdh             = $enable_module_ecdh"
echo "  module recovery         = $enable_module_recovery"
echo "  module extrakeys        = $enable_module_extrakeys"
echo "  module schnorrsig       = $enable_module_schnorrsig"
echo
echo "  asm                     = $set_asm"
echo "  ecmult window size      = $set_ecmult_window"
echo "  ecmult gen prec. bits   = $set_ecmult_gen_precision"
# Hide test-only options unless they're used.
if test x"$set_widemul" != xauto; then
echo "  wide multiplication     = $set_widemul"
fi
echo
echo "  valgrind                = $enable_valgrind"
echo "  CC                      = $CC"
echo "  CFLAGS                  = $CFLAGS"
echo "  CPPFLAGS                = $CPPFLAGS"
echo "  LDFLAGS                 = $LDFLAGS"
echo
if test x"$set_precomp" = x"yes"; then
echo "  CC_FOR_BUILD            = $CC_FOR_BUILD"
echo "  CFLAGS_FOR_BUILD        = $CFLAGS_FOR_BUILD"
echo "  CPPFLAGS_FOR_BUILD      = $CPPFLAGS_FOR_BUILD"
echo "  LDFLAGS_FOR_BUILD       = $LDFLAGS_FOR_BUILD"
fi
