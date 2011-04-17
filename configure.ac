dnl ==============================================================
dnl Process this file with autoconf to produce a configure script.
dnl ==============================================================

dnl require autoconf 2.60 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.60])

AC_INIT([Bitcoin],[0.4.0],[info@bitcoin.org],[bitcoin])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST

dnl backwards compatibility for autoconf >= 2.64
dnl PACKAGE_URL should be the fifth argument of AC_INIT
m4_define([AC_PACKAGE_URL], [http://bitcoin.org])
AC_DEFINE(PACKAGE_URL, "AC_PACKAGE_URL", [Package URL])
AC_SUBST(PACKAGE_URL, AC_PACKAGE_URL)


dnl ==============================================================
dnl Get the operating system and version number...
dnl ==============================================================
AC_MSG_CHECKING([for which platform we are compiling])
case "$host_cpu" in
  powerpc*)
    have_ppc=yes
  ;;
  x86_64*)
    have_x86_64=yes
    AC_DEFINE(HAVE_X86_64BIT,1,[define if host is an x86 64 bit])
  ;;
  i*86*)
    have_x86_32=yes
    AC_DEFINE(ARCH_X86_32BIT,1,[define if host is an x86 32 bit])
  ;;
esac

# TODO: until big endian is not supported this message should appear for
#       all BE platforms, not just ppc.
if test x$have_ppc = xyes; then
       AC_MSG_ERROR([[
[!] Your system architecture (Big Endian, PPC) is not supported
[!] if you are interested in porting Bitcoin to your architecture
[!] you are very welcome to contact us on http://bitcoin.org
]], 0)
fi

HOST_LIBS=""
HOST_CPPFLAGS=""

case "$host_os" in
  *linux*)
    have_linux=yes
    AC_MSG_RESULT([Linux])
    AC_DEFINE(HAVE_LINUX,1,[define if compiling for GNU/Linux])
    HOST_LIBS="-ldl"
  ;;

  *darwin*)
    have_osx=yes
    AC_MSG_RESULT([Apple/OSX])
    AC_DEFINE(HAVE_MACOSX,1,[define if compiling for Apple OSX])
    AC_DEFINE(BSD,1,[define if compiling on a BSD operating system])
    AC_DEFINE(CRYPTOPP_DISABLE_ASM,1,[define to deactivate ASM optimization in cryptopp lib])
    HOST_CPPFLAGS="-mmacosx-version-min=10.5 -arch i386 -arch x86_64"
    # other defines present in old mkfile: -DNOPCH -DMSG_NOSIGNAL=0
  ;;

  *freebsd*)
    have_freebsd=yes
    AC_MSG_RESULT([FreeBSD])
    AC_DEFINE(BSD,1,[define if compiling on a BSD operating system])
    AC_DEFINE(HAVE_FREEBSD,1,[define if compiling for FreeBSD])
    AC_DEFINE(HAVE_SYS_SOCKET_H,1,[usually needed for FreeBSD includes])
    HOST_CPPFLAGS="-I/usr/local/include -L/usr/local/lib"
  ;;

  *win*)
    AC_MSG_RESULT([MS/Windows (TODO)])
    # TODO
  ;;

  *)
    AC_MSG_RESULT([$host_os?!])
    AC_MSG_ERROR([[
[!] Your system architecture is not supported by FreeJ
[!] if you are interested in porting FreeJ to your architecture
[!] you are very welcome to contact me <jaromil@dyne.org>
]], 0)
  ;;
esac

AC_SUBST(HOST_LIBS)
AC_SUBST(HOST_CPPFLAGS)

dnl ==============================================================
dnl Setup for automake
dnl ==============================================================

AM_INIT_AUTOMAKE([dist-bzip2 subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AS

dnl Libtool
AC_PROG_LIBTOOL
dnl versioning info
BITCOIN_CURRENT=0
BITCOIN_REVISION=3
BITCOIN_AGE=20
BITCOIN_VERSION_INFO="$BITCOIN_CURRENT:$BITCOIN_REVISION:$BITCOIN_AGE"
dnl this is the ABI version, not the library version
AC_SUBST(BITCOIN_VERSION_INFO)

AC_CHECK_HEADERS([stdio.h stdlib.h unistd.h strings.h sys/types.h sys/stat.h])

dnl Advanced network address translating functions
AC_CHECK_FUNCS(inet_ntop)

dnl Checks for libraries
AC_CHECK_LIB(crypto, MD5_Init, CRYPTO_LIBS=-lcrypto)
AX_BOOST_BASE
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_THREAD
BOOST_LIBS="$BOOST_LDFLAGS $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_PROGRAM_OPTIONS_LIB $BOOST_THREAD_LIB"
AC_SUBST(BOOST_LIBS)

PKG_CHECK_MODULES(SSL, libssl >= 0.9, :,
		  AC_MSG_ERROR([ SSL development library not found]))
AC_DEFINE(USE_SSL,1,[Define if building support for SSL using libssl])

PKG_CHECK_MODULES(CRYPTO, libcrypto, :,
		  AC_MSG_ERROR([ Crypto development library not found]))

PKG_CHECK_MODULES(GTHREAD2, gthread-2.0, :,
		  AC_MSG_ERROR([ GThread-2 development library not found]))

dnl TODO: make sure of those on all platforms
COMMON_LIBS="-lpthread -lz -ldl -ldb_cxx"
AC_SUBST(COMMON_LIBS)

dnl ==============================================================
dnl enable UPNP
dnl ==============================================================
AC_ARG_ENABLE(upnp,
	AS_HELP_STRING([--enable-upnp],[compile with UPNP support linking to miniupnp]),
	[have_upnp=$enableval],
	[have_upnp=no])
AC_MSG_CHECKING([if UPNP support (miniupnp) is enabled])
UPNP_LIBS=""
if test x$have_upnp = xyes; then
   AC_MSG_RESULT(yes)
   AC_DEFINE(USE_UPNP,1,[Define if building support for UPNP functions via miniupnp])
   if test x$have_linux = xyes; then
      UPNP_LIBS="-lminiupnpc"
   fi
   # TODO OSX and WIN
else
   AC_MSG_RESULT(no)
fi
AC_SUBST(UPNP_LIBS)


dnl ==============================================================
dnl compile with full warnings and debugging symbols
dnl ==============================================================
AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug],[compile with debug symbols (no)]),
    [enable_debug=$enableval],
    [enable_debug=no])

if test x$enable_debug = xyes; then
    GLOBAL_CFLAGS="$GLOBAL_CFLAGS -Wall -g -ggdb"
fi


AC_CONFIG_FILES([
Makefile
src/Makefile
src/cryptopp/Makefile
])
AC_OUTPUT


