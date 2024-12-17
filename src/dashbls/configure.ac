AC_PREREQ([2.60])
AC_INIT([libdashbls],[1.3.5])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])

dnl Unless the user specified ARFLAGS, force it to be cr
AC_ARG_VAR(ARFLAGS, [Flags for the archiver, defaults to <cr> if not set])
if test "x${ARFLAGS+set}" != "xset"; then
  ARFLAGS="qc"
fi

AC_CANONICAL_HOST

AH_TOP([#ifndef RLC_CONF_H])
AH_TOP([#define RLC_CONF_H])
AH_BOTTOM([#endif /*RLC_CONF_H*/])
AM_INIT_AUTOMAKE([foreign no-define subdir-objects])

dnl faketime messes with timestamps and causes configure to be re-run.
dnl --disable-maintainer-mode can be used to bypass this.
AM_MAINTAINER_MODE([enable])

dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Compiler checks (here before libtool).
if test "x${CFLAGS+set}" = "xset"; then
  CFLAGS_overridden=yes
else
  CFLAGS_overridden=no
fi
if test "x${CXXFLAGS+set}" = "xset"; then
  CXXFLAGS_overridden=yes
else
  CXXFLAGS_overridden=no
fi

AC_PROG_CC

AC_PROG_CXX

dnl By default, libtool for mingw refuses to link static libs into a dll for
dnl fear of mixing pic/non-pic objects, and import/export complications. Since
dnl we have those under control, re-enable that functionality.
case $host in
  *mingw*)
     lt_cv_deplibs_check_method="pass_all"
  ;;
esac
dnl Require C++14 compiler (no GNU extensions)
AX_CXX_COMPILE_STDCXX([14], [noext], [mandatory], [nodefault])

dnl Libtool init checks.
LT_INIT([pic-only])

AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)
AC_PATH_TOOL(STRIP, strip)

AM_PROG_AS

AC_ARG_ENABLE([tests],
  [AS_HELP_STRING([--enable-tests],
  [Compile with tests [default=yes]])],
  [use_tests=$enableval],
  [use_tests=yes])

AC_ARG_ENABLE([bench],
  [AS_HELP_STRING([--enable-bench],
  [Compile with benchmarks [default=yes]])],
  [use_bench=$enableval],
  [use_bench=yes])

AC_ARG_ENABLE([hardening],
  [AS_HELP_STRING([--enable-hardening],
  [Enable hardening flags and arguments [default=auto]])],
  [use_hardening=$enableval],
  [use_hardening=auto])

AC_ARG_ENABLE([optimizations],
  [AS_HELP_STRING([--enable-optimizations],
  [Enable compiler and library optimizations [default=yes]])],
  [use_optimizations=$enableval],
  [use_optimizations=yes])

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
  [Build with debugging support [default=no]])],
  [use_debug=$enableval],
  [use_debug=no])

AC_LANG_PUSH([C])

dnl Check for a flag to turn compiler warnings into errors. This is helpful for checks which may
dnl appear to succeed because by default they merely emit warnings when they fail.
dnl
dnl Note that this is not necessarily a check to see if -Werror is supported, but rather to see if
dnl a compile with -Werror can succeed. This is important because the compiler may already be
dnl warning about something unrelated, for example about some path issue. If that is the case,
dnl -Werror cannot be used because all of those warnings would be turned into errors.
AX_CHECK_COMPILE_FLAG([-Werror], [FLAG_WERROR="-Werror"], [FLAG_WERROR=""])

if test x"$use_debug" = x"yes" -o x"$use_optimizations" = x"no"; then
  dnl Clear default -g -O2 flags
  if test x"$CFLAGS_overridden" = x"no"; then
    CFLAGS=""
  fi
  if test x"$CXXFLAGS_overridden" = x"no"; then
    CXXFLAGS=""
  fi

  dnl Disable optimizations
  AX_CHECK_COMPILE_FLAG([-O0], [[DEBUG_FLAGS="$DEBUG_FLAGS -O0"]], [], [[$FLAG_WERROR]])

  if test x"$use_debug" = x"yes"; then
    dnl Prefer -g3, fall back to -g if that is unavailable.
    AX_CHECK_COMPILE_FLAG(
      [-g3],
      [[DEBUG_FLAGS="$DEBUG_FLAGS -g3"]],
      [AX_CHECK_COMPILE_FLAG([-g],[[DEBUG_FLAGS="$DEBUG_FLAGS -g"]])], [[$FLAG_WERROR]])

    AX_CHECK_COMPILE_FLAG([-ftrapv],[DEBUG_FLAGS="$DEBUG_FLAGS -ftrapv"], [], [[$FLAG_WERROR]])
  else
    dnl Reintroduce -g flag deleted during C(XX)FLAGS reset
    AX_CHECK_COMPILE_FLAG([-g],[[DEBUG_FLAGS="$DEBUG_FLAGS -g"]], [], [[$FLAG_WERROR]])

    dnl -ftrapv and -fwrapv conflict, so we only set this if use_debug=no
    AX_CHECK_COMPILE_FLAG([-fwrapv],[DEBUG_FLAGS="$DEBUG_FLAGS -fwrapv"], [], [[$FLAG_WERROR]])
  fi
fi

dnl Define enum mappings for relic config
AC_DEFINE([RLC_VERSION], ["0.5.0"], [Relic Version])

dnl dashbls: Library processor architecture
AC_DEFINE([AVR],     [1], [Atmel AVR ATMega128 8-bit architecture.])
AC_DEFINE([MSP],     [2], [MSP430 16-bit architecture.])
AC_DEFINE([ARM],     [3], [ARM 32-bit architecture.])
AC_DEFINE([X86],     [4], [Intel x86-compatible 32-bit architecture.])
AC_DEFINE([X64],     [5], [AMD64-compatible 64-bit architecture.])

dnl Relic: Generic options
AC_DEFINE([RELIC_NONE],    [0], [Unknown or unrecognized parameter.])

dnl Relic: Arithmetic backends
AC_DEFINE([EASY],    [1], [Easy C-only backend.])
AC_DEFINE([GMP],     [2], [Backend based on GNU Multiple Precision library.])
AC_DEFINE([GMP_SEC], [3], [Backend based on GNU Multiple Precision library, but using constant-time code.])

AC_DEFUN([GMP_CHECK],[
  AC_REQUIRE([AC_PROG_CC])

  CPPFLAGS_TEMP="$CPPFLAGS"
  CPPFLAGS="$GMP_CPPFLAGS $CPPFLAGS"
  LIBS_TEMP="$LIBS"
  LIBS="$GMP_LDFLAGS $LIBS"

  AC_CHECK_HEADER(
    gmp.h,
    [AC_CHECK_LIB(gmp, __gmpz_init, [], [gmp_fail_reason="libgmp not found or unusable"; has_gmp=no;])],
    [gmp_fail_reason="gmp header not found"; has_gmp=no;]
  )

  if test x"$has_gmp" != x"no"; then
    # Proper support for macOS aarch64 was introduced in 6.3.0, but LTS distros
    # like Ubuntu 20.04 (focal) use 6.2.0, so, the minimum supported version is
    # determined by platform and architecture.
    case $host in
      *darwin*)
        case $host_cpu in
          aarch*)
            gmp_major_version=6
            gmp_minor_version=3
            ;;
        esac
        ;;
      *)
        gmp_major_version=6
        gmp_minor_version=2
        ;;
    esac
    AC_MSG_CHECKING([gmp version >= $gmp_major_version.$gmp_minor_version])
    AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM(
        [[@%:@include <gmp.h>]],
        [[
          @%:@if __GNU_MP_VERSION < $gmp_major_version || __GNU_MP_VERSION_MINOR < $gmp_minor_version
            @%:@error Unsupported GMP version
          @%:@endif]])],
      [AC_MSG_RESULT([yes]); GMP_LIBS="-lgmp"; has_gmp=yes;],
      [AC_MSG_RESULT([no]); gmp_fail_reason="unsupported gmp version"; has_gmp=no;]
    )
  fi

  if test x"$want_backend" = x"gmp" && test x"$has_gmp" = x"no"; then
    AC_MSG_ERROR([$gmp_fail_reason])
  fi

  CPPFLAGS="$CPPFLAGS_TEMP"
  LIBS="$LIBS_TEMP"
])

AC_ARG_WITH([backend],
  [AS_HELP_STRING([--with-backend=auto|easy|gmp],
  [backend implementation to use [default=auto]])],
  [want_backend=$withval],
  [want_backend=auto]
)

case $host in
  *darwin*)
    AC_PATH_PROG([BREW],brew,)
    if test x$BREW != x; then
      # These Homebrew packages may be keg-only, meaning that they won't be found
      # in expected paths because they may conflict with system files. Ask
      # Homebrew where each one is located, then adjust paths accordingly.
      gmp_prefix=`$BREW --prefix gmp 2>/dev/null`
      if test x$gmp_prefix != x; then
        GMP_CPPFLAGS="-I$gmp_prefix/include"
        GMP_LDFLAGS="-L$gmp_prefix/lib"
      fi
    fi
    ;;
  *freebsd*)
    GMP_CPPFLAGS="-I/usr/local/include"
    GMP_LDFLAGS="-L/usr/local/lib"
    ;;
esac

if test x"$want_backend" = x"auto"; then
  if test x"$use_optimizations" = x"no"; then
    want_backend=easy
  else
    GMP_CHECK
    if test x"$has_gmp" = x"yes"; then
      want_backend=gmp
    else
      want_backend=easy
    fi
  fi
else
  case $want_backend in
  gmp)
    dnl GMP_CHECK will report the reason if and why the backend cannot be used.
    GMP_CHECK
    ;;
  easy)
    ;;
  *)
    AC_MSG_ERROR([invalid backend explicitly requested])
    ;;
  esac
fi

case $want_backend in
gmp)
  AC_DEFINE(ARITH, GMP, [Define if GNU MP backend is requested])
  ;;
easy)
  AC_DEFINE(ARITH, EASY, [Define if C-only backend is requested].)
  ;;
*)
  AC_MSG_ERROR([invalid backend explicitly requested])
  ;;
esac

dnl Relic: Multithreading APIs
AC_DEFINE([OPENMP],  [1], [OpenMP multithreading support.])
AC_DEFINE([PTHREAD], [2], [POSIX multithreading support.])

dnl Relic: Supported operating systems
AC_DEFINE([LINUX],   [1], [GNU/Linux operating system.])
AC_DEFINE([FREEBSD], [2], [FreeBSD operating system.])
AC_DEFINE([MACOSX],  [3], [MacOS operating system.])
AC_DEFINE([WINDOWS], [4], [Windows operating system.])
AC_DEFINE([DROID],   [5], [Android operating system.])
AC_DEFINE([DUINO],   [6], [Arduino platform.])
dnl AC_DEFINE([OPENBSD], [7], [OpenBSD operating system.])

dnl Relic: Supported timers
AC_DEFINE([HREAL],   [1], [Per-process high-resolution timer.])
AC_DEFINE([HPROC],   [2], [Per-process high-resolution timer.])
AC_DEFINE([HTHRD],   [3], [Per-thread high-resolution timer.])
AC_DEFINE([ANSI],    [5], [ANSI-compatible timer.])
AC_DEFINE([POSIX],   [4], [POSIX-compatible timer.])
AC_DEFINE([CYCLE],   [6], [Cycle-counting timer.])
AC_DEFINE([PERF],    [7], [Performance monitoring framework.])

dnl Relic: Memory-allocation policies
AC_DEFINE([AUTO],    [1], [Automatic memory allocation.])
AC_DEFINE([DYNAMIC], [2], [Dynamic memory allocation.])

dnl Relic (CP): Support for faster CRT-based exponentiation in factoring-based cryptosystems
AC_DEFINE([PKCS1],   [2], [RSA PKCS#1 v1.5 padding.])
AC_DEFINE([PKCS2],   [3], [RSA PKCS#1 v2.1 padding.])

dnl Relic (MD): Available hash functions
AC_DEFINE([SH224],   [2], [SHA-224 hash function.])
AC_DEFINE([SH256],   [3], [SHA-256 hash function.])
AC_DEFINE([SH384],   [4], [SHA-384 hash function.])
AC_DEFINE([SH512],   [5], [SHA-512 hash function.])
AC_DEFINE([B2S160],  [6], [BLAKE2s-160 hash function.])
AC_DEFINE([B2S256],  [7], [BLAKE2s-256 hash function.])

dnl Relic (RAND): Available pseudo-random number generators
AC_DEFINE([HASHD],   [1], [NIST HASH-DRBG generator.])
AC_DEFINE([RDRND],   [2], [Intel RdRand instruction.])
AC_DEFINE([UDEV],    [3], [Operating system underlying generator.])
AC_DEFINE([CALL],    [4], [Override library generator with the callback.])

dnl Relic (RAND): Available random number generator seeders
AC_DEFINE([LIBC],    [1], [Standard C library generator.])
dnl AC_DEFINE([RDRND],   [2], [Intel RdRand instruction.])
dnl AC_DEFINE([UDEV],    [3], [Operating system underlying generator.])
AC_DEFINE([WCGR],    [4], [Use Windows' CryptGenRandom.])

dnl Relic (BN): Options for the multiple precision
AC_DEFINE([SINGLE],  [1], [A multiple precision integer can store w words.])
AC_DEFINE([CARRY],   [2], [A multiple precision integer can store the result of an addition.])
AC_DEFINE([DOUBLE],  [3], [A multiple precision integer can store the result of a multiplication.])

AC_DEFINE([BASIC],   [1], [Basic method.])
AC_DEFINE([PRIME],   [1], [Prime curves.])
AC_DEFINE([TATEP],   [1], [Tate pairing.])
AC_DEFINE([COMBA],   [2], [Comba method.])
AC_DEFINE([LEHME],   [2], [Lehmer's fast GCD Algorithm.])
AC_DEFINE([SAFEP],   [2], [Safe prime generation.])
AC_DEFINE([QUICK],   [2], [Fast reduction modulo special form prime.])
AC_DEFINE([BINAR],   [2], [Binary inversion.])
AC_DEFINE([LAZYR],   [2], [Lazy-reduced extension field arithmetic.])
AC_DEFINE([LODAH],   [2], [Lopez-Dahab multiplication.])
AC_DEFINE([SLIDE],   [2], [Sliding window modular exponentiation.])
AC_DEFINE([PROJC],   [2], [Projective coordinates.])
AC_DEFINE([COMBS],   [2], [Single-table comb method.])
AC_DEFINE([TRICK],   [2], [Shamir's trick.])
AC_DEFINE([CHAR2],   [2], [Binary curves.])
AC_DEFINE([WEILP],   [2], [Weil pairing.])
AC_DEFINE([MONTY],   [3], [Montgomery method.])
AC_DEFINE([STEIN],   [3], [Stein's binary GCD Algorithm.])
AC_DEFINE([STRON],   [3], [Strong prime generation.])
AC_DEFINE([INTEG],   [3], [Integrated modular addtion.])
AC_DEFINE([INTER],   [3], [Interleaving of w-(T)NAFs.])
AC_DEFINE([ALMOS],   [3], [Almost inverse algorithm.])
AC_DEFINE([JACOB],   [3], [Jacobian coordinates.])
AC_DEFINE([COMBD],   [3], [Double-table comb method.])
AC_DEFINE([HALVE],   [3], [Halving.])
AC_DEFINE([EDDIE],   [3], [Edwards curves.])
AC_DEFINE([EXTND],   [3], [Extended projective twisted Edwards coordinates.])
AC_DEFINE([OATEP],   [3], [Optimal ate pairing.])
AC_DEFINE([PMERS],   [4], [Pseudo-Mersenne method.])
AC_DEFINE([MULTP],   [4], [Reuse multiplication for squaring.])
AC_DEFINE([EXGCD],   [4], [Extended Euclidean algorithm.])
AC_DEFINE([LWNAF],   [4], [Left-to-right Width-w NAF.])
AC_DEFINE([JOINT],   [4], [Joint sparse form.])
AC_DEFINE([DIVST],   [5], [Constant-time inversion by Bernstein-Yang division steps.])
AC_DEFINE([ITOHT],   [5], [Itoh-Tsuji inversion.])
AC_DEFINE([LWREG],   [5], [Left-to-right Width-w NAF.])
AC_DEFINE([BRUCH],   [6], [Hardware-friendly inversion by Brunner-Curiger-Hofstetter.])
AC_DEFINE([CTAIA],   [7], [Constant-time version of almost inverse.])
AC_DEFINE([LOWER],   [8], [Use implementation provided by the lower layer.])

if test x"$use_debug" = x"yes"; then
  AC_DEFINE([DEBUG],   [1], [Define this symbol if debugging support is enabled])
fi

dnl Define relic switches
AC_ARG_ENABLE([profiling],
  [AS_HELP_STRING([--enable-profiling],
  [Build with profiling support [default=no]])],
  [use_profiling=$enableval],
  [use_profiling=no])

if test x$use_profiling = xyes; then
  AC_DEFINE([PROFL],   [1], [Define this symbol if profiling support is enabled])
fi

AC_ARG_ENABLE([check],
  [AS_HELP_STRING([--enable-check],
  [Build with error-checking support [default=no]])],
  [use_check=$enableval],
  [use_check=no])

if test x$use_check = xyes; then
  AC_DEFINE([CHECK],   [1], [Define this symbol if error-checking support is enabled])
fi

AC_ARG_ENABLE([verbs],
  [AS_HELP_STRING([--enable-verbs],
  [Build with detailed error messages [default=no]])],
  [use_verbs=$enableval],
  [use_verbs=no])

if test x$use_verbs = xyes; then
  AC_DEFINE([VERBS],   [1], [Define this symbol if detailed error messages are enabled])
fi

AC_ARG_ENABLE([overhead],
  [AS_HELP_STRING([--enable-overhead],
  [Build with overhead estimation [default=no]])],
  [use_overhead=$enableval],
  [use_overhead=no])

if test x$use_overhead = xyes; then
  AC_DEFINE([OVERH],   [1], [Define this symbol if overhead estimation is enabled])
fi

AC_ARG_ENABLE([quiet],
  [AS_HELP_STRING([--enable-quiet],
  [Build with printing disabled [default=yes]])],
  [use_quiet=$enableval],
  [use_quiet=yes])

if test x$use_quiet = xyes; then
  AC_DEFINE([QUIET],   [1], [Define this symbol if printing is disabled])
fi

AC_ARG_ENABLE([color],
  [AS_HELP_STRING([--enable-color],
  [Build with colored output support [default=yes]])],
  [use_color=$enableval],
  [use_color=yes])

if test x$use_color = xyes; then
  AC_DEFINE([COLOR],   [1], [Define this symbol if colored output is enabled])
fi

AC_ARG_ENABLE([big_endian],
  [AS_HELP_STRING([--enable-big-endian],
  [Build with big-endian support [default=no]])],
  [use_big_endian=$enableval],
  [use_big_endian=no])

if test x$use_big_endian = xyes; then
  AC_DEFINE([BIGED],   [1], [Define this symbol if big-endian support is enabled])
fi

use_pkgconfig=yes

if test x"$use_optimizations" = x"yes"; then
  case $host_cpu in
    amd64 | x86_64)
      dnl Support for AMD64 (also known as x86_64 on some platforms) processors
      CPU_ARCH="x64"
      AC_DEFINE([ARCH], [X64], [Architecture.])
      AC_DEFINE([WSIZE], [64], [Size of word in this architecture.])
      ;;
    aarch*)
      dnl Support for 64-bit ARM processors
      dnl Relic does not support aarch64 yet, set CPU_ARCH to none and ARCH to RELIC_NONE.
      CPU_ARCH="none"
      AC_DEFINE([ARCH], [RELIC_NONE], [Architecture.])
      AC_DEFINE([WSIZE], [64], [Size of word in this architecture.])
      ;;
    i?86)
      dnl Support for Intel x86 processors
      CPU_ARCH="x86"
      AC_DEFINE([ARCH], [X86], [Architecture.])
      AC_DEFINE([WSIZE], [32], [Size of word in this architecture.])
      ;;
    arm*)
      dnl Support for 32-bit native ARM processors
      CPU_ARCH="arm"
      AC_DEFINE([ARCH], [ARM], [Architecture.])
      AC_DEFINE([WSIZE], [32], [Size of word in this architecture.])
      ;;
    *64*)
      dnl Support for an undefined 64-bit architecture
      CPU_ARCH="none"
      AC_DEFINE([ARCH], [RELIC_NONE], [Architecture.])
      AC_DEFINE([WSIZE], [64], [Size of word in this architecture.])
      ;;
    *32*)
      dnl Support for an undefined 32-bit architecture
      CPU_ARCH="none"
      AC_DEFINE([ARCH], [RELIC_NONE], [Architecture.])
      AC_DEFINE([WSIZE], [32], [Size of word in this architecture.])
      ;;
    *)
      AC_MSG_ERROR([Unable to determine host architecture, may not be supported!])
      ;;
  esac
else
  case $host_cpu in
    *64*)
      dnl Support for an undefined 64-bit architecture
      CPU_ARCH="none"
      AC_DEFINE([ARCH], [RELIC_NONE], [Architecture.])
      AC_DEFINE([WSIZE], [64], [Size of word in this architecture.])
      ;;
    *32* | arm* | i?86)
      dnl Support for an undefined 32-bit architecture
      CPU_ARCH="none"
      AC_DEFINE([ARCH], [RELIC_NONE], [Architecture.])
      AC_DEFINE([WSIZE], [32], [Size of word in this architecture.])
      ;;
    *)
      AC_MSG_ERROR([Unable to determine host architecture, may not be supported!])
      ;;
  esac
fi

case $host in
  *darwin*)
     AC_DEFINE([OPSYS], [MACOSX], [Detected operation system.])
     TARGET_OS=darwin

     AC_PATH_PROG([BREW],brew,)
     if test x$BREW = x; then
       AC_PATH_PROG([PORT],port,)
       # If homebrew isn't installed and macports is, add the macports default paths
       # as a last resort.
       if test x$PORT != x; then
         CPPFLAGS="$CPPFLAGS -isystem /opt/local/include"
         LDFLAGS="$LDFLAGS -L/opt/local/lib"
       fi
     fi
     ;;
  *mingw*)
     AC_DEFINE([OPSYS], [WINDOWS], [Detected operation system.])
     TARGET_OS=windows

     use_pkgconfig=no
     LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static"

     dnl libtool insists upon adding -nostdlib and a list of objects/libs to link against.
     dnl That breaks our ability to build dll's with static libgcc/libstdc++/libssp. Override
     dnl its command here, with the predeps/postdeps removed, and -static inserted. Postdeps are
     dnl also overridden to prevent their insertion later.
     dnl This should only affect dll's.
     archive_cmds_CXX="\$CC -shared \$libobjs \$deplibs \$compiler_flags -static -o \$output_objdir/\$soname \${wl}--enable-auto-image-base -Xlinker --out-implib -Xlinker \$lib"
     postdeps_CXX=
     ;;
  *linux*)
    AC_DEFINE([OPSYS], [LINUX], [Detected operation system.])
    TARGET_OS=linux

    RELIC_CPPFLAGS="-D_GNU_SOURCE"
    ;;
  *android*)
    AC_DEFINE([OPSYS], [DROID], [Detected operation system.])
    TARGET_OS=android
    ;;
  *freebsd*)
    AC_DEFINE([OPSYS], [FREEBSD], [Detected operation system.])
    TARGET_OS=freebsd
    ;;
  *netbsd*)
    AC_DEFINE([OPSYS], [NETBSD], [Detected operation system.])
    TARGET_OS=netbsd
    ;;
  *)
    AC_DEFINE([OPSYS], [RELIC_NONE], [Detected operation system.])
    ;;
esac

case $host in
  *darwin*)
    dnl Relic: Don't define FP_QNRES on Darwin
    AC_DEFINE([SEED], [UDEV], [Chosen random generator seeder.])
    ;;
  *mingw*)
    AC_DEFINE([FP_QNRES], [], [Use -1 as quadratic non-residue.])
    AC_DEFINE([SEED], [WCGR], [Chosen random generator seeder.])
    ;;
  *)
    AC_DEFINE([FP_QNRES], [], [Use -1 as quadratic non-residue.])
    AC_DEFINE([SEED], [UDEV], [Chosen random generator seeder.])
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

saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -pipe"
AC_MSG_CHECKING([if ${CC} supports -pipe])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fomit-frame-pointer"
AC_MSG_CHECKING([if ${CC} supports -fomit-frame-pointer])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

if test x"$CFLAGS_overridden" = x"no" -a x"$CXXFLAGS_overridden" = x"no"; then
  dnl Enable warnings
  AX_CHECK_COMPILE_FLAG([-Wall],[WARN_FLAGS="$WARN_FLAGS -Wall"], [], [[$FLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wcast-align],[WARN_FLAGS="$WARN_FLAGS -Wcast-align"], [], [[$FLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wextra],[WARN_FLAGS="$WARN_FLAGS -Wextra"], [], [[$FLAG_WERROR]])

  dnl Exclude known warnings
  AX_CHECK_COMPILE_FLAG([-Wlong-long],[NOWARN_FLAGS="$NOWARN_FLAGS -Wno-long-long"], [], [[$FLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wshadow],[NOWARN_FLAGS="$NOWARN_FLAGS -Wno-shadow"], [], [[$FLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wsign-compare],[NOWARN_FLAGS="$NOWARN_FLAGS -Wno-sign-compare"], [], [[$FLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wstrict-prototypes],[NOWARN_FLAGS="$NOWARN_FLAGS -Wno-strict-prototypes"], [], [[$FLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wunused-function],[NOWARN_FLAGS="$NOWARN_FLAGS -Wno-unused-function"], [], [[$FLAG_WERROR]])
  AX_CHECK_COMPILE_FLAG([-Wunused-parameter],[NOWARN_FLAGS="$NOWARN_FLAGS -Wno-unused-parameter"], [], [[$FLAG_WERROR]])
fi

dnl set default settings for relic
AC_DEFINE([ALIGN], [1], [Byte boundary to align digit vectors.])

AC_DEFINE(WITH_BN,, Build multiple precision integer module.)
AC_DEFINE(WITH_DV,, Temporary double-precision digit vectors.)
AC_DEFINE(WITH_FP,, Build prime field module.)
AC_DEFINE(WITH_FPX,, Build prime field extension module.)
AC_DEFINE(WITH_FB,, Build binary field module.)
AC_DEFINE(WITH_FBX,, Build binary field extension module.)
AC_DEFINE(WITH_EP,, Build prime elliptic curve module.)
AC_DEFINE(WITH_EPX,, Build prime field extension elliptic curve module.)
AC_DEFINE(WITH_EB,, Build binary elliptic curve module.)
AC_DEFINE(WITH_ED,, Build elliptic Edwards curve module.)
AC_DEFINE(WITH_EC,, Build elliptic curve cryptography module.)
AC_DEFINE(WITH_PP,, Build pairings over prime curves module.)
AC_DEFINE(WITH_PC,, Build pairing-based cryptography module.)
AC_DEFINE(WITH_BC,, Build block ciphers.)
AC_DEFINE(WITH_MD,, Build hash functions.)
AC_DEFINE(WITH_CP,, Build cryptographic protocols.)
AC_DEFINE(WITH_MPC,, Build Multi-party computation primitives.)

AC_DEFINE(BN_PRECI, 1024, Required precision in bits.)
AC_DEFINE(BN_KARAT, 0, Number of Karatsuba steps.)
AC_DEFINE(BN_MAGNI, [DOUBLE], Effective size of a multiple precision integer.)

AC_DEFINE(BN_METHD, "COMBA;COMBA;MONTY;SLIDE;BASIC;BASIC", Multiple precision arithmetic method.)
AC_DEFINE(BN_MUL, COMBA, Chosen multiple precision multiplication method.)
AC_DEFINE(BN_SQR, COMBA, Chosen multiple precision multiplication method.)
AC_DEFINE(BN_MOD, MONTY, Chosen multiple precision modular reduction method.)
AC_DEFINE(BN_MXP, SLIDE, Chosen multiple precision modular exponentiation method.)
AC_DEFINE(BN_GCD, BASIC, Chosen multiple precision greatest common divisor method.)
AC_DEFINE(BN_GEN, BASIC, Chosen prime generation algorithm.)

AC_DEFINE(FP_PRIME, 381, Prime field size in bits.)
AC_DEFINE(FP_METHD, "INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE", Prime field arithmetic method.)
AC_DEFINE(FP_KARAT, 0, Number of Karatsuba steps.)
AC_DEFINE(FP_WIDTH, 4, Width of window processing for exponentiation methods.)
AC_DEFINE(FP_ADD, INTEG, Chosen prime field multiplication method.)
AC_DEFINE(FP_MUL, INTEG, Chosen prime field multiplication method.)
AC_DEFINE(FP_SQR, INTEG, Chosen prime field multiplication method.)
AC_DEFINE(FP_RDC, MONTY, Chosen prime field reduction method.)
AC_DEFINE(FP_INV, LOWER, Chosen prime field inversion method.)
AC_DEFINE(FP_EXP, SLIDE, Chosen multiple precision modular exponentiation method.)

AC_DEFINE(FPX_METHD, "INTEG;INTEG;LAZYR", Prime extension field arithmetic method.)
AC_DEFINE(FPX_QDR, INTEG, Chosen extension field arithmetic method.)
AC_DEFINE(FPX_CBC, INTEG, Chosen extension field arithmetic method.)
AC_DEFINE(FPX_RDC, LAZYR, Chosen extension field arithmetic method.)

AC_DEFINE(FB_METHD, "LODAH;QUICK;QUICK;QUICK;QUICK;QUICK;EXGCD;SLIDE;QUICK", Binary field arithmetic method)
AC_DEFINE(FB_POLYN, 283, Irreducible polynomial size in bits.)
AC_DEFINE(FB_KARAT, 0, Number of Karatsuba levels.)
AC_DEFINE(FB_TRINO,, Prefer trinomials over pentanomials.)
AC_DEFINE(FB_PRECO,, Precompute multiplication table for sqrt(z).)
AC_DEFINE(FB_WIDTH, 4, Width of window processing for exponentiation methods.)
AC_DEFINE(FB_MUL, LODAH, Chosen binary field multiplication method.)
AC_DEFINE(FB_SQR, QUICK, Chosen binary field squaring method.)
AC_DEFINE(FB_RDC, QUICK, Chosen binary field modular reduction method.)
AC_DEFINE(FB_SRT, QUICK, Chosen binary field modular reduction method.)
AC_DEFINE(FB_TRC, QUICK, Chosen trace computation method.)
AC_DEFINE(FB_SLV, QUICK, Chosen method to solve a quadratic equation.)
AC_DEFINE(FB_INV, EXGCD, Chosen binary field inversion method.)
AC_DEFINE(FB_EXP, SLIDE, Chosen multiple precision modular exponentiation method.)
AC_DEFINE(FB_ITR, QUICK, Chosen method to solve a quadratic equation.)

AC_DEFINE(EP_METHD, "PROJC;LWNAF;COMBS;INTER", Prime elliptic curve arithmetic method.)
AC_DEFINE(EP_ENDOM,, Support for prime curves with efficient endormorphisms.)
AC_DEFINE(EP_MIXED,, Use mixed coordinates.)
AC_DEFINE(EP_PRECO,, Build precomputation table for generator.)
AC_DEFINE(EP_CTMAP,, Enable isogeny map for SSWU map-to-curve.)
AC_DEFINE(EP_DEPTH, 4, Width of precomputation table for fixed point methods.)
AC_DEFINE(EP_WIDTH, 4, Width of window processing for unknown point methods.)
AC_DEFINE(EP_ADD, PROJC, Chosen prime elliptic curve coordinate method.)
AC_DEFINE(EP_MUL, LWNAF, Chosen prime elliptic curve point multiplication method.)
AC_DEFINE(EP_FIX, COMBS, Chosen prime elliptic curve point multiplication method.)
AC_DEFINE(EP_SIM, INTER, Chosen prime elliptic curve simulteanous point multiplication method.)

AC_DEFINE(EB_METHD, "PROJC;LWNAF;COMBS;INTER", Binary elliptic curve arithmetic method.)
AC_DEFINE(EB_PLAIN,, Support for ordinary curves without endormorphisms.)
AC_DEFINE(EB_KBLTZ,, Support for Koblitz anomalous binary curves.)
AC_DEFINE(EB_MIXED,, Use mixed coordinates.)
AC_DEFINE(EB_PRECO,, Build precomputation table for generator.)
AC_DEFINE(EB_DEPTH, 4, Width of precomputation table for fixed point methods.)
AC_DEFINE(EB_WIDTH, 4, Width of window processing for unknown point methods.)
AC_DEFINE(EB_ADD, PROJC, Chosen binary elliptic curve coordinate method.)
AC_DEFINE(EB_MUL, LWNAF, Chosen binary elliptic curve point multiplication method.)
AC_DEFINE(EB_FIX, COMBS, Chosen binary elliptic curve point multiplication method.)
AC_DEFINE(EB_SIM, INTER, Chosen binary elliptic curve simulteanous point multiplication method.)

AC_DEFINE(ED_METHD, "PROJC;LWNAF;COMBS;INTER", Edwards elliptic curve arithmetic method.)
AC_DEFINE(ED_PRECO,, Build precomputation table for generator.)
AC_DEFINE(ED_DEPTH, 4, Width of precomputation table for fixed point methods.)
AC_DEFINE(ED_WIDTH, 4, Width of window processing for unknown point methods.)
AC_DEFINE(ED_ADD, PROJC, Chosen binary elliptic curve coordinate method.)
AC_DEFINE(ED_MUL, LWNAF, Chosen prime elliptic twisted Edwards curve point multiplication method.)
AC_DEFINE(ED_FIX, COMBS, Chosen prime elliptic twisted Edwards curve point multiplication method.)
AC_DEFINE(ED_SIM, INTER, Chosen prime elliptic curve simulteanous point multiplication method.)

AC_DEFINE(EC_METHD, "PRIME", Chosen elliptic curve cryptography method.)
AC_DEFINE(EC_CUR, PRIME, Chosen elliptic curve type.)

AC_DEFINE(PP_METHD, "LAZYR;OATEP", Bilinear pairing method.)
AC_DEFINE(PP_EXT, LAZYR, Chosen extension field arithmetic method.)
AC_DEFINE(PP_MAP, OATEP, Chosen pairing method over prime elliptic curves.)

AC_DEFINE(MD_METHD, "SH256", Choice of hash function.)
AC_DEFINE(MD_MAP, SH256, Chosen hash function.)

AC_DEFINE(CP_CRT,, Support for faster CRT-based exponentiation in factoring-based cryptosystems.)
AC_DEFINE(CP_RSAPD, PKCS2, Chosen RSA padding method.)

AC_DEFINE(ALLOC, AUTO, Chosen memory allocation policy.)
AC_DEFINE(RAND, HASHD, Chosen random generator.)
AC_DEFINE(MULTI, PTHREAD, Chosen multithreading API.)
AC_DEFINE(TIMER, CYCLE, Chosen timer.)

dnl Check for pthread compile/link requirements
AX_PTHREAD

AC_SEARCH_LIBS([clock_gettime],[rt])

if test "$TARGET_OS" != "windows"; then
  dnl All windows code is PIC, forcing it on just adds useless compile warnings
  AX_CHECK_COMPILE_FLAG([-fPIC], [HARDENED_FLAGS="$HARDENED_FLAGS -fPIC"], [], [[$FLAG_WERROR]])
fi

dnl All versions of gcc that we commonly use for building are subject to bug
dnl https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90348. To work around that, set
dnl -fstack-reuse=none for all gcc builds. (Only gcc understands this flag)
AX_CHECK_COMPILE_FLAG([-fstack-reuse=none], [HARDENED_FLAGS="$HARDENED_FLAGS -fstack-reuse=none"], [], [[$FLAG_WERROR]])
if test "$use_hardening" != "no"; then
  use_hardening=yes
  AX_CHECK_COMPILE_FLAG([-Wstack-protector], [HARDENED_FLAGS="$HARDENED_FLAGS -Wstack-protector"])
  AX_CHECK_COMPILE_FLAG([-fstack-protector-all], [HARDENED_FLAGS="$HARDENED_FLAGS -fstack-protector-all"], [], [[$FLAG_WERROR]])

  AX_CHECK_COMPILE_FLAG([-fcf-protection=full], [HARDENED_FLAGS="$HARDENED_FLAGS -fcf-protection=full"], [], [[$FLAG_WERROR]])

  case $host in
    *mingw*)
      dnl stack-clash-protection doesn't currently work, and likely should just be skipped for Windows.
      dnl See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90458 for more details.
      ;;
    *)
      AX_CHECK_COMPILE_FLAG([-fstack-clash-protection], [HARDENED_FLAGS="$HARDENED_FLAGS -fstack-clash-protection"], [], [[$FLAG_WERROR]])
      ;;
  esac

  case $host in
    *aarch64*)
      AX_CHECK_COMPILE_FLAG([-mbranch-protection=bti], [HARDENED_CXXFLAGS="$HARDENED_CXXFLAGS -mbranch-protection=bti"], [], [[$CXXFLAG_WERROR]])
    ;;
  esac

  AX_CHECK_LINK_FLAG([-Wl,--enable-reloc-section], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--enable-reloc-section"], [], [])
  AX_CHECK_LINK_FLAG([-Wl,--dynamicbase], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--dynamicbase"], [], [])
  AX_CHECK_LINK_FLAG([-Wl,--nxcompat], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--nxcompat"], [], [])
  AX_CHECK_LINK_FLAG([-Wl,--high-entropy-va], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,--high-entropy-va"], [], [])
  AX_CHECK_LINK_FLAG([-Wl,-z,relro], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,relro"], [], [])
  AX_CHECK_LINK_FLAG([-Wl,-z,now], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,now"], [], [])
  AX_CHECK_LINK_FLAG([-Wl,-z,separate-code], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-z,separate-code"], [], [])
  AX_CHECK_LINK_FLAG([-fPIE -pie], [HARDENED_FLAGS="$HARDENED_FLAGS -fPIE"; HARDENED_LDFLAGS="$HARDENED_LDFLAGS -pie"], [], [])

  case $host in
    *mingw*)
       AC_CHECK_LIB([ssp], [main], [], [AC_MSG_ERROR([libssp missing])])
    ;;
  esac
fi

CORE_CPPFLAGS="$CORE_CPPFLAGS -DHAVE_BUILD_INFO"

case $host in
  *mingw*)
     AC_CHECK_LIB([user32],   [main],                    [], [AC_MSG_ERROR([libuser32 missing])])
     AC_CHECK_LIB([shell32],  [SHGetSpecialFolderPathW], [], [AC_MSG_ERROR([libshell32 missing])])
     AC_CHECK_LIB([advapi32], [CryptAcquireContextW],    [], [AC_MSG_ERROR([libadvapi32 missing])])
     AC_CHECK_LIB([psapi],    [GetProcessMemoryInfo],    [], [AC_MSG_ERROR([libpsapi missing])])
     AC_CHECK_LIB([bcrypt],   [main],                    [], [AC_MSG_ERROR([libbcrypt missing])])

     dnl -static is interpreted by libtool, where it has a different meaning.
     dnl In libtool-speak, it's -all-static.
     AX_CHECK_LINK_FLAG([-static], [LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static"])

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
     AX_CHECK_LINK_FLAG([-Wl,--major-subsystem-version -Wl,6 -Wl,--minor-subsystem-version -Wl,1], [CORE_LDFLAGS="$CORE_LDFLAGS -Wl,--major-subsystem-version -Wl,6 -Wl,--minor-subsystem-version -Wl,1"], [], [])
     ;;
  *darwin*)
     AX_CHECK_LINK_FLAG([-Wl,-headerpad_max_install_names], [CORE_LDFLAGS="$CORE_LDFLAGS -Wl,-headerpad_max_install_names"], [], [])
     CORE_CPPFLAGS="$CORE_CPPFLAGS -DMAC_OSX -DOBJC_OLD_DISPATCH_PROTOTYPES=0"
     OBJCXXFLAGS="$CXXFLAGS"
     ;;
esac

dnl These flags are specific to ld64, and may cause issues with other linkers.
dnl For example: GNU ld will interpret -dead_strip as -de and then try and use
dnl "ad_strip" as the symbol for the entry point.
if test "$TARGET_OS" = "darwin"; then
  AX_CHECK_LINK_FLAG([-Wl,-dead_strip], [CORE_LDFLAGS="$CORE_LDFLAGS -Wl,-dead_strip"], [], [])
  AX_CHECK_LINK_FLAG([-Wl,-dead_strip_dylibs], [CORE_LDFLAGS="$CORE_LDFLAGS -Wl,-dead_strip_dylibs"], [], [])
  AX_CHECK_LINK_FLAG([-Wl,-bind_at_load], [HARDENED_LDFLAGS="$HARDENED_LDFLAGS -Wl,-bind_at_load"], [], [])
fi

dnl Don't allow extended (non-ASCII) symbols in identifiers. This is easier for code review.
AX_CHECK_COMPILE_FLAG([-fno-extended-identifiers], [CORE_FLAGS="$CORE_FLAGS -fno-extended-identifiers"], [], [[$FLAG_WERROR]])

AC_LANG_POP([C])

AC_MSG_CHECKING([whether to build runtest])
if test x$use_tests = xyes; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING([whether to build runbench])
if test x$use_bench = xyes; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

AM_CONDITIONAL([TARGET_DARWIN], [test "$TARGET_OS" = "darwin"])
AM_CONDITIONAL([TARGET_LINUX], [test "$TARGET_OS" = "linux"])
AM_CONDITIONAL([TARGET_WINDOWS], [test "$TARGET_OS" = "windows"])

AM_CONDITIONAL(WITH_BN, test 1 -eq 1)
AM_CONDITIONAL(WITH_FP, test 1 -eq 1)
AM_CONDITIONAL(WITH_FPX, test 1 -eq 1)
AM_CONDITIONAL(WITH_FB, test 1 -eq 1)
AM_CONDITIONAL(WITH_EP, test 1 -eq 1)
AM_CONDITIONAL(WITH_EPX, test 1 -eq 1)
AM_CONDITIONAL(WITH_EB, test 1 -eq 1)
AM_CONDITIONAL(WITH_ED, test 1 -eq 1)
AM_CONDITIONAL(WITH_EC, test 1 -eq 1)
AM_CONDITIONAL(WITH_PP, test 1 -eq 1)
AM_CONDITIONAL(WITH_PC, test 1 -eq 1)
AM_CONDITIONAL(WITH_BC, test 1 -eq 1)
AM_CONDITIONAL(WITH_MD, test 1 -eq 1)
AM_CONDITIONAL(WITH_CP, test 1 -eq 1)
AM_CONDITIONAL(WITH_MPC, test 1 -eq 1)
AM_CONDITIONAL(WITH_DV, test 1 -eq 1)
AM_CONDITIONAL(WITH_FBX, test 1 -eq 1)

AM_CONDITIONAL([USE_TESTS], [test x"$use_tests" = x"yes"])
AM_CONDITIONAL([USE_BENCH], [test x"$use_bench" = x"yes"])
AM_CONDITIONAL([HARDEN], [test "$use_hardening" = "yes"])
AM_CONDITIONAL([OPTIMIZE], [test "$use_optimizations" = "yes"])

AM_CONDITIONAL([ARITH_EASY], [test "$want_backend" = "easy"])
AM_CONDITIONAL([ARITH_GMP], [test "$want_backend" = "gmp"])

AC_SUBST(CPU_ARCH)
AC_SUBST(DEBUG_FLAGS)
AC_SUBST(RAND_PATH, hashd)
AC_SUBST(RELIC_CPPFLAGS)
AC_SUBST(WARN_FLAGS)
AC_SUBST(NOWARN_FLAGS)
AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(HARDENED_FLAGS)
AC_SUBST(HARDENED_LDFLAGS)
AC_SUBST(CORE_LDFLAGS)
AC_SUBST(CORE_CPPFLAGS)
AC_SUBST(CORE_FLAGS)

AC_SUBST(GMP_LDFLAGS)
AC_SUBST(GMP_CPPFLAGS)
AC_SUBST(GMP_LIBS)

AC_CONFIG_HEADERS([depends/relic/include/relic_conf.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

dnl Peplace conflict-prone PACKAGE-prefixed macros with DASHBLS
sed -i.old 's/PACKAGE/DASHBLS/g' depends/relic/include/relic_conf.h
sed -i.old 's/PACKAGE/DASHBLS/g' config.status

echo
echo "Options used to compile and link:"
echo "  target os           = $TARGET_OS"
echo "  backend             = $want_backend"
echo "  build bench         = $use_tests"
echo "  build test          = $use_bench"
echo "  use debug           = $use_debug"
echo "  use hardening       = $use_hardening"
echo "  use optimizations   = $use_optimizations"
echo
echo "  LDFLAGS             = $HARDENED_LDFLAGS $CORE_LDFLAGS $GMP_LDFLAGS"
echo "  CFLAGS              = $DEBUG_FLAGS $HARDENED_FLAGS $CORE_FLAGS $WARN_FLAGS $NOWARN_FLAGS"
echo "  CPPFLAGS            = $CORE_CPPFLAGS $GMP_CPPFLAGS"
echo "  CXXFLAGS            = $DEBUG_FLAGS $HARDENED_FLAGS $CORE_FLAGS $WARN_FLAGS $NOWARN_FLAGS"
echo "  PTHREAD_FLAGS       = $PTHREAD_CFLAGS $PTHREAD_LIBS"
echo
