# $Id$

# The SC_* macros in this file are from the unix/tcl.m4 files in the Tcl
# 8.3.0 distribution, with some minor changes.  For this reason, license
# terms for the Berkeley DB distribution dist/aclocal/tcl.m4 file are as
# follows (copied from the license.terms file in the Tcl 8.3 distribution):
#
# This software is copyrighted by the Regents of the University of
# California, Sun Microsystems, Inc., Scriptics Corporation,
# and other parties.  The following terms apply to all files associated
# with the software unless explicitly disclaimed in individual files.
#
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors
# and need not follow the licensing terms described here, provided that
# the new terms are clearly indicated on the first page of each file where
# they apply.
#
# IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
# FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
# ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
# DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
# IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
# NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
# MODIFICATIONS.
#
# GOVERNMENT USE: If you are acquiring this software on behalf of the
# U.S. government, the Government shall have only "Restricted Rights"
# in the software and related documentation as defined in the Federal
# Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
# are acquiring the software on behalf of the Department of Defense, the
# software shall be classified as "Commercial Computer Software" and the
# Government shall have only "Restricted Rights" as defined in Clause
# 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
# authors grant the U.S. Government and others acting in its behalf
# permission to use and distribute the software in accordance with the
# terms specified in this license.

AC_DEFUN(SC_PATH_TCLCONFIG, [
	AC_CACHE_VAL(ac_cv_c_tclconfig,[

	    # First check to see if --with-tclconfig was specified.
	    if test "${with_tclconfig}" != no; then
		if test -f "${with_tclconfig}/tclConfig.sh" ; then
		    ac_cv_c_tclconfig=`(cd ${with_tclconfig}; pwd)`
		else
		    AC_MSG_ERROR([${with_tclconfig} directory doesn't contain tclConfig.sh])
		fi
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d /usr/local/lib 2>/dev/null` ; do
		    if test -f "$i/tclConfig.sh" ; then
			ac_cv_c_tclconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi

	])

	if test x"${ac_cv_c_tclconfig}" = x ; then
	    TCL_BIN_DIR="# no Tcl configs found"
	    AC_MSG_ERROR(can't find Tcl configuration definitions)
	else
	    TCL_BIN_DIR=${ac_cv_c_tclconfig}
	fi
])

AC_DEFUN(SC_LOAD_TCLCONFIG, [
	AC_MSG_CHECKING([for existence of $TCL_BIN_DIR/tclConfig.sh])

	if test -f "$TCL_BIN_DIR/tclConfig.sh" ; then
		AC_MSG_RESULT([loading])
		. $TCL_BIN_DIR/tclConfig.sh
	else
		AC_MSG_RESULT([file not found])
	fi

	# DB requires at least version 8.4.
	if test ${TCL_MAJOR_VERSION} -lt 8 \
	    -o ${TCL_MAJOR_VERSION} -eq 8 -a ${TCL_MINOR_VERSION} -lt 4; then
		AC_MSG_ERROR([Berkeley DB requires Tcl version 8.4 or better.])
	fi

	# The eval is required to do substitution (for example, the TCL_DBGX
	# substitution in the TCL_LIB_FILE variable.
	eval "TCL_INCLUDE_SPEC=\"${TCL_INCLUDE_SPEC}\""
	eval "TCL_LIB_FILE=\"${TCL_LIB_FILE}\""
	eval "TCL_LIB_FLAG=\"${TCL_LIB_FLAG}\""
	eval "TCL_LIB_SPEC=\"${TCL_LIB_SPEC}\""

	#
	# If the DB Tcl library isn't loaded with the Tcl spec and library
	# flags on AIX, the resulting libdb_tcl-X.Y.so.0 will drop core at
	# load time. [#4843]  Furthermore, with Tcl 8.3, the link flags
	# given by the Tcl spec are insufficient for our use.  [#5779],[#17109]
	#
	case "$host_os" in
	aix*)
		LIBTSO_LIBS="$LIBTSO_LIBS $TCL_LIB_SPEC $TCL_LIB_FLAG"
		LIBTSO_LIBS="$LIBTSO_LIBS -L$TCL_EXEC_PREFIX/lib -ltcl$TCL_VERSION";;
	esac
	AC_SUBST(TCL_BIN_DIR)
	AC_SUBST(TCL_INCLUDE_SPEC)
	AC_SUBST(TCL_LIB_FILE)
	AC_SUBST(TCL_SRC_DIR)

	AC_SUBST(TCL_TCLSH)
	TCL_TCLSH="${TCL_PREFIX}/bin/tclsh${TCL_VERSION}"
])

# Optional Tcl API.
AC_DEFUN(AM_TCL_LOAD, [
	if test "$enable_shared" != "yes"; then
		AC_MSG_ERROR([Tcl requires shared libraries])
	fi

	SC_PATH_TCLCONFIG
	SC_LOAD_TCLCONFIG

	INSTALL_LIBS="${INSTALL_LIBS} \$(libtso_target)"
])
