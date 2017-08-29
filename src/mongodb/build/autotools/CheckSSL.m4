AC_MSG_CHECKING([whether to enable crypto and TLS])
AC_ARG_ENABLE([ssl],
              [AS_HELP_STRING([--enable-ssl=@<:@auto/no/openssl/libressl/darwin@:>@],
                              [Enable TLS connections and SCRAM-SHA-1 authentication.])],
              [],
              [enable_ssl=auto])
AC_MSG_RESULT([$enable_ssl])

AC_MSG_CHECKING([whether to use system crypto profile])
AC_ARG_ENABLE(crypto-system-profile,
    AC_HELP_STRING([--enable-crypto-system-profile], [use system crypto profile (OpenSSL only) [default=no]]),
    [],
    [enable_crypto_system_profile="no"])
AC_MSG_RESULT([$enable_crypto_system_profile])

AS_IF([test "$enable_ssl" != "no"],[
   AS_IF([test "$enable_ssl" != "darwin" -a "$enable_ssl" != "libressl"],[
      PKG_CHECK_MODULES(SSL, [openssl], [enable_openssl=auto], [
         AC_CHECK_LIB([ssl],[SSL_library_init],[have_ssl_lib=yes],[have_ssl_lib=no])
         AC_CHECK_LIB([crypto],[EVP_DigestInit_ex],[have_crypto_lib=yes],[have_crypto_lib=no])

         if test "$have_ssl_lib" = "no" -o "$have_crypto_lib" = "no" ; then
            if test "$enable_ssl" = "openssl"; then
               AC_MSG_ERROR([You must install the OpenSSL libraries and development headers to enable OpenSSL support.])
            else
               AC_MSG_WARN([You must install the OpenSSL libraries and development headers to enable OpenSSL support.])
            fi
         fi

         if test "$have_ssl_lib" = "yes" -a "$have_crypto_lib" = "yes"; then
            SSL_LIBS="-lssl -lcrypto"
            enable_ssl=openssl
         fi
      ])
   ])
   AS_IF([test "$enable_ssl" = "libressl"],[
      PKG_CHECK_MODULES(SSL, [libtls], [enable_ssl=libressl], [
         AC_CHECK_LIB([tls],[tls_init],[
            SSL_LIBS="-ltls -lcrypto"
            enable_ssl=libressl
         ])
      ])
   ])

   dnl PKG_CHECK_MODULES() doesn't check for headers
   dnl OSX for example has the lib, but not headers, so double confirm if OpenSSL works
   AS_IF([test "$enable_ssl" = "openssl" -o "$enable_openssl" = "auto"], [
      old_CFLAGS=$CFLAGS
      CFLAGS=$SSL_CFLAGS
      AC_CHECK_HEADERS([openssl/bio.h openssl/ssl.h openssl/err.h openssl/crypto.h],
         [have_ssl_headers=yes],
         [have_ssl_headers=no])
      if test "$have_ssl_headers" = "yes"; then
         enable_ssl=openssl
      elif test "$enable_ssl" = "openssl"; then
         AC_MSG_ERROR([You must install the OpenSSL development headers to enable OpenSSL support.])
      else
         SSL_LIBS=""
         enable_ssl=auto
      fi
      AC_CHECK_DECLS([ASN1_STRING_get0_data], [have_ASN1_STRING_get0_data=yes], [have_ASN1_STRING_get0_data=no], [[#include <openssl/asn1.h>]])
      CFLAGS=$old_CFLAGS
   ])
   AS_IF([test "$enable_ssl" != "openssl" -a "$os_darwin" = "yes"],[
      SSL_LIBS="-framework Security -framework CoreFoundation"
      enable_ssl="darwin"
   ])
   dnl If its still auto, its no.
   AS_IF([test "$enable_ssl" = "auto"],[
      enable_ssl="no"
   ])
   AC_MSG_CHECKING([which TLS library to use])
   AC_MSG_RESULT([$enable_ssl])
], [enable_ssl=no])



AC_SUBST(SSL_CFLAGS)
AC_SUBST(SSL_LIBS)


dnl Disable Windows SSL+Crypto
AC_SUBST(MONGOC_ENABLE_SSL_SECURE_CHANNEL, 0)
AC_SUBST(MONGOC_ENABLE_CRYPTO_CNG, 0)

if test "$enable_ssl" = "darwin" -o "$enable_ssl" = "openssl" -o "$enable_ssl" = "libressl"; then
   AC_SUBST(MONGOC_ENABLE_SSL, 1)
   AC_SUBST(MONGOC_ENABLE_CRYPTO, 1)
   if test "$enable_ssl" = "darwin"; then
      AC_SUBST(MONGOC_ENABLE_SSL_OPENSSL, 0)
      AC_SUBST(MONGOC_ENABLE_SSL_LIBRESSL, 0)
      AC_SUBST(MONGOC_ENABLE_SSL_SECURE_TRANSPORT, 1)
      AC_SUBST(MONGOC_ENABLE_CRYPTO_LIBCRYPTO, 0)
      AC_SUBST(MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO, 1)
   elif test "$enable_ssl" = "openssl"; then
      AC_SUBST(MONGOC_ENABLE_SSL_OPENSSL, 1)
      AC_SUBST(MONGOC_ENABLE_SSL_LIBRESSL, 0)
      AC_SUBST(MONGOC_ENABLE_SSL_SECURE_TRANSPORT, 0)
      AC_SUBST(MONGOC_ENABLE_CRYPTO_LIBCRYPTO, 1)
      AC_SUBST(MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO, 0)
   elif test "$enable_ssl" = "libressl"; then
      AC_SUBST(MONGOC_ENABLE_SSL_LIBRESSL, 1)
      AC_SUBST(MONGOC_ENABLE_SSL_OPENSSL, 0)
      AC_SUBST(MONGOC_ENABLE_SSL_SECURE_TRANSPORT, 0)
      AC_SUBST(MONGOC_ENABLE_CRYPTO_LIBCRYPTO, 1)
      AC_SUBST(MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO, 0)
   fi
else
   AC_SUBST(MONGOC_ENABLE_SSL, 0)
   AC_SUBST(MONGOC_ENABLE_SSL_LIBRESSL, 0)
   AC_SUBST(MONGOC_ENABLE_SSL_OPENSSL, 0)
   AC_SUBST(MONGOC_ENABLE_SSL_SECURE_TRANSPORT, 0)
   AC_SUBST(MONGOC_ENABLE_CRYPTO, 0)
   AC_SUBST(MONGOC_ENABLE_CRYPTO_LIBCRYPTO, 0)
   AC_SUBST(MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO, 0)
fi
if test "$have_ASN1_STRING_get0_data" = "yes"; then
   AC_SUBST(MONGOC_HAVE_ASN1_STRING_GET0_DATA, 1)
else
   AC_SUBST(MONGOC_HAVE_ASN1_STRING_GET0_DATA, 0)
fi

if test "x$enable_crypto_system_profile" = xyes; then
   if test "$enable_ssl" = "openssl"; then
      AC_SUBST(MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE, 1)
   else
      AC_MSG_ERROR([--enable-crypto-system-profile only available with OpenSSL.])
   fi
else
    AC_SUBST(MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE, 0)
fi

AM_CONDITIONAL([ENABLE_SSL],                  [test "$enable_ssl" = "darwin" -o "$enable_ssl" = "openssl" -o "$enable_ssl" = "libressl"])
AM_CONDITIONAL([ENABLE_SSL_LIBRESSL],         [test "$enable_ssl" = "libressl"])
AM_CONDITIONAL([ENABLE_SSL_OPENSSL],          [test "$enable_ssl" = "openssl"])
AM_CONDITIONAL([ENABLE_SSL_SECURE_TRANSPORT], [test "$enable_ssl" = "darwin"])
AM_CONDITIONAL([ENABLE_SSL_SECURE_CHANNEL],    false)
AM_CONDITIONAL([ENABLE_CRYPTO],               [test "$enable_ssl" = "darwin" -o "$enable_ssl" = "openssl" -o "$enable_ssl" = "libressl"])
AM_CONDITIONAL([ENABLE_CRYPTO_LIBCRYPTO],     [test "$enable_ssl" = "openssl" -o "$enable_ssl" = "libressl"])
AM_CONDITIONAL([ENABLE_CRYPTO_CNG],            false)
AM_CONDITIONAL([ENABLE_CRYPTO_COMMON_CRYPTO], [test "$enable_ssl" = "darwin"])

