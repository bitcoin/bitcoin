dnl require autoconf 2.60 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.60])
define(_CLIENT_VERSION_MAJOR, 0)
define(_CLIENT_VERSION_MINOR, 9)
define(_CLIENT_VERSION_REVISION, 1)
define(_CLIENT_VERSION_BUILD, 0)
define(_CLIENT_VERSION_IS_RELEASE, true)
define(_COPYRIGHT_YEAR, 2014)
AC_INIT([Bitcoin Core],[_CLIENT_VERSION_MAJOR._CLIENT_VERSION_MINOR._CLIENT_VERSION_REVISION],[info@bitcoin.org],[bitcoin])
AC_CONFIG_AUX_DIR([src/build-aux])
AC_CONFIG_MACRO_DIR([src/m4])
AC_CANONICAL_HOST
AH_TOP([#ifndef BITCOIN_CONFIG_H])
AH_TOP([#define BITCOIN_CONFIG_H])
AH_BOTTOM([#endif //BITCOIN_CONFIG_H])

# This m4 will only be used if a system copy cannot be found. This is helpful
# on systems where autotools are installed but the pkg-config macros are not in
# a default location. It is currently used for building on OSX where autotools
# are preinstalled but pkg-config comes from macports or homebrew. It should
# probably be removed when building on <= 10.6 is no longer supported.
m4_include([pkg.m4])

dnl faketime breaks configure and is only needed for make. Disable it here.
unset FAKETIME

if test "x${CXXFLAGS+set}" = "xset"; then
  CXXFLAGS_overridden=yes
else
  CXXFLAGS_overridden=no
fi

dnl ==============================================================
dnl Setup for automake
dnl ==============================================================

AM_INIT_AUTOMAKE([no-define subdir-objects foreign])

dnl faketime messes with timestamps and causes configure to be re-run.
dnl --disable-maintainer-mode can be used to bypass this.
AM_MAINTAINER_MODE([enable])

dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Enable wallet
AC_ARG_ENABLE([wallet],
  [AS_HELP_STRING([--enable-wallet],
  [enable wallet (default is yes)])],
  [enable_wallet=$enableval],
  [enable_wallet=yes])

AC_ARG_WITH([miniupnpc],
  [AS_HELP_STRING([--with-miniupnpc],
  [enable UPNP (default is yes if libminiupnpc is found)])],
  [use_upnp=$withval],
  [use_upnp=auto])

AC_ARG_ENABLE([upnp-default],
  [AS_HELP_STRING([--enable-upnp-default],
  [if UPNP is enabled, turn it on at startup (default is no)])],
  [use_upnp_default=$enableval],
  [use_upnp_default=no])

dnl enable ipv6 support
AC_ARG_ENABLE([ipv6],
  [AS_HELP_STRING([--enable-ipv6],
  [enable ipv6 (default is yes)])],
  [use_ipv6=$enableval],
  [use_ipv6=yes])

AC_ARG_ENABLE(tests,
    AS_HELP_STRING([--enable-tests],[compile tests (default is yes)]),
    [use_tests=$enableval],
    [use_tests=yes])

AC_ARG_WITH([comparison-tool],
    AS_HELP_STRING([--with-comparison-tool],[path to java comparison tool (requires --enable-tests)]),
    [use_comparison_tool=$withval],
    [use_comparison_tool=no])

AC_ARG_ENABLE([comparison-tool-reorg-tests],
    AS_HELP_STRING([--enable-comparison-tool-reorg-tests],[enable expensive reorg tests in the comparison tool (default no)]),
    [use_comparison_tool_reorg_tests=$enableval],
    [use_comparison_tool_reorg_tests=no])

AC_ARG_WITH([qrencode],
  [AS_HELP_STRING([--with-qrencode],
  [enable QR code support (default is yes if qt is enabled and libqrencode is found)])],
  [use_qr=$withval],
  [use_qr=auto])

AC_ARG_ENABLE([hardening],
  [AS_HELP_STRING([--enable-hardening],
  [attempt to harden the resulting executables (default is yes)])],
  [use_hardening=$enableval],
  [use_hardening=yes])

AC_ARG_ENABLE([ccache],
  [AS_HELP_STRING([--enable-ccache],
  [use ccache for building (default is yes if ccache is found)])],
  [use_ccache=$enableval],
  [use_ccache=auto])

AC_ARG_ENABLE([lcov],
  [AS_HELP_STRING([--enable-lcov],
  [enable lcov testing (default is no)])],
  [use_lcov=yes],
  [use_lcov=no])

AC_ARG_WITH([protoc-bindir],[AS_HELP_STRING([--with-protoc-bindir=BIN_DIR],[specify protoc bin path])], [protoc_bin_path=$withval], [])


AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([src/bitcoin-config.h])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_OBJC
m4_ifdef([AC_PROG_OBJCXX],[AC_PROG_OBJCXX])
AC_PROG_MKDIR_P
AC_PROG_SED
AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)
AC_PATH_TOOL(STRIP, strip)
AC_PATH_TOOL(GCOV, gcov)
AC_PATH_PROG(LCOV, lcov)
AC_PATH_PROG(JAVA, java)
AC_PATH_PROG(GENHTML, genhtml)
AC_PATH_PROG([GIT], [git])
AC_PATH_PROG(CCACHE,ccache)
AC_PATH_PROG(XGETTEXT,xgettext)
AC_PATH_PROG(HEXDUMP,hexdump)
PKG_PROG_PKG_CONFIG

# Enable debug 
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [use debug compiler flags and macros (default is no)])],
    [enable_debug=$enableval],
    [enable_debug=no])

if test "x$enable_debug" = xyes; then
    if test "x$GCC" = xyes; then
        CFLAGS="-g3 -O0 -DDEBUG"
    fi
    
    if test "x$GXX" = xyes; then
        CXXFLAGS="-g3 -O0 -DDEBUG"
    fi
fi 

## TODO: Remove these hard-coded paths and flags. They are here for the sake of
##       compatibility with the legacy buildsystem.
##
if test "x$CXXFLAGS_overridden" = "xno"; then
  CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wformat -Wformat-security -Wno-unused-parameter"
fi
CPPFLAGS="$CPPFLAGS -DBOOST_SPIRIT_THREADSAFE -DHAVE_BUILD_INFO -D__STDC_FORMAT_MACROS"

AC_LANG_PUSH([C++])

use_pkgconfig=yes
case $host in
  *mingw*)

     #pkgconfig does more harm than good with MinGW
     use_pkgconfig=no

     TARGET_OS=windows
     AC_CHECK_LIB([mingwthrd],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([kernel32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([user32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([gdi32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([comdlg32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([winspool],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([winmm],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([shell32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([comctl32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([ole32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([oleaut32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([uuid],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([rpcrt4],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([advapi32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([ws2_32],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([mswsock],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([shlwapi],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([iphlpapi],      [main],, AC_MSG_ERROR(lib missing))
     AC_CHECK_LIB([crypt32],      [main],, AC_MSG_ERROR(lib missing))

     AX_CHECK_LINK_FLAG([[-static]],[LDFLAGS="$LDFLAGS -static"])
     AX_CHECK_LINK_FLAG([[-static-libgcc]],[LDFLAGS="$LDFLAGS -static-libgcc"])
     AX_CHECK_LINK_FLAG([[-static-libstdc++]],[LDFLAGS="$LDFLAGS -static-libstdc++"])

     AC_PATH_PROG([MAKENSIS], [makensis], none)
     if test x$MAKENSIS = xnone; then
       AC_MSG_WARN("makensis not found. Cannot create installer.")
     fi

     AC_PATH_TOOL(WINDRES, windres, none)
     if test x$WINDRES = xnone; then
       AC_MSG_ERROR("windres not found")
     fi

     CPPFLAGS="$CPPFLAGS -D_MT -DWIN32 -D_WINDOWS -DBOOST_THREAD_USE_LIB"
     LEVELDB_TARGET_FLAGS="TARGET_OS=OS_WINDOWS_CROSSCOMPILE"
     if test "x$CXXFLAGS_overridden" = "xno"; then
       CXXFLAGS="$CXXFLAGS -w"
     fi
     case $host in
       i?86-*) WINDOWS_BITS=32 ;;
       x86_64-*) WINDOWS_BITS=64 ;;
       *) AC_MSG_ERROR("Could not determine win32/win64 for installer") ;;
     esac
     AC_SUBST(WINDOWS_BITS)
     ;;
  *darwin*)
     TARGET_OS=darwin
     LEVELDB_TARGET_FLAGS="TARGET_OS=Darwin"
     if  test x$cross_compiling != xyes; then
       AC_CHECK_PROG([PORT],port, port)
       if test x$PORT = xport; then
         dnl add default macports paths
         CPPFLAGS="$CPPFLAGS -isystem /opt/local/include -I/opt/local/include/db48"
         LIBS="$LIBS -L/opt/local/lib -L/opt/local/lib/db48"
       fi

       AC_CHECK_PROG([BREW],brew, brew)
       if test x$BREW = xbrew; then
         dnl add default homebrew paths
         openssl_prefix=`$BREW --prefix openssl`
         bdb_prefix=`$BREW --prefix berkeley-db4`
         export PKG_CONFIG_PATH="$openssl_prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
         CPPFLAGS="$CPPFLAGS -I$bdb_prefix/include"
         LIBS="$LIBS -L$bdb_prefix/lib"
       fi
     fi

     CPPFLAGS="$CPPFLAGS -DMAC_OSX"
     ;;
   *)
     ;;
esac

if test x$use_comparison_tool != xno; then
  AC_SUBST(JAVA_COMPARISON_TOOL, $use_comparison_tool)
fi

if test x$use_comparison_tool_reorg_tests != xno; then
  if test x$use_comparison_tool == x; then
    AC_MSG_ERROR("comparison tool reorg tests but comparison tool was not specified")
  fi
  AC_SUBST(COMPARISON_TOOL_REORG_TESTS, 1)
else
  AC_SUBST(COMPARISON_TOOL_REORG_TESTS, 0)
fi

if test x$use_lcov == xyes; then
  if test x$LCOV == x; then
    AC_MSG_ERROR("lcov testing requested but lcov not found")
  fi
  if test x$GCOV == x; then
    AC_MSG_ERROR("lcov testing requested but gcov not found")
  fi
  if test x$JAVA == x; then
    AC_MSG_ERROR("lcov testing requested but java not found")
  fi
  if test x$GENHTML == x; then
    AC_MSG_ERROR("lcov testing requested but genhtml not found")
  fi
  if test x$use_comparison_tool == x; then
    AC_MSG_ERROR("lcov testing requested but comparison tool was not specified")
  fi
  LCOV="$LCOV --gcov-tool=$GCOV"
  AX_CHECK_COMPILE_FLAG([--coverage],[CXXFLAGS="$CXXFLAGS --coverage"],
    [AC_MSG_ERROR("lcov testing requested but --coverage flag does not work")])
fi

dnl Require little endian
AC_C_BIGENDIAN([AC_MSG_ERROR("Big Endian not supported")])

dnl Check for pthread compile/link requirements
AX_PTHREAD
INCLUDES="$INCLUDES $PTHREAD_CFLAGS"

# The following macro will add the necessary defines to bitcoin-config.h, but
# they also need to be passed down to any subprojects. Pull the results out of
# the cache and add them to CPPFLAGS.
AC_SYS_LARGEFILE

if test x$ac_cv_sys_file_offset_bits != x &&
   test x$ac_cv_sys_file_offset_bits != xno &&
   test x$ac_cv_sys_file_offset_bits != xunknown; then
  CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=$ac_cv_sys_file_offset_bits"
fi

if test x$ac_cv_sys_large_files != x &&
   test x$ac_cv_sys_large_files != xno &&
   test x$ac_cv_sys_large_files != xunknown; then
  CPPFLAGS="$CPPFLAGS -D_LARGE_FILES=$ac_cv_sys_large_files"
fi

AX_CHECK_LINK_FLAG([[-Wl,--large-address-aware]], [LDFLAGS="$LDFLAGS -Wl,--large-address-aware"])

if test x$use_hardening != xno; then
  AX_CHECK_COMPILE_FLAG([-Wstack-protector],[HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -Wstack-protector"])
  AX_CHECK_COMPILE_FLAG([-fstack-protector-all],[HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fstack-protector-all"])
  AX_CHECK_COMPILE_FLAG([-fPIE],[HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fPIE"])

  AX_CHECK_PREPROC_FLAG([-D_FORTIFY_SOURCE=2],[
    AX_CHECK_PREPROC_FLAG([-U_FORTIFY_SOURCE],[
      HARDENED_CPPFLAGS="$HARDENED_CPPFLAGS -U_FORTIFY_SOURCE"
    ])
    HARDENED_CPPFLAGS="$HARDENED_CPPFLAGS -D_FORTIFY_SOURCE=2"
  ])

  AX_CHECK_LINK_FLAG([[-Wl,--dynamicbase]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--dynamicbase"])
  AX_CHECK_LINK_FLAG([[-Wl,--nxcompat]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--nxcompat"])
  AX_CHECK_LINK_FLAG([[-Wl,-z,relro]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,relro"])
  AX_CHECK_LINK_FLAG([[-Wl,-z,now]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,now"])

  if test x$TARGET_OS != xwindows; then
    # -pie will link successfully with MinGW, but it's unsupported and leads to undeterministic binaries
    AX_CHECK_LINK_FLAG([[-pie]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -pie"])
  fi

  CXXFLAGS="$CXXFLAGS $HARDENED_CXXFLAGS"
  CPPFLAGS="$CPPFLAGS $HARDENED_CPPFLAGS"
  LDFLAGS="$LDFLAGS $HARDENED_LDFLAGS"
  OBJCXXFLAGS="$CXXFLAGS"
fi

dnl this flag screws up non-darwin gcc even when the check fails. special-case it.
if test x$TARGET_OS = xdarwin; then
  AX_CHECK_LINK_FLAG([[-Wl,-dead_strip]], [LDFLAGS="$LDFLAGS -Wl,-dead_strip"])
fi

AC_CHECK_HEADERS([stdio.h stdlib.h unistd.h strings.h sys/types.h sys/stat.h])

dnl Check for MSG_NOSIGNAL
AC_MSG_CHECKING(for MSG_NOSIGNAL)
AC_TRY_COMPILE([#include <sys/socket.h>],
 [ int f = MSG_NOSIGNAL; ],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_MSG_NOSIGNAL, 1,[Define this symbol if you have MSG_NOSIGNAL]) ],
 [ AC_MSG_RESULT(no)]
)

LEVELDB_CPPFLAGS=
LIBLEVELDB=
LIBMEMENV=
AM_CONDITIONAL([EMBEDDED_LEVELDB],[true])
AC_SUBST(LEVELDB_CPPFLAGS)
AC_SUBST(LIBLEVELDB)
AC_SUBST(LIBMEMENV)

if test x$enable_wallet != xno; then
    dnl Check for libdb_cxx only if wallet enabled
    BITCOIN_FIND_BDB48
fi

dnl Check for libminiupnpc (optional)
if test x$use_upnp != xno; then
  AC_CHECK_HEADERS(
    [miniupnpc/miniwget.h miniupnpc/miniupnpc.h miniupnpc/upnpcommands.h miniupnpc/upnperrors.h],
    [AC_CHECK_LIB([miniupnpc], [main],, [have_miniupnpc=no])],
    [have_miniupnpc=no]
  )
fi

dnl Check for boost libs
AX_BOOST_BASE
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_THREAD
AX_BOOST_CHRONO

if test x$use_tests = xyes; then

  if test x$HEXDUMP = x; then
    AC_MSG_ERROR(hexdump is required for tests)
  fi


  AX_BOOST_UNIT_TEST_FRAMEWORK

  dnl Determine if -DBOOST_TEST_DYN_LINK is needed
  AC_MSG_CHECKING([for dynamic linked boost test])
  TEMP_LIBS="$LIBS"
  LIBS="$LIBS $BOOST_UNIT_TEST_FRAMEWORK_LIB"
  TEMP_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
  AC_LINK_IFELSE([AC_LANG_SOURCE([
       #define BOOST_TEST_DYN_LINK
       #define BOOST_TEST_MAIN
        #include <boost/test/unit_test.hpp>

       ])],
    [AC_MSG_RESULT(yes)]
    [TESTDEFS="$TESTDEFS -DBOOST_TEST_DYN_LINK"],
    [AC_MSG_RESULT(no)])
  LIBS="$TEMP_LIBS"
  CPPFLAGS="$TEMP_CPPFLAGS"
fi

BOOST_LIBS="$BOOST_LDFLAGS $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_PROGRAM_OPTIONS_LIB $BOOST_THREAD_LIB"
BOOST_INCLUDES="$BOOST_CPPFLAGS"

dnl Boost >= 1.50 uses sleep_for rather than the now-deprecated sleep, however
dnl it was broken from 1.50 to 1.52 when backed by nanosleep. Use sleep_for if
dnl a working version is available, else fall back to sleep. sleep was removed
dnl after 1.56.
dnl If neither is available, abort.
dnl If sleep_for is used, boost_chrono becomes a requirement.
if test x$ax_cv_boost_chrono = xyes; then
dnl Allow passing extra needed dependency libraries for boost-chrono from static gitian build
BOOST_CHRONO_LIB="$BOOST_CHRONO_LIB $BOOST_CHRONO_EXTRALIBS"

TEMP_LIBS="$LIBS"
LIBS="$LIBS $BOOST_LIBS $BOOST_CHRONO_LIB"
TEMP_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_TRY_LINK([
  #include <boost/thread/thread.hpp>
  #include <boost/version.hpp>
  ],[
  #if BOOST_VERSION >= 105000 && (!defined(BOOST_HAS_NANOSLEEP) || BOOST_VERSION >= 105200)
      boost::this_thread::sleep_for(boost::chrono::milliseconds(0));
  #else
   choke me
  #endif
  ],
  [boost_sleep=yes; BOOST_LIBS="$BOOST_LIBS $BOOST_CHRONO_LIB";
     AC_DEFINE(HAVE_WORKING_BOOST_SLEEP_FOR, 1, [Define this symbol if boost sleep_for works])],
  [boost_sleep=no])
LIBS="$TEMP_LIBS"
CPPFLAGS="$TEMP_CPPFLAGS"
fi

if test x$boost_sleep != xyes; then
TEMP_LIBS="$LIBS"
LIBS="$LIBS $BOOST_LIBS"
TEMP_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_TRY_LINK([
  #include <boost/version.hpp>
  #include <boost/thread.hpp>
  #include <boost/date_time/posix_time/posix_time_types.hpp>
  ],[
  #if BOOST_VERSION <= 105600
      boost::this_thread::sleep(boost::posix_time::milliseconds(0));
  #else
   choke me
  #endif
  ],
  [boost_sleep=yes; AC_DEFINE(HAVE_WORKING_BOOST_SLEEP, 1, [Define this symbol if boost sleep works])],
  [boost_sleep=no])
LIBS="$TEMP_LIBS"
CPPFLAGS="$TEMP_CPPFLAGS"
fi

if test x$boost_sleep != xyes; then
  AC_MSG_ERROR(No working boost sleep implementation found. If on ubuntu 13.10 with libboost1.54-all-dev remove libboost.1.54-all-dev and use libboost1.53-all-dev)
fi

AC_ARG_WITH([cli],
  [AS_HELP_STRING([--with-cli],
  [with CLI (default is yes)])],
  [build_bitcoin_cli=$withval],
  [build_bitcoin_cli=yes])

AC_ARG_WITH([daemon],
  [AS_HELP_STRING([--with-daemon],
  [with daemon (default is yes)])],
  [build_bitcoind=$withval],
  [build_bitcoind=yes])

BITCOIN_QT_INIT

if test x$use_pkgconfig = xyes; then

  if test x$PKG_CONFIG == x; then
    AC_MSG_ERROR(pkg-config not found.)
  fi

  : #NOP
  m4_ifdef(
    [PKG_CHECK_MODULES],
    [
      PKG_CHECK_MODULES([SSL], [libssl], [INCLUDES="$INCLUDES $SSL_CFLAGS"; LIBS="$LIBS $SSL_LIBS"], [AC_MSG_ERROR(openssl  not found.)])
      PKG_CHECK_MODULES([CRYPTO], [libcrypto], [INCLUDES="$INCLUDES $CRYPTO_CFLAGS"; LIBS="$LIBS $CRYPTO_LIBS"], [AC_MSG_ERROR(libcrypto  not found.)])
      BITCOIN_QT_CHECK([PKG_CHECK_MODULES([PROTOBUF], [protobuf], [have_protobuf=yes], [BITCOIN_QT_FAIL(libprotobuf not found)])])
      if test x$use_qr != xno; then
        BITCOIN_QT_CHECK([PKG_CHECK_MODULES([QR], [libqrencode], [have_qrencode=yes], [have_qrencode=no])])
      fi
    ]
  )
else
  AC_CHECK_HEADER([openssl/crypto.h],,AC_MSG_ERROR(libcrypto headers missing))
  AC_CHECK_LIB([crypto],      [main],, AC_MSG_ERROR(libcrypto missing))

  AC_CHECK_HEADER([openssl/ssl.h],, AC_MSG_ERROR(libssl headers missing),)
  AC_CHECK_LIB([ssl],         [main],, AC_MSG_ERROR(libssl missing))

  BITCOIN_QT_CHECK(AC_CHECK_LIB([protobuf] ,[main],,BITCOIN_QT_FAIL(libprotobuf not found)))
  if test x$use_qr != xno; then
    BITCOIN_QT_CHECK([AC_CHECK_LIB([qrencode], [main],, [have_qrencode=no])])
    BITCOIN_QT_CHECK([AC_CHECK_HEADER([qrencode.h],, have_qrencode=no)])
  fi
fi

BITCOIN_QT_PATH_PROGS([PROTOC], [protoc],$protoc_bin_path)

AC_MSG_CHECKING([whether to build bitcoind])
AM_CONDITIONAL([BUILD_BITCOIND], [test x$build_bitcoind = xyes])
AC_MSG_RESULT($build_bitcoind)

AC_MSG_CHECKING([whether to build bitcoin-cli])
AM_CONDITIONAL([BUILD_BITCOIN_CLI], [test x$build_bitcoin_cli = xyes])
AC_MSG_RESULT($build_bitcoin_cli)

dnl sets $bitcoin_enable_qt, $bitcoin_enable_qt_test, $bitcoin_enable_qt_dbus
BITCOIN_QT_CONFIGURE([$use_pkgconfig], [qt4])


if test x$use_ipv6 = xyes; then
  dnl Check for ipv6 build requirements
  AC_MSG_CHECKING(for operating system IPv6 support)
  AC_TRY_LINK([
  #if defined(_WINDOWS)
  #include <winsock2.h>
  #else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #endif
  ],[
  #if !defined(_WINDOWS)
  struct in6_addr ipv6Addr;
  struct sockaddr_in6 addr;
  #endif
  int temp = socket(AF_INET6, SOCK_STREAM, 0);],
  [AC_MSG_RESULT(yes); have_ipv6=yes; AC_DEFINE(HAVE_IPV6, 1, [Define this symbol if you have operating system IPv6 support])],
  [AC_MSG_RESULT(no)]; have_ipv6=no)
fi
AC_LANG_POP

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
    CC="$ac_cv_path_CCACHE $CC"
    CXX="$ac_cv_path_CCACHE $CXX"
  fi
  AC_MSG_RESULT($use_ccache)
fi

dnl enable wallet
AC_MSG_CHECKING([if wallet should be enabled])
if test x$enable_wallet != xno; then
  AC_MSG_RESULT(yes)
  AC_DEFINE_UNQUOTED([ENABLE_WALLET],[1],[Define to 1 to enable wallet functions])

else
  AC_MSG_RESULT(no)
fi

dnl enable ipv6 support
AC_MSG_CHECKING([whether to build with support for IPv6])
if test x$have_ipv6 = xno; then
  if test x$use_ipv6 = xyes; then
     AC_MSG_ERROR([IPv6 requested, but cannot be built. use --disable-ipv6])
  fi
  AC_MSG_RESULT(no)
else
  if test x$use_ipv6 = xyes; then
      AC_MSG_RESULT(yes)
      AC_DEFINE([USE_IPV6],[1],[Define if IPv6 support should be compiled in])
  else
      AC_MSG_RESULT(no)
  fi
fi

dnl enable upnp support
AC_MSG_CHECKING([whether to build with support for UPnP])
if test x$have_miniupnpc = xno; then
  if test x$use_upnp = xyes; then
     AC_MSG_ERROR("UPnP requested but cannot be built. use --without-miniupnpc")
  fi
  AC_MSG_RESULT(no)
else
  if test x$use_upnp != xno; then
    AC_MSG_RESULT(yes)
    AC_MSG_CHECKING([whether to build with UPnP enabled by default])
    use_upnp=yes
    upnp_setting=0
    if test x$use_upnp_default != xno; then
      use_upnp_default=yes
      upnp_setting=1
    fi
    AC_MSG_RESULT($use_upnp_default)
    AC_DEFINE_UNQUOTED([USE_UPNP],[$upnp_setting],[UPnP support not compiled if undefined, otherwise value (0 or 1) determines default state])
    if test x$TARGET_OS = xwindows; then
      CPPFLAGS="$CPPFLAGS -DSTATICLIB"
    fi
  else
    AC_MSG_RESULT(no)
  fi
fi

dnl these are only used when qt is enabled
if test x$bitcoin_enable_qt != xno; then
  BUILD_QT=qt
  dnl enable dbus support
  AC_MSG_CHECKING([whether to build GUI with support for D-Bus])
  if test x$bitcoin_enable_qt_dbus != xno; then
    AC_DEFINE([USE_DBUS],[1],[Define if dbus support should be compiled in])
  fi
  AC_MSG_RESULT($bitcoin_enable_qt_dbus)

  dnl enable qr support
  AC_MSG_CHECKING([whether to build GUI with support for QR codes])
  if test x$have_qrencode = xno; then
    if test x$use_qr == xyes; then
     AC_MSG_ERROR("QR support requested but cannot be built. use --without-qrencode")
    fi
    AC_MSG_RESULT(no)
  else
    if test x$use_qr != xno; then
      AC_MSG_RESULT(yes)
      AC_DEFINE([USE_QRCODE],[1],[Define if QR support should be compiled in])
      use_qr=yes
    else
      AC_MSG_RESULT(no)
    fi
  fi

  if test x$XGETTEXT == x; then
    AC_MSG_WARN("xgettext is required to update qt translations")
  fi

  AC_MSG_CHECKING([whether to build test_bitcoin-qt])
  if test x$use_tests$bitcoin_enable_qt_test = xyesyes; then
    AC_MSG_RESULT([yes])
    BUILD_TEST_QT="test"
  else
    AC_MSG_RESULT([no])
  fi
fi

AC_MSG_CHECKING([whether to build test_bitcoin])
if test x$use_tests = xyes; then
  AC_MSG_RESULT([yes])
  BUILD_TEST="test"
else
  AC_MSG_RESULT([no])
fi

if test "x$use_tests$build_bitcoind$use_qt" = "xnonono"; then
  AC_MSG_ERROR([No targets! Please specify at least one of: --enable-cli --enable-daemon --enable-gui or --enable-tests])
fi

AM_CONDITIONAL([TARGET_DARWIN], [test x$TARGET_OS = xdarwin])
AM_CONDITIONAL([TARGET_WINDOWS], [test x$TARGET_OS = xwindows])
AM_CONDITIONAL([ENABLE_WALLET],[test x$enable_wallet == xyes])
AM_CONDITIONAL([USE_QRCODE], [test x$use_qr = xyes])
AM_CONDITIONAL([USE_LCOV],[test x$use_lcov == xyes])
AM_CONDITIONAL([USE_COMPARISON_TOOL],[test x$use_comparison_tool != xno])
AM_CONDITIONAL([USE_COMPARISON_TOOL_REORG_TESTS],[test x$use_comparison_tool_reorg_test != xno])

AC_DEFINE(CLIENT_VERSION_MAJOR, _CLIENT_VERSION_MAJOR, [Major version])
AC_DEFINE(CLIENT_VERSION_MINOR, _CLIENT_VERSION_MINOR, [Minor version])
AC_DEFINE(CLIENT_VERSION_REVISION, _CLIENT_VERSION_REVISION, [Build revision])
AC_DEFINE(CLIENT_VERSION_BUILD, _CLIENT_VERSION_BUILD, [Version Build])
AC_DEFINE(CLIENT_VERSION_IS_RELEASE, _CLIENT_VERSION_IS_RELEASE, [Version is release])
AC_DEFINE(COPYRIGHT_YEAR, _COPYRIGHT_YEAR, [Version is release])
AC_SUBST(CLIENT_VERSION_MAJOR, _CLIENT_VERSION_MAJOR)
AC_SUBST(CLIENT_VERSION_MINOR, _CLIENT_VERSION_MINOR)
AC_SUBST(CLIENT_VERSION_REVISION, _CLIENT_VERSION_REVISION)
AC_SUBST(CLIENT_VERSION_BUILD, _CLIENT_VERSION_BUILD)
AC_SUBST(CLIENT_VERSION_IS_RELEASE, _CLIENT_VERSION_IS_RELEASE)
AC_SUBST(COPYRIGHT_YEAR, _COPYRIGHT_YEAR)


AC_SUBST(USE_UPNP)
AC_SUBST(USE_QRCODE)
AC_SUBST(USE_IPV6)
AC_SUBST(INCLUDES)
AC_SUBST(BOOST_LIBS)
AC_SUBST(TESTDEFS)
AC_SUBST(LEVELDB_TARGET_FLAGS)
AC_SUBST(BUILD_TEST)
AC_SUBST(BUILD_QT)
AC_SUBST(BUILD_TEST_QT)
AC_CONFIG_FILES([Makefile src/Makefile src/test/Makefile src/qt/Makefile src/qt/test/Makefile share/setup.nsi share/qt/Info.plist])
AC_CONFIG_FILES([qa/pull-tester/run-bitcoind-for-test.sh],[chmod +x qa/pull-tester/run-bitcoind-for-test.sh])
AC_CONFIG_FILES([qa/pull-tester/build-tests.sh],[chmod +x qa/pull-tester/build-tests.sh])
AC_OUTPUT
