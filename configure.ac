dnl ===================================================
dnl Autotools build-system for Bitcoin
dnl Run 'autoreconf -i' to produce a configure script
dnl Run 'make distcheck' to produce a tarball release
dnl See debugging and profiling development facilities
dnl ===================================================
dnl Copyright (C) 2011 by Denis Roio <jaromil@dyne.org>
dnl Distributed under the terms of the MIT/X11 license
dnl ===================================================

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
dnl DISTCHECK_CONFIGURE_FLAGS="V=1"
dnl AC_SUBST(DISTCHECK_CONFIGURE_FLAGS)

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
    HOST_LIBS="-lpthread -lz -ldl"
  ;;

  *darwin*)
    have_osx=yes
    AC_MSG_RESULT([Apple/OSX])
    AC_DEFINE(HAVE_MACOSX,1,[define if compiling for Apple OSX])
    AC_DEFINE(BSD,1,[define if compiling on a BSD operating system])
    AC_DEFINE(CRYPTOPP_DISABLE_ASM,1,[define to deactivate ASM optimization in cryptopp lib])
    HOST_CPPFLAGS="-mmacosx-version-min=10.5"
    HOST_CXXFLAGS="-I/opt/local/include/db46"
    HOST_LIBS="-L/opt/local/lib/db46 -lpthread -lz -ldl"
    # other defines present in old mkfile: -DNOPCH -DMSG_NOSIGNAL=0
  ;;

  *freebsd*)
    have_freebsd=yes
    AC_MSG_RESULT([FreeBSD])
    AC_DEFINE(BSD,1,[define if compiling on a BSD operating system])
    AC_DEFINE(HAVE_FREEBSD,1,[define if compiling for FreeBSD])
    AC_DEFINE(HAVE_SYS_SOCKET_H,1,[usually needed for FreeBSD includes])
    HOST_CPPFLAGS="-I/usr/local/include -L/usr/local/lib"
    HOST_LIBS="-lpthread -lz -ldl"
  ;;

  *win*)
    have_mswin=yes
    AC_MSG_RESULT([MS/Windows (TODO)])
    # TODO
  ;;

  *mingw*)
    have_mingw=yes
    AC_MSG_RESULT([MINGW cross compile])
    HOST_CXXFLAGS="-DWIN32 -D__WXMSW__ -D_WINDOWS -DNOPCH -mwindows -mthreads"
    HOST_CPPFLAGS="-I/usr/i586-mingw32msvc/include"
    HOST_LIBS="-L/usr/i586-mingw32msvc/lib/ -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lwinspool -lwinmm -lshell32 -lcomctl32 -lole32 -loleaut32 -luuid -lrpcrt4 -ladvapi32 -lws2_32 -lshlwapi"
    # fix the -I. bug
    DEFAULT_INCLUDES="-I.."
    AC_SUBST(DEFAULT_INCLUDES)
  ;;

  *)
    AC_MSG_RESULT([$host_os?!])
    AC_MSG_ERROR([[
[!] Your system architecture is not supported by Bitcoin
[!] if you are interested in porting Bitcoin to your architecture
[!] you can reach developers via #bitcoin-dev channel on freenode
[!] or the web forum on http://bitcoin.org
]], 0)
  ;;
esac

AC_SUBST(HOST_LIBS)
AC_SUBST(HOST_CPPFLAGS)
AC_SUBST(HOST_CXXFLAGS)

dnl ==============================================================
dnl Setup for automake
dnl ==============================================================

AM_INIT_AUTOMAKE([dist-bzip2 subdir-objects no-define])
dnl the no-define flag is to avoid definition of VERSION by automake
dnl our VERSION is defined in sourcecode: src/serialize.h as (int)

dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([auto-config.h])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AS
AC_PROG_RANLIB

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

# platform dependent threading libs
case "$host_os" in
  *mingw*)
    COMMON_LIBS="-mthreads"
  ;;
  *linux*)
    PKG_CHECK_MODULES(GTHREAD2, gthread-2.0, :,
    			AC_MSG_ERROR([ GThread-2 development library not found]))
esac

dnl platform independent database libs
COMMON_LIBS="$COMMON_LIBS -ldb_cxx"
AC_SUBST(COMMON_LIBS)

dnl ==============================================================
dnl enable UPNP
dnl ==============================================================
AC_ARG_ENABLE(upnp,
	AS_HELP_STRING([--enable-upnp],[compile with UPNP support (use on/off to set default behaviour)]),
	[have_upnp=$enableval],
	[have_upnp=omit])
AC_MSG_CHECKING([if UPNP support (miniupnp) is enabled])
UPNP_LIBS=""
if test x$have_upnp = xyes; then
# this is the default behaviour for --enable-upnp with no arguments
   AC_MSG_RESULT([compiled - on by default])
   AC_DEFINE(USE_UPNP,1,[Define if building support for UPNP functions via miniupnp])
elif test x$have_upnp = xon; then
   AC_MSG_RESULT([compiled - on by default])
   AC_DEFINE(USE_UPNP,1,[Define if building support for UPNP functions via miniupnp])
elif test x$have_upnp = xoff; then
   AC_MSG_RESULT([compiled - off by default])
   AC_DEFINE(USE_UPNP,0,[Define if building support for UPNP functions via miniupnp])
else
   AC_MSG_RESULT(no)
fi
if test x$have_upnp != xomit; then
   if test x$have_linux = xyes; then
      UPNP_LIBS="-lminiupnpc"
   fi
   # TODO OSX and WIN
fi


AC_SUBST(UPNP_LIBS)

dnl ==============================================================
dnl Graphical User Interface
dnl ==============================================================
AC_ARG_ENABLE(gui,
	AS_HELP_STRING([--enable-gui],[compile a graphical user interface: wx or none]),
	[have_gui=$enableval],
	[have_gui=none])
AC_MSG_CHECKING([which GUI is selected])

case "$have_gui" in

     dnl ============
     dnl WxWidget GUI
     yes)
     dnl we can have more GUIs in future by checking the value here
     dnl like using --enable-gui=wx --enable-gui=gtk etc.
	AC_MSG_RESULT(WxWidgets)
	if test x$have_linux = xyes; then
	   AM_OPTIONS_WXCONFIG
	   AM_PATH_WXCONFIG(2.9.1, have_wx=yes)
	   if test x$have_wx != xyes; then
	      AC_MSG_ERROR([WxWidget GUI selected, but wx-config version 2.9.1 is not found])
	   else
	      AC_DEFINE(__WXGTK__,1,[Define if building the WX GUI on GNU/Linux using GTK])
	      GUI_CPPFLAGS="$WX_CPPFLAGS"
	      GUI_CXXFLAGS="$WX_CXXFLAGS_ONLY -Iwx"
	      GUI_LIBS="$WX_LIBS wx/libbtcwxgui.a"
	      GUI_SUBDIRS="wx"
	   fi
	fi

	if test x$have_osx = xyes; then
	   # TODO
	   AC_DEFINE(__WXMAC_OSX__,1,[Define if building the WX GUI on Apple/OSX])	
	fi

	if test x$have_mswin = xyes; then
	   # TODO
	   AC_DEFINE(__WXMSW__,1,[Define if building the WX GUI on MS/Windows])	
	fi
     ;;
     
     none)
	AC_MSG_RESULT(none)
     ;;
esac
AC_SUBST(GUI_LIBS)
AC_SUBST(GUI_CXXFLAGS)
AC_SUBST(GUI_CPPFLAGS)
AC_SUBST(GUI_SUBDIRS)
AM_CONDITIONAL([BUILD_WX], [test x$have_wx = xyes])

dnl **********************************************************************
dnl Check for MSG_NOSIGNAL
dnl **********************************************************************
AC_MSG_CHECKING(for MSG_NOSIGNAL)
AC_TRY_COMPILE([#include <sys/socket.h>],
 [ int f = MSG_NOSIGNAL; ],
 [ AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_MSG_NOSIGNAL, 1,
             [Define this symbol if you have MSG_NOSIGNAL]) ],
 [ AC_MSG_RESULT(no)]
)

dnl **********************************************************************
dnl Check for SO_NOSIGPIPE (Darwin equivalent for MSG_NOSIGNAL)
dnl **********************************************************************
AC_MSG_CHECKING(for SO_NOSIGPIPE )
AC_TRY_COMPILE([#include <sys/socket.h>],
 [ int f = SO_NOSIGPIPE; ],
 [ AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_SO_NOSIGPIPE, 1,
             [Define this symbol if you have SO_NOSIGPIPE]) ],
 [ AC_MSG_RESULT(no)]
)

dnl ==============================================================
dnl compile with full warnings and debugging symbols
dnl ==============================================================
AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug],[compile with debug symbols (no)]),
    [enable_debug=$enableval],
    [enable_debug=no])
    AC_MSG_CHECKING([if compiling with debugging symbols])

if test x$enable_debug = xyes; then
    # default debug flags
    DEBUG_CXXFLAGS="-Wall -g -ggdb"
    if test x$have_wx = xyes; then
       DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -D__WXDEBUG__"
    fi
    AC_MSG_RESULT(yes)
elif test x$enable_debug != xno; then
    # specified at configure with --enable-debug="..."
    DEBUG_CXXFLAGS="$enableval"
    AC_MSG_RESULT([$enableval])
else
    # usual optimizations and warnings
    DEBUG_CXXFLAGS="-O2 -Wno-invalid-offsetof -Wformat"
    AC_MSG_RESULT(no)
fi

dnl ==============================================================================
dnl CHECK to use profiling flags when compiling, for execution analysis with gprof
dnl ==============================================================================
AC_ARG_ENABLE(profiling,
    AS_HELP_STRING([--enable-profiling],[compile using flags for execution analysis with gprof (no)]),
    [enable_profiling=$enableval],
    [enable_profiling=no])
    AC_MSG_CHECKING([if compiling for the GNU profiler, see gprof(1)])

if test x$enable_profiling = xyes; then
  DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -pg -fprofile-arcs -ftest-coverage"
  PROFILING_LIBS="-lgcov"
  AC_SUBST(PROFILING_LIBS)
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi

CXXFLAGS="$HOST_CXXFLAGS $DEBUG_CXXFLAGS"
AC_SUBST(CXXFLAGS)

AC_CONFIG_FILES([
Makefile
src/Makefile
src/xpm/Makefile
src/json/Makefile
src/cryptopp/Makefile
src/wx/Makefile
])
AC_OUTPUT

