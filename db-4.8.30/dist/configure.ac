# $Id$
# Process this file with autoconf to produce a configure script.

PACKAGE=db
AC_INIT(Berkeley DB, __EDIT_DB_VERSION__,
	[Oracle Technology Network Berkeley DB forum],
	db-__EDIT_DB_VERSION__)
AC_CONFIG_SRCDIR([../db/db.c])
AC_CONFIG_HEADERS([db_config.h:config.hin])

AC_CONFIG_MACRO_DIR([aclocal])

# Configure setup.
AC_CANONICAL_HOST()
AC_ARG_PROGRAM()

# Don't build in the top-level or dist directories.
AC_MSG_CHECKING(if building in the top-level or dist directories)
if [ test -d db_archive -o -f configure.ac ] ; then
	AC_MSG_RESULT(yes)
	AC_MSG_ERROR([\
Berkeley DB should not be built in the top-level or "dist" directories. \
Change directory to the build_unix directory and run ../dist/configure \
from there.])

fi
AC_MSG_RESULT(no)

# Substitution variables.
AC_SUBST(ADDITIONAL_INCS)
AC_SUBST(ADDITIONAL_LANG)
AC_SUBST(ADDITIONAL_OBJS)
AC_SUBST(ADDITIONAL_PROGS)
AC_SUBST(BUILD_TARGET)
AC_SUBST(CFLAGS)
AC_SUBST(CONFIGURATION_ARGS)
AC_SUBST(CONFIGURATION_PATH)
AC_SUBST(CPPFLAGS)
AC_SUBST(CRYPTO_OBJS)
AC_SUBST(CXX)
AC_SUBST(CXXFLAGS)
AC_SUBST(DB_CONST)
AC_SUBST(DB_PROTO1)
AC_SUBST(DB_PROTO2)
AC_SUBST(DEFAULT_LIB)
AC_SUBST(DEFAULT_LIB_CXX)
AC_SUBST(DEFAULT_LIB_STL)
AC_SUBST(INSTALLER)
AC_SUBST(INSTALL_LIBS)
AC_SUBST(INSTALL_TARGET)
AC_SUBST(JAR)
AC_SUBST(JAVACFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(LIBCSO_LIBS)
AC_SUBST(LIBJSO_LIBS)
AC_SUBST(LIBS)
AC_SUBST(LIBSO_LIBS)
AC_SUBST(LIBTOOL)
AC_SUBST(LIBTSO_LIBS)
AC_SUBST(LIBTSO_MODSUFFIX)
AC_SUBST(LIBTSO_MODULE)
AC_SUBST(LIBXSO_LIBS)
AC_SUBST(MAKEFILE_CC)
AC_SUBST(MAKEFILE_CCLINK)
AC_SUBST(MAKEFILE_CXX)
AC_SUBST(MAKEFILE_CXXLINK)
AC_SUBST(MAKEFILE_SOLINK)
AC_SUBST(MAKEFILE_XSOLINK)
AC_SUBST(OSDIR)
AC_SUBST(PATH_SEPARATOR)
AC_SUBST(POSTLINK)
AC_SUBST(REPLACEMENT_OBJS)
AC_SUBST(RPC_CLIENT_OBJS)
AC_SUBST(RPC_SERVER_H)
AC_SUBST(SOFLAGS)
AC_SUBST(SWIGCFLAGS)
AC_SUBST(TEST_LIBS)
AC_SUBST(db_int_def)
AC_SUBST(o)

# The Windows public header has two extra symbols we need to remove.
AC_SUBST(platform_header)
AC_SUBST(platform_footer)

# Set the default installation location.
AC_PREFIX_DEFAULT(/usr/local/BerkeleyDB.__EDIT_DB_VERSION_MAJOR__.__EDIT_DB_VERSION_MINOR__)

# Configure the version information.
AC_SUBST(DB_VERSION_MAJOR)
DB_VERSION_MAJOR="__EDIT_DB_VERSION_MAJOR__"
AC_SUBST(DB_VERSION_MINOR)
DB_VERSION_MINOR="__EDIT_DB_VERSION_MINOR__"
AC_SUBST(DB_VERSION_PATCH)
DB_VERSION_PATCH="__EDIT_DB_VERSION_PATCH__"
AC_SUBST(DB_VERSION_STRING)
DB_VERSION_STRING='"__EDIT_DB_VERSION_STRING__"'
AC_SUBST(DB_VERSION_UNIQUE_NAME)

# Process all options before using them.
AM_OPTIONS_SET

# Set some #defines based on configuration options.
if test "$db_cv_diagnostic" = "yes"; then
	AC_DEFINE(DIAGNOSTIC)
	AH_TEMPLATE(DIAGNOSTIC,
    [Define to 1 if you want a version with run-time diagnostic checking.])
fi
if test "$db_cv_debug_rop" = "yes"; then
	AC_DEFINE(DEBUG_ROP)
	AH_TEMPLATE(DEBUG_ROP,
    [Define to 1 if you want a version that logs read operations.])
fi
if test "$db_cv_debug_wop" = "yes"; then
	AC_DEFINE(DEBUG_WOP)
	AH_TEMPLATE(DEBUG_WOP,
    [Define to 1 if you want a version that logs write operations.])
fi
if test "$db_cv_umrw" = "yes"; then
	AC_DEFINE(UMRW)
	AH_TEMPLATE(UMRW,
    [Define to 1 to mask harmless uninitialized memory read/writes.])

fi
if test "$db_cv_test" = "yes"; then
	AC_DEFINE(CONFIG_TEST)
	AH_TEMPLATE(CONFIG_TEST,
    [Define to 1 if you want to build a version for running the test suite.])
fi

AH_TEMPLATE(HAVE_UPGRADE_SUPPORT,
    [Define to 1 if port includes historic database upgrade support.])
AC_DEFINE(HAVE_UPGRADE_SUPPORT)

# Check for programs used in building and installation.
AM_PROGRAMS_SET
AC_PROG_INSTALL

BUILD_TARGET="library_build"
INSTALL_TARGET="library_install"

# Respect the environment LIBS settings
LIBSO_LIBS="$LIBS"

# This is where we handle stuff that autoconf can't handle: compiler,
# preprocessor and load flags, libraries that the standard tests don't
# look for.
#
# There are additional libraries we need for some compiler/architecture
# combinations.
#
# Some architectures require DB to be compiled with special flags and/or
# libraries for threaded applications
#
# The makefile CC may be different than the CC used in config testing,
# because the makefile CC may be set to use $(LIBTOOL).
#
# Don't override anything if it's already set from the environment.
optimize_debug="-O"
case "$host_os" in
aix4.3.*|aix[[56]]*)
	case "$host_os" in
	aix4.3.*)
		CPPFLAGS="$CPPFLAGS -D_LINUX_SOURCE_COMPAT";;
	esac
	# IBM's XLC compilers (at least versions 7/8/9) generate incorrect code
	# when ordinary optimization is enabled because they make strong
	# assumptions about the types held at each memory location, and some
	# Berkeley DB code violates those assumptions.  [#16141]
	optimize_debug="-O2 -qalias=noansi"
	CC=${CC-"xlc_r"}
	CPPFLAGS="$CPPFLAGS -D_THREAD_SAFE"
	LDFLAGS="$LDFLAGS -Wl,-brtl";;
bsdi3*)	CC=${CC-"shlicc2"}
	LIBSO_LIBS="$LIBSO_LIBS -lipc";;
cygwin*)
	CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE -D_REENTRANT";;
freebsd*)
	CPPFLAGS="$CPPFLAGS -D_THREAD_SAFE"
	LDFLAGS="$LDFLAGS -pthread";;
gnu*|k*bsd*-gnu|linux*)
	CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE -D_REENTRANT";;
hpux*)	CPPFLAGS="$CPPFLAGS -D_REENTRANT";;
irix*)	optimize_debug="-O2"
	CPPFLAGS="$CPPFLAGS -D_SGI_MP_SOURCE";;
mpeix*)	CPPFLAGS="$CPPFLAGS -D_POSIX_SOURCE -D_SOCKET_SOURCE"
	LIBSO_LIBS="$LIBSO_LIBS -lsocket -lsvipc";;
osf*)	CPPFLAGS="$CPPFLAGS -pthread";;
*qnx*)	qnx_build="yes"
	AC_DEFINE(HAVE_QNX)
	AH_TEMPLATE(HAVE_QNX, [Define to 1 if building on QNX.]);;
solaris*)
	CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS ";;
esac

# If the user wants a debugging environment, change any compiler optimization
# flags to -g.  We used to add -g to the -O compiler flags, but compilers are
# good enough at code re-organization that debugging with -O no longer works.
# If you want to compile with a different set of flags, specify CFLAGS in the
# environment before configuring.
if test "$db_cv_debug" = "yes"; then
	AC_DEFINE(DEBUG)
	AH_TEMPLATE(DEBUG, [Define to 1 if you want a debugging version.])

	optimize_debug="-g"
fi

# Set CFLAGS/CXXFLAGS.  We MUST set the flags before we call autoconf
# compiler configuration macros, because if we don't, they set CFLAGS
# to no optimization and -g, which isn't what we want.
CFLAGS=${CFLAGS-$optimize_debug}
CXXFLAGS=${CXXFLAGS-"$CFLAGS"}

# The default compiler is cc (NOT gcc), the default CFLAGS is as specified
# above, NOT what is set by AC_PROG_CC, as it won't set optimization flags
# for any compiler other than gcc.
AC_PROG_CC(cc gcc)

# We know what compiler we're going to use, now.  Set per-compiler flags.
if test "$GCC" = "yes"; then
	# Use -O3 if we're using gcc, unless we're doing a small build, in
	# which case we use -Os alone.  The code size for -O3 is quite a
	# bit larger than -O2: a compromise is "-Os -finline-functions",
	# it's smaller and explicitly inlining the functions helps Berkeley
	# DB.
	CFLAGS="$CFLAGS "
	if test "$db_cv_smallbuild" = "yes"; then
		CFLAGS=`echo "$CFLAGS" | sed 's/-O /-Os /g'`
	else
		CFLAGS=`echo "$CFLAGS" | sed 's/-O /-O3 /g'`
	fi
else
	case "$host_os" in
	hpux11.0*)	;;
	hpux11*)	CPPFLAGS="$CPPFLAGS -mt";;
	esac
fi

# Check for "const" and "inline" keywords.
AC_C_CONST
AC_C_INLINE

# We use prototypes and the keyword "const" in db.h which doesn't include
# db_config.h, so we have to figure out what to do there.
#
# There is an autoconf AC_C_PROTOTYPES macro, but as all it does is define
# db_config.h variables, it doesn't help us.
#
# We don't have much choice, we look at internal autoconf variables.
if test "$ac_cv_c_const" != "yes"; then
	DB_CONST="#define const"
fi

# Clear __P, some other systems use it too.
DB_PROTO1="#undef __P"
if test "$ac_cv_prog_cc_c89" = "no"; then
	DB_PROTO2="#define	__P(protos)	()"
else
	DB_PROTO2="#define	__P(protos)	protos"
fi

# Because of shared library building, the ${CC} used for config tests
# may be different than the ${CC} we want to put in the Makefile.
# The latter is known as ${MAKEFILE_CC} in this script.
MAKEFILE_CC="${CC}"
MAKEFILE_CCLINK="${CC}"
MAKEFILE_CXX="nocxx"
MAKEFILE_CXXLINK="nocxx"

# See if we need the C++ compiler at all.  If so, we'd like to find one that
# interoperates with the C compiler we chose.  Since we prefered cc over gcc,
# we'll also prefer the vendor's compiler over g++/gcc.  If we're wrong, the
# user can set CC and CXX in their environment before running configure.
#
# AC_PROG_CXX sets CXX, but it uses $CXX and $CCC (in that order) as its
# first choices.
if test "$db_cv_cxx" = "yes"; then
	if test "$GCC" != "yes"; then
		case "$host_os" in
		aix*)		AC_CHECK_TOOL(CCC, xlC_r)
				LIBXSO_LIBS="-lC_r $LIBXSO_LIBS"
				LIBSO_LIBS="-lC_r $LIBSO_LIBS";;
		hpux*)		AC_CHECK_TOOL(CCC, aCC);;
		irix*)		AC_CHECK_TOOL(CCC, CC);;
		osf*)		AC_CHECK_TOOL(CCC, cxx)
				CXXFLAGS="$CXXFLAGS -D__USE_STD_IOSTREAM"
				test -d /usr/include.dtk &&
				    CXXFLAGS="$CXXFLAGS -I/usr/include.dtk";;
		solaris*)	AC_CHECK_TOOL(CCC, CC);;
		esac
	fi
	AC_PROG_CXX
	###### WORKAROUND: SEE SR #7938
	AC_PROG_CXXCPP
	###############################
	AC_CXX_STDHEADERS
	MAKEFILE_CXX="${CXX}"
	MAKEFILE_CXXLINK="${CXX}"
fi

# Do some gcc specific configuration.
AC_GCC_CONFIG1

# We need the -Kthread/-pthread flag when compiling on SCO/Caldera's UnixWare
# and OpenUNIX releases.  We can't make the test until we know which compiler
# we're using.
case "$host_os" in
sysv5UnixWare*|sysv5OpenUNIX8*)
	if test "$GCC" == "yes"; then
		CPPFLAGS="$CPPFLAGS -pthread"
		LDFLAGS="$LDFLAGS -pthread"
	else
		CPPFLAGS="$CPPFLAGS -Kthread"
		LDFLAGS="$LDFLAGS -Kthread"
	fi;;
esac

# Export our compiler preferences for the libtool configuration.
export CC CCC
CCC=CXX

# Libtool configuration.
AC_PROG_LIBTOOL

SOFLAGS="-rpath \$(libdir)"

# Set SOSUFFIX and friends
SOSUFFIX_CONFIG
MODSUFFIX_CONFIG
JMODSUFFIX_CONFIG

LIBTOOL="./libtool"

INSTALLER="\$(LIBTOOL) --mode=install cp -p"

MAKEFILE_CC="\$(LIBTOOL) --mode=compile ${MAKEFILE_CC}"
MAKEFILE_SOLINK="\$(LIBTOOL) --mode=link ${MAKEFILE_CCLINK} -avoid-version"
MAKEFILE_CCLINK="\$(LIBTOOL) --mode=link ${MAKEFILE_CCLINK}"
MAKEFILE_CXX="\$(LIBTOOL) --mode=compile ${MAKEFILE_CXX}"
MAKEFILE_XSOLINK="\$(LIBTOOL) --mode=link ${MAKEFILE_CXXLINK} -avoid-version"
MAKEFILE_CXXLINK="\$(LIBTOOL) --mode=link ${MAKEFILE_CXXLINK}"


case "$host_os" in
cygwin* | mingw*)
	MAKEFILE_SOLINK="$MAKEFILE_SOLINK -no-undefined"
	MAKEFILE_XSOLINK="$MAKEFILE_XSOLINK -no-undefined";;
esac

case "$host_os" in
    darwin*)
        LIBTSO_MODULE=""
        LIBTSO_MODSUFFIX=".dylib";;
    *qnx*)
        LIBTSO_MODULE=""
        LIBTSO_MODSUFFIX=$MODSUFFIX;;
    *)
        LIBTSO_MODULE="-module"
        LIBTSO_MODSUFFIX=$MODSUFFIX;;
esac

# C API.
if test "$enable_shared" = "no"; then
	DEFAULT_LIB="\$(libdb_version)"
	POSTLINK=": "
	o=".o"
else
	DEFAULT_LIB="\$(libso_target)"
	POSTLINK="\$(LIBTOOL) --mode=execute true"
	o=".lo"
fi
INSTALL_LIBS="$DEFAULT_LIB"
if test "$enable_static" = "yes"; then
	INSTALL_LIBS="$INSTALL_LIBS \$(libdb)"
fi

# Optional C++ API.
if test "$db_cv_cxx" = "yes"; then
	if test "$enable_shared" = "no"; then
		DEFAULT_LIB_CXX="\$(libcxx_version)"
	fi
	if test "$enable_shared" = "yes"; then
		DEFAULT_LIB_CXX="\$(libxso_target)"
	fi
	INSTALL_LIBS="$INSTALL_LIBS $DEFAULT_LIB_CXX"
	if test "$enable_static" = "yes"; then
		INSTALL_LIBS="$INSTALL_LIBS \$(libcxx)"
	fi
fi

# Optional Java API.
if test "$db_cv_java" = "yes"; then
	# Java requires shared libraries.
	if test "$enable_shared" = "no"; then
		AC_MSG_ERROR([Java requires shared libraries])
	fi

        # A classpath that includes . is needed to check for Java
	# Since Cygwin uses Windows' javac, we need Windows path separators
	case "$host_os" in 
	cygwin*)	CLASSPATH=".;$CLASSPATH";;
	*)		CLASSPATH=".:$CLASSPATH";;
	esac
	export CLASSPATH
	AC_PROG_JAVAC
	AC_PROG_JAR
	AC_PROG_JAVA
	AC_JNI_INCLUDE_DIR

	AC_MSG_CHECKING(java version)
        case "$JAVA" in
	*kaffe* )
		JAVA_VERSION=`$JAVA -version 2>&1 |
			sed -e '/Java Version:/!d' -e 's/.*Java Version: \([[^ 	]]*\)[[ 	]]*/\1/'` ;;
	* )	JAVA_VERSION=`$JAVA -version 2>&1 |
        	       	sed -e '/ version /!d' -e 's/.*"\(.*\)".*/\1/'` ;;
	esac
	AC_MSG_RESULT($JAVA_VERSION)
	case "$JAVA_VERSION" in
	1.[[3456789]]* | 1.[[1-9]][[0-9]]* | [[23456789]]* ) ;;
	* )
		AC_MSG_ERROR([Java version 1.3 or higher required, got $JAVA_VERSION]) ;;
	esac

	# Because of the code that SWIG generates to cast between pointers and
	# integers, we need to add the flag "-fno-strict-aliasing" to the gcc
	# command line when compiling the JNI code.  This is documented in
	# [#14953] and at http://www.swig.org/Doc1.3/Java.html
	if test "${GCC}" = "yes"; then
		SWIGCFLAGS="-fno-strict-aliasing"
	fi

	for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
	do
		CPPFLAGS="$CPPFLAGS -I$JNI_INCLUDE_DIR"
	done

	ADDITIONAL_LANG="$ADDITIONAL_LANG java"
	INSTALL_LIBS="$INSTALL_LIBS \$(libjso_target)"
else
	JAVAC=nojavac
fi

# MinGW support.
if test "$db_cv_mingw" = "yes"; then
	OSDIR=os_windows
	PATH_SEPARATOR="\\\\/:"

	AC_DEFINE(DB_WIN32)
	AC_DEFINE(STDC_HEADERS)
else
	OSDIR=os
	PATH_SEPARATOR="/"
	AC_DEFINE(HAVE_SYSTEM_INCLUDE_FILES)
fi

# Optional STL API.
if test "$db_cv_stl" = "yes"; then
	AC_CXX_SUPPORTS_TEMPLATES
	AC_CXX_WSTRING
	AX_TLS
	if test "$enable_shared" = "no"; then
		DEFAULT_LIB_STL="\$(libstl_version)"
	fi
	if test "$enable_shared" = "yes"; then
		DEFAULT_LIB_STL="\$(libstlso_target)"
	fi
	ADDITIONAL_INCS="dbstl_common.h dbstl_set.h dbstl_vector.h dbstl_exception.h dbstl_map.h dbstl_utility.h dbstl_dbc.h dbstl_dbt.h dbstl_base_iterator.h dbstl_container.h dbstl_element_ref.h dbstl_inner_utility.h dbstl_resource_manager.h $ADDITIONAL_INCS"
	INSTALL_LIBS="$INSTALL_LIBS $DEFAULT_LIB_STL"
	if test "$enable_static" = "yes"; then
		INSTALL_LIBS="$INSTALL_LIBS \$(libstl)"
	fi
fi

# Checks for include files, structures, C types.
AC_HEADER_STAT
AC_HEADER_TIME
AC_HEADER_DIRENT
AC_CHECK_HEADERS(execinfo.h sys/select.h sys/socket.h sys/time.h)
AC_CHECK_MEMBERS([struct stat.st_blksize])
AM_TYPES

AC_CACHE_CHECK([for ANSI C exit success/failure values], db_cv_exit_defines, [
AC_TRY_COMPILE([#include <stdlib.h>], return (EXIT_SUCCESS);,
    [db_cv_exit_defines=yes], [db_cv_exit_defines=no])])
if test "$db_cv_exit_defines" = "yes"; then
	AC_DEFINE(HAVE_EXIT_SUCCESS)
	AH_TEMPLATE(HAVE_EXIT_SUCCESS,
	    [Define to 1 if platform has EXIT_SUCCESS/EXIT_FAILURE #defines.])
fi

AC_CACHE_CHECK([for getopt optreset variable], db_cv_optreset, [
AC_TRY_LINK([#include <unistd.h>], extern int optreset; optreset = 1;,
    [db_cv_optreset=yes], [db_cv_optreset=no])])
if test "$db_cv_optreset" = "yes"; then
	AC_DEFINE(HAVE_GETOPT_OPTRESET)
	AH_TEMPLATE(HAVE_GETOPT_OPTRESET,
	    [Define to 1 if getopt supports the optreset variable.])
fi

# Check for mutexes.
# We do this first because it changes $LIBSO_LIBS.
AM_DEFINE_MUTEXES

# Check for native (system call or instruction set) support for
# atomic increment, decrement, and compare & exchange.
AM_DEFINE_ATOMIC

# Test for various functions/libraries -- do tests that change library values
# first.
#
# Update LIBS, so we're testing against the current list of libraries.
LIBS="$LIBSO_LIBS"

# The yield function on Solaris is almost certainly pthread_yield (LWP threads
# or POSIX pthreads), or thr_yield (UI threads).  There's an outside chance it
# is sched_yield() though, only available in -lrt on Solaris.
AC_SEARCH_LIBS(sched_yield, rt)

# The Berkeley DB library calls fdatasync, only available in -lrt on Solaris.
AC_SEARCH_LIBS(fdatasync, rt)

AC_SEARCH_LIBS(getaddrinfo, nsl socket)
AC_SEARCH_LIBS(hstrerror, resolv)

# Those tests updated LIBS, update our internal list.
LIBSO_LIBS="$LIBS"

# !!!
# We could be more exact about whether these libraries are needed, but don't
# bother -- if they exist, we load them, it's only the test programs anyway.
AC_HAVE_LIBRARY(m, TEST_LIBS="$TEST_LIBS -lm")
AC_HAVE_LIBRARY(nsl, TEST_LIBS="$TEST_LIBS -lnsl")
AC_HAVE_LIBRARY(socket, TEST_LIBS="$TEST_LIBS -lsocket")

# Checks for system functions for which we have replacements.
#
# The only portable getcwd call is getcwd(char *, size_t), where the
# buffer is non-NULL -- Solaris can't handle a NULL buffer, and they
# deleted getwd().
AC_REPLACE_FUNCS(\
	abort atoi atol getcwd getenv getopt isalpha isdigit isprint\
	isspace memcmp memcpy memmove printf qsort raise rand strcasecmp\
	strcat strchr strdup strerror strncat strncmp strrchr strsep\
	strtol strtoul)

# Check for system functions we optionally use.
AC_CHECK_FUNCS(\
	_fstati64 backtrace backtrace_symbols directio fchmod fclose\
	fcntl fdatasync fgetc fgets fopen fwrite getgid\
	getrusage getuid hstrerror mprotect pstat_getdynamic\
	pthread_self pthread_yield random sched_yield select setgid setuid\
	sigaction snprintf stat sysconf vsnprintf yield)

AC_TIMERS

# Ftruncate.
# We've run into a problem with ftruncate on Alpha/Tru64, the issue is that
# after a truncate the last page of the file mmaps as all zeros.  So just don't
# use ftruncate.
case "$host_os" in
osf*)
	AC_MSG_WARN(
	    [ftruncate ignored on $host_os-$host_vendor.]);;
*)
	AC_CHECK_FUNCS(ftruncate);;
esac

# Pread/pwrite.
# HP-UX has pread/pwrite, but it doesn't work with largefile support.
# NCR's version of System V R 4.3 has pread/pwrite symbols, but no support.
case "$host_os-$host_vendor" in
hpux*|sysv4.3*-ncr)
	AC_MSG_WARN(
	    [pread/pwrite interfaces ignored on $host_os-$host_vendor.]);;
*)
	AC_CHECK_FUNCS(pread pwrite);;
esac

# Check for getaddrinfo; do the test explicitly instead of using AC_CHECK_FUNCS
# because <netdb.h> isn't a standard include file.
AC_CACHE_CHECK([for getaddrinfo], db_cv_getaddrinfo, [
AC_TRY_LINK([
#include <sys/types.h>
#include <netdb.h>], [
	getaddrinfo(0, 0, 0, 0);
], [db_cv_getaddrinfo=yes], [db_cv_getaddrinfo=no])])
if test "$db_cv_getaddrinfo" = "yes"; then
	AC_DEFINE(HAVE_GETADDRINFO)
	AH_TEMPLATE(HAVE_GETADDRINFO,
	    [Define to 1 if you have the `getaddrinfo' function.])
fi

# Check for the fcntl F_SETFD flag to deny child process access to file
# descriptors.
AC_CACHE_CHECK([for fcntl/F_SETFD], db_cv_fcntl_f_setfd, [
AC_TRY_LINK([
#include <sys/types.h>
#include <fcntl.h>], [
	fcntl(1, F_SETFD, 1);
], [db_cv_fcntl_f_setfd=yes], [db_cv_fcntl_f_setfd=no])])
if test "$db_cv_fcntl_f_setfd" = "yes"; then
	AC_DEFINE(HAVE_FCNTL_F_SETFD)
	AH_TEMPLATE(HAVE_FCNTL_F_SETFD,
    [Define to 1 if fcntl/F_SETFD denies child access to file descriptors.])
fi

# A/UX has a broken getopt(3).
case "$host_os" in
aux*)	AC_LIBOBJ([getopt]);;
esac

# Linux has a broken O_DIRECT flag, but you can't detect it at configure time.
# Linux and SGI require buffer alignment we may not match, otherwise writes
# will fail.  Default to not using the O_DIRECT flag.
if test "$db_cv_o_direct" = "yes"; then
	AC_CACHE_CHECK([for open/O_DIRECT], db_cv_open_o_direct, [
	AC_TRY_LINK([
	#include <sys/types.h>
	#include <fcntl.h>], [
		open("a", O_RDONLY | O_DIRECT, 0);
	], [db_cv_open_o_direct=yes], [db_cv_open_o_direct=no])])
	if test \
	    "$db_cv_o_direct" = "yes" -a "$db_cv_open_o_direct" = "yes"; then
		AC_DEFINE(HAVE_O_DIRECT)
		AH_TEMPLATE(HAVE_O_DIRECT,
		    [Define to 1 if you have the O_DIRECT flag.])
	fi
fi

# Check for largefile support.
AC_SYS_LARGEFILE

# Figure out how to create shared regions.
#
# First, we look for mmap.
#
# BSD/OS has mlock(2), but it doesn't work until the 4.1 release.
#
# Nextstep (version 3.3) apparently supports mmap(2) (the mmap symbol
# is defined in the C library) but does not support munmap(2).  Don't
# try to use mmap if we can't find munmap.
#
# Ultrix has mmap(2), but it doesn't work.
mmap_ok=no
case "$host_os" in
bsdi3*|bsdi4.0)
	AC_MSG_WARN([mlock(2) interface ignored on $host_os-$host_vendor.])
	mmap_ok=yes
	AC_CHECK_FUNCS(mmap munmap, , mmap_ok=no);;
ultrix*)
	AC_MSG_WARN([mmap(2) interface ignored on $host_os-$host_vendor.]);;
*)
	mmap_ok=yes
	AC_CHECK_FUNCS(mlock munlock)
	AC_CHECK_FUNCS(mmap munmap, , mmap_ok=no);;
esac

# Second, we look for shmget.
#
# SunOS has the shmget(2) interfaces, but there appears to be a missing
# #include <debug/debug.h> file, so we ignore them.
shmget_ok=no
case "$host_os" in
sunos*)
	AC_MSG_WARN([shmget(2) interface ignored on $host_os-$host_vendor.]);;
*)
	shmget_ok=yes
	AC_CHECK_FUNCS(shmget, , shmget_ok=no)

	# Check for shmctl to lock down shared memory segments.
	AC_CACHE_CHECK([for shmctl], db_cv_shmctl_shm_lock, [
	AC_TRY_LINK([
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>], [
		shmctl(0, SHM_LOCK, NULL);
	], [db_cv_shmctl_shm_lock=yes], [db_cv_shmctl_shm_lock=no])])
	if test "$db_cv_shmctl_shm_lock" = "yes"; then
		AC_DEFINE(HAVE_SHMCTL_SHM_LOCK)
		AH_TEMPLATE(HAVE_SHMCTL_SHM_LOCK,
	    [Define to 1 if shmctl/SHM_LOCK locks down shared memory segments.])
	fi;;
esac

# We require either mmap/munmap(2) or shmget(2).
if test "$mmap_ok" = "no" -a "$shmget_ok" = "no"; then
	AC_MSG_WARN([Neither mmap/munmap(2) or shmget(2) library functions.])
fi

# Optional RPC client/server.
if test "$db_cv_rpc" = "yes"; then
	AM_RPC_CONFIGURE
fi

# Optional Tcl support.
if test "$db_cv_tcl" = "yes"; then
	AM_TCL_LOAD
fi

# Optional sequence code.
AM_SEQUENCE_CONFIGURE

# Optional DB 1.85 compatibility API.
if test "$db_cv_compat185" = "yes"; then
	ADDITIONAL_INCS="db_185.h $ADDITIONAL_INCS"

	ADDITIONAL_OBJS="db185${o} $ADDITIONAL_OBJS"
fi

# Optional utilities.
if test "$db_cv_dump185" = "yes"; then
	ADDITIONAL_PROGS="db_dump185 $ADDITIONAL_PROGS"
fi

# You can disable pieces of functionality to save space.
#
# Btree is always configured: it is the standard method, and Hash off-page
# duplicates require it.
ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(BTREE_OBJS)"

# Compression can be disabled.
if test "$db_cv_build_compression" = "yes"; then
	AC_DEFINE(HAVE_COMPRESSION)
	AH_TEMPLATE(HAVE_COMPRESSION, [Define to 1 if building compression support.])
fi

# Partitioning can be disabled.
if test "$db_cv_build_partition" = "yes"; then
	AC_DEFINE(HAVE_PARTITION)
	AH_TEMPLATE(HAVE_PARTITION, [Define to 1 if building partitioned database support.])
fi

# Hash can be disabled.
if test "$db_cv_build_hash" = "yes"; then
	AC_DEFINE(HAVE_HASH)
	AH_TEMPLATE(HAVE_HASH, [Define to 1 if building Hash access method.])
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(HASH_OBJS)"
	if test "$db_cv_build_verify" = "yes"; then
		ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(HASH_VRFY_OBJS)"
	fi
else
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS hash_stub${o}"
fi

# Queue can be disabled.
if test "$db_cv_build_queue" = "yes"; then
	AC_DEFINE(HAVE_QUEUE)
	AH_TEMPLATE(HAVE_QUEUE, [Define to 1 if building Queue access method.])
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(QUEUE_OBJS)"
	if test "$db_cv_build_verify" = "yes"; then
		ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(QUEUE_VRFY_OBJS)"
	fi
else
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS qam_stub${o}"
fi

# Replication can be disabled.
if test "$db_cv_build_replication" = "yes"; then
	AC_DEFINE(HAVE_REPLICATION)
	AH_TEMPLATE(HAVE_REPLICATION,
	    [Define to 1 if building replication support.])
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(REP_OBJS)"

	# If we're building replication and detected POSIX threads, build the
	# replication manager.
	AH_TEMPLATE(HAVE_REPLICATION_THREADS,
	    [Define to 1 if building the Berkeley DB replication framework.])

	if test "$ac_cv_header_pthread_h" = yes; then
		AC_DEFINE(HAVE_REPLICATION_THREADS)

		# Solaris requires the socket and nsl libraries to build the
		# replication manager.  Don't add nsl regardless of the OS,
		# it causes RPC to fail on AIX 4.3.3.
		case "$host_os" in
		solaris*)
			AC_HAVE_LIBRARY(nsl, LIBSO_LIBS="$LIBSO_LIBS -lnsl")
			AC_HAVE_LIBRARY(socket,
			    LIBSO_LIBS="$LIBSO_LIBS -lsocket");;
		esac
		ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(REPMGR_OBJS)"
	else
		ADDITIONAL_OBJS="$ADDITIONAL_OBJS repmgr_stub${o}"
	fi
else
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS rep_stub${o} repmgr_stub${o}"
fi

# The statistics code can be disabled.
if test "$db_cv_build_statistics" = "yes"; then
	AC_DEFINE(HAVE_STATISTICS)
	AH_TEMPLATE(HAVE_STATISTICS,
	    [Define to 1 if building statistics support.])
fi

# The verification code can be disabled.
if test "$db_cv_build_verify" = "yes"; then
	AC_DEFINE(HAVE_VERIFY)
	AH_TEMPLATE(HAVE_VERIFY,
	    [Define to 1 if building access method verification support.])
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(BTREE_VRFY_OBJS)"
else
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS db_vrfy_stub${o}"
fi

# The crypto code can be disabled.
if test -d "$srcdir/../crypto" -a "$db_cv_build_cryptography" = "yes"; then
	AC_DEFINE(HAVE_CRYPTO)
	AH_TEMPLATE(HAVE_CRYPTO,
    [Define to 1 if Berkeley DB release includes strong cryptography.])

	CRYPTO_OBJS="\$(CRYPTO_OBJS)"
else
	CRYPTO_OBJS="crypto_stub${o}"
fi

# The mutex code can be disabled, and if there aren't any mutexes, then there's
# no reason to include the locking code.
if test "$db_cv_build_mutexsupport" = "yes"; then
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(LOCK_OBJS) \$(MUTEX_OBJS)"
else
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS lock_stub${o} mut_stub${o}"
fi

# If DIAGNOSTIC is defined, include the log print routines in the library
# itself, various diagnostic modes use them.
if test "$db_cv_diagnostic" = "yes"; then
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS \$(PRINT_OBJS)"
fi

# If building for QNX, we need additional OS files.
if test "$qnx_build" = "yes"; then
	ADDITIONAL_OBJS="$ADDITIONAL_OBJS os_qnx_fsync${o} os_qnx_open${o}"
fi

# We need to add the additional object files into the Makefile with the correct
# suffix.  We can't use $LTLIBOBJS itself, because that variable has $U encoded
# in it for automake, and that's not what we want.  See SR #7227 for additional
# information.
#
# XXX: I'm not sure this is correct.
REPLACEMENT_OBJS=`echo "$LIB@&t@OBJS" |
		sed "s,\.[[^.]]* ,$o ,g;s,\.[[^.]]*$,$o,"`

# This is necessary so that .o files in LIBOBJS are also built via
# the ANSI2KNR-filtering rules.
LIB@&t@OBJS=`echo "$LIB@&t@OBJS" |
		sed 's,\.[[^.]]* ,$U&,g;s,\.[[^.]]*$,$U&,'`
LTLIBOBJS=`echo "$LIB@&t@OBJS" |
		sed 's,\.[[^.]]* ,.lo ,g;s,\.[[^.]]*$,.lo,'`
AC_SUBST(LTLIBOBJS)

# Initial output file list.
CREATE_LIST="Makefile
    db_cxx.h:$srcdir/../dbinc/db_cxx.in
    db_int.h:$srcdir/../dbinc/db_int.in
    clib_port.h:$srcdir/../dist/clib_port.in
    include.tcl:$srcdir/../test/include.tcl"

# Create the db.h file from a source file, a list of global function
# prototypes, and, if configured for unique names, a list of #defines
# to do DB_VERSION_UNIQUE_NAME substitution.
if test "$db_cv_uniquename" = "yes"; then
	CREATE_LIST="$CREATE_LIST
	db.h:$srcdir/../dbinc/db.in:$srcdir/../dbinc_auto/api_flags.in:$srcdir/../dbinc_auto/ext_def.in:$srcdir/../dbinc_auto/ext_prot.in"
else
	CREATE_LIST="$CREATE_LIST
	db.h:$srcdir/../dbinc/db.in:$srcdir/../dbinc_auto/api_flags.in:$srcdir/../dbinc_auto/ext_prot.in"
fi

# If configured for unique names, create the db_int_uext.h file (which
# does the DB_VERSION_UNIQUE_NAME substitution), which is included by
# the db_int.h file.
if test "$db_cv_uniquename" = "yes"; then
	CREATE_LIST="$CREATE_LIST
	db_int_def.h:$srcdir/../dbinc_auto/int_def.in"
	db_int_def='#include "db_int_def.h"'
fi

# Create the db_185.h and db185_int.h files from source files, a list of
# global function prototypes, and, if configured for unique names, a list
# of #defines to do DB_VERSION_UNIQUE_NAME substitution.
if test "$db_cv_compat185" = "yes"; then
	if test "$db_cv_uniquename" = "yes"; then
		CREATE_LIST="$CREATE_LIST
		db_185.h:$srcdir/../dbinc/db_185.in:$srcdir/../dbinc_auto/ext_185_def.in:$srcdir/../dbinc_auto/ext_185_prot.in
		db185_int.h:$srcdir/../db185/db185_int.in:$srcdir/../dbinc_auto/ext_185_def.in:$srcdir/../dbinc_auto/ext_185_prot.in"
	else
		CREATE_LIST="$CREATE_LIST
		db_185.h:$srcdir/../dbinc/db_185.in:$srcdir/../dbinc_auto/ext_185_prot.in
		db185_int.h:$srcdir/../db185/db185_int.in:$srcdir/../dbinc_auto/ext_185_prot.in"
	fi
fi

if test "$db_cv_stl" = "yes"; then
	CREATE_LIST="$CREATE_LIST
	dbstl_common.h:$srcdir/../stl/dbstl_common.in"
fi

AC_CONFIG_FILES($CREATE_LIST)
AC_OUTPUT
