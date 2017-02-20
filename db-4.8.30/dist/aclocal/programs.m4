# $Id$

# Check for programs used in building/installation.
AC_DEFUN(AM_PROGRAMS_SET, [

AC_CHECK_TOOL(CHMOD, chmod, none)
test "$CHMOD" = "none" && AC_MSG_ERROR([No chmod utility found.])

AC_CHECK_TOOL(CP, cp, none)
test "$CP" = "none" && AC_MSG_ERROR([No cp utility found.])

# The Tcl test suite requires a kill utility.
if test "$db_cv_test" = "yes"; then
	AC_CHECK_TOOL(KILL, kill, none)
	test "$KILL" = "none" && AC_MSG_ERROR([No kill utility found.])
fi

AC_CHECK_TOOL(LN, ln, none)
test "$LN" = "none" && AC_MSG_ERROR([No ln utility found.])

AC_CHECK_TOOL(MKDIR, mkdir, none)
test "$MKDIR" = "none" && AC_MSG_ERROR([No mkdir utility found.])

AC_CHECK_TOOL(RM, rm, none)
test "$RM" = "none" && AC_MSG_ERROR([No rm utility found.])

# We always want to force removes, and libtool assumes the same.
RM="$RM -f"

if test "$db_cv_rpc" = "yes"; then
	AC_CHECK_TOOL(RPCGEN, rpcgen, none)
	test "$RPCGEN" = "none" && AC_MSG_ERROR([No rpcgen utility found.])
fi

# We need a complete path for sh, because some make utility implementations get
# upset if SHELL is set to just the command name.  Don't use the SHELL variable
# here because the user likely has the SHELL variable set to something other
# than the Bourne shell, which is what Make wants.
AC_PATH_TOOL(db_cv_path_sh, sh, none)
test "$db_cv_path_sh" = "none" && AC_MSG_ERROR([No sh utility found.])

# Don't strip the binaries if --enable-debug was specified.
if test "$db_cv_debug" = yes; then
	STRIP=":"
fi])
