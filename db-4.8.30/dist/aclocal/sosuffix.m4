# $Id$
# Determine shared object suffixes.
#
# Our method is to use the libtool variable $library_names_spec,
# set by using AC_PROG_LIBTOOL.  This variable is a snippet of shell
# defined in terms of $versuffix, $release, $libname and $module
# We want to eval it and grab the suffix used for shared objects.
# By setting $module to yes/no, we obtain the suffixes
# used to create dlloadable, or java loadable modules.
# On many (*nix) systems, these all evaluate to .so, but there
# are some notable exceptions.
# Before calling this macro, libtool must have been configured.

# This macro is used internally to discover the suffix for the current
# settings of $module.  The result is stored in $_SOSUFFIX.
AC_DEFUN(_SOSUFFIX_INTERNAL, [
	versuffix=""
	release=""
	libname=libfoo
	eval _SOSUFFIX=\"$shrext_cmds\"
	if test "$_SOSUFFIX" = "" ; then
		_SOSUFFIX=".so"
		if test "$enable_shared" != "yes"; then
			if test "$_SOSUFFIX_MESSAGE" = ""; then
				_SOSUFFIX_MESSAGE=yes
        			AC_MSG_WARN([libtool may not know about this architecture.])
               			AC_MSG_WARN([assuming $_SOSUFFIX suffix for dynamic libraries.])
			fi
        	fi
        fi
])

# SOSUFFIX_CONFIG will set the variable SOSUFFIX to be the
# shared library extension used for general linking, not dlopen.
AC_DEFUN(SOSUFFIX_CONFIG, [
	AC_MSG_CHECKING([SOSUFFIX from libtool])
	module=no
        _SOSUFFIX_INTERNAL
        SOSUFFIX=$_SOSUFFIX
	AC_MSG_RESULT($SOSUFFIX)
	AC_SUBST(SOSUFFIX)
])

# MODSUFFIX_CONFIG will set the variable MODSUFFIX to be the
# shared library extension used for dlopen'ed modules.
# To discover this, we set $module, simulating libtool's -module option.
AC_DEFUN(MODSUFFIX_CONFIG, [
	AC_MSG_CHECKING([MODSUFFIX from libtool])
	module=yes
        _SOSUFFIX_INTERNAL
        MODSUFFIX=$_SOSUFFIX
	AC_MSG_RESULT($MODSUFFIX)
	AC_SUBST(MODSUFFIX)
])

# JMODSUFFIX_CONFIG will set the variable JMODSUFFIX to be the
# shared library extension used JNI modules opened by Java.
# To discover this, we set $jnimodule, simulating libtool's -shrext option.
##########################################################################
# Robert Boehne:  Not much point in this macro any more because apparently
# Darwin is the only OS that wants or needs the .jnilib extension.
##########################################################################
AC_DEFUN(JMODSUFFIX_CONFIG, [
	AC_MSG_CHECKING([JMODSUFFIX from libtool])
	module=yes
        _SOSUFFIX_INTERNAL
	if test `uname` = "Darwin"; then
	    JMODSUFFIX=".jnilib"
	else
            JMODSUFFIX=$_SOSUFFIX
	fi
	AC_MSG_RESULT($JMODSUFFIX)
	AC_SUBST(JMODSUFFIX)
])

