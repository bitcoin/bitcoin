dnl libsecp25k1 helper checks
AC_DEFUN([SECP_INT128_CHECK],[
has_int128=$ac_cv_type___int128
if test x"$has_int128" != x"yes" && test x"$set_field" = x"64bit"; then
  AC_MSG_ERROR([$set_field field support explicitly requested but is not compatible with this host])
fi
if test x"$has_int128" != x"yes" && test x"$set_scalar" = x"64bit"; then
  AC_MSG_ERROR([$set_scalar scalar support explicitly requested but is not compatible with this host])
fi
])

dnl 
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

dnl
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
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <openssl/ec.h>
    #include <openssl/ecdsa.h>
    #include <openssl/obj_mac.h>]],[[
    EC_KEY *eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    ECDSA_sign(0, NULL, 0, NULL, NULL, eckey);
    ECDSA_verify(0, NULL, 0, NULL, 0, eckey);
    EC_KEY_free(eckey);
  ]])],[has_openssl_ec=yes],[has_openssl_ec=no])
  AC_MSG_RESULT([$has_openssl_ec])
fi
])

dnl
AC_DEFUN([SECP_GMP_CHECK],[
if test x"$has_gmp" != x"yes"; then
  CPPFLAGS_TEMP="$CPPFLAGS"
  CPPFLAGS="$GMP_CPPFLAGS $CPPFLAGS"
  LIBS_TEMP="$LIBS"
  LIBS="$GMP_LIBS $LIBS"
  AC_CHECK_HEADER(gmp.h,[AC_CHECK_LIB(gmp, __gmpz_init,[has_gmp=yes; GMP_LIBS="$GMP_LIBS -lgmp"; AC_DEFINE(HAVE_LIBGMP,1,[Define this symbol if libgmp is installed])])])
  CPPFLAGS="$CPPFLAGS_TEMP"
  LIBS="$LIBS_TEMP"
fi
if test x"$set_field" = x"gmp" && test x"$has_gmp" != x"yes"; then
    AC_MSG_ERROR([$set_field field support explicitly requested but libgmp was not found])
fi
if test x"$set_bignum" = x"gmp" && test x"$has_gmp" != x"yes"; then
    AC_MSG_ERROR([$set_bignum field support explicitly requested but libgmp was not found])
fi
])

