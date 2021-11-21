AC_PREREQ([2.69])
define(_CLIENT_VERSION_MAJOR, 22)
define(_CLIENT_VERSION_MINOR, 99)
define(_CLIENT_VERSION_BUILD, 0)
define(_CLIENT_VERSION_RC, 0)
define(_CLIENT_VERSION_IS_RELEASE, false)
define(_COPYRIGHT_YEAR, 2021)
define(_COPYRIGHT_HOLDERS,[The %s developers])
define(_COPYRIGHT_HOLDERS_SUBSTITUTION,[[Bitcoin Core]])
AC_INIT([Bitcoin Core],m4_join([.], _CLIENT_VERSION_MAJOR, _CLIENT_VERSION_MINOR, _CLIENT_VERSION_BUILD)m4_if(_CLIENT_VERSION_RC, [0], [], [rc]_CLIENT_VERSION_RC),[https://github.com/bitcoin/bitcoin/issues],[bitcoin],[https://bitcoincore.org/])
AC_CONFIG_SRCDIR([src/validation.cpp])
AC_CONFIG_HEADERS([src/config/bitcoin-config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])

m4_ifndef([PKG_PROG_PKG_CONFIG], [AC_MSG_ERROR([PKG_PROG_PKG_CONFIG macro not found. Please install pkg-config and re-run autogen.sh])])
PKG_PROG_PKG_CONFIG
if test "x$PKG_CONFIG" = x; then
  AC_MSG_ERROR([pkg-config not found])
fi

BITCOIN_DAEMON_NAME=bitcoind
BITCOIN_GUI_NAME=bitcoin-qt
BITCOIN_CLI_NAME=bitcoin-cli
BITCOIN_TX_NAME=bitcoin-tx
BITCOIN_UTIL_NAME=bitcoin-util
BITCOIN_WALLET_TOOL_NAME=bitcoin-wallet
dnl Multi Process
BITCOIN_MP_NODE_NAME=bitcoin-node
BITCOIN_MP_GUI_NAME=bitcoin-gui

dnl Unless the user specified ARFLAGS, force it to be cr
AC_ARG_VAR([ARFLAGS], [Flags for the archiver, defaults to <cr> if not set])
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
AM_INIT_AUTOMAKE([1.13 no-define subdir-objects foreign])

dnl faketime messes with timestamps and causes configure to be re-run.
dnl --disable-maintainer-mode can be used to bypass this.
AM_MAINTAINER_MODE([enable])

dnl make the compilation flags quiet unless V=1 is used
AM_SILENT_RULES([yes])

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

AC_ARG_WITH([seccomp],
  [AS_HELP_STRING([--with-seccomp],
  [enable experimental syscall sandbox feature (-sandbox), default is yes if seccomp-bpf is detected under Linux x86_64])],
  [seccomp_found=$withval],
  [seccomp_found=auto])

dnl Require C++17 compiler (no GNU extensions)
AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])

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

dnl Since libtool 1.5.2 (released 2004-01-25), on Linux libtool no longer
dnl sets RPATH for any directories in the dynamic linker search path.
dnl See more: https://wiki.debian.org/RpathIssue
LT_PREREQ([1.5.2])
dnl Libtool init checks.
LT_INIT([pic-only])

dnl Check/return PATH for base programs.
AC_PATH_TOOL([AR], [ar])
AC_PATH_TOOL([RANLIB], [ranlib])
AC_PATH_TOOL([STRIP], [strip])
AC_PATH_TOOL([GCOV], [gcov])
AC_PATH_TOOL([LLVM_COV], [llvm-cov])
AC_PATH_PROG([LCOV], [lcov])
dnl Python 3.6 is specified in .python-version and should be used if available, see doc/dependencies.md
AC_PATH_PROGS([PYTHON], [python3.6 python3.7 python3.8 python3.9 python3.10 python3.11 python3 python])
AC_PATH_PROG([GENHTML], [genhtml])
AC_PATH_PROG([GIT], [git])
AC_PATH_PROG([CCACHE], [ccache])
AC_PATH_PROG([XGETTEXT], [xgettext])
AC_PATH_PROG([HEXDUMP], [hexdump])
AC_PATH_TOOL([OBJCOPY], [objcopy])
AC_PATH_PROG([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AC_ARG_VAR([PYTHONPATH], [Augments the default search path for python module files])

AC_ARG_ENABLE([wallet],
  [AS_HELP_STRING([--disable-wallet],
  [disable wallet (enabled by default)])],
  [enable_wallet=$enableval],
  [enable_wallet=auto])

AC_ARG_WITH([sqlite],
  [AS_HELP_STRING([--with-sqlite=yes|no|auto],
  [enable sqlite wallet support (default: auto, i.e., enabled if wallet is enabled and sqlite is found)])],
  [use_sqlite=$withval],
  [use_sqlite=auto])

AC_ARG_WITH([bdb],
  [AS_HELP_STRING([--without-bdb],
  [disable bdb wallet support (default is enabled if wallet is enabled)])],
  [use_bdb=$withval],
  [use_bdb=auto])

AC_ARG_ENABLE([ebpf],
  [AS_HELP_STRING([--enable-ebpf],
  [enable eBPF tracing (default is yes if sys/sdt.h is found)])],
  [use_ebpf=$enableval],
  [use_ebpf=yes])

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

AC_ARG_WITH([natpmp],
            [AS_HELP_STRING([--with-natpmp],
                            [enable NAT-PMP (default is yes if libnatpmp is found)])],
            [use_natpmp=$withval],
            [use_natpmp=auto])

AC_ARG_ENABLE([natpmp-default],
              [AS_HELP_STRING([--enable-natpmp-default],
                              [if NAT-PMP is enabled, turn it on at startup (default is no)])],
              [use_natpmp_default=$enableval],
              [use_natpmp_default=no])

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

AC_ARG_ENABLE([fuzz],
    AS_HELP_STRING([--enable-fuzz],
    [build for fuzzing (default no). enabling this will disable all other targets and override --{enable,disable}-fuzz-binary]),
    [enable_fuzz=$enableval],
    [enable_fuzz=no])

AC_ARG_ENABLE([fuzz-binary],
    AS_HELP_STRING([--enable-fuzz-binary],
    [enable building of fuzz binary (default yes).]),
    [enable_fuzz_binary=$enableval],
    [enable_fuzz_binary=yes])

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

dnl Suppress warnings from external headers (e.g. Boost, Qt).
dnl May be useful if warnings from external headers clutter the build output
dnl too much, so that it becomes difficult to spot Bitcoin Core warnings
dnl or if they cause a build failure with --enable-werror.
AC_ARG_ENABLE([suppress-external-warnings],
  [AS_HELP_STRING([--enable-suppress-external-warnings],
                  [Suppress warnings from external headers (default is no)])],
  [suppress_external_warnings=$enableval],
  [suppress_external_warnings=no])

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

AC_ARG_ENABLE([threadlocal],
  [AS_HELP_STRING([--enable-threadlocal],
  [enable features that depend on the c++ thread_local keyword (currently just thread names in debug logs). (default is to enable if there is platform support)])],
  [use_thread_local=$enableval],
  [use_thread_local=auto])

AC_ARG_ENABLE([asm],
  [AS_HELP_STRING([--disable-asm],
  [disable assembly routines (enabled by default)])],
  [use_asm=$enableval],
  [use_asm=yes])

if test "x$use_asm" = xyes; then
  AC_DEFINE([USE_ASM], [1], [Define this symbol to build in assembly routines])
fi

AC_ARG_ENABLE([zmq],
  [AS_HELP_STRING([--disable-zmq],
  [disable ZMQ notifications])],
  [use_zmq=$enableval],
  [use_zmq=yes])

AC_ARG_WITH([libmultiprocess],
  [AS_HELP_STRING([--with-libmultiprocess=yes|no|auto],
  [Build with libmultiprocess library. (default: auto, i.e. detect with pkg-config)])],
  [with_libmultiprocess=$withval],
  [with_libmultiprocess=auto])

AC_ARG_WITH([mpgen],
  [AS_HELP_STRING([--with-mpgen=yes|no|auto|PREFIX],
  [Build with libmultiprocess codegen tool. Useful to specify different libmultiprocess host system library and build system codegen tool prefixes when cross-compiling (default is host system libmultiprocess prefix)])],
  [with_mpgen=$withval],
  [with_mpgen=auto])

AC_ARG_ENABLE([multiprocess],
  [AS_HELP_STRING([--enable-multiprocess],
  [build multiprocess bitcoin-node, bitcoin-wallet, and bitcoin-gui executables in addition to monolithic bitcoind and bitcoin-qt executables. Requires libmultiprocess library. Experimental (default is no)])],
  [enable_multiprocess=$enableval],
  [enable_multiprocess=no])

AC_ARG_ENABLE(man,
    [AS_HELP_STRING([--disable-man],
                    [do not install man pages (default is to install)])],,
    enable_man=yes)
AM_CONDITIONAL([ENABLE_MAN], [test "$enable_man" != no])

dnl Enable debug
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [use compiler flags and macros suited for debugging (default is no)])],
    [enable_debug=$enableval],
    [enable_debug=no])

dnl Enable different -fsanitize options
AC_ARG_WITH([sanitizers],
    [AS_HELP_STRING([--with-sanitizers],
                    [comma separated list of extra sanitizers to build with (default is none enabled)])],
    [use_sanitizers=$withval])

dnl Enable gprof profiling
AC_ARG_ENABLE([gprof],
    [AS_HELP_STRING([--enable-gprof],
                    [use gprof profiling compiler flags (default is no)])],
    [enable_gprof=$enableval],
    [enable_gprof=no])

dnl Turn warnings into errors
AC_ARG_ENABLE([werror],
    [AS_HELP_STRING([--enable-werror],
                    [Treat compiler warnings as errors (default is no)])],
    [enable_werror=$enableval],
    [enable_werror=no])

AC_ARG_ENABLE([external-signer],
    [AS_HELP_STRING([--enable-external-signer],[compile external signer support (default is yes, requires Boost::Process)])],
    [use_external_signer=$enableval],
    [use_external_signer=yes])

AC_LANG_PUSH([C++])

dnl Check for a flag to turn compiler warnings into errors. This is helpful for checks which may
dnl appear to succeed because by default they merely emit warnings when they fail.
dnl
dnl Note that this is not necessarily a check to see if -Werror is supported, but rather to see if
dnl a compile with -Werror can succeed. This is important because the compiler may already be
dnl warning about something unrelated, for example about some path issue. If that is the case,
dnl -Werror cannot be used because all of those warnings would be turned into errors.
AX_CHECK_COMPILE_FLAG([-Werror], [CXXFLAG_WERROR="-Werror"], [CXXFLAG_WERROR=""])

dnl Check for a flag to turn linker warnings into errors. When flags are passed to linkers via the
dnl compiler driver using a -Wl,-foo flag, linker warnings may be swallowed rather than bubbling up.
dnl See note above, the same applies here as well.
dnl
dnl LDFLAG_WERROR Should only be used when testing -Wl,*
case $host in
 *darwin*)
    AX_CHECK_LINK_FLAG([-Wl,-fatal_warnings], [LDFLAG_WERROR="-Wl,-fatal_warnings"], [LDFLAG_WERROR=""])
    ;;
  *)
    AX_CHECK_LINK_FLAG([-Wl,--fatal-warnings], [LDFLAG_WERROR="-Wl,--fatal-warnings"], [LDFLAG_WERROR=""])
    ;;
esac

if test "x$enable_debug" = xyes; then
  dnl Clear default -g -O2 flags
  if test "x$CXXFLAGS_overridden" = xno; then
	CXXFLAGS=""
  fi

  dnl Disable all optimizations
  AX_CHECK_COMPILE_FLAG([-O0], [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -O0"], [], [$CXXFLAG_WERROR])

  dnl Prefer -g3, fall back to -g if that is unavailable.
  AX_CHECK_COMPILE_FLAG(
    [-g3],
    [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -g3"],
    [AX_CHECK_COMPILE_FLAG([-g], [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -g"], [], [$CXXFLAG_WERROR])],
    [$CXXFLAG_WERROR])

  AX_CHECK_PREPROC_FLAG([-DDEBUG], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DDEBUG"], [], [$CXXFLAG_WERROR])
  AX_CHECK_PREPROC_FLAG([-DDEBUG_LOCKORDER], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DDEBUG_LOCKORDER"], [], [$CXXFLAG_WERROR])
  AX_CHECK_PREPROC_FLAG([-DABORT_ON_FAILED_ASSUME], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DABORT_ON_FAILED_ASSUME"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-ftrapv], [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -ftrapv"], [], [$CXXFLAG_WERROR])
fi

if test x$use_sanitizers != x; then
  dnl First check if the compiler accepts flags. If an incompatible pair like
  dnl -fsanitize=address,thread is used here, this check will fail. This will also
  dnl fail if a bad argument is passed, e.g. -fsanitize=undfeined
  AX_CHECK_COMPILE_FLAG(
    [-fsanitize=$use_sanitizers],
    [SANITIZER_CXXFLAGS="-fsanitize=$use_sanitizers"],
    [AC_MSG_ERROR([compiler did not accept requested flags])])

  dnl Some compilers (e.g. GCC) require additional libraries like libasan,
  dnl libtsan, libubsan, etc. Make sure linking still works with the sanitize
  dnl flag. This is a separate check so we can give a better error message when
  dnl the sanitize flags are supported by the compiler but the actual sanitizer
  dnl libs are missing.
  AX_CHECK_LINK_FLAG(
    [-fsanitize=$use_sanitizers],
    [SANITIZER_LDFLAGS="-fsanitize=$use_sanitizers"],
    [AC_MSG_ERROR([linker did not accept requested flags, you are missing required libraries])],
    [],
    [AC_LANG_PROGRAM([[
    #include <cstdint>
    #include <cstddef>
    extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) { return 0; }
    __attribute__((weak)) // allow for libFuzzer linking
    ]],[[]])])
fi

ERROR_CXXFLAGS=
if test "x$enable_werror" = "xyes"; then
  if test "x$CXXFLAG_WERROR" = "x"; then
    AC_MSG_ERROR([enable-werror set but -Werror is not usable])
  fi
  ERROR_CXXFLAGS=$CXXFLAG_WERROR

  dnl -Wreturn-type is broken in GCC for MinGW-w64.
  dnl https://sourceforge.net/p/mingw-w64/bugs/306/
  AX_CHECK_COMPILE_FLAG([-Werror=return-type], [], [ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Wno-error=return-type"], [$CXXFLAG_WERROR],
                        [AC_LANG_SOURCE([[#include <cassert>
                                          int f(){ assert(false); }]])])
fi

if test "x$CXXFLAGS_overridden" = "xno"; then
  AX_CHECK_COMPILE_FLAG([-Wall], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wall"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wextra], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wextra"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wgnu], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wgnu"], [], [$CXXFLAG_WERROR])
  dnl some compilers will ignore -Wformat-security without -Wformat, so just combine the two here.
  AX_CHECK_COMPILE_FLAG([-Wformat -Wformat-security], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wformat -Wformat-security"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wvla], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wvla"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wshadow-field], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wshadow-field"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wthread-safety], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wthread-safety"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wloop-analysis], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wrange-loop-analysis"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wredundant-decls], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wredundant-decls"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wunused-member-function], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wunused-member-function"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wdate-time], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wdate-time"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wconditional-uninitialized], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wconditional-uninitialized"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wduplicated-branches], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wduplicated-branches"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wduplicated-cond], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wduplicated-cond"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wlogical-op], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wlogical-op"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Woverloaded-virtual], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Woverloaded-virtual"], [], [$CXXFLAG_WERROR])
  dnl -Wsuggest-override is broken with GCC before 9.2
  dnl https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78010
  AX_CHECK_COMPILE_FLAG([-Wsuggest-override], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wsuggest-override"], [], [$CXXFLAG_WERROR],
                        [AC_LANG_SOURCE([[struct A { virtual void f(); }; struct B : A { void f() final; };]])])
  AX_CHECK_COMPILE_FLAG([-Wunreachable-code-loop-increment], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wunreachable-code-loop-increment"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wimplicit-fallthrough], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wimplicit-fallthrough"], [], [$CXXFLAG_WERROR])

  if test x$suppress_external_warnings != xno ; then
    AX_CHECK_COMPILE_FLAG([-Wdocumentation], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wdocumentation"], [], [$CXXFLAG_WERROR])
  fi

  dnl Some compilers (gcc) ignore unknown -Wno-* options, but warn about all
  dnl unknown options if any other warning is produced. Test the -Wfoo case, and
  dnl set the -Wno-foo case if it works.
  AX_CHECK_COMPILE_FLAG([-Wunused-parameter], [NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-unused-parameter"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wself-assign], [NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-self-assign"], [], [$CXXFLAG_WERROR])
  if test x$suppress_external_warnings != xyes ; then
    AX_CHECK_COMPILE_FLAG([-Wdeprecated-copy], [NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-deprecated-copy"], [], [$CXXFLAG_WERROR])
  fi
fi

dnl Don't allow extended (non-ASCII) symbols in identifiers. This is easier for code review.
AX_CHECK_COMPILE_FLAG([-fno-extended-identifiers], [CXXFLAGS="$CXXFLAGS -fno-extended-identifiers"], [], [$CXXFLAG_WERROR])

enable_sse42=no
enable_sse41=no
enable_avx2=no
enable_shani=no

if test "x$use_asm" = "xyes"; then

dnl Check for optional instruction set support. Enabling these does _not_ imply that all code will
dnl be compiled with them, rather that specific objects/libs may use them after checking for runtime
dnl compatibility.

dnl x86
AX_CHECK_COMPILE_FLAG([-msse4.2], [SSE42_CXXFLAGS="-msse4.2"], [], [$CXXFLAG_WERROR])
AX_CHECK_COMPILE_FLAG([-msse4.1], [SSE41_CXXFLAGS="-msse4.1"], [], [$CXXFLAG_WERROR])
AX_CHECK_COMPILE_FLAG([-mavx -mavx2], [AVX2_CXXFLAGS="-mavx -mavx2"], [], [$CXXFLAG_WERROR])
AX_CHECK_COMPILE_FLAG([-msse4 -msha], [SHANI_CXXFLAGS="-msse4 -msha"], [], [$CXXFLAG_WERROR])

enable_clmul=
AX_CHECK_COMPILE_FLAG([-mpclmul], [enable_clmul=yes], [], [$CXXFLAG_WERROR], [AC_LANG_PROGRAM([
  #include <stdint.h>
  #include <x86intrin.h>
], [
  __m128i a = _mm_cvtsi64_si128((uint64_t)7);
  __m128i b = _mm_clmulepi64_si128(a, a, 37);
  __m128i c = _mm_srli_epi64(b, 41);
  __m128i d = _mm_xor_si128(b, c);
  uint64_t e = _mm_cvtsi128_si64(d);
  return e == 0;
])])

if test x$enable_clmul = xyes; then
  CLMUL_CXXFLAGS="-mpclmul"
  AC_DEFINE([HAVE_CLMUL], [1], [Define this symbol if clmul instructions can be used])
fi

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $SSE42_CXXFLAGS"
AC_MSG_CHECKING([for SSE4.2 intrinsics])
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
 [ AC_MSG_RESULT([yes]); enable_sse42=yes],
 [ AC_MSG_RESULT([no])]
)
CXXFLAGS="$TEMP_CXXFLAGS"

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $SSE41_CXXFLAGS"
AC_MSG_CHECKING([for SSE4.1 intrinsics])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <stdint.h>
    #include <immintrin.h>
  ]],[[
    __m128i l = _mm_set1_epi32(0);
    return _mm_extract_epi32(l, 3);
  ]])],
 [ AC_MSG_RESULT([yes]); enable_sse41=yes; AC_DEFINE([ENABLE_SSE41], [1], [Define this symbol to build code that uses SSE4.1 intrinsics]) ],
 [ AC_MSG_RESULT([no])]
)
CXXFLAGS="$TEMP_CXXFLAGS"

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $AVX2_CXXFLAGS"
AC_MSG_CHECKING([for AVX2 intrinsics])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <stdint.h>
    #include <immintrin.h>
  ]],[[
    __m256i l = _mm256_set1_epi32(0);
    return _mm256_extract_epi32(l, 7);
  ]])],
 [ AC_MSG_RESULT([yes]); enable_avx2=yes; AC_DEFINE([ENABLE_AVX2], [1], [Define this symbol to build code that uses AVX2 intrinsics]) ],
 [ AC_MSG_RESULT([no])]
)
CXXFLAGS="$TEMP_CXXFLAGS"

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $SHANI_CXXFLAGS"
AC_MSG_CHECKING([for SHA-NI intrinsics])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <stdint.h>
    #include <immintrin.h>
  ]],[[
    __m128i i = _mm_set1_epi32(0);
    __m128i j = _mm_set1_epi32(1);
    __m128i k = _mm_set1_epi32(2);
    return _mm_extract_epi32(_mm_sha256rnds2_epu32(i, i, k), 0);
  ]])],
 [ AC_MSG_RESULT([yes]); enable_shani=yes; AC_DEFINE([ENABLE_SHANI], [1], [Define this symbol to build code that uses SHA-NI intrinsics]) ],
 [ AC_MSG_RESULT([no])]
)
CXXFLAGS="$TEMP_CXXFLAGS"

# ARM
AX_CHECK_COMPILE_FLAG([-march=armv8-a+crc+crypto], [ARM_CRC_CXXFLAGS="-march=armv8-a+crc+crypto"], [], [$CXXFLAG_WERROR])

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $ARM_CRC_CXXFLAGS"
AC_MSG_CHECKING([for AArch64 CRC32 intrinsics])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <arm_acle.h>
    #include <arm_neon.h>
  ]],[[
#ifdef __aarch64__
    __crc32cb(0, 0); __crc32ch(0, 0); __crc32cw(0, 0); __crc32cd(0, 0);
    vmull_p64(0, 0);
#else
#error "crc32c library does not support hardware acceleration on 32-bit ARM"
#endif
  ]])],
 [ AC_MSG_RESULT([yes]); enable_arm_crc=yes; ],
 [ AC_MSG_RESULT([no])]
)
CXXFLAGS="$TEMP_CXXFLAGS"

fi

CPPFLAGS="$CPPFLAGS -DHAVE_BUILD_INFO"

AC_ARG_WITH([utils],
  [AS_HELP_STRING([--with-utils],
  [build bitcoin-cli bitcoin-tx bitcoin-util bitcoin-wallet (default=yes)])],
  [build_bitcoin_utils=$withval],
  [build_bitcoin_utils=yes])

AC_ARG_ENABLE([util-cli],
  [AS_HELP_STRING([--enable-util-cli],
  [build bitcoin-cli])],
  [build_bitcoin_cli=$enableval],
  [build_bitcoin_cli=$build_bitcoin_utils])

AC_ARG_ENABLE([util-tx],
  [AS_HELP_STRING([--enable-util-tx],
  [build bitcoin-tx])],
  [build_bitcoin_tx=$enableval],
  [build_bitcoin_tx=$build_bitcoin_utils])

AC_ARG_ENABLE([util-wallet],
  [AS_HELP_STRING([--enable-util-wallet],
  [build bitcoin-wallet])],
  [build_bitcoin_wallet=$enableval],
  [build_bitcoin_wallet=$build_bitcoin_utils])

AC_ARG_ENABLE([util-util],
  [AS_HELP_STRING([--enable-util-util],
  [build bitcoin-util])],
  [build_bitcoin_util=$enableval],
  [build_bitcoin_util=$build_bitcoin_utils])

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

case $host in
  *mingw*)
     TARGET_OS=windows
     AC_CHECK_LIB([kernel32], [GetModuleFileNameA],      [], [AC_MSG_ERROR([libkernel32 missing])])
     AC_CHECK_LIB([user32],   [main],                    [], [AC_MSG_ERROR([libuser32 missing])])
     AC_CHECK_LIB([gdi32],    [main],                    [], [AC_MSG_ERROR([libgdi32 missing])])
     AC_CHECK_LIB([comdlg32], [main],                    [], [AC_MSG_ERROR([libcomdlg32 missing])])
     AC_CHECK_LIB([winmm],    [main],                    [], [AC_MSG_ERROR([libwinmm missing])])
     AC_CHECK_LIB([shell32],  [SHGetSpecialFolderPathW], [], [AC_MSG_ERROR([libshell32 missing])])
     AC_CHECK_LIB([comctl32], [main],                    [], [AC_MSG_ERROR([libcomctl32 missing])])
     AC_CHECK_LIB([ole32],    [CoCreateInstance],        [], [AC_MSG_ERROR([libole32 missing])])
     AC_CHECK_LIB([oleaut32], [main],                    [], [AC_MSG_ERROR([liboleaut32 missing])])
     AC_CHECK_LIB([uuid],     [main],                    [], [AC_MSG_ERROR([libuuid missing])])
     AC_CHECK_LIB([advapi32], [CryptAcquireContextW],    [], [AC_MSG_ERROR([libadvapi32 missing])])
     AC_CHECK_LIB([ws2_32],   [WSAStartup],              [], [AC_MSG_ERROR([libws2_32 missing])])
     AC_CHECK_LIB([shlwapi],  [PathRemoveFileSpecW],     [], [AC_MSG_ERROR([libshlwapi missing])])
     AC_CHECK_LIB([iphlpapi], [GetAdaptersAddresses],    [], [AC_MSG_ERROR([libiphlpapi missing])])

     dnl -static is interpreted by libtool, where it has a different meaning.
     dnl In libtool-speak, it's -all-static.
     AX_CHECK_LINK_FLAG([-static], [LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static"])

     AC_PATH_PROG([MAKENSIS], [makensis], [none])
     if test x$MAKENSIS = xnone; then
       AC_MSG_WARN([makensis not found. Cannot create installer.])
     fi

     AC_PATH_TOOL([WINDRES], [windres], [none])
     if test x$WINDRES = xnone; then
       AC_MSG_ERROR([windres not found])
     fi

     CPPFLAGS="$CPPFLAGS -D_MT -DWIN32 -D_WINDOWS -D_WIN32_WINNT=0x0601 -D_WIN32_IE=0x0501 -DWIN32_LEAN_AND_MEAN"

     dnl libtool insists upon adding -nostdlib and a list of objects/libs to link against.
     dnl That breaks our ability to build dll's with static libgcc/libstdc++/libssp. Override
     dnl its command here, with the predeps/postdeps removed, and -static inserted. Postdeps are
     dnl also overridden to prevent their insertion later.
     dnl This should only affect dll's.
     archive_cmds_CXX="\$CC -shared \$libobjs \$deplibs \$compiler_flags -static -o \$output_objdir/\$soname \${wl}--enable-auto-image-base -Xlinker --out-implib -Xlinker \$lib"
     postdeps_CXX=

     dnl We require Windows 7 (NT 6.1) or later
     AX_CHECK_LINK_FLAG([-Wl,--major-subsystem-version -Wl,6 -Wl,--minor-subsystem-version -Wl,1], [LDFLAGS="$LDFLAGS -Wl,--major-subsystem-version -Wl,6 -Wl,--minor-subsystem-version -Wl,1"], [], [$LDFLAG_WERROR])
     ;;
  *darwin*)
     TARGET_OS=darwin
     if  test x$cross_compiling != xyes; then
       BUILD_OS=darwin
       AC_PATH_PROGS([RSVG_CONVERT], [rsvg-convert rsvg], [rsvg-convert])
       AC_CHECK_PROG([BREW], [brew], [brew])
       if test x$BREW = xbrew; then
         dnl These Homebrew packages may be keg-only, meaning that they won't be found
         dnl in expected paths because they may conflict with system files. Ask
         dnl Homebrew where each one is located, then adjust paths accordingly.
         dnl It's safe to add these paths even if the functionality is disabled by
         dnl the user (--without-wallet or --without-gui for example).

         if test "x$use_bdb" != xno && $BREW list --versions berkeley-db@4 >/dev/null && test "x$BDB_CFLAGS" = "x" && test "x$BDB_LIBS" = "x"; then
           bdb_prefix=$($BREW --prefix berkeley-db@4 2>/dev/null)
           dnl This must precede the call to BITCOIN_FIND_BDB48 below.
           BDB_CFLAGS="-I$bdb_prefix/include"
           BDB_LIBS="-L$bdb_prefix/lib -ldb_cxx-4.8"
         fi

         if test "x$use_sqlite" != xno && $BREW list --versions sqlite3 >/dev/null; then
           export PKG_CONFIG_PATH="$($BREW --prefix sqlite3 2>/dev/null)/lib/pkgconfig:$PKG_CONFIG_PATH"
         fi

         if $BREW list --versions qt5 >/dev/null; then
           export PKG_CONFIG_PATH="$($BREW --prefix qt5 2>/dev/null)/lib/pkgconfig:$PKG_CONFIG_PATH"
         fi

         case $host in
           *aarch64*)
             dnl The preferred Homebrew prefix for Apple Silicon is /opt/homebrew.
             dnl Therefore, as we do not use pkg-config to detect miniupnpc and libnatpmp
             dnl packages, we should set the CPPFLAGS and LDFLAGS variables for them
             dnl explicitly.
             if test "x$use_upnp" != xno && $BREW list --versions miniupnpc >/dev/null; then
               miniupnpc_prefix=$($BREW --prefix miniupnpc 2>/dev/null)
               if test "x$suppress_external_warnings" != xno; then
                 CPPFLAGS="$CPPFLAGS -isystem $miniupnpc_prefix/include"
               else
                 CPPFLAGS="$CPPFLAGS -I$miniupnpc_prefix/include"
               fi
               LDFLAGS="$LDFLAGS -L$miniupnpc_prefix/lib"
             fi
             if test "x$use_natpmp" != xno && $BREW list --versions libnatpmp >/dev/null; then
               libnatpmp_prefix=$($BREW --prefix libnatpmp 2>/dev/null)
               if test "x$suppress_external_warnings" != xno; then
                 CPPFLAGS="$CPPFLAGS -isystem $libnatpmp_prefix/include"
               else
                 CPPFLAGS="$CPPFLAGS -I$libnatpmp_prefix/include"
               fi
               LDFLAGS="$LDFLAGS -L$libnatpmp_prefix/lib"
             fi
             ;;
         esac
       fi
     else
       case $build_os in
         *darwin*)
           BUILD_OS=darwin
           ;;
         *)
           AC_PATH_TOOL([DSYMUTIL], [dsymutil], [dsymutil])
           AC_PATH_TOOL([INSTALLNAMETOOL], [install_name_tool], [install_name_tool])
           AC_PATH_TOOL([OTOOL], [otool], [otool])
           AC_PATH_PROGS([XORRISOFS], [xorrisofs], [xorrisofs])
           AC_PATH_PROGS([DMG], [dmg], [dmg])
           AC_PATH_PROGS([RSVG_CONVERT], [rsvg-convert rsvg], [rsvg-convert])
           AC_PATH_PROGS([IMAGEMAGICK_CONVERT], [convert], [convert])
           AC_PATH_PROGS([TIFFCP], [tiffcp], [tiffcp])

           dnl libtool will try to strip the static lib, which is a problem for
           dnl cross-builds because strip attempts to call a hard-coded ld,
           dnl which may not exist in the path. Stripping the .a is not
           dnl necessary, so just disable it.
           old_striplib=
           ;;
       esac
     fi

     AX_CHECK_LINK_FLAG([-Wl,-headerpad_max_install_names], [LDFLAGS="$LDFLAGS -Wl,-headerpad_max_install_names"], [], [$LDFLAG_WERROR])
     CPPFLAGS="$CPPFLAGS -DMAC_OSX -DOBJC_OLD_DISPATCH_PROTOTYPES=0"
     OBJCXXFLAGS="$CXXFLAGS"
     ;;
   *android*)
     dnl make sure android stays above linux for hosts like *linux-android*
     TARGET_OS=android
     case $host in
       *x86_64*)
          ANDROID_ARCH=x86_64
          ;;
        *aarch64*)
          ANDROID_ARCH=arm64-v8a
          ;;
        *armv7a*)
          ANDROID_ARCH=armeabi-v7a
          ;;
        *i686*)
          ANDROID_ARCH=i686
          ;;
        *) AC_MSG_ERROR([Could not determine Android arch]) ;;
      esac
     ;;
   *linux*)
     TARGET_OS=linux
     ;;
esac

if test x$use_extended_functional_tests != xno; then
  AC_SUBST(EXTENDED_FUNCTIONAL_TESTS, --extended)
fi

if test x$use_lcov = xyes; then
  if test x$LCOV = x; then
    AC_MSG_ERROR([lcov testing requested but lcov not found])
  fi
  if test x$PYTHON = x; then
    AC_MSG_ERROR([lcov testing requested but python not found])
  fi
  if test x$GENHTML = x; then
    AC_MSG_ERROR([lcov testing requested but genhtml not found])
  fi

  AC_MSG_CHECKING([whether compiler is Clang])
  AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
      #if defined(__clang__) && defined(__llvm__)
      // Compiler is Clang
      #else
      #  error Compiler is not Clang
      #endif
    ]])],[
      AC_MSG_RESULT([yes])
      if test x$LLVM_COV = x; then
        AC_MSG_ERROR([lcov testing requested but llvm-cov not found])
      fi
      COV_TOOL="$LLVM_COV gcov"
    ],[
      AC_MSG_RESULT([no])
      if test x$GCOV = x; then
        AC_MSG_ERROR([lcov testing requested but gcov not found])
      fi
      COV_TOOL="$GCOV"
  ])
  AC_SUBST(COV_TOOL)
  AC_SUBST(COV_TOOL_WRAPPER, "cov_tool_wrapper.sh")
  LCOV="$LCOV --gcov-tool $(pwd)/$COV_TOOL_WRAPPER"

  AX_CHECK_LINK_FLAG([--coverage], [LDFLAGS="$LDFLAGS --coverage"],
    [AC_MSG_ERROR([lcov testing requested but --coverage linker flag does not work])])
  AX_CHECK_COMPILE_FLAG([--coverage],[CXXFLAGS="$CXXFLAGS --coverage"],
    [AC_MSG_ERROR([lcov testing requested but --coverage flag does not work])])
  CXXFLAGS="$CXXFLAGS -Og"
fi

if test x$use_lcov_branch != xno; then
  AC_SUBST(LCOV_OPTS, "$LCOV_OPTS --rc lcov_branch_coverage=1")
fi

dnl Check for __int128
AC_CHECK_TYPES([__int128])

dnl Check for endianness
AC_C_BIGENDIAN

dnl Check for pthread compile/link requirements
AX_PTHREAD

dnl The following macro will add the necessary defines to bitcoin-config.h, but
dnl they also need to be passed down to any subprojects. Pull the results out of
dnl the cache and add them to CPPFLAGS.
AC_SYS_LARGEFILE
dnl detect POSIX or GNU variant of strerror_r
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

AC_SEARCH_LIBS([clock_gettime],[rt])

if test "x$enable_gprof" = xyes; then
    dnl -pg is incompatible with -pie. Since hardening and profiling together doesn't make sense,
    dnl we simply make them mutually exclusive here. Additionally, hardened toolchains may force
    dnl -pie by default, in which case it needs to be turned off with -no-pie.

    if test x$use_hardening = xyes; then
        AC_MSG_ERROR([gprof profiling is not compatible with hardening. Reconfigure with --disable-hardening or --disable-gprof])
    fi
    use_hardening=no
    AX_CHECK_COMPILE_FLAG([-pg],[GPROF_CXXFLAGS="-pg"],
        [AC_MSG_ERROR([gprof profiling requested but not available])], [$CXXFLAG_WERROR])

    AX_CHECK_LINK_FLAG([-no-pie], [GPROF_LDFLAGS="-no-pie"])
    AX_CHECK_LINK_FLAG([-pg], [GPROF_LDFLAGS="$GPROF_LDFLAGS -pg"],
        [AC_MSG_ERROR([gprof profiling requested but not available])], [$GPROF_LDFLAGS])
fi

if test x$TARGET_OS != xwindows; then
  dnl All windows code is PIC, forcing it on just adds useless compile warnings
  AX_CHECK_COMPILE_FLAG([-fPIC], [PIC_FLAGS="-fPIC"])
fi

dnl All versions of gcc that we commonly use for building are subject to bug
dnl https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90348. To work around that, set
dnl -fstack-reuse=none for all gcc builds. (Only gcc understands this flag)
AX_CHECK_COMPILE_FLAG([-fstack-reuse=none], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fstack-reuse=none"])
if test x$use_hardening != xno; then
  use_hardening=yes
  AX_CHECK_COMPILE_FLAG([-Wstack-protector], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -Wstack-protector"])
  AX_CHECK_COMPILE_FLAG([-fstack-protector-all], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fstack-protector-all"])

  dnl -fcf-protection used with Clang 7 causes ld to emit warnings:
  dnl ld: error: ... <corrupt x86 feature size: 0x8>
  dnl Use CHECK_LINK_FLAG & --fatal-warnings to ensure we won't use the flag in this case.
  AX_CHECK_LINK_FLAG([-fcf-protection=full], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fcf-protection=full"], [], [$LDFLAG_WERROR])

  case $host in
    *mingw*)
      dnl stack-clash-protection doesn't currently work, and likely should just be skipped for Windows.
      dnl See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90458 for more details.
      ;;
    *)
      AX_CHECK_COMPILE_FLAG([-fstack-clash-protection], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fstack-clash-protection"], [], [$CXXFLAG_WERROR])
      ;;
  esac


  dnl When enable_debug is yes, all optimizations are disabled.
  dnl However, FORTIFY_SOURCE requires that there is some level of optimization, otherwise it does nothing and just creates a compiler warning.
  dnl Since FORTIFY_SOURCE is a no-op without optimizations, do not enable it when enable_debug is yes.
  if test x$enable_debug != xyes; then
    AX_CHECK_PREPROC_FLAG([-D_FORTIFY_SOURCE=2],[
      AX_CHECK_PREPROC_FLAG([-U_FORTIFY_SOURCE],[
        HARDENED_CPPFLAGS="$HARDENED_CPPFLAGS -U_FORTIFY_SOURCE"
      ])
      HARDENED_CPPFLAGS="$HARDENED_CPPFLAGS -D_FORTIFY_SOURCE=2"
    ])
  fi

  AX_CHECK_LINK_FLAG([-Wl,--enable-reloc-section], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--enable-reloc-section"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,--dynamicbase], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--dynamicbase"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,--nxcompat], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--nxcompat"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,--high-entropy-va], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--high-entropy-va"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,-z,relro], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,relro"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,-z,now], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,now"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,-z,separate-code], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,separate-code"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-fPIE -pie], [PIE_FLAGS="-fPIE"; HARDENED_LDFLAGS="$HARDENED_LDFLAGS -pie"], [], [$CXXFLAG_WERROR])

  case $host in
    *mingw*)
       AC_CHECK_LIB([ssp], [main], [], [AC_MSG_ERROR([libssp missing])])
    ;;
  esac
fi

dnl These flags are specific to ld64, and may cause issues with other linkers.
dnl For example: GNU ld will interpret -dead_strip as -de and then try and use
dnl "ad_strip" as the symbol for the entry point.
if test x$TARGET_OS = xdarwin; then
  AX_CHECK_LINK_FLAG([-Wl,-dead_strip], [LDFLAGS="$LDFLAGS -Wl,-dead_strip"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,-dead_strip_dylibs], [LDFLAGS="$LDFLAGS -Wl,-dead_strip_dylibs"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,-bind_at_load], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-bind_at_load"], [], [$LDFLAG_WERROR])
fi

AC_CHECK_HEADERS([endian.h sys/endian.h byteswap.h stdio.h stdlib.h unistd.h strings.h sys/types.h sys/stat.h sys/select.h sys/prctl.h sys/sysctl.h vm/vm_param.h sys/vmmeter.h sys/resources.h])

AC_CHECK_DECLS([getifaddrs, freeifaddrs],[CHECK_SOCKET],,
    [#include <sys/types.h>
    #include <ifaddrs.h>]
)
AC_CHECK_DECLS([strnlen])

dnl These are used for daemonization in bitcoind
AC_CHECK_DECLS([fork])
AC_CHECK_DECLS([setsid])

AC_CHECK_DECLS([pipe2])

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

AC_MSG_CHECKING([for __builtin_clzl])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
 (void) __builtin_clzl(0);
  ]])],
 [ AC_MSG_RESULT([yes]); have_clzl=yes; AC_DEFINE([HAVE_BUILTIN_CLZL], [1], [Define this symbol if you have __builtin_clzl])],
 [ AC_MSG_RESULT([no]); have_clzl=no;]
)

AC_MSG_CHECKING([for __builtin_clzll])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
  (void) __builtin_clzll(0);
  ]])],
 [ AC_MSG_RESULT([yes]); have_clzll=yes; AC_DEFINE([HAVE_BUILTIN_CLZLL], [1], [Define this symbol if you have __builtin_clzll])],
 [ AC_MSG_RESULT([no]); have_clzll=no;]
)

dnl Check for malloc_info (for memory statistics information in getmemoryinfo)
AC_MSG_CHECKING([for getmemoryinfo])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <malloc.h>]],
 [[ int f = malloc_info(0, NULL); ]])],
 [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_MALLOC_INFO], [1], [Define this symbol if you have malloc_info]) ],
 [ AC_MSG_RESULT([no])]
)

dnl Check for mallopt(M_ARENA_MAX) (to set glibc arenas)
AC_MSG_CHECKING([for mallopt M_ARENA_MAX])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <malloc.h>]],
 [[ mallopt(M_ARENA_MAX, 1); ]])],
 [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_MALLOPT_ARENA_MAX], [1], [Define this symbol if you have mallopt with M_ARENA_MAX]) ],
 [ AC_MSG_RESULT([no])]
)

dnl Check for posix_fallocate
AC_MSG_CHECKING([for posix_fallocate])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
                   // same as in src/util/system.cpp
                   #ifdef __linux__
                   #ifdef _POSIX_C_SOURCE
                   #undef _POSIX_C_SOURCE
                   #endif
                   #define _POSIX_C_SOURCE 200112L
                   #endif // __linux__
                   #include <fcntl.h>]],
                   [[ int f = posix_fallocate(0, 0, 0); ]])],
 [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_POSIX_FALLOCATE], [1], [Define this symbol if you have posix_fallocate]) ],
 [ AC_MSG_RESULT([no])]
)

AC_MSG_CHECKING([for default visibility attribute])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
  int foo(void) __attribute__((visibility("default")));
  int main(){}
  ])],
  [
    AC_DEFINE([HAVE_DEFAULT_VISIBILITY_ATTRIBUTE], [1], [Define if the visibility attribute is supported.])
    AC_MSG_RESULT([yes])
  ],
  [
    AC_MSG_RESULT([no])
    if test x$use_reduce_exports = xyes; then
      AC_MSG_ERROR([Cannot find a working visibility attribute. Use --disable-reduce-exports.])
    fi
  ]
)

AC_MSG_CHECKING([for dllexport attribute])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
  __declspec(dllexport) int foo(void);
  int main(){}
  ])],
  [
    AC_DEFINE([HAVE_DLLEXPORT_ATTRIBUTE], [1], [Define if the dllexport attribute is supported.])
    AC_MSG_RESULT([yes])
  ],
  [AC_MSG_RESULT([no])]
)

if test "x$use_thread_local" = xyes || test "x$use_thread_local" = xauto; then
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
     case $host in
       *mingw*)
          dnl mingw32's implementation of thread_local has also been shown to behave
          dnl erroneously under concurrent usage; see:
          dnl https://gist.github.com/jamesob/fe9a872051a88b2025b1aa37bfa98605
          AC_MSG_RESULT([no])
          ;;
        *freebsd*)
          dnl FreeBSD's implementation of thread_local is also buggy (per
          dnl https://groups.google.com/d/msg/bsdmailinglist/22ncTZAbDp4/Dii_pII5AwAJ)
          AC_MSG_RESULT([no])
          ;;
        *)
          AC_DEFINE([HAVE_THREAD_LOCAL], [1], [Define if thread_local is supported.])
          AC_MSG_RESULT([yes])
          ;;
      esac
    ],
    [
      AC_MSG_RESULT([no])
    ]
  )
  LDFLAGS="$TEMP_LDFLAGS"
fi

dnl check for gmtime_r(), fallback to gmtime_s() if that is unavailable
dnl fail if neither are available.
AC_MSG_CHECKING([for gmtime_r])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <ctime>]],
  [[ gmtime_r((const time_t *) nullptr, (struct tm *) nullptr); ]])],
  [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_GMTIME_R], [1], [Define this symbol if gmtime_r is available]) ],
  [ AC_MSG_RESULT([no]);
    AC_MSG_CHECKING([for gmtime_s]);
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <ctime>]],
       [[ gmtime_s((struct tm *) nullptr, (const time_t *) nullptr); ]])],
       [ AC_MSG_RESULT([yes])],
       [ AC_MSG_RESULT([no]); AC_MSG_ERROR([Both gmtime_r and gmtime_s are unavailable]) ]
    )
  ]
)

dnl Check for different ways of gathering OS randomness
AC_MSG_CHECKING([for Linux getrandom syscall])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>
  #include <sys/syscall.h>
  #include <linux/random.h>]],
 [[ syscall(SYS_getrandom, nullptr, 32, 0); ]])],
 [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_SYS_GETRANDOM], [1], [Define this symbol if the Linux getrandom system call is available]) ],
 [ AC_MSG_RESULT([no])]
)

AC_MSG_CHECKING([for getentropy])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]],
 [[ getentropy(nullptr, 32) ]])],
 [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_GETENTROPY], [1], [Define this symbol if the BSD getentropy system call is available]) ],
 [ AC_MSG_RESULT([no])]
)

AC_MSG_CHECKING([for getentropy via random.h])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>
 #include <sys/random.h>]],
 [[ getentropy(nullptr, 32) ]])],
 [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_GETENTROPY_RAND], [1], [Define this symbol if the BSD getentropy system call is available with sys/random.h]) ],
 [ AC_MSG_RESULT([no])]
)

AC_MSG_CHECKING([for sysctl])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
  #include <sys/sysctl.h>]],
 [[ #ifdef __linux__
    #error "Don't use sysctl on Linux, it's deprecated even when it works"
    #endif
    sysctl(nullptr, 2, nullptr, nullptr, nullptr, 0); ]])],
 [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_SYSCTL], [1], [Define this symbol if the BSD sysctl() is available]) ],
 [ AC_MSG_RESULT([no])]
)

AC_MSG_CHECKING([for sysctl KERN_ARND])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
  #include <sys/sysctl.h>]],
 [[ #ifdef __linux__
    #error "Don't use sysctl on Linux, it's deprecated even when it works"
    #endif
    static int name[2] = {CTL_KERN, KERN_ARND};
    sysctl(name, 2, nullptr, nullptr, nullptr, 0); ]])],
 [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_SYSCTL_ARND], [1], [Define this symbol if the BSD sysctl(KERN_ARND) is available]) ],
 [ AC_MSG_RESULT([no])]
)

AC_MSG_CHECKING([for if type char equals int8_t])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdint.h>
  #include <type_traits>]],
 [[ static_assert(std::is_same<int8_t, char>::value, ""); ]])],
 [ AC_MSG_RESULT([yes]); AC_DEFINE([CHAR_EQUALS_INT8], [1], [Define this symbol if type char equals int8_t]) ],
 [ AC_MSG_RESULT([no])]
)

AC_MSG_CHECKING([for fdatasync])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>]],
 [[ fdatasync(0); ]])],
 [ AC_MSG_RESULT([yes]); HAVE_FDATASYNC=1 ],
 [ AC_MSG_RESULT([no]); HAVE_FDATASYNC=0 ]
)
AC_DEFINE_UNQUOTED([HAVE_FDATASYNC], [$HAVE_FDATASYNC], [Define to 1 if fdatasync is available.])

AC_MSG_CHECKING([for F_FULLFSYNC])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <fcntl.h>]],
 [[ fcntl(0, F_FULLFSYNC, 0); ]])],
 [ AC_MSG_RESULT([yes]); HAVE_FULLFSYNC=1 ],
 [ AC_MSG_RESULT([no]); HAVE_FULLFSYNC=0 ]
)

AC_MSG_CHECKING([for O_CLOEXEC])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <fcntl.h>]],
 [[ open("", O_CLOEXEC); ]])],
 [ AC_MSG_RESULT([yes]); HAVE_O_CLOEXEC=1 ],
 [ AC_MSG_RESULT([no]); HAVE_O_CLOEXEC=0 ]
)
AC_DEFINE_UNQUOTED([HAVE_O_CLOEXEC], [$HAVE_O_CLOEXEC], [Define to 1 if O_CLOEXEC flag is available.])

dnl crc32c platform checks
AC_MSG_CHECKING([for __builtin_prefetch])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
  char data = 0;
  const char* address = &data;
  __builtin_prefetch(address, 0, 0);
  ]])],
 [ AC_MSG_RESULT([yes]); HAVE_BUILTIN_PREFETCH=1 ],
 [ AC_MSG_RESULT([no]); HAVE_BUILTIN_PREFETCH=0 ]
)

AC_MSG_CHECKING([for _mm_prefetch])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <xmmintrin.h>]], [[
  char data = 0;
  const char* address = &data;
  _mm_prefetch(address, _MM_HINT_NTA);
  ]])],
 [ AC_MSG_RESULT([yes]); HAVE_MM_PREFETCH=1 ],
 [ AC_MSG_RESULT([no]); HAVE_MM_PREFETCH=0 ]
)

AC_MSG_CHECKING([for strong getauxval support in the system headers])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <sys/auxv.h>
  ]], [[
    getauxval(AT_HWCAP);
  ]])],
 [ AC_MSG_RESULT([yes]); HAVE_STRONG_GETAUXVAL=1; AC_DEFINE([HAVE_STRONG_GETAUXVAL], [1], [Define this symbol to build code that uses getauxval)]) ],
 [ AC_MSG_RESULT([no]); HAVE_STRONG_GETAUXVAL=0 ]
)

have_any_system=no
AC_MSG_CHECKING([for std::system])
AC_LINK_IFELSE(
    [ AC_LANG_PROGRAM(
        [[ #include <cstdlib> ]],
        [[ int nErr = std::system(""); ]]
    )],
    [ AC_MSG_RESULT([yes]); have_any_system=yes],
    [ AC_MSG_RESULT([no]) ]
)

AC_MSG_CHECKING([for ::_wsystem])
AC_LINK_IFELSE(
    [ AC_LANG_PROGRAM(
        [[ ]],
        [[ int nErr = ::_wsystem(""); ]]
    )],
    [ AC_MSG_RESULT([yes]); have_any_system=yes],
    [ AC_MSG_RESULT([no]) ]
)

if test "x$have_any_system" != "xno"; then
  AC_DEFINE([HAVE_SYSTEM], [1], [Define to 1 if std::system or ::wsystem is available.])
fi

dnl SUPPRESSED_CPPFLAGS=SUPPRESS_WARNINGS([$SOME_CPPFLAGS])
dnl Replace -I with -isystem in $SOME_CPPFLAGS to suppress warnings from
dnl headers from its include directories and return the result.
dnl See -isystem documentation:
dnl https://gcc.gnu.org/onlinedocs/gcc/Directory-Options.html
dnl https://clang.llvm.org/docs/ClangCommandLineReference.html#cmdoption-clang-isystem-directory
dnl Do not change "-I/usr/include" to "-isystem /usr/include" because that
dnl is not necessary (/usr/include is already a system directory) and because
dnl it would break GCC's #include_next.
AC_DEFUN([SUPPRESS_WARNINGS],
         [$(echo $1 |${SED} -E -e 's/(^| )-I/\1-isystem /g' -e 's;-isystem /usr/include([/ ]|$);-I/usr/include\1;g')])

dnl enable-fuzz should disable all other targets
if test "x$enable_fuzz" = "xyes"; then
  AC_MSG_WARN([enable-fuzz will disable all other targets and force --enable-fuzz-binary=yes])
  build_bitcoin_utils=no
  build_bitcoin_cli=no
  build_bitcoin_tx=no
  build_bitcoin_util=no
  build_bitcoin_wallet=no
  build_bitcoind=no
  build_bitcoin_libs=no
  bitcoin_enable_qt=no
  bitcoin_enable_qt_test=no
  bitcoin_enable_qt_dbus=no
  use_bench=no
  use_external_signer=no
  use_upnp=no
  use_natpmp=no
  use_zmq=no
  enable_fuzz_binary=yes

  AX_CHECK_PREPROC_FLAG([-DABORT_ON_FAILED_ASSUME], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DABORT_ON_FAILED_ASSUME"], [], [$CXXFLAG_WERROR])

  AC_MSG_CHECKING([whether main function is needed for fuzz binary])
  AX_CHECK_LINK_FLAG(
    [-fsanitize=$use_sanitizers],
    [AC_MSG_RESULT([no])],
    [AC_MSG_RESULT([yes]); CPPFLAGS="$CPPFLAGS -DPROVIDE_FUZZ_MAIN_FUNCTION"],
    [],
    [AC_LANG_PROGRAM([[
      #include <cstdint>
      #include <cstddef>
      extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) { return 0; }
      /* comment to remove the main function ...
     ]],[[
      */ int not_main() {
     ]])])
else
  BITCOIN_QT_INIT

  dnl sets $bitcoin_enable_qt, $bitcoin_enable_qt_test, $bitcoin_enable_qt_dbus
  BITCOIN_QT_CONFIGURE([5.9.5])

  dnl Keep a copy of the original $QT_INCLUDES and use it when invoking qt's moc
  QT_INCLUDES_UNSUPPRESSED=$QT_INCLUDES
  if test x$suppress_external_warnings != xno ; then
    QT_INCLUDES=SUPPRESS_WARNINGS($QT_INCLUDES)
    QT_DBUS_INCLUDES=SUPPRESS_WARNINGS($QT_DBUS_INCLUDES)
    QT_TEST_INCLUDES=SUPPRESS_WARNINGS($QT_TEST_INCLUDES)
  fi

  CPPFLAGS="$CPPFLAGS -DPROVIDE_FUZZ_MAIN_FUNCTION"
fi

if test x$enable_wallet != xno; then
    dnl Check for libdb_cxx only if wallet enabled
    if test "x$use_bdb" != "xno"; then
      BITCOIN_FIND_BDB48
      if test x$suppress_external_warnings != xno ; then
        BDB_CPPFLAGS=SUPPRESS_WARNINGS($BDB_CPPFLAGS)
      fi
    fi

    dnl Check for sqlite3
    if test "x$use_sqlite" != "xno"; then
      PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.7.17], [have_sqlite=yes], [have_sqlite=no])
    fi
    AC_MSG_CHECKING([whether to build wallet with support for sqlite])
    if test "x$use_sqlite" = "xno"; then
      use_sqlite=no
    elif test "x$have_sqlite" = "xno"; then
      if test "x$use_sqlite" = "xyes"; then
        AC_MSG_ERROR([sqlite support requested but cannot be built. Use --without-sqlite])
      fi
      use_sqlite=no
    else
      if test x$use_sqlite != xno; then
        AC_DEFINE([USE_SQLITE],[1],[Define if sqlite support should be compiled in])
        use_sqlite=yes
      fi
    fi
    AC_MSG_RESULT([$use_sqlite])

    dnl Disable wallet if both --without-bdb and --without-sqlite
    if test "x$use_bdb$use_sqlite" = "xnono"; then
        if test "x$enable_wallet" = "xyes"; then
            AC_MSG_ERROR([wallet functionality requested but no BDB or SQLite support available.])
        fi
        enable_wallet=no
    fi
fi

if test x$use_ebpf != xno; then
  AC_MSG_CHECKING([whether eBPF tracepoints are supported])
  AC_COMPILE_IFELSE([
    AC_LANG_PROGRAM(
      [#include <sys/sdt.h>],
      [DTRACE_PROBE("context", "event");]
    )],
    [AC_MSG_RESULT([yes]); have_sdt=yes; AC_DEFINE([ENABLE_TRACING], [1], [Define to 1 to enable eBPF user static defined tracepoints])],
    [AC_MSG_RESULT([no]); have_sdt=no;]
  )
fi

if test x$build_bitcoin_cli$build_bitcoin_tx$build_bitcoin_util$build_bitcoind$bitcoin_enable_qt$use_bench$use_tests = xnonononononono; then
  use_upnp=no
  use_natpmp=no
  use_zmq=no
fi

dnl Check for libminiupnpc (optional)
if test x$use_upnp != xno; then
  AC_CHECK_HEADERS(
    [miniupnpc/miniupnpc.h miniupnpc/upnpcommands.h miniupnpc/upnperrors.h],
    [AC_CHECK_LIB([miniupnpc], [upnpDiscover], [MINIUPNPC_LIBS=-lminiupnpc], [have_miniupnpc=no])],
    [have_miniupnpc=no]
  )
dnl The minimum supported miniUPnPc API version is set to 10. This keeps compatibility
dnl with Ubuntu 16.04 LTS and Debian 8 libminiupnpc-dev packages.
if test x$have_miniupnpc != xno; then
  AC_MSG_CHECKING([whether miniUPnPc API version is supported])
  AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[
      @%:@include <miniupnpc/miniupnpc.h>
    ]], [[
      #if MINIUPNPC_API_VERSION >= 10
      // Everything is okay
      #else
      #  error miniUPnPc API version is too old
      #endif
    ]])],[
      AC_MSG_RESULT([yes])
    ],[
    AC_MSG_RESULT([no])
    AC_MSG_WARN([miniUPnPc API version < 10 is unsupported, disabling UPnP support.])
    have_miniupnpc=no
  ])
fi
fi

dnl Check for libnatpmp (optional).
if test "x$use_natpmp" != xno; then
  AC_CHECK_HEADERS([natpmp.h],
                   [AC_CHECK_LIB([natpmp], [initnatpmp], [NATPMP_LIBS=-lnatpmp], [have_natpmp=no])],
                   [have_natpmp=no])
fi

if test x$build_bitcoin_wallet$build_bitcoin_cli$build_bitcoin_tx$build_bitcoind$bitcoin_enable_qt$use_tests$use_bench = xnonononononono; then
  use_boost=no
else
  use_boost=yes
fi

if test x$use_boost = xyes; then

  dnl Check for Boost headers
  AX_BOOST_BASE([1.64.0],[],[AC_MSG_ERROR([Boost is not available!])])
  if test x$want_boost = xno; then
    AC_MSG_ERROR([only libbitcoinconsensus can be built without Boost])
  fi
  AX_BOOST_SYSTEM
  AX_BOOST_FILESYSTEM

  if test x$suppress_external_warnings != xno; then
    BOOST_CPPFLAGS=SUPPRESS_WARNINGS($BOOST_CPPFLAGS)
  fi

  BOOST_LIBS="$BOOST_LDFLAGS $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB"
fi

if test "x$use_external_signer" != xno; then
  AC_DEFINE([ENABLE_EXTERNAL_SIGNER], [], [Define if external signer support is enabled])
fi
AM_CONDITIONAL([ENABLE_EXTERNAL_SIGNER], [test "x$use_external_signer" = "xyes"])

dnl Do not compile with syscall sandbox support when compiling under the sanitizers.
dnl The sanitizers introduce use of syscalls that are not typically used in bitcoind
dnl (such as execve when the sanitizers execute llvm-symbolizer).
if test x$use_sanitizers != x; then
  AC_MSG_WARN([Specifying --with-sanitizers forces --without-seccomp since the sanitizers introduce use of syscalls not allowed by the bitcoind syscall sandbox (-sandbox=<mode>).])
  seccomp_found=no
fi
if test "x$seccomp_found" != "xno"; then
  AC_MSG_CHECKING([for seccomp-bpf (Linux x86-64)])
  AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[
      @%:@include <linux/seccomp.h>
    ]], [[
      #if !defined(__x86_64__)
      #  error Syscall sandbox is an experimental feature currently available only under Linux x86-64.
      #endif
    ]])],[
      AC_MSG_RESULT([yes])
      seccomp_found="yes"
      AC_DEFINE([USE_SYSCALL_SANDBOX], [1], [Define this symbol to build with syscall sandbox support.])
    ],[
      AC_MSG_RESULT([no])
      seccomp_found="no"
  ])
fi
dnl Currently only enable -sandbox=<mode> feature if seccomp is found.
dnl In the future, sandboxing could be also be supported with other
dnl sandboxing mechanisms besides seccomp.
use_syscall_sandbox=$seccomp_found
AM_CONDITIONAL([ENABLE_SYSCALL_SANDBOX], [test "x$use_syscall_sandbox" != "xno"])

dnl Check for reduced exports
if test x$use_reduce_exports = xyes; then
  AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CXXFLAGS="$CXXFLAGS -fvisibility=hidden"],
  [AC_MSG_ERROR([Cannot set hidden symbol visibility. Use --disable-reduce-exports.])], [$CXXFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,--exclude-libs,ALL], [RELDFLAGS="-Wl,--exclude-libs,ALL"], [], [$LDFLAG_WERROR])
fi

if test x$use_tests = xyes; then

  if test x$HEXDUMP = x; then
    AC_MSG_ERROR([hexdump is required for tests])
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
      [AC_MSG_RESULT([yes])]
      [TESTDEFS="$TESTDEFS -DBOOST_TEST_DYN_LINK"],
      [AC_MSG_RESULT([no])])
    LIBS="$TEMP_LIBS"
    CPPFLAGS="$TEMP_CPPFLAGS"

  fi
fi

dnl libevent check

if test x$build_bitcoin_cli$build_bitcoind$bitcoin_enable_qt$use_tests$use_bench != xnonononono; then
  PKG_CHECK_MODULES([EVENT], [libevent >= 2.0.21], [use_libevent=yes], [AC_MSG_ERROR([libevent version 2.0.21 or greater not found.])])
  if test x$TARGET_OS != xwindows; then
    PKG_CHECK_MODULES([EVENT_PTHREADS], [libevent_pthreads >= 2.0.21],, [AC_MSG_ERROR([libevent_pthreads version 2.0.21 or greater not found.])])
  fi

  if test x$suppress_external_warnings != xno; then
    EVENT_CFLAGS=SUPPRESS_WARNINGS($EVENT_CFLAGS)
  fi
fi

dnl QR Code encoding library check

if test "x$use_qr" != xno; then
  BITCOIN_QT_CHECK([PKG_CHECK_MODULES([QR], [libqrencode], [have_qrencode=yes], [have_qrencode=no])])
fi

dnl ZMQ check

if test "x$use_zmq" = xyes; then
  PKG_CHECK_MODULES([ZMQ], [libzmq >= 4],
    AC_DEFINE([ENABLE_ZMQ], [1], [Define to 1 to enable ZMQ functions]),
    [AC_DEFINE([ENABLE_ZMQ], [0], [Define to 1 to enable ZMQ functions])
    AC_MSG_WARN([libzmq version 4.x or greater not found, disabling])
    use_zmq=no])
else
  AC_DEFINE_UNQUOTED([ENABLE_ZMQ], [0], [Define to 1 to enable ZMQ functions])
fi

if test "x$use_zmq" = xyes; then
  dnl Assume libzmq was built for static linking
  case $host in
    *mingw*)
      ZMQ_CFLAGS="$ZMQ_CFLAGS -DZMQ_STATIC"
    ;;
  esac
fi

dnl libmultiprocess library check

libmultiprocess_found=no
if test "x$with_libmultiprocess" = xyes || test "x$with_libmultiprocess" = xauto; then
  m4_ifdef([PKG_CHECK_MODULES], [PKG_CHECK_MODULES([LIBMULTIPROCESS], [libmultiprocess], [
     libmultiprocess_found=yes;
     libmultiprocess_prefix=`$PKG_CONFIG --variable=prefix libmultiprocess`;
  ], [true])])
elif test "x$with_libmultiprocess" != xno; then
  AC_MSG_ERROR([--with-libmultiprocess=$with_libmultiprocess value is not yes, auto, or no])
fi
AC_SUBST(LIBMULTIPROCESS_CFLAGS)
AC_SUBST(LIBMULTIPROCESS_LIBS)

dnl Enable multiprocess check

if test "x$enable_multiprocess" = xyes; then
  if test "x$libmultiprocess_found" != xyes; then
    AC_MSG_ERROR([--enable-multiprocess=yes option specified but libmultiprocess library was not found. May need to install libmultiprocess library, or specify install path with PKG_CONFIG_PATH environment variable. Running 'pkg-config --debug libmultiprocess' may be helpful for debugging.])
  fi
  build_multiprocess=yes
elif test "x$enable_multiprocess" = xauto; then
  build_multiprocess=$libmultiprocess_found
else
  build_multiprocess=no
fi

AM_CONDITIONAL([BUILD_MULTIPROCESS], [test "x$build_multiprocess" = xyes])
AM_CONDITIONAL([BUILD_BITCOIN_NODE], [test "x$build_multiprocess" = xyes])
AM_CONDITIONAL([BUILD_BITCOIN_GUI], [test "x$build_multiprocess" = xyes])

dnl codegen tools check

if test x$build_multiprocess != xno; then
  if test "x$with_mpgen" = xyes || test "x$with_mpgen" = xauto; then
    MPGEN_PREFIX="$libmultiprocess_prefix"
  elif test "x$with_mpgen" != xno; then
    MPGEN_PREFIX="$with_mpgen";
  fi
  AC_SUBST(MPGEN_PREFIX)
fi

AC_MSG_CHECKING([whether to build bitcoind])
AM_CONDITIONAL([BUILD_BITCOIND], [test x$build_bitcoind = xyes])
AC_MSG_RESULT($build_bitcoind)

AC_MSG_CHECKING([whether to build bitcoin-cli])
AM_CONDITIONAL([BUILD_BITCOIN_CLI], [test x$build_bitcoin_cli = xyes])
AC_MSG_RESULT($build_bitcoin_cli)

AC_MSG_CHECKING([whether to build bitcoin-tx])
AM_CONDITIONAL([BUILD_BITCOIN_TX], [test x$build_bitcoin_tx = xyes])
AC_MSG_RESULT($build_bitcoin_tx)

AC_MSG_CHECKING([whether to build bitcoin-wallet])
AM_CONDITIONAL([BUILD_BITCOIN_WALLET], [test x$build_bitcoin_wallet = xyes])
AC_MSG_RESULT($build_bitcoin_wallet)

AC_MSG_CHECKING([whether to build bitcoin-util])
AM_CONDITIONAL([BUILD_BITCOIN_UTIL], [test x$build_bitcoin_util = xyes])
AC_MSG_RESULT($build_bitcoin_util)

AC_MSG_CHECKING([whether to build libraries])
AM_CONDITIONAL([BUILD_BITCOIN_LIBS], [test x$build_bitcoin_libs = xyes])
if test x$build_bitcoin_libs = xyes; then
  AC_DEFINE([HAVE_CONSENSUS_LIB], [1], [Define this symbol if the consensus lib has been built])
  AC_CONFIG_FILES([libbitcoinconsensus.pc:libbitcoinconsensus.pc.in])
fi
AC_MSG_RESULT($build_bitcoin_libs)

AC_LANG_POP

if test "x$use_ccache" != "xno"; then
  AC_MSG_CHECKING([if ccache should be used])
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
  if test "x$use_ccache" = "xyes"; then
    AX_CHECK_COMPILE_FLAG([-fdebug-prefix-map=A=B], [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -fdebug-prefix-map=\$(abs_top_srcdir)=."], [], [$CXXFLAG_WERROR])
    AX_CHECK_PREPROC_FLAG([-fmacro-prefix-map=A=B], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -fmacro-prefix-map=\$(abs_top_srcdir)=."], [], [$CXXFLAG_WERROR])
  fi
fi

dnl enable wallet
AC_MSG_CHECKING([if wallet should be enabled])
if test x$enable_wallet != xno; then
  AC_MSG_RESULT([yes])
  AC_DEFINE_UNQUOTED([ENABLE_WALLET],[1],[Define to 1 to enable wallet functions])
  enable_wallet=yes

else
  AC_MSG_RESULT([no])
fi

dnl enable upnp support
AC_MSG_CHECKING([whether to build with support for UPnP])
if test x$have_miniupnpc = xno; then
  if test x$use_upnp = xyes; then
     AC_MSG_ERROR([UPnP requested but cannot be built. Use --without-miniupnpc])
  fi
  AC_MSG_RESULT([no])
  use_upnp=no
else
  if test x$use_upnp != xno; then
    AC_MSG_RESULT([yes])
    AC_MSG_CHECKING([whether to build with UPnP enabled by default])
    use_upnp=yes
    upnp_setting=0
    if test x$use_upnp_default != xno; then
      use_upnp_default=yes
      upnp_setting=1
    fi
    AC_MSG_RESULT([$use_upnp_default])
    AC_DEFINE_UNQUOTED([USE_UPNP],[$upnp_setting],[UPnP support not compiled if undefined, otherwise value (0 or 1) determines default state])
    if test x$TARGET_OS = xwindows; then
      MINIUPNPC_CPPFLAGS="-DSTATICLIB -DMINIUPNP_STATICLIB"
    fi
  else
    AC_MSG_RESULT([no])
  fi
fi

dnl Enable NAT-PMP support.
AC_MSG_CHECKING([whether to build with support for NAT-PMP])
if test "x$have_natpmp" = xno; then
  if test "x$use_natpmp" = xyes; then
     AC_MSG_ERROR([NAT-PMP requested but cannot be built. Use --without-natpmp])
  fi
  AC_MSG_RESULT([no])
  use_natpmp=no
else
  if test "x$use_natpmp" != xno; then
    AC_MSG_RESULT([yes])
    AC_MSG_CHECKING([whether to build with NAT-PMP enabled by default])
    use_natpmp=yes
    natpmp_setting=0
    if test "x$use_natpmp_default" != xno; then
      use_natpmp_default=yes
      natpmp_setting=1
    fi
    AC_MSG_RESULT($use_natpmp_default)
    AC_DEFINE_UNQUOTED([USE_NATPMP], [$natpmp_setting], [NAT-PMP support not compiled if undefined, otherwise value (0 or 1) determines default state])
    if test x$TARGET_OS = xwindows; then
      NATPMP_CPPFLAGS="-DSTATICLIB -DNATPMP_STATICLIB"
    fi
  else
    AC_MSG_RESULT([no])
  fi
fi

dnl these are only used when qt is enabled
BUILD_TEST_QT=""
if test x$bitcoin_enable_qt != xno; then
  dnl enable dbus support
  AC_MSG_CHECKING([whether to build GUI with support for D-Bus])
  if test x$bitcoin_enable_qt_dbus != xno; then
    AC_DEFINE([USE_DBUS], [1], [Define if dbus support should be compiled in])
  fi
  AC_MSG_RESULT([$bitcoin_enable_qt_dbus])

  dnl enable qr support
  AC_MSG_CHECKING([whether to build GUI with support for QR codes])
  if test x$have_qrencode = xno; then
    if test x$use_qr = xyes; then
      AC_MSG_ERROR([QR support requested but cannot be built. Use --without-qrencode])
    fi
    use_qr=no
  else
    if test x$use_qr != xno; then
      AC_DEFINE([USE_QRCODE], [1], [Define if QR support should be compiled in])
      use_qr=yes
    fi
  fi
  AC_MSG_RESULT([$use_qr])

  if test x$XGETTEXT = x; then
    AC_MSG_WARN([xgettext is required to update qt translations])
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
  if test "x$enable_fuzz" = "xyes"; then
    AC_MSG_RESULT([no, because fuzzing is enabled])
  else
    AC_MSG_RESULT([yes])
  fi
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

if test x$build_bitcoin_wallet$build_bitcoin_cli$build_bitcoin_tx$build_bitcoin_libs$build_bitcoind$bitcoin_enable_qt$use_bench$use_tests = xnononononononono; then
  AC_MSG_ERROR([No targets! Please specify at least one of: --with-utils --with-libs --with-daemon --with-gui --enable-bench or --enable-tests])
fi

if test x$enable_fuzz_binary = xyes; then
  CHECK_RUNTIME_LIB
fi

AM_CONDITIONAL([TARGET_DARWIN], [test x$TARGET_OS = xdarwin])
AM_CONDITIONAL([BUILD_DARWIN], [test x$BUILD_OS = xdarwin])
AM_CONDITIONAL([TARGET_LINUX], [test x$TARGET_OS = xlinux])
AM_CONDITIONAL([TARGET_WINDOWS], [test x$TARGET_OS = xwindows])
AM_CONDITIONAL([ENABLE_WALLET], [test x$enable_wallet = xyes])
AM_CONDITIONAL([USE_SQLITE], [test "x$use_sqlite" = "xyes"])
AM_CONDITIONAL([USE_BDB], [test "x$use_bdb" = "xyes"])
AM_CONDITIONAL([ENABLE_TRACING], [test x$have_sdt = xyes])
AM_CONDITIONAL([ENABLE_TESTS], [test x$BUILD_TEST = xyes])
AM_CONDITIONAL([ENABLE_FUZZ], [test x$enable_fuzz = xyes])
AM_CONDITIONAL([ENABLE_FUZZ_BINARY], [test x$enable_fuzz_binary = xyes])
AM_CONDITIONAL([ENABLE_QT], [test x$bitcoin_enable_qt = xyes])
AM_CONDITIONAL([ENABLE_QT_TESTS], [test x$BUILD_TEST_QT = xyes])
AM_CONDITIONAL([ENABLE_BENCH], [test x$use_bench = xyes])
AM_CONDITIONAL([USE_QRCODE], [test x$use_qr = xyes])
AM_CONDITIONAL([USE_LCOV], [test x$use_lcov = xyes])
AM_CONDITIONAL([USE_LIBEVENT], [test x$use_libevent = xyes])
AM_CONDITIONAL([HARDEN], [test x$use_hardening = xyes])
AM_CONDITIONAL([ENABLE_SSE42], [test x$enable_sse42 = xyes])
AM_CONDITIONAL([ENABLE_SSE41], [test x$enable_sse41 = xyes])
AM_CONDITIONAL([ENABLE_AVX2], [test x$enable_avx2 = xyes])
AM_CONDITIONAL([ENABLE_SHANI], [test x$enable_shani = xyes])
AM_CONDITIONAL([ENABLE_ARM_CRC], [test x$enable_arm_crc = xyes])
AM_CONDITIONAL([USE_ASM], [test x$use_asm = xyes])
AM_CONDITIONAL([WORDS_BIGENDIAN], [test x$ac_cv_c_bigendian = xyes])
AM_CONDITIONAL([USE_NATPMP], [test x$use_natpmp = xyes])
AM_CONDITIONAL([USE_UPNP], [test x$use_upnp = xyes])

dnl for minisketch
AM_CONDITIONAL([ENABLE_CLMUL], [test x$enable_clmul = xyes])
AM_CONDITIONAL([HAVE_CLZ], [test x$have_clzl$have_clzll = xyesyes])

AC_DEFINE([CLIENT_VERSION_MAJOR], [_CLIENT_VERSION_MAJOR], [Major version])
AC_DEFINE([CLIENT_VERSION_MINOR], [_CLIENT_VERSION_MINOR], [Minor version])
AC_DEFINE([CLIENT_VERSION_BUILD], [_CLIENT_VERSION_BUILD], [Version Build])
AC_DEFINE([CLIENT_VERSION_IS_RELEASE], [_CLIENT_VERSION_IS_RELEASE], [Version is release])
AC_DEFINE([COPYRIGHT_YEAR], [_COPYRIGHT_YEAR], [Copyright year])
AC_DEFINE([COPYRIGHT_HOLDERS], ["_COPYRIGHT_HOLDERS"], [Copyright holder(s) before %s replacement])
AC_DEFINE([COPYRIGHT_HOLDERS_SUBSTITUTION], ["_COPYRIGHT_HOLDERS_SUBSTITUTION"], [Replacement for %s in copyright holders string])
define(_COPYRIGHT_HOLDERS_FINAL, [patsubst(_COPYRIGHT_HOLDERS, [%s], [_COPYRIGHT_HOLDERS_SUBSTITUTION])])
AC_DEFINE([COPYRIGHT_HOLDERS_FINAL], ["_COPYRIGHT_HOLDERS_FINAL"], [Copyright holder(s)])
AC_SUBST(CLIENT_VERSION_MAJOR, _CLIENT_VERSION_MAJOR)
AC_SUBST(CLIENT_VERSION_MINOR, _CLIENT_VERSION_MINOR)
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
AC_SUBST(BITCOIN_UTIL_NAME)
AC_SUBST(BITCOIN_WALLET_TOOL_NAME)
AC_SUBST(BITCOIN_MP_NODE_NAME)
AC_SUBST(BITCOIN_MP_GUI_NAME)

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
AC_SUBST(CLMUL_CXXFLAGS)
AC_SUBST(AVX2_CXXFLAGS)
AC_SUBST(SHANI_CXXFLAGS)
AC_SUBST(ARM_CRC_CXXFLAGS)
AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(USE_SQLITE)
AC_SUBST(USE_BDB)
AC_SUBST(ENABLE_EXTERNAL_SIGNER)
AC_SUBST(USE_UPNP)
AC_SUBST(USE_QRCODE)
AC_SUBST(BOOST_LIBS)
AC_SUBST(SQLITE_LIBS)
AC_SUBST(TESTDEFS)
AC_SUBST(MINIUPNPC_CPPFLAGS)
AC_SUBST(MINIUPNPC_LIBS)
AC_SUBST(NATPMP_CPPFLAGS)
AC_SUBST(NATPMP_LIBS)
AC_SUBST(EVENT_LIBS)
AC_SUBST(EVENT_PTHREADS_LIBS)
AC_SUBST(ZMQ_LIBS)
AC_SUBST(QR_LIBS)
AC_SUBST(HAVE_GMTIME_R)
AC_SUBST(HAVE_FDATASYNC)
AC_SUBST(HAVE_FULLFSYNC)
AC_SUBST(HAVE_O_CLOEXEC)
AC_SUBST(HAVE_BUILTIN_PREFETCH)
AC_SUBST(HAVE_MM_PREFETCH)
AC_SUBST(HAVE_STRONG_GETAUXVAL)
AC_SUBST(ANDROID_ARCH)
AC_CONFIG_FILES([Makefile src/Makefile doc/man/Makefile share/setup.nsi share/qt/Info.plist test/config.ini])
AC_CONFIG_FILES([contrib/devtools/split-debug.sh],[chmod +x contrib/devtools/split-debug.sh])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/Doxyfile])])
AC_CONFIG_LINKS([contrib/devtools/security-check.py:contrib/devtools/security-check.py])
AC_CONFIG_LINKS([contrib/devtools/symbol-check.py:contrib/devtools/symbol-check.py])
AC_CONFIG_LINKS([contrib/devtools/test-security-check.py:contrib/devtools/test-security-check.py])
AC_CONFIG_LINKS([contrib/devtools/test-symbol-check.py:contrib/devtools/test-symbol-check.py])
AC_CONFIG_LINKS([contrib/filter-lcov.py:contrib/filter-lcov.py])
AC_CONFIG_LINKS([test/functional/test_runner.py:test/functional/test_runner.py])
AC_CONFIG_LINKS([test/fuzz/test_runner.py:test/fuzz/test_runner.py])
AC_CONFIG_LINKS([test/util/test_runner.py:test/util/test_runner.py])
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

ac_configure_args="${ac_configure_args} --disable-shared --with-pic --enable-benchmark=no --enable-module-recovery --enable-module-schnorrsig --enable-experimental --disable-openssl-tests"
AC_CONFIG_SUBDIRS([src/secp256k1])

AC_OUTPUT

dnl Replace the BUILDDIR path with the correct Windows path if compiling on Native Windows
case ${OS} in
   *Windows*)
     sed  's/BUILDDIR="\/\([[a-z]]\)/BUILDDIR="\1:/'  test/config.ini > test/config-2.ini
     mv test/config-2.ini test/config.ini
   ;;
esac

echo
echo "Options used to compile and link:"
echo "  external signer = $use_external_signer"
echo "  multiprocess    = $build_multiprocess"
echo "  with experimental syscall sandbox support = $use_syscall_sandbox"
echo "  with libs       = $build_bitcoin_libs"
echo "  with wallet     = $enable_wallet"
if test "x$enable_wallet" != "xno"; then
    echo "    with sqlite   = $use_sqlite"
    echo "    with bdb      = $use_bdb"
fi
echo "  with gui / qt   = $bitcoin_enable_qt"
if test x$bitcoin_enable_qt != xno; then
    echo "    with qr       = $use_qr"
fi
echo "  with zmq        = $use_zmq"
if test x$enable_fuzz == xno; then
    echo "  with test       = $use_tests"
else
    echo "  with test       = not building test_bitcoin because fuzzing is enabled"
    echo "    with fuzz     = $enable_fuzz"
fi
echo "  with bench      = $use_bench"
echo "  with upnp       = $use_upnp"
echo "  with natpmp     = $use_natpmp"
echo "  use asm         = $use_asm"
echo "  ebpf tracing    = $have_sdt"
echo "  sanitizers      = $use_sanitizers"
echo "  debug enabled   = $enable_debug"
echo "  gprof enabled   = $enable_gprof"
echo "  werror          = $enable_werror"
echo
echo "  target os       = $TARGET_OS"
echo "  build os        = $build_os"
echo
echo "  CC              = $CC"
echo "  CFLAGS          = $PTHREAD_CFLAGS $CFLAGS"
echo "  CPPFLAGS        = $DEBUG_CPPFLAGS $HARDENED_CPPFLAGS $CPPFLAGS"
echo "  CXX             = $CXX"
echo "  CXXFLAGS        = $DEBUG_CXXFLAGS $HARDENED_CXXFLAGS $WARN_CXXFLAGS $NOWARN_CXXFLAGS $ERROR_CXXFLAGS $GPROF_CXXFLAGS $CXXFLAGS"
echo "  LDFLAGS         = $PTHREAD_LIBS $HARDENED_LDFLAGS $GPROF_LDFLAGS $LDFLAGS"
echo "  ARFLAGS         = $ARFLAGS"
echo
