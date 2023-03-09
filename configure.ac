AC_PREREQ([2.69])
define(_CLIENT_VERSION_MAJOR, 4)
define(_CLIENT_VERSION_MINOR, 4)
define(_CLIENT_VERSION_BUILD, 0)
define(_CLIENT_VERSION_RC, 0)
define(_CLIENT_VERSION_IS_RELEASE, true)
define(_COPYRIGHT_YEAR, 2023)
define(_COPYRIGHT_HOLDERS,[The %s developers])
define(_COPYRIGHT_HOLDERS_SUBSTITUTION,[[Syscoin Core]])
AC_INIT([Syscoin Core],m4_join([.], _CLIENT_VERSION_MAJOR, _CLIENT_VERSION_MINOR, _CLIENT_VERSION_BUILD)m4_if(_CLIENT_VERSION_RC, [0], [], [rc]_CLIENT_VERSION_RC),[https://github.com/syscoin/syscoin/issues],[syscoin],[https://syscoincore.org/])
AC_CONFIG_SRCDIR([src/validation.cpp])
AC_CONFIG_HEADERS([src/config/syscoin-config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])

m4_ifndef([PKG_PROG_PKG_CONFIG], [m4_fatal([PKG_PROG_PKG_CONFIG macro not found. Please install pkg-config and re-run autogen.sh])])
PKG_PROG_PKG_CONFIG
if test "$PKG_CONFIG" = ""; then
  AC_MSG_ERROR([pkg-config not found])
fi

# When compiling with depends, the `PKG_CONFIG_PATH` and `PKG_CONFIG_LIBDIR` variables,
# being set in a `config.site` file, are not exported to let the `--config-cache` option
# work properly.
if test -n "$PKG_CONFIG_PATH"; then
  PKG_CONFIG="env PKG_CONFIG_PATH=$PKG_CONFIG_PATH $PKG_CONFIG"
fi
if test -n "$PKG_CONFIG_LIBDIR"; then
  PKG_CONFIG="env PKG_CONFIG_LIBDIR=$PKG_CONFIG_LIBDIR $PKG_CONFIG"
fi

SYSCOIN_DAEMON_NAME=syscoind
SYSCOIN_GUI_NAME=syscoin-qt
SYSCOIN_TEST_NAME=test_syscoin
SYSCOIN_CLI_NAME=syscoin-cli
SYSCOIN_TX_NAME=syscoin-tx
SYSCOIN_UTIL_NAME=syscoin-util
SYSCOIN_CHAINSTATE_NAME=syscoin-chainstate
SYSCOIN_WALLET_TOOL_NAME=syscoin-wallet
SYSCOIN_GETH=sysgeth
dnl Multi Process
SYSCOIN_MP_NODE_NAME=syscoin-node
SYSCOIN_MP_GUI_NAME=syscoin-gui

dnl Unless the user specified ARFLAGS, force it to be cr
dnl This is also the default as-of libtool 2.4.7
AC_ARG_VAR([ARFLAGS], [Flags for the archiver, defaults to <cr> if not set])
if test "${ARFLAGS+set}" != "set"; then
  ARFLAGS="cr"
fi

AC_CANONICAL_HOST

AH_TOP([#ifndef SYSCOIN_CONFIG_H])
AH_TOP([#define SYSCOIN_CONFIG_H])
AH_BOTTOM([#endif //SYSCOIN_CONFIG_H])

dnl Automake init set-up and checks
AM_INIT_AUTOMAKE([1.13 no-define subdir-objects foreign])

AM_MAINTAINER_MODE([enable])

dnl make the compilation flags quiet unless V=1 is used
AM_SILENT_RULES([yes])

dnl Compiler checks (here before libtool).
if test "${CXXFLAGS+set}" = "set"; then
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

     dnl Remove unwanted -DDLL_EXPORT from these variables.
     dnl We do not use this macro, but system headers may export unwanted symbols
     dnl if it's set.
     lt_cv_prog_compiler_pic="-DPIC"
     lt_cv_prog_compiler_pic_CXX="-DPIC"
  ;;
esac

AC_ARG_WITH([seccomp],
  [AS_HELP_STRING([--with-seccomp],
  [enable experimental syscall sandbox feature (-sandbox), default is yes if seccomp-bpf is detected under Linux x86_64])],
  [seccomp_found=$withval],
  [seccomp_found=auto])

AC_ARG_ENABLE([c++20],
  [AS_HELP_STRING([--enable-c++20],
  [enable compilation in c++20 mode (disabled by default)])],
  [use_cxx20=$enableval],
  [use_cxx20=no])

dnl Require C++17 compiler (no GNU extensions)
if test "$use_cxx20" = "no"; then
AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])
else
AX_CXX_COMPILE_STDCXX([20], [noext], [mandatory])
fi

dnl check if additional link flags are required for std::filesystem
CHECK_FILESYSTEM

dnl Unless the user specified OBJCXX, force it to be the same as CXX. This ensures
dnl that we get the same -std flags for both.
m4_ifdef([AC_PROG_OBJCXX],[
if test "${OBJCXX+set}" = ""; then
  OBJCXX="${CXX}"
fi
AC_PROG_OBJCXX
])

dnl OpenBSD ships with 2.4.2
LT_PREREQ([2.4.2])
dnl Libtool init checks.
LT_INIT([pic-only win32-dll])

dnl Check/return PATH for base programs.
AC_PATH_TOOL([AR], [ar])
AC_PATH_TOOL([GCOV], [gcov])
AC_PATH_TOOL([LLVM_COV], [llvm-cov])
AC_PATH_PROG([LCOV], [lcov])
dnl Python 3.7 is specified in .python-version and should be used if available, see doc/dependencies.md
AC_PATH_PROGS([PYTHON], [python3.7 python3.8 python3.9 python3.10 python3.11 python3.12 python3 python])
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

AC_ARG_ENABLE([usdt],
  [AS_HELP_STRING([--enable-usdt],
  [enable tracepoints for Userspace, Statically Defined Tracing (default is yes if sys/sdt.h is found)])],
  [use_usdt=$enableval],
  [use_usdt=yes])

AC_ARG_WITH([miniupnpc],
  [AS_HELP_STRING([--with-miniupnpc],
  [enable UPNP (default is yes if libminiupnpc is found)])],
  [use_upnp=$withval],
  [use_upnp=auto])

AC_ARG_WITH([natpmp],
            [AS_HELP_STRING([--with-natpmp],
                            [enable NAT-PMP (default is yes if libnatpmp is found)])],
            [use_natpmp=$withval],
            [use_natpmp=auto])

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
dnl too much, so that it becomes difficult to spot Syscoin Core warnings
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

if test "$use_asm" = "yes"; then
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
  [build multiprocess syscoin-node, syscoin-wallet, and syscoin-gui executables in addition to monolithic syscoind and syscoin-qt executables. Requires libmultiprocess library. Experimental (default is no)])],
  [enable_multiprocess=$enableval],
  [enable_multiprocess=no])

AC_ARG_ENABLE(man,
    [AS_HELP_STRING([--disable-man],
                    [do not install man pages (default is to install)])],,
    enable_man=yes)
AM_CONDITIONAL([ENABLE_MAN], [test "$enable_man" != "no"])

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
                    [Treat certain compiler warnings as errors (default is no)])],
    [enable_werror=$enableval],
    [enable_werror=no])

AC_ARG_ENABLE([external-signer],
    [AS_HELP_STRING([--enable-external-signer],[compile external signer support (default is auto, requires Boost::Process)])],
    [use_external_signer=$enableval],
    [use_external_signer=auto])

AC_ARG_ENABLE([lto],
    [AS_HELP_STRING([--enable-lto],[build using LTO (default is no)])],
    [enable_lto=$enableval],
    [enable_lto=no])

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

if test "$enable_debug" = "yes"; then
  dnl If debugging is enabled, and the user hasn't overridden CXXFLAGS, clear
  dnl them, to prevent autoconfs "-g -O2" being added. Otherwise we'd end up
  dnl with "-O0 -g3 -g -O2".
  if test "$CXXFLAGS_overridden" = "no"; then
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
  AX_CHECK_PREPROC_FLAG([-DDEBUG_LOCKCONTENTION], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DDEBUG_LOCKCONTENTION"], [], [$CXXFLAG_WERROR])
  AX_CHECK_PREPROC_FLAG([-DRPC_DOC_CHECK], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DRPC_DOC_CHECK"], [], [$CXXFLAG_WERROR])
  AX_CHECK_PREPROC_FLAG([-DABORT_ON_FAILED_ASSUME], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DABORT_ON_FAILED_ASSUME"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-ftrapv], [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -ftrapv"], [], [$CXXFLAG_WERROR])
fi

if test "$enable_lto" = "yes"; then
  AX_CHECK_COMPILE_FLAG([-flto], [LTO_CXXFLAGS="$LTO_CXXFLAGS -flto"], [AC_MSG_ERROR([compile failed with -flto])], [$CXXFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-flto], [LTO_LDFLAGS="$LTO_LDFLAGS -flto"], [AC_MSG_ERROR([link failed with -flto])], [$CXXFLAG_WERROR])
fi

if test "$use_sanitizers" != ""; then
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
if test "$enable_werror" = "yes"; then
  if test "$CXXFLAG_WERROR" = ""; then
    AC_MSG_ERROR([enable-werror set but -Werror is not usable])
  fi
  AX_CHECK_COMPILE_FLAG([-Werror=gnu],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=gnu -Wno-error=gnu-zero-variadic-macro-arguments"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Werror=vla],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=vla"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Werror=switch],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=switch"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Werror=thread-safety],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=thread-safety"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Werror=range-loop-analysis],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=range-loop-analysis"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Werror=unused-variable],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=unused-variable"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Werror=date-time],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=date-time"],,[[$CXXFLAG_WERROR]])

  dnl -Wreturn-type is broken in GCC for MinGW-w64.
  dnl https://sourceforge.net/p/mingw-w64/bugs/306/
  AX_CHECK_COMPILE_FLAG([-Werror=return-type], [], [ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Wno-error=return-type"], [$CXXFLAG_WERROR],
                        [AC_LANG_SOURCE([[#include <cassert>
                                          int f(){ assert(false); }]])])

  AX_CHECK_COMPILE_FLAG([-Werror=conditional-uninitialized],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=conditional-uninitialized"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Werror=sign-compare],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=sign-compare"],,[[$CXXFLAG_WERROR]])
  dnl -Wsuggest-override is broken with GCC before 9.2
  dnl https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78010
  AX_CHECK_COMPILE_FLAG([-Werror=suggest-override],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=suggest-override"],,[[$CXXFLAG_WERROR]],
                        [AC_LANG_SOURCE([[struct A { virtual void f(); }; struct B : A { void f() final; };]])])
  AX_CHECK_COMPILE_FLAG([-Werror=unreachable-code-loop-increment],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=unreachable-code-loop-increment"],,[[$CXXFLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Werror=mismatched-tags], [ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=mismatched-tags"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Werror=implicit-fallthrough], [ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=implicit-fallthrough"], [], [$CXXFLAG_WERROR])

  if test x$suppress_external_warnings = xno ; then
    AX_CHECK_COMPILE_FLAG([-Werror=documentation],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=documentation"],,[[$CXXFLAG_WERROR]])
  fi
fi

if test "$CXXFLAGS_overridden" = "no"; then
  AX_CHECK_COMPILE_FLAG([-Wall], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wall"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wextra], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wextra"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wgnu], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wgnu"], [], [$CXXFLAG_WERROR])
  dnl some compilers will ignore -Wformat-security without -Wformat, so just combine the two here.
  AX_CHECK_COMPILE_FLAG([-Wformat -Wformat-security], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wformat -Wformat-security"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wvla], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wvla"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wshadow-field], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wshadow-field"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wthread-safety], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wthread-safety"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wloop-analysis], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wloop-analysis"], [], [$CXXFLAG_WERROR])
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

  if test "$suppress_external_warnings" != "no" ; then
    AX_CHECK_COMPILE_FLAG([-Wdocumentation], [WARN_CXXFLAGS="$WARN_CXXFLAGS -Wdocumentation"], [], [$CXXFLAG_WERROR])
  fi

  dnl Some compilers (gcc) ignore unknown -Wno-* options, but warn about all
  dnl unknown options if any other warning is produced. Test the -Wfoo case, and
  dnl set the -Wno-foo case if it works.
  AX_CHECK_COMPILE_FLAG([-Wunused-parameter], [NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-unused-parameter"], [], [$CXXFLAG_WERROR])
  AX_CHECK_COMPILE_FLAG([-Wself-assign], [NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-self-assign"], [], [$CXXFLAG_WERROR])
  if test "$suppress_external_warnings" != "yes" ; then
    AX_CHECK_COMPILE_FLAG([-Wdeprecated-copy], [NOWARN_CXXFLAGS="$NOWARN_CXXFLAGS -Wno-deprecated-copy"], [], [$CXXFLAG_WERROR])
  fi
fi

dnl Don't allow extended (non-ASCII) symbols in identifiers. This is easier for code review.
AX_CHECK_COMPILE_FLAG([-fno-extended-identifiers], [CORE_CXXFLAGS="$CORE_CXXFLAGS -fno-extended-identifiers"], [], [$CXXFLAG_WERROR])

enable_arm_crc=no
enable_arm_shani=no
enable_sse42=no
enable_sse41=no
enable_avx2=no
enable_x86_shani=no

if test "$use_asm" = "yes"; then

dnl Check for optional instruction set support. Enabling these does _not_ imply that all code will
dnl be compiled with them, rather that specific objects/libs may use them after checking for runtime
dnl compatibility.

dnl x86
AX_CHECK_COMPILE_FLAG([-msse4.2], [SSE42_CXXFLAGS="-msse4.2"], [], [$CXXFLAG_WERROR])
AX_CHECK_COMPILE_FLAG([-msse4.1], [SSE41_CXXFLAGS="-msse4.1"], [], [$CXXFLAG_WERROR])
AX_CHECK_COMPILE_FLAG([-mavx -mavx2], [AVX2_CXXFLAGS="-mavx -mavx2"], [], [$CXXFLAG_WERROR])
AX_CHECK_COMPILE_FLAG([-msse4 -msha], [X86_SHANI_CXXFLAGS="-msse4 -msha"], [], [$CXXFLAG_WERROR])

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

if test "$enable_clmul" = "yes"; then
  CLMUL_CXXFLAGS="-mpclmul"
  AC_DEFINE([HAVE_CLMUL], [1], [Define this symbol if clmul instructions can be used])
fi

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$SSE42_CXXFLAGS $CXXFLAGS"
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
CXXFLAGS="$SSE41_CXXFLAGS $CXXFLAGS"
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
CXXFLAGS="$AVX2_CXXFLAGS $CXXFLAGS"
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
CXXFLAGS="$X86_SHANI_CXXFLAGS $CXXFLAGS"
AC_MSG_CHECKING([for x86 SHA-NI intrinsics])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <stdint.h>
    #include <immintrin.h>
  ]],[[
    __m128i i = _mm_set1_epi32(0);
    __m128i j = _mm_set1_epi32(1);
    __m128i k = _mm_set1_epi32(2);
    return _mm_extract_epi32(_mm_sha256rnds2_epu32(i, i, k), 0);
  ]])],
 [ AC_MSG_RESULT([yes]); enable_x86_shani=yes; AC_DEFINE([ENABLE_X86_SHANI], [1], [Define this symbol to build code that uses x86 SHA-NI intrinsics]) ],
 [ AC_MSG_RESULT([no])]
)
CXXFLAGS="$TEMP_CXXFLAGS"

# ARM
AX_CHECK_COMPILE_FLAG([-march=armv8-a+crc], [ARM_CRC_CXXFLAGS="-march=armv8-a+crc"], [], [$CXXFLAG_WERROR])
AX_CHECK_COMPILE_FLAG([-march=armv8-a+crypto], [ARM_SHANI_CXXFLAGS="-march=armv8-a+crypto"], [], [$CXXFLAG_WERROR])

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$ARM_CRC_CXXFLAGS $CXXFLAGS"
AC_MSG_CHECKING([for ARMv8 CRC32 intrinsics])
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

TEMP_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$ARM_SHANI_CXXFLAGS $CXXFLAGS"
AC_MSG_CHECKING([for ARMv8 SHA-NI intrinsics])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <arm_acle.h>
    #include <arm_neon.h>
  ]],[[
    uint32x4_t a, b, c;
    vsha256h2q_u32(a, b, c);
    vsha256hq_u32(a, b, c);
    vsha256su0q_u32(a, b);
    vsha256su1q_u32(a, b, c);
  ]])],
 [ AC_MSG_RESULT([yes]); enable_arm_shani=yes; AC_DEFINE([ENABLE_ARM_SHANI], [1], [Define this symbol to build code that uses ARMv8 SHA-NI intrinsics]) ],
 [ AC_MSG_RESULT([no])]
)
CXXFLAGS="$TEMP_CXXFLAGS"

fi

CORE_CPPFLAGS="$CORE_CPPFLAGS -DHAVE_BUILD_INFO"

AC_ARG_WITH([utils],
  [AS_HELP_STRING([--with-utils],
  [build syscoin-cli syscoin-tx syscoin-util syscoin-wallet (default=yes)])],
  [build_syscoin_utils=$withval],
  [build_syscoin_utils=yes])

AC_ARG_ENABLE([util-cli],
  [AS_HELP_STRING([--enable-util-cli],
  [build syscoin-cli])],
  [build_syscoin_cli=$enableval],
  [build_syscoin_cli=$build_syscoin_utils])

AC_ARG_ENABLE([util-tx],
  [AS_HELP_STRING([--enable-util-tx],
  [build syscoin-tx])],
  [build_syscoin_tx=$enableval],
  [build_syscoin_tx=$build_syscoin_utils])

AC_ARG_ENABLE([util-wallet],
  [AS_HELP_STRING([--enable-util-wallet],
  [build syscoin-wallet])],
  [build_syscoin_wallet=$enableval],
  [build_syscoin_wallet=$build_syscoin_utils])

AC_ARG_ENABLE([util-util],
  [AS_HELP_STRING([--enable-util-util],
  [build syscoin-util])],
  [build_syscoin_util=$enableval],
  [build_syscoin_util=$build_syscoin_utils])

AC_ARG_ENABLE([experimental-util-chainstate],
  [AS_HELP_STRING([--enable-experimental-util-chainstate],
  [build experimental syscoin-chainstate executable (default=no)])],
  [build_syscoin_chainstate=$enableval],
  [build_syscoin_chainstate=no])

AC_ARG_WITH([libs],
  [AS_HELP_STRING([--with-libs],
  [build libraries (default=yes)])],
  [build_syscoin_libs=$withval],
  [build_syscoin_libs=yes])

AC_ARG_WITH([experimental-kernel-lib],
  [AS_HELP_STRING([--with-experimental-kernel-lib],
  [build experimental syscoinkernel library (default is to build if we're building libraries and the experimental build-chainstate executable)])],
  [build_experimental_kernel_lib=$withval],
  [build_experimental_kernel_lib=auto])

AC_ARG_WITH([daemon],
  [AS_HELP_STRING([--with-daemon],
  [build syscoind daemon (default=yes)])],
  [build_syscoind=$withval],
  [build_syscoind=yes])

case $host in
  *mingw*)
     TARGET_OS=windows
     AC_CHECK_LIB([kernel32], [GetModuleFileNameA],, AC_MSG_ERROR(libkernel32 missing))
     AC_CHECK_LIB([user32],   [main],, AC_MSG_ERROR(libuser32 missing))
     AC_CHECK_LIB([gdi32],    [main],, AC_MSG_ERROR(libgdi32 missing))
     AC_CHECK_LIB([comdlg32], [main],, AC_MSG_ERROR(libcomdlg32 missing))
     AC_CHECK_LIB([winmm],    [main],, AC_MSG_ERROR(libwinmm missing))
     AC_CHECK_LIB([shell32],  [SHGetSpecialFolderPathW],, AC_MSG_ERROR(libshell32 missing))
     AC_CHECK_LIB([comctl32], [main],, AC_MSG_ERROR(libcomctl32 missing))
     AC_CHECK_LIB([ole32],    [CoCreateInstance],, AC_MSG_ERROR(libole32 missing))
     AC_CHECK_LIB([oleaut32], [main],, AC_MSG_ERROR(liboleaut32 missing))
     AC_CHECK_LIB([uuid],     [main],, AC_MSG_ERROR(libuuid missing))
     AC_CHECK_LIB([advapi32], [CryptAcquireContextW],, AC_MSG_ERROR(libadvapi32 missing))
     AC_CHECK_LIB([ws2_32],   [WSAStartup],, AC_MSG_ERROR(libws2_32 missing))
     AC_CHECK_LIB([shlwapi],  [PathRemoveFileSpecW],, AC_MSG_ERROR(libshlwapi missing))
     AC_CHECK_LIB([iphlpapi], [GetAdaptersAddresses],, AC_MSG_ERROR(libiphlpapi missing))
     AC_CHECK_LIB([psapi],    [GetProcessMemoryInfo],, [AC_MSG_ERROR([libpsapi missing])])
     AC_CHECK_LIB([bcrypt],   [main],, [AC_MSG_ERROR([libbcrypt missing])])

     dnl -static is interpreted by libtool, where it has a different meaning.
     dnl In libtool-speak, it's -all-static.
     AX_CHECK_LINK_FLAG([-static], [LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static"])

     AC_PATH_PROG([MAKENSIS], [makensis], [none])
     if test "$MAKENSIS" = "none"; then
       AC_MSG_WARN([makensis not found. Cannot create installer.])
     fi

     AC_PATH_TOOL([WINDRES], [windres], [none])
     if test "$WINDRES" = "none"; then
       AC_MSG_ERROR([windres not found])
     fi

     CORE_CPPFLAGS="$CORE_CPPFLAGS -D_MT -DWIN32 -D_WINDOWS -D_WIN32_WINNT=0x0601 -D_WIN32_IE=0x0501 -DWIN32_LEAN_AND_MEAN"
     dnl Prevent the definition of min/max macros.
     dnl We always want to use the standard library.
     CORE_CPPFLAGS="$CORE_CPPFLAGS -DNOMINMAX"

     dnl libtool insists upon adding -nostdlib and a list of objects/libs to link against.
     dnl That breaks our ability to build dll's with static libgcc/libstdc++/libssp. Override
     dnl its command here, with the predeps/postdeps removed, and -static inserted. Postdeps are
     dnl also overridden to prevent their insertion later.
     dnl This should only affect dll's.
     archive_cmds_CXX="\$CC -shared \$libobjs \$deplibs \$compiler_flags -static -o \$output_objdir/\$soname \${wl}--enable-auto-image-base -Xlinker --out-implib -Xlinker \$lib"
     postdeps_CXX=

     dnl We require Windows 7 (NT 6.1) or later
     AX_CHECK_LINK_FLAG([-Wl,--major-subsystem-version -Wl,6 -Wl,--minor-subsystem-version -Wl,1], [CORE_LDFLAGS="$CORE_LDFLAGS -Wl,--major-subsystem-version -Wl,6 -Wl,--minor-subsystem-version -Wl,1"], [], [$LDFLAG_WERROR])
     ;;
  *darwin*)
     TARGET_OS=darwin
     if  test $cross_compiling != "yes"; then
       BUILD_OS=darwin
       AC_CHECK_PROG([BREW], [brew], [brew])
       if test "$BREW" = "brew"; then
         dnl These Homebrew packages may be keg-only, meaning that they won't be found
         dnl in expected paths because they may conflict with system files. Ask
         dnl Homebrew where each one is located, then adjust paths accordingly.
         dnl It's safe to add these paths even if the functionality is disabled by
         dnl the user (--without-wallet or --without-gui for example).

         dnl Homebrew may create symlinks in /usr/local/include for some packages.
         dnl Because MacOS's clang internally adds "-I /usr/local/include" to its search
         dnl paths, this will negate efforts to use -isystem for those packages, as they
         dnl will be found first in /usr/local. Use the internal "-internal-isystem"
         dnl option to system-ify all /usr/local/include paths without adding it to the list
         dnl of search paths in case it's not already there.
         if test "$suppress_external_warnings" != "no"; then
           AX_CHECK_PREPROC_FLAG([-Xclang -internal-isystem/usr/local/include], [CORE_CPPFLAGS="$CORE_CPPFLAGS -Xclang -internal-isystem/usr/local/include"], [], [$CXXFLAG_WERROR])
         fi

         if test "$use_bdb" != "no" && $BREW list --versions berkeley-db@4 >/dev/null && test "$BDB_CFLAGS" = "" && test "$BDB_LIBS" = ""; then
           bdb_prefix=$($BREW --prefix berkeley-db@4 2>/dev/null)
           dnl This must precede the call to SYSCOIN_FIND_BDB48 below.
           BDB_CFLAGS="-I$bdb_prefix/include"
           BDB_LIBS="-L$bdb_prefix/lib -ldb_cxx-4.8"
         fi

         if $BREW list --versions qt@5 >/dev/null; then
           export PKG_CONFIG_PATH="$($BREW --prefix qt@5 2>/dev/null)/lib/pkgconfig:$PKG_CONFIG_PATH"
         fi

         case $host in
           *aarch64*)
             dnl The preferred Homebrew prefix for Apple Silicon is /opt/homebrew.
             dnl Therefore, as we do not use pkg-config to detect miniupnpc and libnatpmp
             dnl packages, we should set the CPPFLAGS and LDFLAGS variables for them
             dnl explicitly.
             if test "$use_upnp" != "no" && $BREW list --versions miniupnpc >/dev/null; then
               miniupnpc_prefix=$($BREW --prefix miniupnpc 2>/dev/null)
               if test "$suppress_external_warnings" != "no"; then
                 MINIUPNPC_CPPFLAGS="-isystem $miniupnpc_prefix/include"
               else
                 MINIUPNPC_CPPFLAGS="-I$miniupnpc_prefix/include"
               fi
               MINIUPNPC_LIBS="-L$miniupnpc_prefix/lib"
             fi
             if test "$use_natpmp" != "no" && $BREW list --versions libnatpmp >/dev/null; then
               libnatpmp_prefix=$($BREW --prefix libnatpmp 2>/dev/null)
               if test "$suppress_external_warnings" != "no"; then
                 NATPMP_CPPFLAGS="-isystem $libnatpmp_prefix/include"
               else
                 NATPMP_CPPFLAGS="-I$libnatpmp_prefix/include"
               fi
               NATPMP_LIBS="-L$libnatpmp_prefix/lib"
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
           AC_PATH_TOOL([INSTALL_NAME_TOOL], [install_name_tool], [install_name_tool])
           AC_PATH_TOOL([OTOOL], [otool], [otool])
           AC_PATH_PROGS([XORRISOFS], [xorrisofs], [xorrisofs])

           dnl libtool will try to strip the static lib, which is a problem for
           dnl cross-builds because strip attempts to call a hard-coded ld,
           dnl which may not exist in the path. Stripping the .a is not
           dnl necessary, so just disable it.
           old_striplib=
           ;;
       esac
     fi

     AX_CHECK_LINK_FLAG([-Wl,-headerpad_max_install_names], [CORE_LDFLAGS="$CORE_LDFLAGS -Wl,-headerpad_max_install_names"], [], [$LDFLAG_WERROR])
     CORE_CPPFLAGS="$CORE_CPPFLAGS -DMAC_OSX -DOBJC_OLD_DISPATCH_PROTOTYPES=0"
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
        *) AC_MSG_ERROR([Could not determine Android arch, or it is unsupported]) ;;
      esac
     ;;
   *linux*)
     TARGET_OS=linux
     ;;
esac

case $host in
  *x86_64*)
     TARGET_ARCH=x86_64
     ;;
   *aarch64*)
     TARGET_ARCH=aarch64
     ;;
   *arm64*)
     TARGET_ARCH=arm64
     ;;
   *arm*)
     TARGET_ARCH=arm
     ;;
   *i686*)
     TARGET_ARCH=x86_32
     ;;
   *x86_32*)
     TARGET_ARCH=x86_32
     ;;
   *) AC_MSG_ERROR([Could not determine target arch, or it is unsupported]) ;;
esac

if test "$use_extended_functional_tests" != "no"; then
  AC_SUBST(EXTENDED_FUNCTIONAL_TESTS, --extended)
fi

if test "$use_lcov" = "yes"; then
  if test "$LCOV" = ""; then
    AC_MSG_ERROR([lcov testing requested but lcov not found])
  fi
  if test "$PYTHON" = ""; then
    AC_MSG_ERROR([lcov testing requested but python not found])
  fi
  if test "$GENHTML" = ""; then
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
      if test "$LLVM_COV" = ""; then
        AC_MSG_ERROR([lcov testing requested but llvm-cov not found])
      fi
      COV_TOOL="$LLVM_COV gcov"
    ],[
      AC_MSG_RESULT([no])
      if test "$GCOV" = "x"; then
        AC_MSG_ERROR([lcov testing requested but gcov not found])
      fi
      COV_TOOL="$GCOV"
  ])
  AC_SUBST(COV_TOOL)
  AC_SUBST(COV_TOOL_WRAPPER, "cov_tool_wrapper.sh")
  LCOV="$LCOV --gcov-tool $(pwd)/$COV_TOOL_WRAPPER"

  AX_CHECK_LINK_FLAG([--coverage], [CORE_LDFLAGS="$CORE_LDFLAGS --coverage"],
    [AC_MSG_ERROR([lcov testing requested but --coverage linker flag does not work])])
  AX_CHECK_COMPILE_FLAG([--coverage],[CORE_CXXFLAGS="$CORE_CXXFLAGS --coverage"],
    [AC_MSG_ERROR([lcov testing requested but --coverage flag does not work])])
  dnl If coverage is enabled, and the user hasn't overridden CXXFLAGS, clear
  dnl them, to prevent autoconfs "-g -O2" being added. Otherwise we'd end up
  dnl with "--coverage -Og -O0 -g -O2".
  if test "$CXXFLAGS_overridden" = "no"; then
  CXXFLAGS=""
  fi
  CORE_CXXFLAGS="$CORE_CXXFLAGS -Og -O0"
fi

if test "$use_lcov_branch" != "no"; then
  AC_SUBST(LCOV_OPTS, "$LCOV_OPTS --rc lcov_branch_coverage=1")
fi

dnl Check for endianness
AC_C_BIGENDIAN

dnl Check for pthread compile/link requirements
AX_PTHREAD

dnl Check if -latomic is required for <std::atomic>
CHECK_ATOMIC

dnl The following macro will add the necessary defines to syscoin-config.h, but
dnl they also need to be passed down to any subprojects. Pull the results out of
dnl the cache and add them to CPPFLAGS.
AC_SYS_LARGEFILE
dnl detect POSIX or GNU variant of strerror_r
AC_FUNC_STRERROR_R

if test "$ac_cv_sys_file_offset_bits" != "" &&
   test "$ac_cv_sys_file_offset_bits" != "no" &&
   test "$ac_cv_sys_file_offset_bits" != "unknown"; then
  CORE_CPPFLAGS="$CORE_CPPFLAGS -D_FILE_OFFSET_BITS=$ac_cv_sys_file_offset_bits"
fi

if test "$ac_cv_sys_large_files" != "" &&
   test "$ac_cv_sys_large_files" != "no" &&
   test "$ac_cv_sys_large_files" != "unknown"; then
  CORE_CPPFLAGS="$CORE_CPPFLAGS -D_LARGE_FILES=$ac_cv_sys_large_files"
fi

AC_SEARCH_LIBS([clock_gettime],[rt])

if test "$enable_gprof" = "yes"; then
    dnl -pg is incompatible with -pie. Since hardening and profiling together doesn't make sense,
    dnl we simply make them mutually exclusive here. Additionally, hardened toolchains may force
    dnl -pie by default, in which case it needs to be turned off with -no-pie.

    if test "$use_hardening" = "yes"; then
        AC_MSG_ERROR([gprof profiling is not compatible with hardening. Reconfigure with --disable-hardening or --disable-gprof])
    fi
    use_hardening=no
    AX_CHECK_COMPILE_FLAG([-pg],[GPROF_CXXFLAGS="-pg"],
        [AC_MSG_ERROR([gprof profiling requested but not available])], [$CXXFLAG_WERROR])

    AX_CHECK_LINK_FLAG([-no-pie], [GPROF_LDFLAGS="-no-pie"])
    AX_CHECK_LINK_FLAG([-pg], [GPROF_LDFLAGS="$GPROF_LDFLAGS -pg"],
        [AC_MSG_ERROR([gprof profiling requested but not available])], [$GPROF_LDFLAGS])
fi

if test "$TARGET_OS" != "windows"; then
  dnl All windows code is PIC, forcing it on just adds useless compile warnings
  AX_CHECK_COMPILE_FLAG([-fPIC], [PIC_FLAGS="-fPIC"])
fi

dnl Versions of gcc prior to 12.1 (commit
dnl https://github.com/gcc-mirror/gcc/commit/551aa75778a4c5165d9533cd447c8fc822f583e1)
dnl are subject to a bug, see the gccbug_90348 test case and
dnl https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90348. To work around that, set
dnl -fstack-reuse=none for all gcc builds. (Only gcc understands this flag)
AX_CHECK_COMPILE_FLAG([-fstack-reuse=none], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fstack-reuse=none"])
if test "$use_hardening" != "no"; then
  use_hardening=yes
  AX_CHECK_COMPILE_FLAG([-Wstack-protector], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -Wstack-protector"])
  AX_CHECK_COMPILE_FLAG([-fstack-protector-all], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fstack-protector-all"])

  AX_CHECK_COMPILE_FLAG([-fcf-protection=full], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -fcf-protection=full"])

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
  if test "$enable_debug" != "yes"; then
    AX_CHECK_PREPROC_FLAG([-D_FORTIFY_SOURCE=3],[
      AX_CHECK_PREPROC_FLAG([-U_FORTIFY_SOURCE],[
        HARDENED_CPPFLAGS="$HARDENED_CPPFLAGS -U_FORTIFY_SOURCE"
      ])
      HARDENED_CPPFLAGS="$HARDENED_CPPFLAGS -D_FORTIFY_SOURCE=3"
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
if test "$TARGET_OS" = "darwin"; then
  AX_CHECK_LINK_FLAG([-Wl,-dead_strip], [CORE_LDFLAGS="$CORE_LDFLAGS -Wl,-dead_strip"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,-dead_strip_dylibs], [CORE_LDFLAGS="$CORE_LDFLAGS -Wl,-dead_strip_dylibs"], [], [$LDFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,-bind_at_load], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-bind_at_load"], [], [$LDFLAG_WERROR])
fi

AC_CHECK_HEADERS([endian.h sys/endian.h byteswap.h sys/select.h sys/prctl.h sys/sysctl.h vm/vm_param.h sys/vmmeter.h sys/resources.h])

AC_CHECK_DECLS([getifaddrs, freeifaddrs],[CHECK_SOCKET],,
    [#include <sys/types.h>
    #include <ifaddrs.h>]
)

dnl These are used for daemonization in syscoind
AC_CHECK_DECLS([fork])
AC_CHECK_DECLS([setsid])

AC_CHECK_DECLS([pipe2])

AC_CHECK_FUNCS([timingsafe_bcmp])

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
    if test "$use_reduce_exports" = "yes"; then
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

if test "$use_thread_local" = "yes" || test "$use_thread_local" = "auto"; then
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
        [[ #include <stdlib.h> ]],
        [[ int nErr = ::_wsystem(NULL); ]]
    )],
    [ AC_MSG_RESULT([yes]); have_any_system=yes],
    [ AC_MSG_RESULT([no]) ]
)

if test "$have_any_system" != "no"; then
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
         [[$(echo $1 |${SED} -E -e 's/(^| )-I/\1-isystem /g' -e 's;-isystem /usr/include/*( |$);-I/usr/include\1;g')]])

dnl enable-fuzz should disable all other targets
if test "$enable_fuzz" = "yes"; then
  AC_MSG_WARN([enable-fuzz will disable all other targets and force --enable-fuzz-binary=yes])
  build_syscoin_utils=no
  build_syscoin_cli=no
  build_syscoin_tx=no
  build_syscoin_util=no
  build_syscoin_chainstate=no
  build_syscoin_wallet=no
  build_syscoind=no
  build_syscoin_libs=no
  syscoin_enable_qt=no
  syscoin_enable_qt_test=no
  syscoin_enable_qt_dbus=no
  use_bench=no
  use_tests=no
  use_external_signer=no
  use_upnp=no
  use_natpmp=no
  use_zmq=no
  enable_fuzz_binary=yes

  AX_CHECK_PREPROC_FLAG([-DABORT_ON_FAILED_ASSUME], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -DABORT_ON_FAILED_ASSUME"], [], [$CXXFLAG_WERROR])
else
  SYSCOIN_QT_INIT

  dnl sets $syscoin_enable_qt, $syscoin_enable_qt_test, $syscoin_enable_qt_dbus
  SYSCOIN_QT_CONFIGURE([5.11.3])

  dnl Keep a copy of the original $QT_INCLUDES and use it when invoking qt's moc
  QT_INCLUDES_UNSUPPRESSED=$QT_INCLUDES
  if test "$suppress_external_warnings" != "no" ; then
    QT_INCLUDES=SUPPRESS_WARNINGS($QT_INCLUDES)
    QT_DBUS_INCLUDES=SUPPRESS_WARNINGS($QT_DBUS_INCLUDES)
    QT_TEST_INCLUDES=SUPPRESS_WARNINGS($QT_TEST_INCLUDES)
  fi
fi

if test "$enable_fuzz_binary" = "yes"; then
  AC_MSG_CHECKING([whether main function is needed for fuzz binary])
  AX_CHECK_LINK_FLAG(
    [],
    [AC_MSG_RESULT([no])],
    [AC_MSG_RESULT([yes]); CORE_CPPFLAGS="$CORE_CPPFLAGS -DPROVIDE_FUZZ_MAIN_FUNCTION"],
    [$SANITIZER_LDFLAGS],
    [AC_LANG_PROGRAM([[
      #include <cstdint>
      #include <cstddef>
      extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) { return 0; }
      /* comment to remove the main function ...
     ]],[[
      */ int not_main() {
     ]])])

  CHECK_RUNTIME_LIB
fi

if test "$enable_wallet" != "no"; then
    dnl Check for libdb_cxx only if wallet enabled
    if test "$use_bdb" != "no"; then
      SYSCOIN_FIND_BDB48
      if test "$suppress_external_warnings" != "no" ; then
        BDB_CPPFLAGS=SUPPRESS_WARNINGS($BDB_CPPFLAGS)
      fi
    fi

    dnl Check for sqlite3
    if test "$use_sqlite" != "no"; then
      PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.7.17], [have_sqlite=yes], [have_sqlite=no])
    fi
    AC_MSG_CHECKING([whether to build wallet with support for sqlite])
    if test "$use_sqlite" = "no"; then
      use_sqlite=no
    elif test "$have_sqlite" = "no"; then
      if test "$use_sqlite" = "yes"; then
        AC_MSG_ERROR([sqlite support requested but cannot be built. Use --without-sqlite])
      fi
      use_sqlite=no
    else
      if test "$use_sqlite" != "no"; then
        AC_DEFINE([USE_SQLITE],[1],[Define if sqlite support should be compiled in])
        use_sqlite=yes
      fi
    fi
    AC_MSG_RESULT([$use_sqlite])

    dnl Disable wallet if both --without-bdb and --without-sqlite
    if test "$use_bdb$use_sqlite" = "nono"; then
        if test "$enable_wallet" = "yes"; then
            AC_MSG_ERROR([wallet functionality requested but no BDB or SQLite support available.])
        fi
        enable_wallet=no
    fi
fi

if test "$use_usdt" != "no"; then
  AC_MSG_CHECKING([whether Userspace, Statically Defined Tracing tracepoints are supported])
  AC_COMPILE_IFELSE([
    AC_LANG_PROGRAM(
      [#include <sys/sdt.h>],
      [DTRACE_PROBE("context", "event");]
    )],
    [AC_MSG_RESULT([yes]); AC_DEFINE([ENABLE_TRACING], [1], [Define to 1 to enable tracepoints for Userspace, Statically Defined Tracing])],
    [AC_MSG_RESULT([no]); use_usdt=no;]
  )
fi
AM_CONDITIONAL([ENABLE_USDT_TRACEPOINTS], [test "$use_usdt" = "yes"])

if test "$build_syscoind$syscoin_enable_qt$use_bench$use_tests" = "nononono"; then
  use_upnp=no
  use_natpmp=no
  use_zmq=no
fi

dnl Check for libminiupnpc (optional)
if test "$use_upnp" != "no"; then
  TEMP_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $MINIUPNPC_CPPFLAGS"
  AC_CHECK_HEADERS(
    [miniupnpc/miniupnpc.h miniupnpc/upnpcommands.h miniupnpc/upnperrors.h],
    [AC_CHECK_LIB([miniupnpc], [upnpDiscover], [MINIUPNPC_LIBS="$MINIUPNPC_LIBS -lminiupnpc"], [have_miniupnpc=no], [$MINIUPNPC_LIBS])],
    [have_miniupnpc=no]
  )

  dnl The minimum supported miniUPnPc API version is set to 17. This excludes
  dnl versions with known vulnerabilities.
  if test "$have_miniupnpc" != "no"; then
    AC_MSG_CHECKING([whether miniUPnPc API version is supported])
    AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[
        @%:@include <miniupnpc/miniupnpc.h>
      ]], [[
        #if MINIUPNPC_API_VERSION >= 17
        // Everything is okay
        #else
        #  error miniUPnPc API version is too old
        #endif
      ]])],[
        AC_MSG_RESULT([yes])
      ],[
      AC_MSG_RESULT([no])
      AC_MSG_WARN([miniUPnPc API version < 17 is unsupported, disabling UPnP support.])
      have_miniupnpc=no
    ])
  fi
  CPPFLAGS="$TEMP_CPPFLAGS"
fi

dnl Check for libnatpmp (optional).
if test "$use_natpmp" != "no"; then
  TEMP_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $NATPMP_CPPFLAGS"
  AC_CHECK_HEADERS([natpmp.h],
                   [AC_CHECK_LIB([natpmp], [initnatpmp], [NATPMP_LIBS="$NATPMP_LIBS -lnatpmp"], [have_natpmp=no], [$NATPMP_LIBS])],
                   [have_natpmp=no])
  CPPFLAGS="$TEMP_CPPFLAGS"
fi

if test "$build_syscoin_wallet$build_syscoin_cli$build_syscoin_tx$build_syscoin_util$build_syscoind$syscoin_enable_qt$use_tests$use_bench$enable_fuzz_binary" = "nonononononononono"; then
  use_boost=no
else
  use_boost=yes
fi

if test "$use_boost" = "yes"; then

  dnl Check for Boost headers
  AX_BOOST_BASE([1.64.0],[],[AC_MSG_ERROR([Boost is not available!])])
  if test "$want_boost" = "no"; then
    AC_MSG_ERROR([only libsyscoinconsensus can be built without Boost])
  fi

  dnl we don't use multi_index serialization
  BOOST_CPPFLAGS="$BOOST_CPPFLAGS -DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION"

  dnl Prevent use of std::unary_function, which was removed in C++17,
  dnl and will generate warnings with newer compilers for Boost
  dnl older than 1.80.
  dnl See: https://github.com/boostorg/config/pull/430.
  AX_CHECK_PREPROC_FLAG([-DBOOST_NO_CXX98_FUNCTION_BASE], [BOOST_CPPFLAGS="$BOOST_CPPFLAGS -DBOOST_NO_CXX98_FUNCTION_BASE"], [], [$CXXFLAG_WERROR],
                        [AC_LANG_PROGRAM([[#include <boost/config.hpp>]])])

  if test "$enable_debug" = "yes" || test "$enable_fuzz" = "yes"; then
    BOOST_CPPFLAGS="$BOOST_CPPFLAGS -DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE"
  fi

  if test "$suppress_external_warnings" != "no"; then
    BOOST_CPPFLAGS=SUPPRESS_WARNINGS($BOOST_CPPFLAGS)
  fi
fi

if test "$use_external_signer" != "no"; then
  AC_MSG_CHECKING([whether Boost.Process can be used])
  TEMP_CXXFLAGS="$CXXFLAGS"
  dnl Boost 1.78 requires the following workaround.
  dnl See: https://github.com/boostorg/process/issues/235
  CXXFLAGS="$CXXFLAGS -Wno-error=narrowing"
  TEMP_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
  TEMP_LDFLAGS="$LDFLAGS"
  dnl Boost 1.73 and older require the following workaround.
  LDFLAGS="$LDFLAGS $PTHREAD_CFLAGS"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #define BOOST_PROCESS_USE_STD_FS
    #include <boost/process.hpp>
  ]],[[
    namespace bp = boost::process;
    bp::opstream stdin_stream;
    bp::ipstream stdout_stream;
    bp::child c("dummy", bp::std_out > stdout_stream, bp::std_err > stdout_stream, bp::std_in < stdin_stream);
    stdin_stream << std::string{"test"} << std::endl;
    if (c.running()) c.terminate();
    c.wait();
    c.exit_code();
  ]])],
    [have_boost_process="yes"],
    [have_boost_process="no"])
  LDFLAGS="$TEMP_LDFLAGS"
  CPPFLAGS="$TEMP_CPPFLAGS"
  CXXFLAGS="$TEMP_CXXFLAGS"
  AC_MSG_RESULT([$have_boost_process])
  if test "$have_boost_process" = "yes"; then
    use_external_signer="yes"
    AC_DEFINE([ENABLE_EXTERNAL_SIGNER], [1], [Define if external signer support is enabled])
    AC_DEFINE([BOOST_PROCESS_USE_STD_FS], [1], [Defined to avoid Boost::Process trying to use Boost Filesystem])
  else
    if test "$use_external_signer" = "yes"; then
      AC_MSG_ERROR([External signing is not supported for this Boost version])
    fi
    use_external_signer="no";
  fi
fi
AM_CONDITIONAL([ENABLE_EXTERNAL_SIGNER], [test "$use_external_signer" = "yes"])

dnl Do not compile with syscall sandbox support when compiling under the sanitizers.
dnl The sanitizers introduce use of syscalls that are not typically used in syscoind
dnl (such as execve when the sanitizers execute llvm-symbolizer).
if test "$use_sanitizers" != ""; then
  AC_MSG_WARN([Specifying --with-sanitizers forces --without-seccomp since the sanitizers introduce use of syscalls not allowed by the syscoind syscall sandbox (-sandbox=<mode>).])
  seccomp_found=no
fi
if test "$seccomp_found" != "no"; then
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
AM_CONDITIONAL([ENABLE_SYSCALL_SANDBOX], [test "$use_syscall_sandbox" != "no"])

dnl Check for reduced exports
if test "$use_reduce_exports" = "yes"; then
  AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CORE_CXXFLAGS="$CORE_CXXFLAGS -fvisibility=hidden"],
  [AC_MSG_ERROR([Cannot set hidden symbol visibility. Use --disable-reduce-exports.])], [$CXXFLAG_WERROR])
  AX_CHECK_LINK_FLAG([-Wl,--exclude-libs,ALL], [RELDFLAGS="-Wl,--exclude-libs,ALL"], [], [$LDFLAG_WERROR])
fi

if test "$use_tests" = "yes"; then

  if test "$HEXDUMP" = ""; then
    AC_MSG_ERROR([hexdump is required for tests])
  fi
fi

dnl libevent check

use_libevent=no
if test "$build_syscoin_cli$build_syscoind$syscoin_enable_qt$enable_fuzz_binary$use_tests$use_bench" != "nononononono"; then
  PKG_CHECK_MODULES([EVENT], [libevent >= 2.1.8], [use_libevent=yes], [AC_MSG_ERROR([libevent version 2.1.8 or greater not found.])])
  if test "$TARGET_OS" != "windows"; then
    PKG_CHECK_MODULES([EVENT_PTHREADS], [libevent_pthreads >= 2.1.8], [], [AC_MSG_ERROR([libevent_pthreads version 2.1.8 or greater not found.])])
  fi

  if test "$suppress_external_warnings" != "no"; then
    EVENT_CFLAGS=SUPPRESS_WARNINGS($EVENT_CFLAGS)
  fi
fi

if test x$use_libevent = xyes; then
  TEMP_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $EVENT_CFLAGS"
  AC_MSG_CHECKING([if evhttp_connection_get_peer expects const char**])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      #include <cstdint>
      #include <event2/http.h>
    ]], [[
      evhttp_connection *conn = (evhttp_connection *)1;
      const char *host;
      uint16_t port;

      evhttp_connection_get_peer(conn, &host, &port);
    ]])],
    [ AC_MSG_RESULT([yes]); AC_DEFINE([HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR], [1], [Define this symbol if evhttp_connection_get_peer expects const char**]) ],
    [ AC_MSG_RESULT([no]) ]
  )
  CXXFLAGS="$TEMP_CXXFLAGS"
fi

dnl QR Code encoding library check

if test "$use_qr" != "no"; then
  SYSCOIN_QT_CHECK([PKG_CHECK_MODULES([QR], [libqrencode], [have_qrencode=yes], [have_qrencode=no])])
fi

dnl ZMQ check

if test "$use_zmq" = "yes"; then
  PKG_CHECK_MODULES([ZMQ], [libzmq >= 4],
    AC_DEFINE([ENABLE_ZMQ], [1], [Define to 1 to enable ZMQ functions]),
    [AC_DEFINE([ENABLE_ZMQ], [0], [Define to 1 to enable ZMQ functions])
    AC_MSG_WARN([libzmq version 4.x or greater not found, disabling])
    use_zmq=no])
else
  AC_DEFINE_UNQUOTED([ENABLE_ZMQ], [0], [Define to 1 to enable ZMQ functions])
fi

if test "$use_zmq" = "yes"; then
  dnl Assume libzmq was built for static linking
  case $host in
    *mingw*)
      ZMQ_CFLAGS="$ZMQ_CFLAGS -DZMQ_STATIC"
    ;;
  esac
fi


dnl libmultiprocess library check

libmultiprocess_found=no
if test "$with_libmultiprocess" = "yes" || test "$with_libmultiprocess" = "auto"; then
  PKG_CHECK_MODULES([LIBMULTIPROCESS], [libmultiprocess], [
     libmultiprocess_found=yes;
     libmultiprocess_prefix=`$PKG_CONFIG --variable=prefix libmultiprocess`;
  ], [true])
elif test "$with_libmultiprocess" != "no"; then
  AC_MSG_ERROR([--with-libmultiprocess=$with_libmultiprocess value is not yes, auto, or no])
fi

dnl Enable multiprocess check

if test "$enable_multiprocess" = "yes"; then
  if test "$libmultiprocess_found" != "yes"; then
    AC_MSG_ERROR([--enable-multiprocess=yes option specified but libmultiprocess library was not found. May need to install libmultiprocess library, or specify install path with PKG_CONFIG_PATH environment variable. Running 'pkg-config --debug libmultiprocess' may be helpful for debugging.])
  fi
  build_multiprocess=yes
elif test "$enable_multiprocess" = "auto"; then
  build_multiprocess=$libmultiprocess_found
else
  build_multiprocess=no
fi

AM_CONDITIONAL([BUILD_MULTIPROCESS], [test "$build_multiprocess" = "yes"])
AM_CONDITIONAL([BUILD_SYSCOIN_NODE], [test "$build_multiprocess" = "yes"])
AM_CONDITIONAL([BUILD_SYSCOIN_GUI], [test "$build_multiprocess" = "yes"])

dnl codegen tools check

if test "$build_multiprocess" != "no"; then
  if test "$with_mpgen" = "yes" || test "$with_mpgen" = "auto"; then
    MPGEN_PREFIX="$libmultiprocess_prefix"
  elif test "$with_mpgen" != "no"; then
    MPGEN_PREFIX="$with_mpgen";
  fi
  AC_SUBST(MPGEN_PREFIX)
fi

dnl check if libgmp is present
AC_CHECK_HEADER([gmp.h],, AC_MSG_ERROR(libgmp headers missing))
AC_CHECK_LIB([gmp],  [__gmpz_init],GMP_LIBS=-lgmp, AC_MSG_ERROR(libgmp missing))

AC_MSG_CHECKING([whether to build syscoind])
AM_CONDITIONAL([BUILD_SYSCOIND], [test $build_syscoind = "yes"])
AC_MSG_RESULT($build_syscoind)

AC_MSG_CHECKING([whether to build syscoin-cli])
AM_CONDITIONAL([BUILD_SYSCOIN_CLI], [test $build_syscoin_cli = "yes"])
AC_MSG_RESULT($build_syscoin_cli)

AC_MSG_CHECKING([whether to build syscoin-tx])
AM_CONDITIONAL([BUILD_SYSCOIN_TX], [test $build_syscoin_tx = "yes"])
AC_MSG_RESULT($build_syscoin_tx)

AC_MSG_CHECKING([whether to build syscoin-wallet])
AM_CONDITIONAL([BUILD_SYSCOIN_WALLET], [test $build_syscoin_wallet = "yes"])
AC_MSG_RESULT($build_syscoin_wallet)

AC_MSG_CHECKING([whether to build syscoin-util])
AM_CONDITIONAL([BUILD_SYSCOIN_UTIL], [test $build_syscoin_util = "yes"])
AC_MSG_RESULT($build_syscoin_util)

AC_MSG_CHECKING([whether to build experimental syscoin-chainstate])
if test "$build_syscoin_chainstate" = "yes"; then
  if test "$build_experimental_kernel_lib" = "no"; then
    AC_MSG_ERROR([experimental syscoin-chainstate cannot be built without the experimental syscoinkernel library. Use --with-experimental-kernel-lib]);
  fi
fi
AM_CONDITIONAL([BUILD_SYSCOIN_CHAINSTATE], [test $build_syscoin_chainstate = "yes"])
AC_MSG_RESULT($build_syscoin_chainstate)

AC_MSG_CHECKING([whether to build libraries])
AM_CONDITIONAL([BUILD_SYSCOIN_LIBS], [test $build_syscoin_libs = "yes"])

if test "$build_syscoin_libs" = "yes"; then
  AC_DEFINE([HAVE_CONSENSUS_LIB], [1], [Define this symbol if the consensus lib has been built])
  AC_CONFIG_FILES([libsyscoinconsensus.pc:libsyscoinconsensus.pc.in])
fi

AM_CONDITIONAL([BUILD_SYSCOIN_KERNEL_LIB], [test "$build_experimental_kernel_lib" != "no" && ( test "$build_experimental_kernel_lib" = "yes" || test "$build_syscoin_chainstate" = "yes" )])

AC_MSG_RESULT($build_syscoin_libs)

AC_LANG_POP

if test "$use_ccache" != "no"; then
  AC_MSG_CHECKING([if ccache should be used])
  if test "$CCACHE" = ""; then
    if test "$use_ccache" = "yes"; then
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
  if test "$use_ccache" = "yes"; then
    AX_CHECK_COMPILE_FLAG([-fdebug-prefix-map=A=B], [DEBUG_CXXFLAGS="$DEBUG_CXXFLAGS -fdebug-prefix-map=\$(abs_top_srcdir)=."], [], [$CXXFLAG_WERROR])
    AX_CHECK_PREPROC_FLAG([-fmacro-prefix-map=A=B], [DEBUG_CPPFLAGS="$DEBUG_CPPFLAGS -fmacro-prefix-map=\$(abs_top_srcdir)=."], [], [$CXXFLAG_WERROR])
  fi
fi

dnl enable wallet
AC_MSG_CHECKING([if wallet should be enabled])
if test "$enable_wallet" != "no"; then
  AC_MSG_RESULT([yes])
  AC_DEFINE_UNQUOTED([ENABLE_WALLET],[1],[Define to 1 to enable wallet functions])
  enable_wallet=yes

else
  AC_MSG_RESULT([no])
fi


dnl enable upnp support
AC_MSG_CHECKING([whether to build with support for UPnP])
if test "$have_miniupnpc" = "no"; then
  if test "$use_upnp" = "yes"; then
     AC_MSG_ERROR([UPnP requested but cannot be built. Use --without-miniupnpc])
  fi
  AC_MSG_RESULT([no])
  use_upnp=no
else
  if test "$use_upnp" != "no"; then
    AC_MSG_RESULT([yes])
    use_upnp=yes
    AC_DEFINE([USE_UPNP], [1], [Define to 1 if UPnP support should be compiled in.])
    if test "$TARGET_OS" = "windows"; then
      MINIUPNPC_CPPFLAGS="$MINIUPNPC_CPPFLAGS -DSTATICLIB -DMINIUPNP_STATICLIB"
    fi
  else
    AC_MSG_RESULT([no])
  fi
fi

dnl Enable NAT-PMP support.
AC_MSG_CHECKING([whether to build with support for NAT-PMP])
if test "$have_natpmp" = "no"; then
  if test "$use_natpmp" = "yes"; then
     AC_MSG_ERROR([NAT-PMP requested but cannot be built. Use --without-natpmp])
  fi
  AC_MSG_RESULT([no])
  use_natpmp=no
else
  if test "$use_natpmp" != "no"; then
    AC_MSG_RESULT([yes])
    use_natpmp=yes
    AC_DEFINE([USE_NATPMP], [1], [Define to 1 if UPnP support should be compiled in.])
    if test "$TARGET_OS" = "windows"; then
      NATPMP_CPPFLAGS="$NATPMP_CPPFLAGS -DSTATICLIB -DNATPMP_STATICLIB"
    fi
  else
    AC_MSG_RESULT([no])
  fi
fi

dnl these are only used when qt is enabled
BUILD_TEST_QT=""
if test "$syscoin_enable_qt" != "no"; then
  dnl enable dbus support
  AC_MSG_CHECKING([whether to build GUI with support for D-Bus])
  if test "$syscoin_enable_qt_dbus" != "no"; then
    AC_DEFINE([USE_DBUS], [1], [Define if dbus support should be compiled in])
  fi
  AC_MSG_RESULT([$syscoin_enable_qt_dbus])

  dnl enable qr support
  AC_MSG_CHECKING([whether to build GUI with support for QR codes])
  if test "$have_qrencode" = "no"; then
    if test "$use_qr" = "yes"; then
      AC_MSG_ERROR([QR support requested but cannot be built. Use --without-qrencode])
    fi
    use_qr=no
  else
    if test "$use_qr" != "no"; then
      AC_DEFINE([USE_QRCODE], [1], [Define if QR support should be compiled in])
      use_qr=yes
    fi
  fi
  AC_MSG_RESULT([$use_qr])

  if test "$XGETTEXT" = ""; then
    AC_MSG_WARN([xgettext is required to update qt translations])
  fi

  AC_MSG_CHECKING([whether to build test_syscoin-qt])
  if test "$use_gui_tests$syscoin_enable_qt_test" = "yesyes"; then
    AC_MSG_RESULT([yes])
    BUILD_TEST_QT="yes"
  else
    AC_MSG_RESULT([no])
  fi
fi

AM_CONDITIONAL([ENABLE_ZMQ], [test "$use_zmq" = "yes"])

AC_MSG_CHECKING([whether to build test_syscoin])
if test "$use_tests" = "yes"; then
  if test "$enable_fuzz" = "yes"; then
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
if test "$use_reduce_exports" = "yes"; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

if test "$build_syscoin_wallet$build_syscoin_cli$build_syscoin_tx$build_syscoin_util$build_syscoin_libs$build_syscoind$syscoin_enable_qt$enable_fuzz_binary$use_bench$use_tests" = "nononononononononono"; then
  AC_MSG_ERROR([No targets! Please specify at least one of: --with-utils --with-libs --with-daemon --with-gui --enable-fuzz(-binary) --enable-bench or --enable-tests])
fi

AM_CONDITIONAL([TARGET_X86_64], [test "$TARGET_ARCH" = "x86_64"])
AM_CONDITIONAL([TARGET_AARCH64], [test "$TARGET_ARCH" = "aarch64"])
AM_CONDITIONAL([TARGET_ARM64], [test "$TARGET_ARCH" = "arm64"])
AM_CONDITIONAL([TARGET_ARM], [test "$TARGET_ARCH" = "arm"])

AM_CONDITIONAL([TARGET_X86_64_LINUX], [test "$TARGET_ARCH-$TARGET_OS" = "x86_64-linux"])
AM_CONDITIONAL([TARGET_ARM_LINUX], [test "$TARGET_ARCH-$TARGET_OS" = "arm-linux"])
AM_CONDITIONAL([TARGET_AARCH64_LINUX], [test "$TARGET_ARCH-$TARGET_OS" = "aarch64-linux"])
AM_CONDITIONAL([TARGET_X86_64_DARWIN], [test "$TARGET_ARCH-$TARGET_OS" = "x86_64-darwin"])
AM_CONDITIONAL([TARGET_ARM64_DARWIN], [test "$TARGET_ARCH-$TARGET_OS" = "arm64-darwin"])

AM_CONDITIONAL([TARGET_DARWIN], [test "$TARGET_OS" = "darwin"])
AM_CONDITIONAL([BUILD_DARWIN], [test "$BUILD_OS" = "darwin"])
AM_CONDITIONAL([TARGET_LINUX], [test "$TARGET_OS" = "linux"])
AM_CONDITIONAL([TARGET_WINDOWS], [test "$TARGET_OS" = "windows"])
AM_CONDITIONAL([ENABLE_WALLET], [test "$enable_wallet" = "yes"])
AM_CONDITIONAL([USE_SQLITE], [test "$use_sqlite" = "yes"])
AM_CONDITIONAL([USE_BDB], [test "$use_bdb" = "yes"])
AM_CONDITIONAL([ENABLE_TESTS], [test "$BUILD_TEST" = "yes"])
AM_CONDITIONAL([ENABLE_FUZZ], [test "$enable_fuzz" = "yes"])
AM_CONDITIONAL([ENABLE_FUZZ_BINARY], [test "$enable_fuzz_binary" = "yes"])
AM_CONDITIONAL([ENABLE_QT], [test "$syscoin_enable_qt" = "yes"])
AM_CONDITIONAL([ENABLE_QT_TESTS], [test "$BUILD_TEST_QT" = "yes"])
AM_CONDITIONAL([ENABLE_BENCH], [test "$use_bench" = "yes"])
AM_CONDITIONAL([USE_QRCODE], [test "$use_qr" = "yes"])
AM_CONDITIONAL([USE_LCOV], [test "$use_lcov" = "yes"])
AM_CONDITIONAL([USE_LIBEVENT], [test "$use_libevent" = "yes"])
AM_CONDITIONAL([HARDEN], [test "$use_hardening" = "yes"])
AM_CONDITIONAL([ENABLE_SSE42], [test "$enable_sse42" = "yes"])
AM_CONDITIONAL([ENABLE_SSE41], [test "$enable_sse41" = "yes"])
AM_CONDITIONAL([ENABLE_AVX2], [test "$enable_avx2" = "yes"])
AM_CONDITIONAL([ENABLE_X86_SHANI], [test "$enable_x86_shani" = "yes"])
AM_CONDITIONAL([ENABLE_ARM_CRC], [test "$enable_arm_crc" = "yes"])
AM_CONDITIONAL([ENABLE_ARM_SHANI], [test "$enable_arm_shani" = "yes"])
AM_CONDITIONAL([USE_ASM], [test "$use_asm" = "yes"])
AM_CONDITIONAL([WORDS_BIGENDIAN], [test "$ac_cv_c_bigendian" = "yes"])
AM_CONDITIONAL([USE_NATPMP], [test "$use_natpmp" = "yes"])
AM_CONDITIONAL([USE_UPNP], [test "$use_upnp" = "yes"])

dnl for minisketch
AM_CONDITIONAL([ENABLE_CLMUL], [test "$enable_clmul" = "yes"])
AM_CONDITIONAL([HAVE_CLZ], [test "$have_clzl$have_clzll" = "yesyes"])

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
AC_SUBST(SYSCOIN_DAEMON_NAME)
AC_SUBST(SYSCOIN_GUI_NAME)
AC_SUBST(SYSCOIN_TEST_NAME)
AC_SUBST(SYSCOIN_CLI_NAME)
AC_SUBST(SYSCOIN_TX_NAME)
AC_SUBST(SYSCOIN_UTIL_NAME)
AC_SUBST(SYSCOIN_CHAINSTATE_NAME)
AC_SUBST(SYSCOIN_WALLET_TOOL_NAME)
AC_SUBST(SYSCOIN_GETH)
AC_SUBST(SYSCOIN_MP_NODE_NAME)
AC_SUBST(SYSCOIN_MP_GUI_NAME)

AC_SUBST(RELDFLAGS)
AC_SUBST(CORE_LDFLAGS)
AC_SUBST(CORE_CPPFLAGS)
AC_SUBST(CORE_CXXFLAGS)
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
AC_SUBST(LTO_CXXFLAGS)
AC_SUBST(LTO_LDFLAGS)
AC_SUBST(PIC_FLAGS)
AC_SUBST(PIE_FLAGS)
AC_SUBST(SANITIZER_CXXFLAGS)
AC_SUBST(SANITIZER_LDFLAGS)
AC_SUBST(SSE42_CXXFLAGS)
AC_SUBST(SSE41_CXXFLAGS)
AC_SUBST(CLMUL_CXXFLAGS)
AC_SUBST(AVX2_CXXFLAGS)
AC_SUBST(X86_SHANI_CXXFLAGS)
AC_SUBST(ARM_CRC_CXXFLAGS)
AC_SUBST(ARM_SHANI_CXXFLAGS)
AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(USE_SQLITE)
AC_SUBST(USE_BDB)
AC_SUBST(ENABLE_EXTERNAL_SIGNER)
AC_SUBST(USE_UPNP)
AC_SUBST(USE_QRCODE)
AC_SUBST(TESTDEFS)
AC_SUBST(MINIUPNPC_CPPFLAGS)
AC_SUBST(MINIUPNPC_LIBS)
AC_SUBST(GMP_LIBS)
AC_SUBST(NATPMP_CPPFLAGS)
AC_SUBST(NATPMP_LIBS)
AC_SUBST(HAVE_GMTIME_R)
AC_SUBST(HAVE_FDATASYNC)
AC_SUBST(HAVE_FULLFSYNC)
AC_SUBST(HAVE_O_CLOEXEC)
AC_SUBST(HAVE_BUILTIN_PREFETCH)
AC_SUBST(HAVE_MM_PREFETCH)
AC_SUBST(HAVE_STRONG_GETAUXVAL)
AC_SUBST(ANDROID_ARCH)
AC_SUBST(HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR)
AC_CONFIG_FILES([Makefile src/Makefile doc/man/Makefile share/setup.nsi share/qt/Info.plist test/config.ini])
AC_CONFIG_FILES([contrib/devtools/split-debug.sh],[chmod +x contrib/devtools/split-debug.sh])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/Doxyfile])])
AC_CONFIG_LINKS([contrib/devtools/iwyu/syscoin.core.imp:contrib/devtools/iwyu/syscoin.core.imp])
AC_CONFIG_LINKS([contrib/filter-lcov.py:contrib/filter-lcov.py])
AC_CONFIG_LINKS([contrib/macdeploy/background.tiff:contrib/macdeploy/background.tiff])
AC_CONFIG_LINKS([src/.bear-tidy-config:src/.bear-tidy-config])
AC_CONFIG_LINKS([src/.clang-tidy:src/.clang-tidy])
AC_CONFIG_LINKS([test/functional/test_runner.py:test/functional/test_runner.py])
AC_CONFIG_LINKS([test/fuzz/test_runner.py:test/fuzz/test_runner.py])
AC_CONFIG_LINKS([test/util/test_runner.py:test/util/test_runner.py])
AC_CONFIG_LINKS([test/util/rpcauth-test.py:test/util/rpcauth-test.py])
AC_CONFIG_LINKS([src/qt/Makefile:src/qt/Makefile])
AC_CONFIG_LINKS([src/qt/test/Makefile:src/qt/test/Makefile])
AC_CONFIG_LINKS([src/test/Makefile:src/test/Makefile])

dnl boost's m4 checks do something really nasty: they export these vars. As a
dnl result, they leak into secp256k1's configure and crazy things happen.
dnl Until this is fixed upstream and we've synced, we'll just un-export them.
CPPFLAGS_TEMP="$CPPFLAGS"
unset CPPFLAGS
CPPFLAGS="$CPPFLAGS_TEMP"

ac_configure_args="${ac_configure_args} --disable-shared --with-pic --enable-benchmark=no --enable-module-recovery --disable-module-ecdh"
AC_CONFIG_SUBDIRS([src/dashbls src/secp256k1])
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
echo "  with libs       = $build_syscoin_libs"
echo "  with wallet     = $enable_wallet"
if test "$enable_wallet" != "no"; then
    echo "    with sqlite   = $use_sqlite"
    echo "    with bdb      = $use_bdb"
fi
echo "  with gui / qt   = $syscoin_enable_qt"
if test $syscoin_enable_qt != "no"; then
    echo "    with qr       = $use_qr"
fi
echo "  with zmq        = $use_zmq"
if test $enable_fuzz = "no"; then
    echo "  with test       = $use_tests"
else
    echo "  with test       = not building test_syscoin because fuzzing is enabled"
fi
echo "  with fuzz binary = $enable_fuzz_binary"
echo "  with bench      = $use_bench"
echo "  with upnp       = $use_upnp"
echo "  with natpmp     = $use_natpmp"
echo "  use asm         = $use_asm"
echo "  USDT tracing    = $use_usdt"
echo "  sanitizers      = $use_sanitizers"
echo "  debug enabled   = $enable_debug"
echo "  gprof enabled   = $enable_gprof"
echo "  werror          = $enable_werror"
echo "  LTO             = $enable_lto"
echo
echo "  target os       = $host_os"
echo "  build os        = $build_os"
echo
echo "  CC              = $CC"
echo "  CFLAGS          = $PTHREAD_CFLAGS $CFLAGS"
echo "  CPPFLAGS        = $DEBUG_CPPFLAGS $HARDENED_CPPFLAGS $CORE_CPPFLAGS $CPPFLAGS"
echo "  CXX             = $CXX"
echo "  CXXFLAGS        = $LTO_CXXFLAGS $DEBUG_CXXFLAGS $HARDENED_CXXFLAGS $WARN_CXXFLAGS $NOWARN_CXXFLAGS $ERROR_CXXFLAGS $GPROF_CXXFLAGS $CORE_CXXFLAGS $CXXFLAGS"
echo "  LDFLAGS         = $LTO_LDFLAGS $PTHREAD_LIBS $HARDENED_LDFLAGS $GPROF_LDFLAGS $CORE_LDFLAGS $LDFLAGS"
echo "  AR              = $AR"
echo "  ARFLAGS         = $ARFLAGS"
echo
