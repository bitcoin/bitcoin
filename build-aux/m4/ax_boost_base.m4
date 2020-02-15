# ===========================================================================
#      https://www.gnu.org/software/autoconf-archive/ax_boost_base.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_BOOST_BASE([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Test for the Boost C++ libraries of a particular version (or newer)
#
#   If no path to the installed boost library is given the macro searchs
#   under /usr, /usr/local, /opt and /opt/local and evaluates the
#   $BOOST_ROOT environment variable. Further documentation is available at
#   <http://randspringer.de/boost/index.html>.
#
#   This macro calls:
#
#     AC_SUBST(BOOST_CPPFLAGS) / AC_SUBST(BOOST_LDFLAGS)
#
#   And sets:
#
#     HAVE_BOOST
#
# LICENSE
#
#   Copyright (c) 2008 Thomas Porschberg <thomas@randspringer.de>
#   Copyright (c) 2009 Peter Adolphs
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 47

# example boost program (need to pass version)
m4_define([_AX_BOOST_BASE_PROGRAM],
          [AC_LANG_PROGRAM([[
#include <boost/version.hpp>
]],[[
(void) ((void)sizeof(char[1 - 2*!!((BOOST_VERSION) < ($1))]));
]])])

AC_DEFUN([AX_BOOST_BASE],
[
AC_ARG_WITH([boost],
  [AS_HELP_STRING([--with-boost@<:@=ARG@:>@],
    [use Boost library from a standard location (ARG=yes),
     from the specified location (ARG=<path>),
     or disable it (ARG=no)
     @<:@ARG=yes@:>@ ])],
    [
     AS_CASE([$withval],
       [no],[want_boost="no";_AX_BOOST_BASE_boost_path=""],
       [yes],[want_boost="yes";_AX_BOOST_BASE_boost_path=""],
       [want_boost="yes";_AX_BOOST_BASE_boost_path="$withval"])
    ],
    [want_boost="yes"])


AC_ARG_WITH([boost-libdir],
  [AS_HELP_STRING([--with-boost-libdir=LIB_DIR],
    [Force given directory for boost libraries.
     Note that this will override library path detection,
     so use this parameter only if default library detection fails
     and you know exactly where your boost libraries are located.])],
  [
   AS_IF([test -d "$withval"],
         [_AX_BOOST_BASE_boost_lib_path="$withval"],
    [AC_MSG_ERROR([--with-boost-libdir expected directory name])])
  ],
  [_AX_BOOST_BASE_boost_lib_path=""])

BOOST_LDFLAGS=""
BOOST_CPPFLAGS=""
AS_IF([test "x$want_boost" = "xyes"],
      [_AX_BOOST_BASE_RUNDETECT([$1],[$2],[$3])])
AC_SUBST(BOOST_CPPFLAGS)
AC_SUBST(BOOST_LDFLAGS)
])


# convert a version string in $2 to numeric and affect to polymorphic var $1
AC_DEFUN([_AX_BOOST_BASE_TONUMERICVERSION],[
  AS_IF([test "x$2" = "x"],[_AX_BOOST_BASE_TONUMERICVERSION_req="1.20.0"],[_AX_BOOST_BASE_TONUMERICVERSION_req="$2"])
  _AX_BOOST_BASE_TONUMERICVERSION_req_shorten=`expr $_AX_BOOST_BASE_TONUMERICVERSION_req : '\([[0-9]]*\.[[0-9]]*\)'`
  _AX_BOOST_BASE_TONUMERICVERSION_req_major=`expr $_AX_BOOST_BASE_TONUMERICVERSION_req : '\([[0-9]]*\)'`
  AS_IF([test "x$_AX_BOOST_BASE_TONUMERICVERSION_req_major" = "x"],
        [AC_MSG_ERROR([You should at least specify libboost major version])])
  _AX_BOOST_BASE_TONUMERICVERSION_req_minor=`expr $_AX_BOOST_BASE_TONUMERICVERSION_req : '[[0-9]]*\.\([[0-9]]*\)'`
  AS_IF([test "x$_AX_BOOST_BASE_TONUMERICVERSION_req_minor" = "x"],
        [_AX_BOOST_BASE_TONUMERICVERSION_req_minor="0"])
  _AX_BOOST_BASE_TONUMERICVERSION_req_sub_minor=`expr $_AX_BOOST_BASE_TONUMERICVERSION_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
  AS_IF([test "X$_AX_BOOST_BASE_TONUMERICVERSION_req_sub_minor" = "X"],
        [_AX_BOOST_BASE_TONUMERICVERSION_req_sub_minor="0"])
  _AX_BOOST_BASE_TONUMERICVERSION_RET=`expr $_AX_BOOST_BASE_TONUMERICVERSION_req_major \* 100000 \+  $_AX_BOOST_BASE_TONUMERICVERSION_req_minor \* 100 \+ $_AX_BOOST_BASE_TONUMERICVERSION_req_sub_minor`
  AS_VAR_SET($1,$_AX_BOOST_BASE_TONUMERICVERSION_RET)
])

dnl Run the detection of boost should be run only if $want_boost
AC_DEFUN([_AX_BOOST_BASE_RUNDETECT],[
    _AX_BOOST_BASE_TONUMERICVERSION(WANT_BOOST_VERSION,[$1])
    succeeded=no


    AC_REQUIRE([AC_CANONICAL_HOST])
    dnl On 64-bit systems check for system libraries in both lib64 and lib.
    dnl The former is specified by FHS, but e.g. Debian does not adhere to
    dnl this (as it rises problems for generic multi-arch support).
    dnl The last entry in the list is chosen by default when no libraries
    dnl are found, e.g. when only header-only libraries are installed!
    AS_CASE([${host_cpu}],
      [x86_64],[libsubdirs="lib64 libx32 lib lib64"],
      [mips*64*],[libsubdirs="lib64 lib32 lib lib64"],
      [ppc64|powerpc64|s390x|sparc64|aarch64|ppc64le|powerpc64le|riscv64],[libsubdirs="lib64 lib lib64"],
      [libsubdirs="lib"]
    )

    dnl allow for real multi-arch paths e.g. /usr/lib/x86_64-linux-gnu. Give
    dnl them priority over the other paths since, if libs are found there, they
    dnl are almost assuredly the ones desired.
    AS_CASE([${host_cpu}],
      [i?86],[multiarch_libsubdir="lib/i386-${host_os}"],
      [multiarch_libsubdir="lib/${host_cpu}-${host_os}"]
    )

    dnl first we check the system location for boost libraries
    dnl this location ist chosen if boost libraries are installed with the --layout=system option
    dnl or if you install boost with RPM
    AS_IF([test "x$_AX_BOOST_BASE_boost_path" != "x"],[
        AC_MSG_CHECKING([for boostlib >= $1 ($WANT_BOOST_VERSION) includes in "$_AX_BOOST_BASE_boost_path/include"])
         AS_IF([test -d "$_AX_BOOST_BASE_boost_path/include" && test -r "$_AX_BOOST_BASE_boost_path/include"],[
           AC_MSG_RESULT([yes])
           BOOST_CPPFLAGS="-I$_AX_BOOST_BASE_boost_path/include"
           for _AX_BOOST_BASE_boost_path_tmp in $multiarch_libsubdir $libsubdirs; do
                AC_MSG_CHECKING([for boostlib >= $1 ($WANT_BOOST_VERSION) lib path in "$_AX_BOOST_BASE_boost_path/$_AX_BOOST_BASE_boost_path_tmp"])
                AS_IF([test -d "$_AX_BOOST_BASE_boost_path/$_AX_BOOST_BASE_boost_path_tmp" && test -r "$_AX_BOOST_BASE_boost_path/$_AX_BOOST_BASE_boost_path_tmp" ],[
                        AC_MSG_RESULT([yes])
                        BOOST_LDFLAGS="-L$_AX_BOOST_BASE_boost_path/$_AX_BOOST_BASE_boost_path_tmp";
                        break;
                ],
      [AC_MSG_RESULT([no])])
           done],[
      AC_MSG_RESULT([no])])
    ],[
        if test X"$cross_compiling" = Xyes; then
            search_libsubdirs=$multiarch_libsubdir
        else
            search_libsubdirs="$multiarch_libsubdir $libsubdirs"
        fi
        for _AX_BOOST_BASE_boost_path_tmp in /usr /usr/local /opt /opt/local ; do
            if test -d "$_AX_BOOST_BASE_boost_path_tmp/include/boost" && test -r "$_AX_BOOST_BASE_boost_path_tmp/include/boost" ; then
                for libsubdir in $search_libsubdirs ; do
                    if ls "$_AX_BOOST_BASE_boost_path_tmp/$libsubdir/libboost_"* >/dev/null 2>&1 ; then break; fi
                done
                BOOST_LDFLAGS="-L$_AX_BOOST_BASE_boost_path_tmp/$libsubdir"
                BOOST_CPPFLAGS="-I$_AX_BOOST_BASE_boost_path_tmp/include"
                break;
            fi
        done
    ])

    dnl overwrite ld flags if we have required special directory with
    dnl --with-boost-libdir parameter
    AS_IF([test "x$_AX_BOOST_BASE_boost_lib_path" != "x"],
          [BOOST_LDFLAGS="-L$_AX_BOOST_BASE_boost_lib_path"])

    AC_MSG_CHECKING([for boostlib >= $1 ($WANT_BOOST_VERSION)])
    CPPFLAGS_SAVED="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
    export CPPFLAGS

    LDFLAGS_SAVED="$LDFLAGS"
    LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
    export LDFLAGS

    AC_REQUIRE([AC_PROG_CXX])
    AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE([_AX_BOOST_BASE_PROGRAM($WANT_BOOST_VERSION)],[
        AC_MSG_RESULT(yes)
    succeeded=yes
    found_system=yes
        ],[
        ])
    AC_LANG_POP([C++])



    dnl if we found no boost with system layout we search for boost libraries
    dnl built and installed without the --layout=system option or for a staged(not installed) version
    if test "x$succeeded" != "xyes" ; then
        CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
        BOOST_CPPFLAGS=
        if test -z "$_AX_BOOST_BASE_boost_lib_path" ; then
            BOOST_LDFLAGS=
        fi
        _version=0
        if test -n "$_AX_BOOST_BASE_boost_path" ; then
            if test -d "$_AX_BOOST_BASE_boost_path" && test -r "$_AX_BOOST_BASE_boost_path"; then
                for i in `ls -d $_AX_BOOST_BASE_boost_path/include/boost-* 2>/dev/null`; do
                    _version_tmp=`echo $i | sed "s#$_AX_BOOST_BASE_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                    V_CHECK=`expr $_version_tmp \> $_version`
                    if test "x$V_CHECK" = "x1" ; then
                        _version=$_version_tmp
                    fi
                    VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                    BOOST_CPPFLAGS="-I$_AX_BOOST_BASE_boost_path/include/boost-$VERSION_UNDERSCORE"
                done
                dnl if nothing found search for layout used in Windows distributions
                if test -z "$BOOST_CPPFLAGS"; then
                    if test -d "$_AX_BOOST_BASE_boost_path/boost" && test -r "$_AX_BOOST_BASE_boost_path/boost"; then
                        BOOST_CPPFLAGS="-I$_AX_BOOST_BASE_boost_path"
                    fi
                fi
                dnl if we found something and BOOST_LDFLAGS was unset before
                dnl (because "$_AX_BOOST_BASE_boost_lib_path" = ""), set it here.
                if test -n "$BOOST_CPPFLAGS" && test -z "$BOOST_LDFLAGS"; then
                    for libsubdir in $libsubdirs ; do
                        if ls "$_AX_BOOST_BASE_boost_path/$libsubdir/libboost_"* >/dev/null 2>&1 ; then break; fi
                    done
                    BOOST_LDFLAGS="-L$_AX_BOOST_BASE_boost_path/$libsubdir"
                fi
            fi
        else
            if test "x$cross_compiling" != "xyes" ; then
                for _AX_BOOST_BASE_boost_path in /usr /usr/local /opt /opt/local ; do
                    if test -d "$_AX_BOOST_BASE_boost_path" && test -r "$_AX_BOOST_BASE_boost_path" ; then
                        for i in `ls -d $_AX_BOOST_BASE_boost_path/include/boost-* 2>/dev/null`; do
                            _version_tmp=`echo $i | sed "s#$_AX_BOOST_BASE_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                            V_CHECK=`expr $_version_tmp \> $_version`
                            if test "x$V_CHECK" = "x1" ; then
                                _version=$_version_tmp
                                best_path=$_AX_BOOST_BASE_boost_path
                            fi
                        done
                    fi
                done

                VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                BOOST_CPPFLAGS="-I$best_path/include/boost-$VERSION_UNDERSCORE"
                if test -z "$_AX_BOOST_BASE_boost_lib_path" ; then
                    for libsubdir in $libsubdirs ; do
                        if ls "$best_path/$libsubdir/libboost_"* >/dev/null 2>&1 ; then break; fi
                    done
                    BOOST_LDFLAGS="-L$best_path/$libsubdir"
                fi
            fi

            if test -n "$BOOST_ROOT" ; then
                for libsubdir in $libsubdirs ; do
                    if ls "$BOOST_ROOT/stage/$libsubdir/libboost_"* >/dev/null 2>&1 ; then break; fi
                done
                if test -d "$BOOST_ROOT" && test -r "$BOOST_ROOT" && test -d "$BOOST_ROOT/stage/$libsubdir" && test -r "$BOOST_ROOT/stage/$libsubdir"; then
                    version_dir=`expr //$BOOST_ROOT : '.*/\(.*\)'`
                    stage_version=`echo $version_dir | sed 's/boost_//' | sed 's/_/./g'`
                        stage_version_shorten=`expr $stage_version : '\([[0-9]]*\.[[0-9]]*\)'`
                    V_CHECK=`expr $stage_version_shorten \>\= $_version`
                    if test "x$V_CHECK" = "x1" && test -z "$_AX_BOOST_BASE_boost_lib_path" ; then
                        AC_MSG_NOTICE(We will use a staged boost library from $BOOST_ROOT)
                        BOOST_CPPFLAGS="-I$BOOST_ROOT"
                        BOOST_LDFLAGS="-L$BOOST_ROOT/stage/$libsubdir"
                    fi
                fi
            fi
        fi

        CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
        export CPPFLAGS
        LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
        export LDFLAGS

        AC_LANG_PUSH(C++)
            AC_COMPILE_IFELSE([_AX_BOOST_BASE_PROGRAM($WANT_BOOST_VERSION)],[
            AC_MSG_RESULT(yes)
        succeeded=yes
        found_system=yes
            ],[
            ])
        AC_LANG_POP([C++])
    fi

    if test "x$succeeded" != "xyes" ; then
        if test "x$_version" = "x0" ; then
            AC_MSG_NOTICE([[We could not detect the boost libraries (version $1 or higher). If you have a staged boost library (still not installed) please specify \$BOOST_ROOT in your environment and do not give a PATH to --with-boost option.  If you are sure you have boost installed, then check your version number looking in <boost/version.hpp>. See http://randspringer.de/boost for more documentation.]])
        else
            AC_MSG_NOTICE([Your boost libraries seems to old (version $_version).])
        fi
        # execute ACTION-IF-NOT-FOUND (if present):
        ifelse([$3], , :, [$3])
    else
        AC_DEFINE(HAVE_BOOST,,[define if the Boost library is available])
        # execute ACTION-IF-FOUND (if present):
        ifelse([$2], , :, [$2])
    fi

    CPPFLAGS="$CPPFLAGS_SAVED"
    LDFLAGS="$LDFLAGS_SAVED"

])
