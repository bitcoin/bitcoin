# $Id$

# Process user-specified options.
AC_DEFUN(AM_OPTIONS_SET, [

# --enable-bigfile was the configuration option that Berkeley DB used before
# autoconf 2.50 was released (which had --enable-largefile integrated in).
AC_ARG_ENABLE(bigfile,
	[AC_HELP_STRING([--disable-bigfile],
			[Obsolete; use --disable-largefile instead.])],
	[AC_MSG_ERROR(
	    [--enable-bigfile no longer supported, use --enable-largefile])])

AC_MSG_CHECKING(if --disable-cryptography option specified)
AC_ARG_ENABLE(cryptography,
	AC_HELP_STRING([--disable-cryptography],
	    [Do not build database cryptography support.]),, enableval="yes")
db_cv_build_cryptography="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --disable-hash option specified)
AC_ARG_ENABLE(hash,
	AC_HELP_STRING([--disable-hash],
	    [Do not build Hash access method.]),, enableval="yes")
db_cv_build_hash="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --disable-partition option specified)
AC_ARG_ENABLE(partition,
	AC_HELP_STRING([--disable-partition],
	    [Do not build partitioned database support.]),, enableval="yes")
db_cv_build_partition="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --disable-compression option specified)
AC_ARG_ENABLE(compression,
	AC_HELP_STRING([--disable-compression],
	    [Do not build compression support.]),, enableval="yes")
db_cv_build_compression="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --disable-mutexsupport option specified)
AC_ARG_ENABLE(mutexsupport,
	AC_HELP_STRING([--disable-mutexsupport],
	    [Do not build any mutex support.]),, enableval="yes")
db_cv_build_mutexsupport="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --disable-atomicsupport option specified)
AC_ARG_ENABLE(atomicsupport,
	AC_HELP_STRING([--disable-atomicsupport],
	    [Do not build any native atomic operation support.]),, enableval="yes")
db_cv_build_atomicsupport="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --disable-queue option specified)
AC_ARG_ENABLE(queue,
	AC_HELP_STRING([--disable-queue],
	    [Do not build Queue access method.]),, enableval="yes")
db_cv_build_queue="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --disable-replication option specified)
AC_ARG_ENABLE(replication,
	AC_HELP_STRING([--disable-replication],
	    [Do not build database replication support.]),, enableval="yes")
db_cv_build_replication="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --disable-statistics option specified)
AC_ARG_ENABLE(statistics,
	AC_HELP_STRING([--disable-statistics],
	    [Do not build statistics support.]),, enableval="yes")
db_cv_build_statistics="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --disable-verify option specified)
AC_ARG_ENABLE(verify,
	AC_HELP_STRING([--disable-verify],
	    [Do not build database verification support.]),, enableval="yes")
db_cv_build_verify="$enableval"
case "$enableval" in
 no) AC_MSG_RESULT(yes);;
yes) AC_MSG_RESULT(no);;
esac

AC_MSG_CHECKING(if --enable-compat185 option specified)
AC_ARG_ENABLE(compat185,
	[AC_HELP_STRING([--enable-compat185],
			[Build DB 1.85 compatibility API.])],
	[db_cv_compat185="$enable_compat185"], [db_cv_compat185="no"])
AC_MSG_RESULT($db_cv_compat185)

AC_MSG_CHECKING(if --enable-cxx option specified)
AC_ARG_ENABLE(cxx,
	[AC_HELP_STRING([--enable-cxx],
			[Build C++ API.])],
	[db_cv_cxx="$enable_cxx"], [db_cv_cxx="no"])
AC_MSG_RESULT($db_cv_cxx)

AC_MSG_CHECKING(if --enable-debug option specified)
AC_ARG_ENABLE(debug,
	[AC_HELP_STRING([--enable-debug],
			[Build a debugging version.])],
	[db_cv_debug="$enable_debug"], [db_cv_debug="no"])
AC_MSG_RESULT($db_cv_debug)

AC_MSG_CHECKING(if --enable-debug_rop option specified)
AC_ARG_ENABLE(debug_rop,
	[AC_HELP_STRING([--enable-debug_rop],
			[Build a version that logs read operations.])],
	[db_cv_debug_rop="$enable_debug_rop"], [db_cv_debug_rop="no"])
AC_MSG_RESULT($db_cv_debug_rop)

AC_MSG_CHECKING(if --enable-debug_wop option specified)
AC_ARG_ENABLE(debug_wop,
	[AC_HELP_STRING([--enable-debug_wop],
			[Build a version that logs write operations.])],
	[db_cv_debug_wop="$enable_debug_wop"], [db_cv_debug_wop="no"])
AC_MSG_RESULT($db_cv_debug_wop)

AC_MSG_CHECKING(if --enable-diagnostic option specified)
AC_ARG_ENABLE(diagnostic,
	[AC_HELP_STRING([--enable-diagnostic],
			[Build a version with run-time diagnostics.])],
	[db_cv_diagnostic="$enable_diagnostic"], [db_cv_diagnostic="no"])
if test "$db_cv_diagnostic" = "yes"; then
	AC_MSG_RESULT($db_cv_diagnostic)
fi
if test "$db_cv_diagnostic" = "no" -a "$db_cv_debug_rop" = "yes"; then
	db_cv_diagnostic="yes"
	AC_MSG_RESULT([by --enable-debug_rop])
fi
if test "$db_cv_diagnostic" = "no" -a "$db_cv_debug_wop" = "yes"; then
	db_cv_diagnostic="yes"
	AC_MSG_RESULT([by --enable-debug_wop])
fi
if test "$db_cv_diagnostic" = "no"; then
	AC_MSG_RESULT($db_cv_diagnostic)
fi

AC_MSG_CHECKING(if --enable-dump185 option specified)
AC_ARG_ENABLE(dump185,
	[AC_HELP_STRING([--enable-dump185],
			[Build db_dump185(1) to dump 1.85 databases.])],
	[db_cv_dump185="$enable_dump185"], [db_cv_dump185="no"])
AC_MSG_RESULT($db_cv_dump185)

AC_MSG_CHECKING(if --enable-java option specified)
AC_ARG_ENABLE(java,
	[AC_HELP_STRING([--enable-java],
			[Build Java API.])],
	[db_cv_java="$enable_java"], [db_cv_java="no"])
AC_MSG_RESULT($db_cv_java)

AC_MSG_CHECKING(if --enable-mingw option specified)
AC_ARG_ENABLE(mingw,
	[AC_HELP_STRING([--enable-mingw],
			[Build Berkeley DB for MinGW.])],
	[db_cv_mingw="$enable_mingw"], [db_cv_mingw="no"])
AC_MSG_RESULT($db_cv_mingw)

AC_MSG_CHECKING(if --enable-o_direct option specified)
AC_ARG_ENABLE(o_direct,
	[AC_HELP_STRING([--enable-o_direct],
			[Enable the O_DIRECT flag for direct I/O.])],
	[db_cv_o_direct="$enable_o_direct"], [db_cv_o_direct="no"])
AC_MSG_RESULT($db_cv_o_direct)

AC_MSG_CHECKING(if --enable-posixmutexes option specified)
AC_ARG_ENABLE(posixmutexes,
	[AC_HELP_STRING([--enable-posixmutexes],
			[Force use of POSIX standard mutexes.])],
	[db_cv_posixmutexes="$enable_posixmutexes"], [db_cv_posixmutexes="no"])
AC_MSG_RESULT($db_cv_posixmutexes)

AC_ARG_ENABLE(pthread_self,,
	[AC_MSG_WARN([--enable-pthread_self is now always enabled])])

AC_ARG_ENABLE(pthread_api,,
	[AC_MSG_WARN([--enable-pthread_api is now always enabled])])

AC_MSG_CHECKING(if --enable-rpc option specified)
AC_ARG_ENABLE(rpc,,
	[if test "x$DB_FORCE_RPC" = x ; then
		AC_MSG_ERROR([RPC support has been removed from Berkeley DB.  Please check the release notes]);
	 else
		db_cv_rpc="$enable_rpc";
	 fi], [db_cv_rpc="no"])
AC_MSG_RESULT($db_cv_rpc)

AC_MSG_CHECKING(if --enable-smallbuild option specified)
AC_ARG_ENABLE(smallbuild,
	[AC_HELP_STRING([--enable-smallbuild],
			[Build small footprint version of the library.])],
	[db_cv_smallbuild="$enable_smallbuild"], [db_cv_smallbuild="no"])
if test "$db_cv_smallbuild" = "yes"; then
	db_cv_build_cryptography="no"
	db_cv_build_hash="no"
	db_cv_build_queue="no"
	db_cv_build_replication="no"
	db_cv_build_statistics="no"
	db_cv_build_verify="no"
	db_cv_build_partition="no"
	db_cv_build_compression="no"
fi
AC_MSG_RESULT($db_cv_smallbuild)

AC_MSG_CHECKING(if --enable-stl option specified)
AC_ARG_ENABLE(stl,
	[AC_HELP_STRING([--enable-stl],
			[Build STL API.])],
	[db_cv_stl="$enable_stl"], [db_cv_stl="no"])
if test "$db_cv_stl" = "yes" -a "$db_cv_cxx" = "no"; then
	db_cv_cxx="yes"
fi
AC_MSG_RESULT($db_cv_stl)

AC_MSG_CHECKING(if --enable-tcl option specified)
AC_ARG_ENABLE(tcl,
	[AC_HELP_STRING([--enable-tcl],
			[Build Tcl API.])],
	[db_cv_tcl="$enable_tcl"], [db_cv_tcl="no"])
AC_MSG_RESULT($db_cv_tcl)

AC_MSG_CHECKING(if --enable-test option specified)
AC_ARG_ENABLE(test,
	[AC_HELP_STRING([--enable-test],
			[Configure to run the test suite.])],
	[db_cv_test="$enable_test"], [db_cv_test="no"])
AC_MSG_RESULT($db_cv_test)

AC_MSG_CHECKING(if --enable-uimutexes option specified)
AC_ARG_ENABLE(uimutexes,
	[AC_HELP_STRING([--enable-uimutexes],
			[Force use of Unix International mutexes.])],
	[db_cv_uimutexes="$enable_uimutexes"], [db_cv_uimutexes="no"])
AC_MSG_RESULT($db_cv_uimutexes)

AC_MSG_CHECKING(if --enable-umrw option specified)
AC_ARG_ENABLE(umrw,
	[AC_HELP_STRING([--enable-umrw],
			[Mask harmless uninitialized memory read/writes.])],
	[db_cv_umrw="$enable_umrw"], [db_cv_umrw="no"])
AC_MSG_RESULT($db_cv_umrw)

AC_MSG_CHECKING(if --with-mutex=MUTEX option specified)
AC_ARG_WITH(mutex,
	[AC_HELP_STRING([--with-mutex=MUTEX],
			[Select non-default mutex implementation.])],
	[with_mutex="$withval"], [with_mutex="no"])
if test "$with_mutex" = "yes"; then
	AC_MSG_ERROR([--with-mutex requires a mutex name argument])
fi
if test "$with_mutex" != "no"; then
	db_cv_mutex="$with_mutex"
fi
AC_MSG_RESULT($with_mutex)

# --with-mutexalign=ALIGNMENT was the configuration option that Berkeley DB
# used before the DbEnv::mutex_set_align method was added.
AC_ARG_WITH(mutexalign,
	[AC_HELP_STRING([--with-mutexalign=ALIGNMENT],
			[Obsolete; use DbEnv::mutex_set_align instead.])],
	[AC_MSG_ERROR(
    [--with-mutexalign no longer supported, use DbEnv::mutex_set_align])])

AC_MSG_CHECKING([if --with-tcl=DIR option specified])
AC_ARG_WITH(tcl,
	[AC_HELP_STRING([--with-tcl=DIR],
			[Directory location of tclConfig.sh.])],
	[with_tclconfig="$withval"], [with_tclconfig="no"])
AC_MSG_RESULT($with_tclconfig)
if test "$with_tclconfig" != "no"; then
	db_cv_tcl="yes"
fi

AC_MSG_CHECKING([if --with-uniquename=NAME option specified])
AC_ARG_WITH(uniquename,
	[AC_HELP_STRING([--with-uniquename=NAME],
			[Build a uniquely named library.])],
	[with_uniquename="$withval"], [with_uniquename="no"])
if test "$with_uniquename" = "no"; then
	db_cv_uniquename="no"
	DB_VERSION_UNIQUE_NAME=""
	AC_MSG_RESULT($with_uniquename)
else
	db_cv_uniquename="yes"
	if test "$with_uniquename" = "yes"; then
		DB_VERSION_UNIQUE_NAME="__EDIT_DB_VERSION_UNIQUE_NAME__"
	else
		DB_VERSION_UNIQUE_NAME="$with_uniquename"
	fi
	AC_MSG_RESULT($DB_VERSION_UNIQUE_NAME)
fi

# Test requires Tcl
if test "$db_cv_test" = "yes"; then
	if test "$db_cv_tcl" = "no"; then
		AC_MSG_ERROR([--enable-test requires --enable-tcl])
	fi
fi])
