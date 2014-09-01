AC_PREREQ([2.60])
AC_INIT([libsecp256k1],[0.1])
AC_CONFIG_AUX_DIR([src/build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST
AH_TOP([#ifndef LIBSECP256K1_CONFIG_H])
AH_TOP([#define LIBSECP256K1_CONFIG_H])
AH_BOTTOM([#endif //LIBSECP256K1_CONFIG_H])
AM_INIT_AUTOMAKE([foreign])
LT_INIT

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_SED
PKG_PROG_PKG_CONFIG

AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)
AC_PATH_TOOL(STRIP, strip)

AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" == x"no"; then
  AC_MSG_ERROR([c99 compiler support required])
fi

case $host in
  *mingw*)
     use_pkgconfig=no
     ;;
   *)
     use_pkgconfig=yes
     ;;
esac

case $host_os in
  darwin*)
     CPPFLAGS="$CPPFLAGS -I/opt/local/include"
     LDFLAGS="$LDFLAGS -L/opt/local/lib"
     ;;
esac

AC_ARG_ENABLE(benchmark,
    AS_HELP_STRING([--enable-benchmark],[compile benchmark (default is yes)]),
    [use_benchmark=$enableval],
    [use_benchmark=yes])

AC_ARG_ENABLE(tests,
    AS_HELP_STRING([--enable-tests],[compile tests (default is yes)]),
    [use_tests=$enableval],
    [use_tests=yes])

AC_ARG_ENABLE(endomorphism,
    AS_HELP_STRING([--enable-endomorphism],[enable endomorphism (default is no)]),
    [use_endomorphism=$enableval],
    [use_endomorphism=no])

AC_ARG_WITH([field], [AS_HELP_STRING([--with-field=gmp|64bit|64bit_asm|32bit|auto],
[Specify Field Implementation. Default is auto])],[req_field=$withval], [req_field=auto])

AC_ARG_WITH([bignum], [AS_HELP_STRING([--with-bignum=gmp|openssl|auto],
[Specify Bignum Implementation. Default is auto])],[req_bignum=$withval], [req_bignum=auto])

AC_CHECK_TYPES([__int128])

AC_DEFUN([SECP_INT128_CHECK],[
has_int128=$ac_cv_type___int128
if test x"$has_int128" != x"yes" && test x"$set_field" = x"64bit"; then
  AC_MSG_ERROR([$set_field field support explicitly requested but is not compatible with this host])
fi
])

AC_DEFUN([SECP_64BIT_ASM_CHECK],[
if test x"$host_cpu" == x"x86_64"; then
  AC_CHECK_PROG(YASM, yasm, yasm)
else
  if test x"$set_field" = x"64bit_asm"; then
    AC_MSG_ERROR([$set_field field support explicitly requested but is not compatible with this host])
  fi
fi
if test x$YASM = x; then
  if test x"$set_field" = x"64bit_asm"; then
    AC_MSG_ERROR([$set_field field support explicitly requested but yasm was not found])
  fi
  has_64bit_asm=no
else
  case x"$host_os" in
  xdarwin*)
    YASM_BINFMT=macho64
    ;;
  x*-gnux32)
    YASM_BINFMT=elfx32
    ;;
  *)
    YASM_BINFMT=elf64
    ;;
  esac
  if $YASM -f help | grep -q $YASM_BINFMT; then
    has_64bit_asm=yes
  else
    if test x"$set_field" = x"64bit_asm"; then
      AC_MSG_ERROR([$set_field field support explicitly requested but yasm doesn't support $YASM_BINFMT format])
    fi
    AC_MSG_WARN([yasm too old for $YASM_BINFMT format])
    has_64bit_asm=no
  fi
fi
])

AC_DEFUN([SECP_OPENSSL_CHECK],[
if test x"$use_pkgconfig" = x"yes"; then
    : #NOP
  m4_ifdef([PKG_CHECK_MODULES],[
    PKG_CHECK_MODULES([CRYPTO], [libcrypto], [has_libcrypto=yes; AC_DEFINE(HAVE_LIBCRYPTO,1,[Define this symbol if libcrypto is installed])],[has_libcrypto=no])
    : #NOP
  ])
else
  AC_CHECK_HEADER(openssl/crypto.h,[AC_CHECK_LIB(crypto, main,[has_libcrypto=yes; CRYPTO_LIBS=-lcrypto; AC_DEFINE(HAVE_LIBCRYPTO,1,[Define this symbol if libcrypto is installed])]
)])
  LIBS=
fi
if test x"$has_libcrypto" == x"yes" && test x"$has_openssl_ec" = x; then
  AC_MSG_CHECKING(for EC functions in libcrypto)
  AC_TRY_COMPILE([
    #include <openssl/ec.h>
    #include <openssl/ecdsa.h>
    #include <openssl/obj_mac.h>],[
    EC_KEY *eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    ECDSA_sign(0, NULL, 0, NULL, NULL, eckey);
    ECDSA_verify(0, NULL, 0, NULL, 0, eckey);
    EC_KEY_free(eckey);
  ],[has_openssl_ec=yes], [has_openssl_ec=no])
  AC_MSG_RESULT([$has_openssl_ec])
fi
if test x"$set_bignum" = x"openssl"; then
  if test x"$has_libcrypto" != x"yes"; then
    AC_MSG_ERROR([$set_bignum field support explicitly requested but libcrypto was not found])
  fi
fi
])

AC_CHECK_DECL(__builtin_expect,AC_DEFINE(HAVE_BUILTIN_EXPECT,1,[Define this symbol if __builtin_expect is available]),,)

AC_DEFUN([SECP_GMP_CHECK],[
if test x"$has_gmp" != x"yes"; then
  AC_CHECK_HEADER(gmp.h,[AC_CHECK_LIB(gmp, __gmpz_init,[has_gmp=yes; GMP_LIBS=-lgmp; AC_DEFINE(HAVE_LIBGMP,1,[Define this symbol if libgmp is installed])])])
fi
if test x"$set_field" = x"gmp" && test x"$has_gmp" != x"yes"; then
    AC_MSG_ERROR([$set_field field support explicitly requested but libgmp was not found])
fi
if test x"$set_bignum" = x"gmp" && test x"$has_gmp" != x"yes"; then
    AC_MSG_ERROR([$set_bignum field support explicitly requested but libgmp was not found])
fi
])

if test x"$req_field" = x"auto"; then
  SECP_64BIT_ASM_CHECK
  if test x"$has_64bit_asm" = x"yes"; then
    set_field=64bit_asm
  fi

  if test x"$set_field" = x; then
    SECP_INT128_CHECK
    if test x"$has_int128" = x"yes"; then
      set_field=64bit
    fi
  fi

  if test x"$set_field" = x; then
    SECP_GMP_CHECK
    if test x"$has_gmp" = x"yes"; then
      set_field=gmp
    fi
  fi

  if test x"$set_field" = x; then
    set_field=32bit
  fi
else
  set_field=$req_field
  case $set_field in
  64bit_asm)
    SECP_64BIT_ASM_CHECK
    ;;
  64bit)
    SECP_INT128_CHECK
    ;;
  gmp)
    SECP_GMP_CHECK
    ;;
  32bit)
    ;;
  *)
    AC_MSG_ERROR([invalid field implementation selection])
    ;;
  esac
fi

if test x"$req_bignum" = x"auto"; then
  SECP_GMP_CHECK
  if test x"$has_gmp" = x"yes"; then
    set_bignum=gmp
  fi

  if test x"$set_bignum" = x; then
    SECP_OPENSSL_CHECK
    if test x"$has_libcrypto" = x"yes"; then
      set_bignum=openssl
    fi
  fi

  if test x"$set_bignum" = x; then
    AC_MSG_ERROR([no working bignum implementation found])
  fi
else
  set_bignum=$req_bignum
  case $set_bignum in
  gmp)
    SECP_GMP_CHECK
    ;;
  openssl)
    SECP_OPENSSL_CHECK
    ;;
  *)
    AC_MSG_ERROR([invalid bignum implementation selection])
    ;;
  esac
fi

# select field implementation
case $set_field in
64bit_asm)
  AC_DEFINE(USE_FIELD_5X52_ASM, 1, [Define this symbol to use the assembly version for the 5x52 field implementation])
  AC_DEFINE(USE_FIELD_5X52, 1, [Define this symbol to use the FIELD_5X52 implementation])
  ;;
64bit)
  AC_DEFINE(USE_FIELD_5X52_INT128, 1, [Define this symbol to use the __int128 version for the 5x52 field implementation])
  AC_DEFINE(USE_FIELD_5X52, 1, [Define this symbol to use the FIELD_5X52 implementation])
  ;;
gmp)
  AC_DEFINE(HAVE_LIBGMP,1,[Define this symbol if libgmp is installed])
  AC_DEFINE(USE_FIELD_GMP, 1, [Define this symbol to use the FIELD_GMP implementation])
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
  AC_DEFINE(HAVE_LIBGMP,1,[Define this symbol if libgmp is installed])
  AC_DEFINE(USE_NUM_GMP, 1, [Define this symbol to use the gmp implementation])
  AC_DEFINE(USE_FIELD_INV_NUM, 1, [Define this symbol to use the USE_FIELD_INV_NUM implementation])
  ;;
openssl)
  AC_DEFINE(USE_NUM_OPENSSL, 1,[Define this symbol to use the openssl implementation])
  AC_DEFINE(USE_FIELD_INV_BUILTIN, 1, [Define this symbol to use the FIELD_INV_BUILTIN implementation])
  SECP_INCLUDES="$SSL_CFLAGS $CRYPTO_CFLAGS"
  SECP_LIBS="$CRYPTO_LIBS"
  ;;
*)
  AC_MSG_ERROR([invalid bignum implementation])
  ;;
esac

if test x"$use_tests" = x"yes"; then
  SECP_OPENSSL_CHECK
  if test x"$has_openssl_ec" == x"yes"; then
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

if test x"$set_field" = x"gmp" || test x"$set_bignum" = x"gmp"; then
  SECP_LIBS="$SECP_LIBS $GMP_LIBS"
fi

if test x"$use_endomorphism" = x"yes"; then
  AC_DEFINE(USE_ENDOMORPHISM, 1, [Define this symbol to use endomorphism])
fi

AC_MSG_NOTICE([Using field implementation: $set_field])
AC_MSG_NOTICE([Using bignum implementation: $set_bignum])

AC_CONFIG_HEADERS([src/libsecp256k1-config.h])
AC_CONFIG_FILES([Makefile libsecp256k1.pc])
AC_SUBST(SECP_INCLUDES)
AC_SUBST(SECP_LIBS)
AC_SUBST(SECP_TEST_LIBS)
AC_SUBST(SECP_TEST_INCLUDES)
AC_SUBST(YASM_BINFMT)
AM_CONDITIONAL([USE_ASM], [test x"$set_field" == x"64bit_asm"])
AM_CONDITIONAL([USE_TESTS], [test x"$use_tests" != x"no"])
AM_CONDITIONAL([USE_BENCHMARK], [test x"$use_benchmark" != x"no"])
AC_OUTPUT
