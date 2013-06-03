dnl require autoconf 2.60 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.60])
define(_CLIENT_VERSION_MAJOR, 0)
define(_CLIENT_VERSION_MINOR, 8)
define(_CLIENT_VERSION_REVISION, 99)
define(_CLIENT_VERSION_BUILD, 0)
define(_CLIENT_VERSION_IS_RELEASE, false)
define(_COPYRIGHT_YEAR, 2013)
AC_INIT([Bitcoin],[_CLIENT_VERSION_MAJOR._CLIENT_VERSION_MINOR._CLIENT_VERSION_REVISION],[info@bitcoin.org],[bitcoin])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST

dnl ==============================================================
dnl Setup for automake
dnl ==============================================================

dnl reset CXXFLAGS if empty (autoconf adds its own defaults otherwise)
:${CXXFLAGS=""}

AM_INIT_AUTOMAKE([no-define subdir-objects foreign])

dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ARG_ENABLE([upnp],
  [AS_HELP_STRING([--enable-upnp],
  [enable UPNP (default is yes if libminiupnpc is found)])],
  [use_upnp=$enableval],
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

dnl enable qt support
AC_ARG_ENABLE([qt],
  [AS_HELP_STRING([--enable-qt],
  [enable qt (default is yes)])],
  [use_qt=$enableval],
  [use_qt=yes])

dnl compile with full warnings and debugging symbols
AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug],[compile with debug symbols (default is no)]),
    [use_debug=$enableval],
    [use_debug=no])
    AC_MSG_CHECKING([if compiling with debugging symbols])

AC_ARG_ENABLE(tests,
    AS_HELP_STRING([--enable-tests],[compile tests (default is yes)]),
    [use_tests=$enableval],
    [use_tests=yes])

AC_ARG_ENABLE([qr],
  [AS_HELP_STRING([--enable-qr],
  [enable QR code support (default is yes if libqrencode is found)])],
  [use_qr=$enableval],
  [use_qr=auto])

AC_ARG_ENABLE([dbus],
  [AS_HELP_STRING([--enable-dbus],
  [enable DBus support (default is yes if QtDBus is found)])],
  [use_dbus=$enableval],
  [use_dbus=auto])

AC_ARG_ENABLE([hardening],
  [AS_HELP_STRING([--enable-hardening],
  [attempt to harden the resulting executables (default is yes)])],
  [use_hardening=$enableval],
  [use_hardening=yes])

AC_ARG_WITH([qt-incdir],[AS_HELP_STRING([--with-qt-incdir=INC_DIR],[specify qt include path (overridden by pkgconfig)])], [qt_include_path=$withval], [qt_include_path=])
AC_ARG_WITH([qt-libdir],[AS_HELP_STRING([--with-qt-libdir=LIB_DIR],[specify qt lib path (overridden by pkgconfig)])], [qt_lib_path=$withval], [qt_lib_path=])
AC_ARG_WITH([qt-bindir],[AS_HELP_STRING([--with-qt-bindir=BIN_DIR],[specify qt bin path (overridden by pkgconfig)])], [qt_bin_path=$withval], [qt_bin_path=])
AC_ARG_WITH([qt-plugindir],[AS_HELP_STRING([--with-qt-plugindir=PLUGIN_DIR],[specify qt plugin path (overridden by pkgconfig)])], [qt_plugin_path=$withval], [qt_plugin_path=])

AC_CONFIG_SRCDIR([.])
AC_CONFIG_HEADERS([bitcoin-config.h])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_OBJC
AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib)
AC_PATH_PROGS([MOC], [moc moc-qt4], moc, $qt_bin_path:$PATH)
AC_PATH_PROGS([UIC], [uic uic-qt4], uic, $qt_bin_path:$PATH)
AC_PATH_PROG([RCC], [rcc], rcc, $qt_bin_path:$PATH)
AC_PATH_PROG([LRELEASE], [lrelease], lrelease, $qt_bin_path:$PATH)

## TODO: Remove these hard-coded paths and flags. They are here for the sake of
##       compatibility with the legacy buildsystem.
##
CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wformat -Wformat-security -Wno-unused-parameter -frandom-seed=bitcoin"
CPPFLAGS="$CPPFLAGS -DBOOST_SPIRIT_THREADSAFE -DHAVE_BUILD_INFO"
DEBUG_CXXFLAGS="-g -ggdb"
RELEASE_CXXFLAGS="-O2"

use_pkgconfig=yes
case $host in
  *mingw*)

     #pkgconfig does more harm than good with mingw
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
     CPPFLAGS="$CPPFLAGS -D_MT -DWIN32 -D_WINDOWS -DBOOST_THREAD_USE_LIB"
     LEVELDB_TARGET_FLAGS="TARGET_OS=OS_WINDOWS_CROSSCOMPILE"
     CXXFLAGS="$CXXFLAGS -w"
     ;;
  *darwin*)
     TARGET_OS=darwin

     if  test x$cross_compiling != xyes; then
       AC_CHECK_PROG([PORT],port, port)
       if test x$PORT = xport; then
         dnl add default macports paths
         CPPFLAGS="$CPPFLAGS -I/opt/local/include -I/opt/local/include/db48"
         LDFLAGS="$LDFLAGS -L/opt/local/lib -L/opt/local/lib/db48"
       fi

       AC_CHECK_PROG([BREW],brew, brew)
       if test x$BREW = xbrew; then
         dnl add default homebrew paths
         CPPFLAGS="$CPPFLAGS -I/usr/local/opt/berkeley-db4/include"
         LDFLAGS="$LDFLAGS -L/usr/local/opt/berkeley-db4/lib"
       fi
     fi

     if test x$use_qt = xyes; then
       MOC_DEFS="-DQ_OS_MAC"
       base_frameworks="-framework Foundation -framework ApplicationServices -framework AppKit"
       AX_CHECK_LINK_FLAG([$base_frameworks],[LDFLAGS="$LDFLAGS $base_frameworks"],[AC_MSG_ERROR(could not find base frameworks)])
     fi

     RELEASE_CXXFLAGS="$RELEASE_CXXFLAGS -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 -arch i386 -O3"
     CPPFLAGS="$CPPFLAGS -DMAC_OSX"
     TESTDEFS="-DBOOST_TEST_DYN_LINK"
     ;;
   *)
     TESTDEFS="-DBOOST_TEST_DYN_LINK"
     ;;
esac

if test x$use_debug != xno; then
  CXXFLAGS="$CXXFLAGS $DEBUG_CXXFLAGS"
else
  CXXFLAGS="$CXXFLAGS $RELEASE_CXXFLAGS"
fi

AC_LANG_PUSH([C++])
dnl Require little endian
AC_C_BIGENDIAN([AC_MSG_ERROR("Big Endian not supported")])

dnl Check for pthread compile/link requirements
AX_PTHREAD
INCLUDES="$INCLUDES $PTHREAD_CFLAGS"

AX_CHECK_LINK_FLAG([-Wl,--large-address-aware], [LDFLAGS="$LDFLAGS -Wl,--large-address-aware"])

if test x$use_hardening != xno; then
  AX_CHECK_COMPILE_FLAG([-Wstack-protector],[HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -Wstack-protector"])
  AX_CHECK_COMPILE_FLAG([-fPIE],[HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fPIE"])

  AX_CHECK_PREPROC_FLAG([-D_FORTIFY_SOURCE=2],[HARDENED_CPPFLAGS="$HARDENED_CPPFLAGS -D_FORTIFY_SOURCE=2"])

  AX_CHECK_LINK_FLAG([-pie], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -pie"])
  AX_CHECK_LINK_FLAG([-Wl,--dynamicbase], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--dynamicbase"])
  AX_CHECK_LINK_FLAG([-Wl,--nxcompat], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--nxcompat"])

  # -fstack-protector-all can produce broken binaries with mingw
  if test x$TARGET_OS != xwindows; then
    AX_CHECK_COMPILE_FLAG([-fno-stack-protector],[HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fno-stack-protector"])
    AX_CHECK_COMPILE_FLAG([-fstack-protector-all],[HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fstack-protector-all"])
  fi

  CXXFLAGS="$CXXFLAGS $HARDENED_CXXFLAGS"
  CPPFLAGS="$CPPFLAGS $HARDENED_CPPFLAGS"
  LDFLAGS="$LDFLAGS $HARDENED_LDFLAGS"
fi

dnl this flag screws up non-darwin gcc even when the check fails. special-case it.
if test x$TARGET_OS = xdarwin; then
  AX_CHECK_LINK_FLAG([-dead-strip], [LDFLAGS="$LDFLAGS -dead_strip"])
fi

AC_CHECK_HEADERS([stdio.h stdlib.h unistd.h strings.h sys/types.h sys/stat.h])

dnl Check for MSG_NOSIGNAL
AC_MSG_CHECKING(for MSG_NOSIGNAL)
AC_TRY_COMPILE([#include <sys/socket.h>],
 [ int f = MSG_NOSIGNAL; ],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_MSG_NOSIGNAL, 1,[Define this symbol if you have MSG_NOSIGNAL]) ],
 [ AC_MSG_RESULT(no)]
)

dnl Check for libdb_cxx
AC_CHECK_LIB([db_cxx], [main],, AC_MSG_ERROR(libdb missing))

dnl Check for libminiupnpc (optional)
if test x$use_upnp != xno; then
  AC_CHECK_LIB([miniupnpc], [main],, [have_miniupnpc=no])
fi

dnl Check for boost libs
AX_BOOST_BASE
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_THREAD
AX_BOOST_UNIT_TEST_FRAMEWORK
AX_BOOST_CHRONO
BOOST_LIBS="$BOOST_LDFLAGS $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_PROGRAM_OPTIONS_LIB $BOOST_THREAD_LIB"
BOOST_INCLUDES="$BOOST_CPPFLAGS"

if test x$TARGET_OS = xwindows; then
  BOOST_LIBS="$BOOST_LIBS $BOOST_CHRONO_LIB"
fi

if test x$use_pkgconfig = xyes; then
  : #NOP
  m4_ifdef(
    [PKG_CHECK_MODULES],
    [
      PKG_CHECK_MODULES([SSL], [libssl], [INCLUDES="$INCLUDES $SSL_CFLAGS"; LIBS="$LIBS $SSL_LIBS"], [AC_MSG_ERROR(openssl  not found.)])
      PKG_CHECK_MODULES([CRYPTO], [libcrypto], [INCLUDES="$CRYPTO_CFLAGS"; LIBS="$LIBS $CRYPTO_LIBS"], [AC_MSG_ERROR(libcrypto  not found.)])

      if test x$use_qt = xyes; then
        PKG_CHECK_MODULES([QT], [QtCore QtGui QtNetwork], [QT_INCLUDES="$QT_CFLAGS"; have_qt=yes], [have_qt=no])
        if test x$use_tests = xyes; then
          PKG_CHECK_MODULES([QT_TEST], [QtTest], [QT_TEST_INCLUDES="$QT_TEST_CFLAGS"; have_qt_test=yes], [have_qt_test=no])
        fi
        if test x$use_dbus != xno; then
          PKG_CHECK_MODULES([QT_DBUS], [QtDBus], [QT_DBUS_INCLUDES="$QT_DBUS_CFLAGS"; have_qt_dbus=yes], [have_qt_dbus=no])
        fi
      fi
      if test x$use_qr != xno; then
        PKG_CHECK_MODULES([QR], [libqrencode], [INCLUDES="$QR_CFLAGS"; LIBS="$LIBS $QR_LIBS"; have_qrencode=yes], [have_qrencode=no])
      fi
    ]
  )
else
    AC_CHECK_HEADER([openssl/crypto.h],,AC_MSG_ERROR(libcrypto headers missing))
    AC_CHECK_LIB([crypto],      [main],, AC_MSG_ERROR(libcrypto missing))

    AC_CHECK_HEADER([openssl/ssl.h],, AC_MSG_ERROR(libssl headers missing),)
    AC_CHECK_LIB([ssl],         [main],, AC_MSG_ERROR(libssl missing))

      TEMP_LIBS="$LIBS"
      LIBS=
      if test x$qt_lib_path != x; then
        QT_LIBS="$QT_LIBS -L$qt_lib_path"
        LIBS="$QT_LIBS"
      fi
      if test x$qt_plugin_path != x; then
        QT_LIBS="$QT_LIBS -L$qt_plugin_path/codecs"
        LIBS="$QT_LIBS"
      fi
      AC_CHECK_LIB([qcncodecs],      [main],, AC_MSG_ERROR(libqcncodecs not found. Install it or use --disable-qt.))
      AC_CHECK_LIB([qjpcodecs],      [main],, AC_MSG_ERROR(libqjpcodecs not found. Install it or use --disable-qt.))
      AC_CHECK_LIB([qkrcodecs],      [main],, AC_MSG_ERROR(libqkrcodecs not found. Install it or use --disable-qt.))
      AC_CHECK_LIB([qtwcodecs],      [main],, AC_MSG_ERROR(libqtwcodecs not found. Install it or use --disable-qt.))
      AC_CHECK_LIB([imm32],      [main],, AC_MSG_ERROR(libimm32 not found. Install it or use --disable-qt.))
      AC_CHECK_LIB([QtCore],      [main],, AC_MSG_ERROR(libQtCore not found. Install it or use --disable-qt.))
      AC_CHECK_LIB([QtGui],      [main],, AC_MSG_ERROR(libQtGui not found. Install it or use --disable-qt.))
      AC_CHECK_LIB([QtNetwork],      [main],, AC_MSG_ERROR(libQtNetwork not found. Install it or use --disable-qt.))
      QT_LIBS="$LIBS"
      LIBS="$TEMP_LIBS"

      TEMP_CPPFLAGS="$CPPFLAGS"
      if test x$qt_include_path != x; then
        QT_INCLUDES="-I$qt_include_path -I$qt_include_path/QtCore -I$qt_include_path/QtGui -I$qt_include_path/QtNetwork -I$qt_include_path/QtTest -I$qt_include_path/QtDBus"
        CPPFLAGS="$CPPFLAGS $QT_INCLUDES"
      fi
      AC_CHECK_HEADER([QtPlugin],, AC_MSG_ERROR(QtCore headers missing. Install them or use --disable-qt.),)
      AC_CHECK_HEADER([QApplication],, AC_MSG_ERROR(QtGUI headers missing. Install them or use --disable-qt.),)
      AC_CHECK_HEADER([QLocalSocket],, AC_MSG_ERROR(QtNetwork headers missing. Install them or use --disable-qt.),)

      if test x$use_tests = xyes; then
        TEMP_LIBS="$LIBS"
        LIBS=
        if test x$qt_lib_path != x; then
          LIBS="-L$qt_lib_path"
        fi
        AC_CHECK_LIB([QtTest],      [main],, have_qt_test=no)
        AC_CHECK_HEADER([QTest],, have_qt_test=no)
        QT_TEST_LIBS="$LIBS"
        LIBS="$TEMP_LIBS"
      fi
      if test x$use_dbus != xno; then
        TEMP_LIBS="$LIBS"
        LIBS=
        if test x$qt_lib_path != x; then
          LIBS="-L$qt_lib_path"
        fi
        AC_CHECK_LIB([QtDBus],      [main],, have_qt_dbus=no)
        AC_CHECK_HEADER([QtDBus],, have_qt_dbus=no)
        QT_DBUS_LIBS="$LIBS"
        LIBS="$TEMP_LIBS"
      fi
      if test x$use_qr != xno; then
        AC_CHECK_LIB([qrencode], [main],, [have_qrencode=no])
        AC_CHECK_HEADER([qrencode.h],, have_qrencode=no)
      fi
      CPPFLAGS="$TEMP_CPPFLAGS"
      AC_LANG_POP
fi

if test x$use_ipv6 = xyes; then
  dnl Check for ipv6 build requirements
  AC_MSG_CHECKING(for IPV6 build support)
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
  [AC_MSG_RESULT(yes); have_ipv6=yes; AC_DEFINE(HAVE_IPV6, 1, [Define this symbol if you have ipv6 build support])],
  [AC_MSG_RESULT(no)]; have_ipv6=no)
fi

dnl enable ipv6 support
AC_MSG_CHECKING([if ipv6 can be enabled])
if test x$have_ipv6 = xno; then
  if test x$use_ipv6 = xyes; then
     AC_MSG_ERROR("ipv6 requested but cannot be built. use --disable-ipv6")
  fi
  AC_MSG_RESULT(no)
else
  if test x$use_ipv6 = xyes; then
      AC_MSG_RESULT(yes)
      AC_DEFINE([USE_IPV6],[1],[Define if ipv6 support should be compiled in])
  else
      AC_MSG_RESULT(no)
  fi
fi

dnl enable qr support
AC_MSG_CHECKING([if qr can be enabled])
if test x$have_qrencode = xno; then
  if test x$use_qr != xno; then
     AC_MSG_ERROR("QR support requested but cannot be built. use --disable-qr")
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

dnl enable upnp support
AC_MSG_CHECKING([if upnp can be enabled])
if test x$have_miniupnpc = xno; then
  if test x$use_upnp = xyes; then
     AC_MSG_ERROR("upnp requested but cannot be built. use --disable-upnp")
  fi
  AC_MSG_RESULT(no)
else
  if test x$use_upnp != xno; then
    AC_MSG_RESULT(yes)
    AC_MSG_CHECKING([if upnp should be on by default])
    use_upnp=yes
    upnp_setting=0
    if test x$use_upnp_default != xno; then
      use_upnp_default=yes
      upnp_setting=1
    fi
    AC_MSG_RESULT($use_upnp_default)
    AC_DEFINE_UNQUOTED([USE_UPNP],[$upnp_setting],[Define to 1 for upnp runtime support])
    if test x$TARGET_OS = xwindows; then
      CPPFLAGS="$CPPFLAGS -DSTATICLIB"
    fi
  else
    AC_MSG_RESULT(no)
  fi
fi

dnl enable qt support
AC_MSG_CHECKING([if qt can be enabled])
if test x$use_qt = xyes; then
  if test x$have_qt = xno; then
     AC_MSG_ERROR("qt support requested but qt could not be located. use --disable-qt")
  fi
  if test x$MOC = x; then
    AC_MSG_ERROR("qt support requested but moc was not found. use --disable-qt")
  fi
  if test x$UIC = x; then
    AC_MSG_WARN("qt support requested but uic was not found.")
  fi
  if test x$RCC = x; then
    AC_MSG_WARN("qt support requested but rcc was not found.")
  fi
  if test x$LRELEASE = x; then
    AC_MSG_WARN("qt support requested but lrelease was not found.")
  fi
  if test x$use_tests = xyes; then
    if test x$have_qt_test = xno; then
      AC_MSG_ERROR("libQtTest not found. Use --disable-tests or --disable-qt.")
    fi
  fi
  if test x$have_qt_dbus = xno;  then
    if test x$use_dbus = xyes; then
      AC_MSG_ERROR("libQtDBus not found. Use --disable-dbus.")
    else
      use_dbus=no
    fi
  fi
fi

AC_MSG_RESULT($use_qt)

AC_MSG_CHECKING([if dbus can be enabled])
if test x$use_dbus != xno; then
  use_dbus=yes
  AC_DEFINE([USE_DBUS],[1],[Define if dbus support should be compiled in])
fi
AC_MSG_RESULT($use_dbus)


if test x$use_tests = xyes; then
  BUILD_TEST_PROGRAMS="$BUILD_TEST_PROGRAMS test_bitcoin${EXEEXT}"
  if test x$use_qt = xyes; then
    BUILD_TEST_PROGRAMS="$BUILD_TEST_PROGRAMS test_bitcoin-qt${EXEEXT}"
  fi
fi

AM_CONDITIONAL([BUILD_QT], [test x$use_qt = xyes])
AM_CONDITIONAL([TARGET_DARWIN], [test x$TARGET_OS = xdarwin])
AM_CONDITIONAL([USE_QRCODE], [test x$use_qr = xyes])

AC_DEFINE(CLIENT_VERSION_MAJOR, _CLIENT_VERSION_MAJOR, [Major version])
AC_DEFINE(CLIENT_VERSION_MINOR, _CLIENT_VERSION_MINOR, [Minor version])
AC_DEFINE(CLIENT_VERSION_REVISION, _CLIENT_VERSION_REVISION, [Build revision])
AC_DEFINE(CLIENT_VERSION_BUILD, _CLIENT_VERSION_BUILD, [Version Build])
AC_DEFINE(CLIENT_VERSION_IS_RELEASE, _CLIENT_VERSION_IS_RELEASE, [Version is release])
AC_DEFINE(COPYRIGHT_YEAR, _COPYRIGHT_YEAR, [Version is release])

AC_SUBST(USE_UPNP)
AC_SUBST(USE_QRCODE)
AC_SUBST(USE_IPV6)
AC_SUBST(INCLUDES)
AC_SUBST(BOOST_LIBS)
AC_SUBST(MOC_DEFS)
AC_SUBST(QT_INCLUDES)
AC_SUBST(QT_TEST_LIBS)
AC_SUBST(QT_LIBS)
AC_SUBST(QT_DBUS_LIBS)
AC_SUBST(QT_DBUS_INCLUDES)
AC_SUBST(QT_TEST_INCLUDES)
AC_SUBST(TESTDEFS)
AC_SUBST(LEVELDB_TARGET_FLAGS)
AC_SUBST(BUILD_TEST_PROGRAMS)
AC_CONFIG_FILES([Makefile])
AC_CONFIG_COMMANDS([build.h], [../share/genbuild.sh obj/build.h])
AC_OUTPUT
