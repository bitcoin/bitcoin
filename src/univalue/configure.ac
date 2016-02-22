m4_define([libunivalue_major_version], [1])
m4_define([libunivalue_minor_version], [1])
m4_define([libunivalue_micro_version], [1])
m4_define([libunivalue_interface_age], [1])
# If you need a modifier for the version number. 
# Normally empty, but can be used to make "fixup" releases.
m4_define([libunivalue_extraversion], [])

dnl libtool versioning from libunivalue
m4_define([libunivalue_current], [m4_eval(100 * libunivalue_minor_version + libunivalue_micro_version - libunivalue_interface_age)])
m4_define([libunivalue_binary_age], [m4_eval(100 * libunivalue_minor_version + libunivalue_micro_version)])
m4_define([libunivalue_revision], [libunivalue_interface_age])
m4_define([libunivalue_age], [m4_eval(libunivalue_binary_age - libunivalue_interface_age)])
m4_define([libunivalue_version], [libunivalue_major_version().libunivalue_minor_version().libunivalue_micro_version()libunivalue_extraversion()])


AC_INIT([univalue], [1.0.1],
        [http://github.com/jgarzik/univalue/])

dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PREREQ(2.60)
AC_CONFIG_SRCDIR([lib/univalue.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AC_CONFIG_HEADERS([univalue-config.h])
AM_INIT_AUTOMAKE([subdir-objects foreign])

LIBUNIVALUE_MAJOR_VERSION=libunivalue_major_version
LIBUNIVALUE_MINOR_VERSION=libunivalue_minor_version
LIBUNIVALUE_MICRO_VERSION=libunivalue_micro_version
LIBUNIVALUE_INTERFACE_AGE=libunivalue_interface_age

# ABI version
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
LIBUNIVALUE_CURRENT=libunivalue_current
LIBUNIVALUE_REVISION=libunivalue_revision
LIBUNIVALUE_AGE=libunivalue_age

AC_SUBST(LIBUNIVALUE_CURRENT)
AC_SUBST(LIBUNIVALUE_REVISION)
AC_SUBST(LIBUNIVALUE_AGE)

LT_INIT
LT_LANG([C++])

case $host in
  *mingw*)
    LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static"
  ;;
esac

BUILD_EXEEXT=
case $build in
  *mingw*)
    BUILD_EXEEXT=".exe"
  ;;
esac

AC_CONFIG_FILES([
    Makefile
    pc/libunivalue.pc
    pc/libunivalue-uninstalled.pc])

AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(BUILD_EXEEXT)
AC_OUTPUT

