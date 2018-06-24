dnl require autoconf 2.60 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.60])
define(_CLIENT_VERSION_MAJOR, 0)
define(_CLIENT_VERSION_MINOR, 16)
define(_CLIENT_VERSION_REVISION, 99)
define(_CLIENT_VERSION_BUILD, 0)
define(_CLIENT_VERSION_IS_RELEASE, false)
define(_COPYRIGHT_YEAR, 2018)
define(_COPYRIGHT_HOLDERS,[The %s developers])
define(_COPYRIGHT_HOLDERS_SUBSTITUTION,[[Bitcoin Core]])
AC_INIT([Bitcoin Core],[_CLIENT_VERSION_MAJOR._CLIENT_VERSION_MINOR._CLIENT_VERSION_REVISION],[https://github.com/bitcoin/bitcoin/issues],[bitcoin],[https://bitcoincore.org/])
AC_CONFIG_SRCDIR([src/validation.cpp])
AC_CONFIG_HEADERS([src/config/bitcoin-config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])

BITCOIN_DAEMON_NAME=bitcoind
BITCOIN_GUI_NAME=bitcoin-qt
BITCOIN_CLI_NAME=bitcoin-cli
BITCOIN_TX_NAME=bitcoin-tx

dnl Unless the user specified ARFLAGS, force it to be cr
AC_ARG_VAR(ARFLAGS, [Flags for the archiver, defaults to <cr> if not set])
if test "x${ARFLAGS+set}" != "xset"; then
  ARFLAGS="cr"
fi

AC_CANONICAL_HOST

AH_TOP([#ifndef BITCOIN_CONFIG_H])
AH_TOP([#define BITCOIN_CONFIG_H])
AH_BOTTOM([#endif //BITCOIN_CONFIG_H])

dnl faketime breaks configure and is only needed for make. Disable it here.
unset FAKETIME

dnl Automake init set-up and checks
AM_INIT_AUTOMAKE([no-define subdir-objects foreign])

dnl faketime messes with timestamps and causes configure to be re-run.
dnl --disable-maintainer-mode can be used to bypass this.
AM_MAINTAINER_MODE([enable])

dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Compiler checks (here before libtool).
if test "x${CXXFLAGS+set}" = "xset"; then
  CXXFLAGS_overridden=yes
else
  CXXFLAGS_overridden=no
fi
AC_PROG_CXX

dnl By default, libtool for mingw refuses to link static libs into a dll for
dnl fear of mixing pic/non-pic objects, and import/export complications. Since
dnl we have those under control, re-enable that functionality.
case $host in
  *mingw*)
     lt_cv_deplibs_check_method="pass_all"
  ;;
esac
dnl Require C++11 compiler (no GNU extensions)
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory], [nodefault])
dnl Check if -latomic is required for <std::atomic>
CHECK_ATOMIC

dnl Unless the user specified OBJCXX, force it to be the same as CXX. This ensures
dnl that we get the same -std flags for both.
m4_ifdef([AC_PROG_OBJCXX],[
if test "x${OBJCXX+set}" = "x"; then
  OBJCXX="${CXX}"
fi
AC_PROG_OBJCXX
])

dnl Libtool init checks.
LT_INIT([pic-only])

dnl Check/return PATH for base programs.
AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)
AC_PATH_TOOL(STRIP, strip)
AC_PATH_TOOL(GCOV, gcov)
AC_PATH_PROG(LCOV, lcov)
dnl Python 3.x is supported from 3.4 on (see https://github.com/bitcoin/bitcoin/issues/7893)
AC_PATH_PROGS([PYTHON], [python3.7 python3.6 python3.5 python3.4 python3 python])
AC_PATH_PROG(GENHTML, genhtml)
AC_PATH_PROG([GIT], [git])
AC_PATH_PROG(CCACHE,ccache)
AC_PATH_PROG(XGETTEXT,xgettext)
AC_PATH_PROG(HEXDUMP,hexdump)
AC_PATH_TOOL(READELF, readelf)
AC_PATH_TOOL(CPPFILT, c++filt)
AC_PATH_TOOL(OBJCOPY, objcopy)
AC_PATH_PROG(DOXYGEN, doxygen)
if test -z "$DOXYGEN"; then
   AC_MSG_WARN([Doxygen not found])
fi
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AC_ARG_VAR(PYTHONPATH, Augments the default search path for python module files)

# Enable wallet
AC_ARG_ENABLE([wallet],
  [AS_HELP_STRING([--disable-wallet],
  [disable wallet (enabled by default)])],
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

AC_ARG_ENABLE(tests,
    AS_HELP_STRING([--disable-tests],[do not compile tests (default is to compile)]),
    [use_tests=$enableval],
    [use_tests=yes])

AC_ARG_ENABLE(gui-tests,
    AS_HELP_STRING([--disable-gui-tests],[do not compile GUI tests (default is to compile if GUI and tests enabled)]),
    [use_gui_tests=$enableval],
    [use_gui_tests=$use_tests])

AC_ARG_ENABLE(bench,
    AS_HELP_STRING([--disable-bench],[do not compile benchmarks (default is to compile)]),
    [use_bench=$enableval],
    [use_bench=yes])

AC_ARG_ENABLE([extended-functional-tests],
    AS_HELP_STRING([--enable-extended-functional-tests],[enable expensive functional tests when using lcov (default no)]),
    [use_extended_functional_tests=$enableval],
    [use_extended_functional_tests=no])

AC_ARG_WITH([qrencode],
  [AS_HELP_STRING([--with-qrencode],
  [enable QR code support (default is yes if qt is enabled and libqrencode is found)])],
  [use_qr=$withval],
  [use_qr=auto])

AC_ARG_ENABLE([hardening],
  [AS_HELP_STRING([--disable-hardening],
  [do not attempt to harden the resulting executables (default is to harden when possible)])],
  [use_hardening=$enableval],
  [use_hardening=auto])

AC_ARG_ENABLE([reduce-exports],
  [AS_HELP_STRING([--enable-reduce-exports],
  [attempt to reduce exported symbols in the resulting executables (default is no)])],
  [use_reduce_exports=$enableval],
  [use_reduce_exports=no])

AC_ARG_ENABLE([ccache],
  [AS_HELP_STRING([--disable-ccache],
  [do not use ccache for building (default is to use if found)])],
  [use_ccache=$enableval],
  [use_ccache=auto])

AC_ARG_ENABLE([lcov],
  [AS_HELP_STRING([--enable-lcov],
  [enable lcov testing (default is no)])],
  [use_lcov=$enableval],
  [use_lcov=no])

AC_ARG_ENABLE([lcov-branch-coverage],
  [AS_HELP_STRING([--enable-lcov-branch-coverage],
  [enable lcov testing branch coverage (default is no)])],
  [use_lcov_branch=yes],
  [use_lcov_branch=no])

AC_ARG_ENABLE([glibc-back-compat],
  [AS_HELP_STRING([--enable-glibc-back-compat],
  [enable backwards compatibility with glibc])],
  [use_glibc_compat=$enableval],
  [use_glibc_compat=no])

AC_ARG_ENABLE([asm],
  [AS_HELP_STRING([--enable-asm],
  [Enable assembly routines (default is yes)])],
  [use_asm=$enableval],
  [use_asm=yes])

if test "x$use_asm" = xyes; then
  AC_DEFINE(USE_ASM, 1, [Define this symbol to build in assembly routines])
fi

AC_ARG_WITH([system-univalue],
  [AS_HELP_STRING([--with-system-univalue],
  [Build with system UniValue (default is no)])],
  [system_univalue=$withval],
  [system_univalue=no]
)
AC_ARG_ENABLE([zmq],
  [AS_HELP_STRING([--disable-zmq],
  [disable ZMQ notifications])],
  [use_zmq=$enableval],
  [use_zmq=yes])

AC_ARG_WITH([protoc-bindir],[AS_HELP_STRING([--with-protoc-bindir=BIN_DIR],[specify protoc bin path])], [protoc_bin_path=$withval], [])

AC_ARG_ENABLE(man,
    [AS_HELP_STRING([--disable-man],
                    [do not install man pages (default is to install)])],,
    enable_man=yes)
AM_CONDITIONAL(ENABLE_MAN, test "$enable_man" != no)

# Enable debug
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [use debug compiler flags and macros (default is no)])],
    [enable_debug=$enableval],
    [enable_debug=no])

# Enable different -fsanitize options
AC_ARG_WITH([sanitizers],
    [AS_HELP_STRING([--with-sanitizers],
                    [comma separated list of extra sanitizers to build with (default is none enabled)])],
    [use_sanitizers=$withval])

# Enable gprof profiling
AC_ARG_ENABLE([gprof],
    [AS_HELP_STRING([--enable-gprof],
                    [use gprof profiling compiler flags (default is no)])],
    [enable_gprof=$enableval],
    [enable_gprof=no])

# Turn warnings into errors
AC_ARG_ENABLE([werror],
    [AS_HELP_STRING([--enable-werror],
                    [Treat certain compiler warnings as errors (default is no)])],
    [enable_werror=$enableval],
    [enable_werror=no])

AC_LANG_PUSH([C++])
AX_CHECK_COMPILE_FLAG([-Werror],[CXXFLAG_WERROR="-Werror"],[CXXFLAG_WERROR=""])

if test "x$enable_debug" = xyes; then
  # Clear default -g -O2 flags
  if test "x$CXXFLAGS_overridden" = xno; then
	CXXFLAGS=""
  fi
  # Prefer -Og, fall back to -O0 if that is unavailable.
  AX_CHECK_COMPILE_FLAG(
    [-Og],
    [[DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -Og"]],
    [AX_CHECK_COMPILE_FLAG([-O0],[[DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -O0"]],,[[$CXXFLAG_WERROR]])],
    [[$CXXFLAG_WERROR]])

  # Prefer -g3, fall back to -g if that is unavailable.
  AX_CHECK_COMPILE_FLAG(
    [-g3],
    [[DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -g3"]],
    [AX_CHECK_COMPILE_FLAG([-g],[[DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -g"]],,[[$CXXFLAG_WERROR]])],
    [[$CXXFLAG_WERROR]])

  AX_CHECK_PREPROC_FLAG([-DDEBUG],[[DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DDEBUG"]],,[[$CXXFLAG_WERROR]])
  AX_CHECK_PREPROC_FLAG([-DDEBUG_LOCKORDER],[[DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DDEBUG_LOCKORDER"]],,[[$CXXFLAG_WERROR]])
fi

if test x$use_sanitizers != x; then
  # First check if the compiler accepts flags. If an incompatible pair like
  # -fsanitize=address,thread is used here, this check will fail. This will also
  # fail if a bad argument is passed, e.g. -fsanitize=undfeined
  AX_CHECK_COMPILE_FLAG(
    [[-fsanitize=$use_sanitizers]],
    [[SANITIZER_CXXFLAGS=-fsanitize=$use_sanitizers]],
    [AC_MSG_ERROR([compiler did not accept requested flags])])

  # Some compilers (e.g. GCC) require additional libraries like libasan,
  # libtsan, libubsan, etc. Make sure linking still works with the sanitize
  # flag. This is a separate check so we can give a better error message when
  # the sanitize flags are supported by the compiler but the actual sanitizer
  # libs are missing.
  AX_CHECK_LINK_FLAG(
    [[-fsanitize=$use_sanitizers]],
    [[SANITIZER_LDFLAGS=-fsanitize=$use_sanitizers]],
    [AC_MSG_ERROR([linker did not accept requested flags, you are missing required libraries])])
fi

ERROR_CXXFLAGS=
if test "x$enable_werror" = "xyes"; then
  if test "x$CXXFLAG_WERROR" = "x"; then
    AC_MSG_ERROR("enable-werror set but -Werror is not usable")
  fi
  AX_CHECK_COMPILE_FLAG([-Werror=vla],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=vla"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Werror=thread-safety-analysis],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=thread-safety-analysis"],,[[$CXXFLAG_WERROR]])
fi

if test "x$CXXFLAGS_overridden" = "xno"; then
  AX_CHECK_COMPILE_FLAG([-Wall],[WARN_CXXFLAGS="$WARN_CXXFLAGS -Wall"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wextra],[WARN_CXXFLAGS="$WARN_CXXFLAGS -Wextra"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wformat],[WARN_CXXFLAGS="$WARN_CXXFLAGS -Wformat"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wvla],[WARN_CXXFLAGS="$WARN_CXXFLAGS -Wvla"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wformat-security],[WARN_CXXFLAGS="$WARN_CXXFLAGS -Wformat-security"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wthread-safety-analysis],[WARN_CXXFLAGS="$WARN_CXXFLAGS -Wthread-safety-analysis"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wrange-loop-analysis],[WARN_CXXFLAGS="$WARN_CXXFLAGS -Wrange-loop-analysis"],,[[$CXXFLAG_WERROR]])

  ## Some compilers (gcc) ignore unknown -Wno-* options, but warn about all
  ## unknown options if any other warning is produced. Test the -Wfoo case, and
  ## set the -Wno-foo case if it works.
  AX_CHECK_COMPILE_FLAG([-Wunused-parameter],[NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-unused-parameter"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wself-assign],[NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-self-assign"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wunused-local-typedef],[NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-unused-local-typedef"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wdeprecated-register],[NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-deprecated-register"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wimplicit-fallthrough],[NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-implicit-fallthrough"],,[[$CXXFLAG_WERROR]])
fi

# Check for optional instruction set support. Enabling these does _not_ imply that all code will
# be compiled with them, rather that specific objects/libs may use them after checking for runtime
# compatibility.
AX_CHECK_COMPILE_FLAG([-msse4.2],[[SSE42_CXXFLAGS="-msse4.2"]],,[[$CXXFLAG_WERROR]])
AX_CHECK_COMPILE_FLAG([-msse4.1],[[SSE41_CXXFLAGS="-msse4.1"]],,[[$CXXFLAG_WERROR]])
AX_CHECK_COMPILE_FLAG([-mavx -mavx2],[[AVX2_CXXFLAGS="-mavx -mavx2"]],,[[$CXXFLAG_WERROR]])
AX_CHECK_COMPILE_FLAG([-msse4 -msha],[[SHANI_CXXFLAGS="-msse4 -msha"]],,[[$CXXFLAG_WERROR]])

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $SSE42_CXXFLAGS"
AC_MSG_CHECKING(for assembler crc32 support)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <stdint.h>
    #if defined(_MSC_VER)
    #include <intrin.h>
    #elif defined(__GNUC__) && defined(__SSE4_2__)
    #include <nmmintrin.h>
    #endif
  ]],[[
    uint64_t l = 0;
    l = _mm_crc32_u8(l, 0);
    l = _mm_crc32_u32(l, 0);
    l = _mm_crc32_u64(l, 0);
    return l;
  ]])],
 [ AC_MSG_RESULT(yes); enable_hwcrc32=yes],
 [ AC_MSG_RESULT(no)]
)
CXXFLAGS="$TEMP_CXXFLAGS"

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $SSE41_CXXFLAGS"
AC_MSG_CHECKING(for SSE4.1 intrinsics)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <stdint.h>
    #if defined(_MSC_VER)
    #include <immintrin.h>
    #elif defined(__GNUC__)
    #include <x86intrin.h>
    #endif
  ]],[[
    __m128i l = _mm_set1_epi32(0);
    return _mm_extract_epi32(l, 3);
  ]])],
 [ AC_MSG_RESULT(yes); enable_sse41=yes; AC_DEFINE(ENABLE_SSE41, 1, [Define this symbol to build code that uses SSE4.1 intrinsics]) ],
 [ AC_MSG_RESULT(no)]
)
CXXFLAGS="$TEMP_CXXFLAGS"

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $AVX2_CXXFLAGS"
AC_MSG_CHECKING(for AVX2 intrinsics)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <stdint.h>
    #if defined(_MSC_VER)
    #include <immintrin.h>
    #elif defined(__GNUC__) && defined(__AVX2__)
    #include <x86intrin.h>
    #endif
  ]],[[
    __m256i l = _mm256_set1_epi32(0);
    return _mm256_extract_epi32(l, 7);
  ]])],
 [ AC_MSG_RESULT(yes); enable_avx2=yes; AC_DEFINE(ENABLE_AVX2, 1, [Define this symbol to build code that uses AVX2 intrinsics]) ],
 [ AC_MSG_RESULT(no)]
)
CXXFLAGS="$TEMP_CXXFLAGS"

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $SHANI_CXXFLAGS"
AC_MSG_CHECKING(for SHA-NI intrinsics)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <stdint.h>
    #if defined(_MSC_VER)
    #include <immintrin.h>
    #elif defined(__GNUC__)
    #include <x86intrin.h>
    #endif
  ]],[[
    __m128i i = _mm_set1_epi32(0);
    __m128i j = _mm_set1_epi32(1);
    __m128i k = _mm_set1_epi32(2);
    return _mm_extract_epi32(_mm_sha256rnds2_epu32(i, i, k), 0);
  ]])],
 [ AC_MSG_RESULT(yes); enable_shani=yes; AC_DEFINE(ENABLE_SHANI, 1, [Define this symbol to build code that uses SHA-NI intrinsics]) ],
 [ AC_MSG_RESULT(no)]
)
CXXFLAGS="$TEMP_CXXFLAGS"

CPPFLAGS="$CPPFLAGS -DHAVE_BUILD_INFO -D__STDC_FORMAT_MACROS"

AC_ARG_WITH([utils],
  [AS_HELP_STRING([--with-utils],
  [build bitcoin-cli bitcoin-tx (default=yes)])],
  [build_bitcoin_utils=$withval],
  [build_bitcoin_utils=yes])

AC_ARG_WITH([libs],
  [AS_HELP_STRING([--with-libs],
  [build libraries (default=yes)])],
  [build_bitcoin_libs=$withval],
  [build_bitcoin_libs=yes])

AC_ARG_WITH([daemon],
  [AS_HELP_STRING([--with-daemon],
  [build bitcoind daemon (default=yes)])],
  [build_bitcoind=$withval],
  [build_bitcoind=yes])

use_pkgconfig=yes
case $host in
  *mingw*)

     #pkgconfig does more harm than good with MinGW
     use_pkgconfig=no

     TARGET_OS=windows
     AC_CHECK_LIB([mingwthrd],      [main],, AC_MSG_ERROR(libmingwthrd missing))
     AC_CHECK_LIB([kernel32],      [main],, AC_MSG_ERROR(libkernel32 missing))
     AC_CHECK_LIB([user32],      [main],, AC_MSG_ERROR(libuser32 missing))
     AC_CHECK_LIB([gdi32],      [main],, AC_MSG_ERROR(libgdi32 missing))
     AC_CHECK_LIB([comdlg32],      [main],, AC_MSG_ERROR(libcomdlg32 missing))
     AC_CHECK_LIB([winspool],      [main],, AC_MSG_ERROR(libwinspool missing))
     AC_CHECK_LIB([winmm],      [main],, AC_MSG_ERROR(libwinmm missing))
     AC_CHECK_LIB([shell32],      [main],, AC_MSG_ERROR(libshell32 missing))
     AC_CHECK_LIB([comctl32],      [main],, AC_MSG_ERROR(libcomctl32 missing))
     AC_CHECK_LIB([ole32],      [main],, AC_MSG_ERROR(libole32 missing))
     AC_CHECK_LIB([oleaut32],      [main],, AC_MSG_ERROR(liboleaut32 missing))
     AC_CHECK_LIB([uuid],      [main],, AC_MSG_ERROR(libuuid missing))
     AC_CHECK_LIB([rpcrt4],      [main],, AC_MSG_ERROR(librpcrt4 missing))
     AC_CHECK_LIB([advapi32],      [main],, AC_MSG_ERROR(libadvapi32 missing))
     AC_CHECK_LIB([ws2_32],      [main],, AC_MSG_ERROR(libws2_32 missing))
     AC_CHECK_LIB([mswsock],      [main],, AC_MSG_ERROR(libmswsock missing))
     AC_CHECK_LIB([shlwapi],      [main],, AC_MSG_ERROR(libshlwapi missing))
     AC_CHECK_LIB([iphlpapi],      [main],, AC_MSG_ERROR(libiphlpapi missing))
     AC_CHECK_LIB([crypt32],      [main],, AC_MSG_ERROR(libcrypt32 missing))

     # -static is interpreted by libtool, where it has a different meaning.
     # In libtool-speak, it's -all-static.
     AX_CHECK_LINK_FLAG([[-static]],[LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static"])

     AC_PATH_PROG([MAKENSIS], [makensis], none)
     if test x$MAKENSIS = xnone; then
       AC_MSG_WARN("makensis not found. Cannot create installer.")
     fi

     AC_PATH_TOOL(WINDRES, windres, none)
     if test x$WINDRES = xnone; then
       AC_MSG_ERROR("windres not found")
     fi

     CPPFLAGS="$CPPFLAGS -D_MT -DWIN32 -D_WINDOWS -DBOOST_THREAD_USE_LIB"
     LEVELDB_TARGET_FLAGS="-DOS_WINDOWS"
     if test "x$CXXFLAGS_overridden" = "xno"; then
       CXXFLAGS="$CXXFLAGS -w"
     fi
     case $host in
       i?86-*) WINDOWS_BITS=32 ;;
       x86_64-*) WINDOWS_BITS=64 ;;
       *) AC_MSG_ERROR("Could not determine win32/win64 for installer") ;;
     esac
     AC_SUBST(WINDOWS_BITS)

     dnl libtool insists upon adding -nostdlib and a list of objects/libs to link against.
     dnl That breaks our ability to build dll's with static libgcc/libstdc++/libssp. Override
     dnl its command here, with the predeps/postdeps removed, and -static inserted. Postdeps are
     dnl also overridden to prevent their insertion later.
     dnl This should only affect dll's.
     archive_cmds_CXX="\$CC -shared \$libobjs \$deplibs \$compiler_flags -static -o \$output_objdir/\$soname \${wl}--enable-auto-image-base -Xlinker --out-implib -Xlinker \$lib"
     postdeps_CXX=

     ;;
  *darwin*)
     TARGET_OS=darwin
     LEVELDB_TARGET_FLAGS="-DOS_MACOSX"
     if  test x$cross_compiling != xyes; then
       BUILD_OS=darwin
       AC_CHECK_PROG([PORT],port, port)
       if test x$PORT = xport; then
         dnl add default macports paths
         CPPFLAGS="$CPPFLAGS -isystem /opt/local/include"
         LIBS="$LIBS -L/opt/local/lib"
         if test -d /opt/local/include/db48; then
           CPPFLAGS="$CPPFLAGS -I/opt/local/include/db48"
           LIBS="$LIBS -L/opt/local/lib/db48"
         fi
       fi

       AC_PATH_PROGS([RSVG_CONVERT], [rsvg-convert rsvg],rsvg-convert)
       AC_CHECK_PROG([BREW],brew, brew)
       if test x$BREW = xbrew; then
         dnl These Homebrew packages may be keg-only, meaning that they won't be found
         dnl in expected paths because they may conflict with system files. Ask
         dnl Homebrew where each one is located, then adjust paths accordingly.
         dnl It's safe to add these paths even if the functionality is disabled by
         dnl the user (--without-wallet or --without-gui for example).

         openssl_prefix=`$BREW --prefix openssl 2>/dev/null`
         bdb_prefix=`$BREW --prefix berkeley-db4 2>/dev/null`
         qt5_prefix=`$BREW --prefix qt5 2>/dev/null`
         if test x$openssl_prefix != x; then
           PKG_CONFIG_PATH="$openssl_prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
           export PKG_CONFIG_PATH
         fi
         if test x$bdb_prefix != x; then
           CPPFLAGS="$CPPFLAGS -I$bdb_prefix/include"
           LIBS="$LIBS -L$bdb_prefix/lib"
         fi
         if test x$qt5_prefix != x; then
           PKG_CONFIG_PATH="$qt5_prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
           export PKG_CONFIG_PATH
         fi
       fi
     else
       case $build_os in
         *darwin*)
           BUILD_OS=darwin
           ;;
         *)
           AC_PATH_TOOL([INSTALLNAMETOOL], [install_name_tool], install_name_tool)
           AC_PATH_TOOL([OTOOL], [otool], otool)
           AC_PATH_PROGS([GENISOIMAGE], [genisoimage mkisofs],genisoimage)
           AC_PATH_PROGS([RSVG_CONVERT], [rsvg-convert rsvg],rsvg-convert)
           AC_PATH_PROGS([IMAGEMAGICK_CONVERT], [convert],convert)
           AC_PATH_PROGS([TIFFCP], [tiffcp],tiffcp)

           dnl libtool will try to strip the static lib, which is a problem for
           dnl cross-builds because strip attempts to call a hard-coded ld,
           dnl which may not exist in the path. Stripping the .a is not
           dnl necessary, so just disable it.
           old_striplib=
           ;;
       esac
     fi

     AX_CHECK_LINK_FLAG([[-Wl,-headerpad_max_install_names]], [LDFLAGS="$LDFLAGS -Wl,-headerpad_max_install_names"])
     CPPFLAGS="$CPPFLAGS -DMAC_OSX"
     OBJCXXFLAGS="$CXXFLAGS"
     ;;
   *linux*)
     TARGET_OS=linux
     LEVELDB_TARGET_FLAGS="-DOS_LINUX"
     ;;
   *freebsd*)
     LEVELDB_TARGET_FLAGS="-DOS_FREEBSD"
     ;;
   *openbsd*)
     LEVELDB_TARGET_FLAGS="-DOS_OPENBSD"
     ;;
   *netbsd*)
     LEVELDB_TARGET_FLAGS="-DOS_NETBSD"
     ;;
   *)
     OTHER_OS=`echo ${host_os} | awk '{print toupper($0)}'`
     AC_MSG_WARN([Guessing LevelDB OS as OS_${OTHER_OS}, please check whether this is correct, if not add an entry to configure.ac.])
     LEVELDB_TARGET_FLAGS="-DOS_${OTHER_OS}"
     ;;
esac

if test x$use_pkgconfig = xyes; then
  m4_ifndef([PKG_PROG_PKG_CONFIG], [AC_MSG_ERROR(PKG_PROG_PKG_CONFIG macro not found. Please install pkg-config and re-run autogen.sh.)])
  m4_ifdef([PKG_PROG_PKG_CONFIG], [
  PKG_PROG_PKG_CONFIG
  if test x"$PKG_CONFIG" = "x"; then
    AC_MSG_ERROR(pkg-config not found.)
  fi
  ])
fi

if test x$use_extended_functional_tests != xno; then
  AC_SUBST(EXTENDED_FUNCTIONAL_TESTS, --extended)
fi

if test x$use_lcov = xyes; then
  if test x$LCOV = x; then
    AC_MSG_ERROR("lcov testing requested but lcov not found")
  fi
  if test x$GCOV = x; then
    AC_MSG_ERROR("lcov testing requested but gcov not found")
  fi
  if test x$PYTHON = x; then
    AC_MSG_ERROR("lcov testing requested but python not found")
  fi
  if test x$GENHTML = x; then
    AC_MSG_ERROR("lcov testing requested but genhtml not found")
  fi
  LCOV="$LCOV --gcov-tool=$GCOV"
  AX_CHECK_LINK_FLAG([[--coverage]], [LDFLAGS="$LDFLAGS --coverage"],
    [AC_MSG_ERROR("lcov testing requested but --coverage linker flag does not work")])
  AX_CHECK_COMPILE_FLAG([--coverage],[CXXFLAGS="$CXXFLAGS --coverage"],
    [AC_MSG_ERROR("lcov testing requested but --coverage flag does not work")])
  AC_DEFINE(USE_COVERAGE, 1, [Define this symbol if coverage is enabled])
  CXXFLAGS="$CXXFLAGS -Og"
fi

if test x$use_lcov_branch != xno; then
  AC_SUBST(LCOV_OPTS, "$LCOV_OPTS --rc lcov_branch_coverage=1")
fi

dnl Check for endianness
AC_C_BIGENDIAN

dnl Check for pthread compile/link requirements
AX_PTHREAD

# The following macro will add the necessary defines to bitcoin-config.h, but
# they also need to be passed down to any subprojects. Pull the results out of
# the cache and add them to CPPFLAGS.
AC_SYS_LARGEFILE
# detect POSIX or GNU variant of strerror_r
AC_FUNC_STRERROR_R

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

AX_GCC_FUNC_ATTRIBUTE([visibility])
AX_GCC_FUNC_ATTRIBUTE([dllexport])
AX_GCC_FUNC_ATTRIBUTE([dllimport])

if test x$use_glibc_compat != xno; then

  #glibc absorbed clock_gettime in 2.17. librt (its previous location) is safe to link
  #in anyway for back-compat.
  AC_CHECK_LIB([rt],[clock_gettime],, AC_MSG_ERROR(librt missing))

  #__fdelt_chk's params and return type have changed from long unsigned int to long int.
  # See which one is present here.
  AC_MSG_CHECKING(__fdelt_chk type)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#ifdef _FORTIFY_SOURCE
                    #undef _FORTIFY_SOURCE
                  #endif
                  #define _FORTIFY_SOURCE 2
                  #include <sys/select.h>
     extern "C" long unsigned int __fdelt_warn(long unsigned int);]],[[]])],
    [ fdelt_type="long unsigned int"],
    [ fdelt_type="long int"])
  AC_MSG_RESULT($fdelt_type)
  AC_DEFINE_UNQUOTED(FDELT_TYPE, $fdelt_type,[parameter and return value type for __fdelt_chk])
else
  AC_SEARCH_LIBS([clock_gettime],[rt])
fi

if test "x$enable_gprof" = xyes; then
    dnl -pg is incompatible with -pie. Since hardening and profiling together doesn't make sense,
    dnl we simply make them mutually exclusive here. Additionally, hardened toolchains may force
    dnl -pie by default, in which case it needs to be turned off with -no-pie.

    if test x$use_hardening = xyes; then
        AC_MSG_ERROR(gprof profiling is not compatible with hardening. Reconfigure with --disable-hardening or --disable-gprof)
    fi
    use_hardening=no
    AX_CHECK_COMPILE_FLAG([-pg],[GPROF_CXXFLAGS="-pg"],
        [AC_MSG_ERROR(gprof profiling requested but not available)], [[$CXXFLAG_WERROR]])

    AX_CHECK_LINK_FLAG([[-no-pie]], [GPROF_LDFLAGS="-no-pie"])
    AX_CHECK_LINK_FLAG([[-pg]],[GPROF_LDFLAGS="$GPROF_LDFLAGS -pg"],
        [AC_MSG_ERROR(gprof profiling requested but not available)], [[$GPROF_LDFLAGS]])
fi

if test x$TARGET_OS != xwindows; then
  # All windows code is PIC, forcing it on just adds useless compile warnings
  AX_CHECK_COMPILE_FLAG([-fPIC],[PIC_FLAGS="-fPIC"])
fi

if test x$use_hardening != xno; then
  use_hardening=yes
  AX_CHECK_COMPILE_FLAG([-Wstack-protector],[HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -Wstack-protector"])
  AX_CHECK_COMPILE_FLAG([-fstack-protector-all],[HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fstack-protector-all"])

  AX_CHECK_PREPROC_FLAG([-D_FORTIFY_SOURCE=2],[
    AX_CHECK_PREPROC_FLAG([-U_FORTIFY_SOURCE],[
      HARDENED_CPPFLAGS="$HARDENED_CPPFLAGS -U_FORTIFY_SOURCE"
    ])
    HARDENED_CPPFLAGS="$HARDENED_CPPFLAGS -D_FORTIFY_SOURCE=2"
  ])

  AX_CHECK_LINK_FLAG([[-Wl,--dynamicbase]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--dynamicbase"])
  AX_CHECK_LINK_FLAG([[-Wl,--nxcompat]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--nxcompat"])
  AX_CHECK_LINK_FLAG([[-Wl,--high-entropy-va]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--high-entropy-va"])
  AX_CHECK_LINK_FLAG([[-Wl,-z,relro]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,relro"])
  AX_CHECK_LINK_FLAG([[-Wl,-z,now]], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,now"])
  AX_CHECK_LINK_FLAG([[-fPIE -pie]], [PIE_FLAGS="-fPIE"; HARDENED_LDFLAGS="$HARDENED_LDFLAGS -pie"],, [[$CXXFLAG_WERROR]])

  case $host in
    *mingw*)
       AC_CHECK_LIB([ssp],      [main],, AC_MSG_ERROR(libssp missing))
    ;;
  esac
fi

dnl this flag screws up non-darwin gcc even when the check fails. special-case it.
if test x$TARGET_OS = xdarwin; then
  AX_CHECK_LINK_FLAG([[-Wl,-dead_strip]], [LDFLAGS="$LDFLAGS -Wl,-dead_strip"])
fi

AC_CHECK_HEADERS([endian.h sys/endian.h byteswap.h stdio.h stdlib.h unistd.h strings.h sys/types.h sys/stat.h sys/select.h sys/prctl.h])

AC_CHECK_DECLS([strnlen])

# Check for daemon(3), unrelated to --with-daemon (although used by it)
AC_CHECK_DECLS([daemon])

AC_CHECK_DECLS([le16toh, le32toh, le64toh, htole16, htole32, htole64, be16toh, be32toh, be64toh, htobe16, htobe32, htobe64],,,
		[#if HAVE_ENDIAN_H
                 #include <endian.h>
                 #elif HAVE_SYS_ENDIAN_H
                 #include <sys/endian.h>
                 #endif])

AC_CHECK_DECLS([bswap_16, bswap_32, bswap_64],,,
		[#if HAVE_BYTESWAP_H
                 #include <byteswap.h>
                 #endif])

AC_CHECK_DECLS([__builtin_clz, __builtin_clzl, __builtin_clzll])

dnl Check for malloc_info (for memory statistics information in getmemoryinfo)
AC_MSG_CHECKING(for getmemoryinfo)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <malloc.h>]],
 [[ int f = malloc_info(0, NULL); ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_MALLOC_INFO, 1,[Define this symbol if you have malloc_info]) ],
 [ AC_MSG_RESULT(no)]
)

dnl Check for mallopt(M_ARENA_MAX) (to set glibc arenas)
AC_MSG_CHECKING(for mallopt M_ARENA_MAX)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <malloc.h>]],
 [[ mallopt(M_ARENA_MAX, 1); ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_MALLOPT_ARENA_MAX, 1,[Define this symbol if you have mallopt with M_ARENA_MAX]) ],
 [ AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for visibility attribute])
AC_LINK_IFELSE([AC_LANG_SOURCE([
  int foo_def( void ) __attribute__((visibility("default")));
  int main(){}
  ])],
  [
    AC_DEFINE(HAVE_VISIBILITY_ATTRIBUTE,1,[Define if the visibility attribute is supported.])
    AC_MSG_RESULT(yes)
  ],
  [
    AC_MSG_RESULT(no)
    if test x$use_reduce_exports = xyes; then
      AC_MSG_ERROR([Cannot find a working visibility attribute. Use --disable-reduce-exports.])
    fi
  ]
)

TEMP_LDFLAGS="$LDFLAGS"
LDFLAGS="$TEMP_LDFLAGS $PTHREAD_CFLAGS"
AC_MSG_CHECKING([for thread_local support])
AC_LINK_IFELSE([AC_LANG_SOURCE([
  #include <thread>
  static thread_local int foo = 0;
  static void run_thread() { foo++;}
  int main(){
  for(int i = 0; i < 10; i++) { std::thread(run_thread).detach();}
  return foo;
  }
  ])],
  [
    AC_DEFINE(HAVE_THREAD_LOCAL,1,[Define if thread_local is supported.])
    AC_MSG_RESULT(yes)
  ],
  [
    AC_MSG_RESULT(no)
  ]
)
LDFLAGS="$TEMP_LDFLAGS"

# Check for different ways of gathering OS randomness
AC_MSG_CHECKING(for Linux getrandom syscall)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>
  #include <sys/syscall.h>
  #include <linux/random.h>]],
 [[ syscall(SYS_getrandom, nullptr, 32, 0); ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_SYS_GETRANDOM, 1,[Define this symbol if the Linux getrandom system call is available]) ],
 [ AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for getentropy)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]],
 [[ getentropy(nullptr, 32) ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_GETENTROPY, 1,[Define this symbol if the BSD getentropy system call is available]) ],
 [ AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for getentropy via random.h)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>
 #include <sys/random.h>]],
 [[ getentropy(nullptr, 32) ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_GETENTROPY_RAND, 1,[Define this symbol if the BSD getentropy system call is available with sys/random.h]) ],
 [ AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(for sysctl KERN_ARND)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
  #include <sys/sysctl.h>]],
 [[ static const int name[2] = {CTL_KERN, KERN_ARND};
    sysctl(name, 2, nullptr, nullptr, nullptr, 0); ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_SYSCTL_ARND, 1,[Define this symbol if the BSD sysctl(KERN_ARND) is available]) ],
 [ AC_MSG_RESULT(no)]
)

# Check for reduced exports
if test x$use_reduce_exports = xyes; then
  AX_CHECK_COMPILE_FLAG([-fvisibility=hidden],[RE_CXXFLAGS="-fvisibility=hidden"],
  [AC_MSG_ERROR([Cannot set default symbol visibility. Use --disable-reduce-exports.])])
fi

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
    [AC_CHECK_LIB([miniupnpc], [upnpDiscover], [MINIUPNPC_LIBS=-lminiupnpc], [have_miniupnpc=no])],
    [have_miniupnpc=no]
  )
fi

BITCOIN_QT_INIT

dnl sets $bitcoin_enable_qt, $bitcoin_enable_qt_test, $bitcoin_enable_qt_dbus
BITCOIN_QT_CONFIGURE([$use_pkgconfig])

if test x$build_bitcoin_utils$build_bitcoind$bitcoin_enable_qt$use_tests$use_bench = xnonononono; then
    use_boost=no
else
    use_boost=yes
fi

if test x$use_boost = xyes; then

dnl Minimum required Boost version
define(MINIMUM_REQUIRED_BOOST, 1.47.0)

dnl Check for boost libs
AX_BOOST_BASE([MINIMUM_REQUIRED_BOOST])
if test x$want_boost = xno; then
    AC_MSG_ERROR([[only libbitcoinconsensus can be built without boost]])
fi
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_THREAD
AX_BOOST_CHRONO

dnl Boost 1.56 through 1.62 allow using std::atomic instead of its own atomic
dnl counter implementations. In 1.63 and later the std::atomic approach is default.
m4_pattern_allow(DBOOST_AC_USE_STD_ATOMIC) dnl otherwise it's treated like a macro
BOOST_CPPFLAGS="-DBOOST_SP_USE_STD_ATOMIC -DBOOST_AC_USE_STD_ATOMIC $BOOST_CPPFLAGS"

if test x$use_reduce_exports = xyes; then
  AC_MSG_CHECKING([for working boost reduced exports])
  TEMP_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$BOOST_CPPFLAGS $CPPFLAGS"
  AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[
      @%:@include <boost/version.hpp>
    ]], [[
      #if BOOST_VERSION >= 104900
      // Everything is okay
      #else
      #  error Boost version is too old
      #endif
    ]])],[
      AC_MSG_RESULT(yes)
    ],[
    AC_MSG_ERROR([boost versions < 1.49 are known to be broken with reduced exports. Use --disable-reduce-exports.])
  ])
  CPPFLAGS="$TEMP_CPPFLAGS"
fi
fi

if test x$use_reduce_exports = xyes; then
    CXXFLAGS="$CXXFLAGS $RE_CXXFLAGS"
    AX_CHECK_LINK_FLAG([[-Wl,--exclude-libs,ALL]], [RELDFLAGS="-Wl,--exclude-libs,ALL"])
fi

if test x$use_tests = xyes; then

  if test x$HEXDUMP = x; then
    AC_MSG_ERROR(hexdump is required for tests)
  fi


  if test x$use_boost = xyes; then

  AX_BOOST_UNIT_TEST_FRAMEWORK

  dnl Determine if -DBOOST_TEST_DYN_LINK is needed
  AC_MSG_CHECKING([for dynamic linked boost test])
  TEMP_LIBS="$LIBS"
  LIBS="$LIBS $BOOST_LDFLAGS $BOOST_UNIT_TEST_FRAMEWORK_LIB"
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
fi

if test x$use_boost = xyes; then

BOOST_LIBS="$BOOST_LDFLAGS $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_PROGRAM_OPTIONS_LIB $BOOST_THREAD_LIB $BOOST_CHRONO_LIB"


dnl If boost (prior to 1.57) was built without c++11, it emulated scoped enums
dnl using c++98 constructs. Unfortunately, this implementation detail leaked into
dnl the abi. This was fixed in 1.57.

dnl When building against that installed version using c++11, the headers pick up
dnl on the native c++11 scoped enum support and enable it, however it will fail to
dnl link. This can be worked around by disabling c++11 scoped enums if linking will
dnl fail.
dnl BOOST_NO_SCOPED_ENUMS was changed to BOOST_NO_CXX11_SCOPED_ENUMS in 1.51.

TEMP_LIBS="$LIBS"
LIBS="$BOOST_LIBS $LIBS"
TEMP_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_MSG_CHECKING([for mismatched boost c++11 scoped enums])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
  #include <boost/config.hpp>
  #include <boost/version.hpp>
  #if !defined(BOOST_NO_SCOPED_ENUMS) && !defined(BOOST_NO_CXX11_SCOPED_ENUMS) && BOOST_VERSION < 105700
  #define BOOST_NO_SCOPED_ENUMS
  #define BOOST_NO_CXX11_SCOPED_ENUMS
  #define CHECK
  #endif
  #include <boost/filesystem.hpp>
  ]],[[
  #if defined(CHECK)
    boost::filesystem::copy_file("foo", "bar");
  #else
    choke;
  #endif
  ]])],
  [AC_MSG_RESULT(mismatched); BOOST_CPPFLAGS="$BOOST_CPPFLAGS -DBOOST_NO_SCOPED_ENUMS -DBOOST_NO_CXX11_SCOPED_ENUMS"], [AC_MSG_RESULT(ok)])
LIBS="$TEMP_LIBS"
CPPFLAGS="$TEMP_CPPFLAGS"

dnl Boost >= 1.50 uses sleep_for rather than the now-deprecated sleep, however
dnl it was broken from 1.50 to 1.52 when backed by nanosleep. Use sleep_for if
dnl a working version is available, else fall back to sleep. sleep was removed
dnl after 1.56.
dnl If neither is available, abort.
TEMP_LIBS="$LIBS"
LIBS="$BOOST_LIBS $LIBS"
TEMP_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
  #include <boost/thread/thread.hpp>
  #include <boost/version.hpp>
  ]],[[
  #if BOOST_VERSION >= 105000 && (!defined(BOOST_HAS_NANOSLEEP) || BOOST_VERSION >= 105200)
      boost::this_thread::sleep_for(boost::chrono::milliseconds(0));
  #else
   choke me
  #endif
  ]])],
  [boost_sleep=yes;
     AC_DEFINE(HAVE_WORKING_BOOST_SLEEP_FOR, 1, [Define this symbol if boost sleep_for works])],
  [boost_sleep=no])
LIBS="$TEMP_LIBS"
CPPFLAGS="$TEMP_CPPFLAGS"

if test x$boost_sleep != xyes; then
TEMP_LIBS="$LIBS"
LIBS="$BOOST_LIBS $LIBS"
TEMP_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
  #include <boost/version.hpp>
  #include <boost/thread.hpp>
  #include <boost/date_time/posix_time/posix_time_types.hpp>
  ]],[[
  #if BOOST_VERSION <= 105600
      boost::this_thread::sleep(boost::posix_time::milliseconds(0));
  #else
   choke me
  #endif
  ]])],
  [boost_sleep=yes; AC_DEFINE(HAVE_WORKING_BOOST_SLEEP, 1, [Define this symbol if boost sleep works])],
  [boost_sleep=no])
LIBS="$TEMP_LIBS"
CPPFLAGS="$TEMP_CPPFLAGS"
fi

if test x$boost_sleep != xyes; then
  AC_MSG_ERROR(No working boost sleep implementation found.)
fi

fi

if test x$use_pkgconfig = xyes; then
  : dnl
  m4_ifdef(
    [PKG_CHECK_MODULES],
    [
      PKG_CHECK_MODULES([SSL], [libssl],, [AC_MSG_ERROR(openssl not found.)])
      PKG_CHECK_MODULES([CRYPTO], [libcrypto],,[AC_MSG_ERROR(libcrypto not found.)])
      BITCOIN_QT_CHECK([PKG_CHECK_MODULES([PROTOBUF], [protobuf], [have_protobuf=yes], [BITCOIN_QT_FAIL(libprotobuf not found)])])
      if test x$use_qr != xno; then
        BITCOIN_QT_CHECK([PKG_CHECK_MODULES([QR], [libqrencode], [have_qrencode=yes], [have_qrencode=no])])
      fi
      if test x$build_bitcoin_utils$build_bitcoind$bitcoin_enable_qt$use_tests != xnononono; then
        PKG_CHECK_MODULES([EVENT], [libevent],, [AC_MSG_ERROR(libevent not found.)])
        if test x$TARGET_OS != xwindows; then
          PKG_CHECK_MODULES([EVENT_PTHREADS], [libevent_pthreads],, [AC_MSG_ERROR(libevent_pthreads not found.)])
        fi
      fi

      if test "x$use_zmq" = "xyes"; then
        PKG_CHECK_MODULES([ZMQ],[libzmq >= 4],
          [AC_DEFINE([ENABLE_ZMQ],[1],[Define to 1 to enable ZMQ functions])],
          [AC_DEFINE([ENABLE_ZMQ],[0],[Define to 1 to enable ZMQ functions])
           AC_MSG_WARN([libzmq version 4.x or greater not found, disabling])
           use_zmq=no])
      else
          AC_DEFINE_UNQUOTED([ENABLE_ZMQ],[0],[Define to 1 to enable ZMQ functions])
      fi
    ]
  )
else
  AC_CHECK_HEADER([openssl/crypto.h],,AC_MSG_ERROR(libcrypto headers missing))
  AC_CHECK_LIB([crypto],      [main],CRYPTO_LIBS=-lcrypto, AC_MSG_ERROR(libcrypto missing))

  AC_CHECK_HEADER([openssl/ssl.h],, AC_MSG_ERROR(libssl headers missing),)
  AC_CHECK_LIB([ssl],         [main],SSL_LIBS=-lssl, AC_MSG_ERROR(libssl missing))

  if test x$build_bitcoin_utils$build_bitcoind$bitcoin_enable_qt$use_tests != xnononono; then
    AC_CHECK_HEADER([event2/event.h],, AC_MSG_ERROR(libevent headers missing),)
    AC_CHECK_LIB([event],[main],EVENT_LIBS=-levent,AC_MSG_ERROR(libevent missing))
    if test x$TARGET_OS != xwindows; then
      AC_CHECK_LIB([event_pthreads],[main],EVENT_PTHREADS_LIBS=-levent_pthreads,AC_MSG_ERROR(libevent_pthreads missing))
    fi
  fi

  if test "x$use_zmq" = "xyes"; then
     AC_CHECK_HEADER([zmq.h],
       [AC_DEFINE([ENABLE_ZMQ],[1],[Define to 1 to enable ZMQ functions])],
       [AC_MSG_WARN([zmq.h not found, disabling zmq support])
        use_zmq=no
        AC_DEFINE([ENABLE_ZMQ],[0],[Define to 1 to enable ZMQ functions])])
     AC_CHECK_LIB([zmq],[zmq_ctx_shutdown],ZMQ_LIBS=-lzmq,
       [AC_MSG_WARN([libzmq >= 4.0 not found, disabling zmq support])
        use_zmq=no
        AC_DEFINE([ENABLE_ZMQ],[0],[Define to 1 to enable ZMQ functions])])
  else
    AC_DEFINE_UNQUOTED([ENABLE_ZMQ],[0],[Define to 1 to enable ZMQ functions])
  fi

  if test "x$use_zmq" = "xyes"; then
    dnl Assume libzmq was built for static linking
    case $host in
      *mingw*)
        ZMQ_CFLAGS="$ZMQ_CFLAGS -DZMQ_STATIC"
      ;;
    esac
  fi

  BITCOIN_QT_CHECK(AC_CHECK_LIB([protobuf] ,[main],[PROTOBUF_LIBS=-lprotobuf], BITCOIN_QT_FAIL(libprotobuf not found)))
  if test x$use_qr != xno; then
    BITCOIN_QT_CHECK([AC_CHECK_LIB([qrencode], [main],[QR_LIBS=-lqrencode], [have_qrencode=no])])
    BITCOIN_QT_CHECK([AC_CHECK_HEADER([qrencode.h],, have_qrencode=no)])
  fi
fi

save_CXXFLAGS="${CXXFLAGS}"
CXXFLAGS="${CXXFLAGS} ${CRYPTO_CFLAGS} ${SSL_CFLAGS}"
AC_CHECK_DECLS([EVP_MD_CTX_new],,,[AC_INCLUDES_DEFAULT
#include <openssl/x509_vfy.h>
])
CXXFLAGS="${save_CXXFLAGS}"

dnl univalue check

need_bundled_univalue=yes

if test x$build_bitcoin_utils$build_bitcoind$bitcoin_enable_qt$use_tests$use_bench = xnonononono; then
  need_bundled_univalue=no
else

if test x$system_univalue != xno ; then
  found_univalue=no
  if test x$use_pkgconfig = xyes; then
    : #NOP
    m4_ifdef(
      [PKG_CHECK_MODULES],
      [
        PKG_CHECK_MODULES([UNIVALUE],[libunivalue >= 1.0.4],[found_univalue=yes],[true])
      ]
    )
  else
    AC_CHECK_HEADER([univalue.h],[
      AC_CHECK_LIB([univalue],  [main],[
        UNIVALUE_LIBS=-lunivalue
        found_univalue=yes
      ],[true])
    ],[true])
  fi

  if test x$found_univalue = xyes ; then
    system_univalue=yes
    need_bundled_univalue=no
  elif test x$system_univalue = xyes ; then
    AC_MSG_ERROR([univalue not found])
  else
    system_univalue=no
  fi
fi

if test x$need_bundled_univalue = xyes ; then
  UNIVALUE_CFLAGS='-I$(srcdir)/univalue/include'
  UNIVALUE_LIBS='univalue/libunivalue.la'
fi

fi

AM_CONDITIONAL([EMBEDDED_UNIVALUE],[test x$need_bundled_univalue = xyes])
AC_SUBST(UNIVALUE_CFLAGS)
AC_SUBST(UNIVALUE_LIBS)

BITCOIN_QT_PATH_PROGS([PROTOC], [protoc],$protoc_bin_path)

AC_MSG_CHECKING([whether to build bitcoind])
AM_CONDITIONAL([BUILD_BITCOIND], [test x$build_bitcoind = xyes])
AC_MSG_RESULT($build_bitcoind)

AC_MSG_CHECKING([whether to build utils (bitcoin-cli bitcoin-tx)])
AM_CONDITIONAL([BUILD_BITCOIN_UTILS], [test x$build_bitcoin_utils = xyes])
AC_MSG_RESULT($build_bitcoin_utils)

AC_MSG_CHECKING([whether to build libraries])
AM_CONDITIONAL([BUILD_BITCOIN_LIBS], [test x$build_bitcoin_libs = xyes])
if test x$build_bitcoin_libs = xyes; then
  AC_DEFINE(HAVE_CONSENSUS_LIB, 1, [Define this symbol if the consensus lib has been built])
  AC_CONFIG_FILES([libbitcoinconsensus.pc:libbitcoinconsensus.pc.in])
fi
AC_MSG_RESULT($build_bitcoin_libs)

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
if test "x$use_ccache" = "xyes"; then
    AX_CHECK_PREPROC_FLAG([-Qunused-arguments],[CPPFLAGS="-Qunused-arguments $CPPFLAGS"])
fi

dnl enable wallet
AC_MSG_CHECKING([if wallet should be enabled])
if test x$enable_wallet != xno; then
  AC_MSG_RESULT(yes)
  AC_DEFINE_UNQUOTED([ENABLE_WALLET],[1],[Define to 1 to enable wallet functions])

else
  AC_MSG_RESULT(no)
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
      MINIUPNPC_CPPFLAGS="-DSTATICLIB -DMINIUPNP_STATICLIB"
    fi
  else
    AC_MSG_RESULT(no)
  fi
fi

dnl these are only used when qt is enabled
BUILD_TEST_QT=""
if test x$bitcoin_enable_qt != xno; then
  dnl enable dbus support
  AC_MSG_CHECKING([whether to build GUI with support for D-Bus])
  if test x$bitcoin_enable_qt_dbus != xno; then
    AC_DEFINE([USE_DBUS],[1],[Define if dbus support should be compiled in])
  fi
  AC_MSG_RESULT($bitcoin_enable_qt_dbus)

  dnl enable qr support
  AC_MSG_CHECKING([whether to build GUI with support for QR codes])
  if test x$have_qrencode = xno; then
    if test x$use_qr = xyes; then
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

  if test x$XGETTEXT = x; then
    AC_MSG_WARN("xgettext is required to update qt translations")
  fi

  AC_MSG_CHECKING([whether to build test_bitcoin-qt])
  if test x$use_gui_tests$bitcoin_enable_qt_test = xyesyes; then
    AC_MSG_RESULT([yes])
    BUILD_TEST_QT="yes"
  else
    AC_MSG_RESULT([no])
  fi
fi

AM_CONDITIONAL([ENABLE_ZMQ], [test "x$use_zmq" = "xyes"])

AC_MSG_CHECKING([whether to build test_bitcoin])
if test x$use_tests = xyes; then
  AC_MSG_RESULT([yes])
  BUILD_TEST="yes"
else
  AC_MSG_RESULT([no])
  BUILD_TEST=""
fi

AC_MSG_CHECKING([whether to reduce exports])
if test x$use_reduce_exports = xyes; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

if test x$build_bitcoin_utils$build_bitcoin_libs$build_bitcoind$bitcoin_enable_qt$use_bench$use_tests = xnononononono; then
  AC_MSG_ERROR([No targets! Please specify at least one of: --with-utils --with-libs --with-daemon --with-gui --enable-bench or --enable-tests])
fi

AM_CONDITIONAL([TARGET_DARWIN], [test x$TARGET_OS = xdarwin])
AM_CONDITIONAL([BUILD_DARWIN], [test x$BUILD_OS = xdarwin])
AM_CONDITIONAL([TARGET_WINDOWS], [test x$TARGET_OS = xwindows])
AM_CONDITIONAL([ENABLE_WALLET],[test x$enable_wallet = xyes])
AM_CONDITIONAL([ENABLE_TESTS],[test x$BUILD_TEST = xyes])
AM_CONDITIONAL([ENABLE_QT],[test x$bitcoin_enable_qt = xyes])
AM_CONDITIONAL([ENABLE_QT_TESTS],[test x$BUILD_TEST_QT = xyes])
AM_CONDITIONAL([ENABLE_BENCH],[test x$use_bench = xyes])
AM_CONDITIONAL([USE_QRCODE], [test x$use_qr = xyes])
AM_CONDITIONAL([USE_LCOV],[test x$use_lcov = xyes])
AM_CONDITIONAL([GLIBC_BACK_COMPAT],[test x$use_glibc_compat = xyes])
AM_CONDITIONAL([HARDEN],[test x$use_hardening = xyes])
AM_CONDITIONAL([ENABLE_HWCRC32],[test x$enable_hwcrc32 = xyes])
AM_CONDITIONAL([ENABLE_SSE41],[test x$enable_sse41 = xyes])
AM_CONDITIONAL([ENABLE_AVX2],[test x$enable_avx2 = xyes])
AM_CONDITIONAL([ENABLE_SHANI],[test x$enable_shani = xyes])
AM_CONDITIONAL([USE_ASM],[test x$use_asm = xyes])

AC_DEFINE(CLIENT_VERSION_MAJOR, _CLIENT_VERSION_MAJOR, [Major version])
AC_DEFINE(CLIENT_VERSION_MINOR, _CLIENT_VERSION_MINOR, [Minor version])
AC_DEFINE(CLIENT_VERSION_REVISION, _CLIENT_VERSION_REVISION, [Build revision])
AC_DEFINE(CLIENT_VERSION_BUILD, _CLIENT_VERSION_BUILD, [Version Build])
AC_DEFINE(CLIENT_VERSION_IS_RELEASE, _CLIENT_VERSION_IS_RELEASE, [Version is release])
AC_DEFINE(COPYRIGHT_YEAR, _COPYRIGHT_YEAR, [Copyright year])
AC_DEFINE(COPYRIGHT_HOLDERS, "_COPYRIGHT_HOLDERS", [Copyright holder(s) before %s replacement])
AC_DEFINE(COPYRIGHT_HOLDERS_SUBSTITUTION, "_COPYRIGHT_HOLDERS_SUBSTITUTION", [Replacement for %s in copyright holders string])
define(_COPYRIGHT_HOLDERS_FINAL, [patsubst(_COPYRIGHT_HOLDERS, [%s], [_COPYRIGHT_HOLDERS_SUBSTITUTION])])
AC_DEFINE(COPYRIGHT_HOLDERS_FINAL, "_COPYRIGHT_HOLDERS_FINAL", [Copyright holder(s)])
AC_SUBST(CLIENT_VERSION_MAJOR, _CLIENT_VERSION_MAJOR)
AC_SUBST(CLIENT_VERSION_MINOR, _CLIENT_VERSION_MINOR)
AC_SUBST(CLIENT_VERSION_REVISION, _CLIENT_VERSION_REVISION)
AC_SUBST(CLIENT_VERSION_BUILD, _CLIENT_VERSION_BUILD)
AC_SUBST(CLIENT_VERSION_IS_RELEASE, _CLIENT_VERSION_IS_RELEASE)
AC_SUBST(COPYRIGHT_YEAR, _COPYRIGHT_YEAR)
AC_SUBST(COPYRIGHT_HOLDERS, "_COPYRIGHT_HOLDERS")
AC_SUBST(COPYRIGHT_HOLDERS_SUBSTITUTION, "_COPYRIGHT_HOLDERS_SUBSTITUTION")
AC_SUBST(COPYRIGHT_HOLDERS_FINAL, "_COPYRIGHT_HOLDERS_FINAL")
AC_SUBST(BITCOIN_DAEMON_NAME)
AC_SUBST(BITCOIN_GUI_NAME)
AC_SUBST(BITCOIN_CLI_NAME)
AC_SUBST(BITCOIN_TX_NAME)

AC_SUBST(RELDFLAGS)
AC_SUBST(DEBUG_CPPFLAGS)
AC_SUBST(WARN_CXXFLAGS)
AC_SUBST(NOWARN_CXXFLAGS)
AC_SUBST(DEBUG_CXXFLAGS)
AC_SUBST(ERROR_CXXFLAGS)
AC_SUBST(GPROF_CXXFLAGS)
AC_SUBST(GPROF_LDFLAGS)
AC_SUBST(HARDENED_CXXFLAGS)
AC_SUBST(HARDENED_CPPFLAGS)
AC_SUBST(HARDENED_LDFLAGS)
AC_SUBST(PIC_FLAGS)
AC_SUBST(PIE_FLAGS)
AC_SUBST(SANITIZER_CXXFLAGS)
AC_SUBST(SANITIZER_LDFLAGS)
AC_SUBST(SSE42_CXXFLAGS)
AC_SUBST(SSE41_CXXFLAGS)
AC_SUBST(AVX2_CXXFLAGS)
AC_SUBST(SHANI_CXXFLAGS)
AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(USE_UPNP)
AC_SUBST(USE_QRCODE)
AC_SUBST(BOOST_LIBS)
AC_SUBST(TESTDEFS)
AC_SUBST(LEVELDB_TARGET_FLAGS)
AC_SUBST(MINIUPNPC_CPPFLAGS)
AC_SUBST(MINIUPNPC_LIBS)
AC_SUBST(CRYPTO_LIBS)
AC_SUBST(SSL_LIBS)
AC_SUBST(EVENT_LIBS)
AC_SUBST(EVENT_PTHREADS_LIBS)
AC_SUBST(ZMQ_LIBS)
AC_SUBST(PROTOBUF_LIBS)
AC_SUBST(QR_LIBS)
AC_CONFIG_FILES([Makefile src/Makefile doc/man/Makefile share/setup.nsi share/qt/Info.plist test/config.ini])
AC_CONFIG_FILES([contrib/devtools/split-debug.sh],[chmod +x contrib/devtools/split-debug.sh])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/Doxyfile])])
AC_CONFIG_LINKS([contrib/filter-lcov.py:contrib/filter-lcov.py])
AC_CONFIG_LINKS([test/functional/test_runner.py:test/functional/test_runner.py])
AC_CONFIG_LINKS([test/util/bitcoin-util-test.py:test/util/bitcoin-util-test.py])
AC_CONFIG_LINKS([test/util/rpcauth-test.py:test/util/rpcauth-test.py])

dnl boost's m4 checks do something really nasty: they export these vars. As a
dnl result, they leak into secp256k1's configure and crazy things happen.
dnl Until this is fixed upstream and we've synced, we'll just un-export them.
CPPFLAGS_TEMP="$CPPFLAGS"
unset CPPFLAGS
CPPFLAGS="$CPPFLAGS_TEMP"

LDFLAGS_TEMP="$LDFLAGS"
unset LDFLAGS
LDFLAGS="$LDFLAGS_TEMP"

LIBS_TEMP="$LIBS"
unset LIBS
LIBS="$LIBS_TEMP"

PKGCONFIG_PATH_TEMP="$PKG_CONFIG_PATH"
unset PKG_CONFIG_PATH
PKG_CONFIG_PATH="$PKGCONFIG_PATH_TEMP"

PKGCONFIG_LIBDIR_TEMP="$PKG_CONFIG_LIBDIR"
unset PKG_CONFIG_LIBDIR
PKG_CONFIG_LIBDIR="$PKGCONFIG_LIBDIR_TEMP"

if test x$need_bundled_univalue = xyes; then
  AC_CONFIG_SUBDIRS([src/univalue])
fi

ac_configure_args="${ac_configure_args} --disable-shared --with-pic --with-bignum=no --enable-module-recovery --disable-jni"
AC_CONFIG_SUBDIRS([src/secp256k1])

AC_OUTPUT

dnl Taken from https://wiki.debian.org/RpathIssue
case $host in
   *-*-linux-gnu)
     AC_MSG_RESULT([Fixing libtool for -rpath problems.])
     sed < libtool > libtool-2 \
     's/^hardcode_libdir_flag_spec.*$'/'hardcode_libdir_flag_spec=" -D__LIBTOOL_IS_A_FOOL__ "/'
     mv libtool-2 libtool
     chmod 755 libtool
   ;;
esac

dnl Replace the BUILDDIR path with the correct Windows path if compiling on Native Windows
case ${OS} in
   *Windows*)
     sed  's/BUILDDIR="\/\([[a-z]]\)/BUILDDIR="\1:/'  test/config.ini > test/config-2.ini
     mv test/config-2.ini test/config.ini
   ;;
esac

echo
echo "Options used to compile and link:"
echo "  with wallet   = $enable_wallet"
echo "  with gui / qt = $bitcoin_enable_qt"
if test x$bitcoin_enable_qt != xno; then
    echo "    with qr     = $use_qr"
fi
echo "  with zmq      = $use_zmq"
echo "  with test     = $use_tests"
echo "  with bench    = $use_bench"
echo "  with upnp     = $use_upnp"
echo "  use asm       = $use_asm"
echo "  sanitizers    = $use_sanitizers"
echo "  debug enabled = $enable_debug"
echo "  gprof enabled = $enable_gprof"
echo "  werror        = $enable_werror"
echo
echo "  target os     = $TARGET_OS"
echo "  build os      = $BUILD_OS"
echo
echo "  CC            = $CC"
echo "  CFLAGS        = $CFLAGS"
echo "  CPPFLAGS      = $DEBUG_CPPFLAGS $HARDENED_CPPFLAGS $CPPFLAGS"
echo "  CXX           = $CXX"
echo "  CXXFLAGS      = $DEBUG_CXXFLAGS $HARDENED_CXXFLAGS $WARN_CXXFLAGS $NOWARN_CXXFLAGS $ERROR_CXXFLAGS $GPROF_CXXFLAGS $CXXFLAGS"
echo "  LDFLAGS       = $PTHREAD_CFLAGS $HARDENED_LDFLAGS $GPROF_LDFLAGS $LDFLAGS"
echo "  ARFLAGS       = $ARFLAGS"
echo
