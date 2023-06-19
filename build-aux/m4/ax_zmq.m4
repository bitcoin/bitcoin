# ===========================================================================
#          https://www.gnu.org/software/autoconf-archive/ax_zmq.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_ZMQ([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Test for the ZMQ libraries of a particular version (or newer). The
#   default version tested for is 4.0.0.
#
#   The macro tests for ZMQ libraries in the library/include path, and, when
#   provided, also in the path given by --with-zmq.
#
#   This macro calls:
#
#     AC_SUBST(ZMQ_CPPFLAGS) / AC_SUBST(ZMQ_LDFLAGS) / AC_SUBST(ZMQ_LIBS)
#
#   And sets:
#
#     HAVE_ZMQ
#
# LICENSE
#
#   Copyright (c) 2016 Jeroen Meijer <jjgmeijer@gmail.com>
#   Copyright (c) 2022 l2xl <l2xl@proton.me>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 3

AC_DEFUN([AX_ZMQ], [
    AC_ARG_WITH([zmq], [AS_HELP_STRING([--with-zmq=@<:@=ARG@:>@],
            [use ZMQ from standard location (default),
             use ZMQ prefix path (ARG=<path>),
             or disable ZMQ (ARG=no)])], [
             AS_CASE(["x${withval}"],
                [x],[with_zmq=yes],
                [xyes],[with_zmq=yes],
                [xno],[with_zmq=no],
                [with_zmq=yes; ZMQ_LDFLAGS="-L${withval}/lib"; ZMQ_CPPFLAGS="-I${withval}/include"]
    )])

    HAVE_ZMQ=0
    if test "$with_zmq" != "no"; then

        LD_FLAGS="$LDFLAGS $ZMQ_LDFLAGS"
        CPPFLAGS="$CPPFLAGS $ZMQ_CPPFLAGS"

        AC_LANG_PUSH([C])
        AC_CHECK_HEADER(zmq.h, [zmq_h=yes], [zmq_h=no])
        AC_LANG_POP([C])

        if test "$zmq_h" = "yes"; then
            version=ifelse([$1], ,4.0.0,$1)
            AC_MSG_CHECKING([for ZMQ version >= $version])
            version=$(echo $version | tr '.' ',')
            AC_EGREP_CPP([version_ok], [
#include <zmq.h>
#if defined(ZMQ_VERSION) && ZMQ_VERSION >= ZMQ_MAKE_VERSION($version)
    version_ok
#endif
            ],[
                AC_MSG_RESULT(yes)
                HAVE_ZMQ=1
                ZMQ_LIBS="-lzmq"
                AC_SUBST(ZMQ_LDFLAGS)
                AC_SUBST(ZMQ_CPPFLAGS)
                AC_SUBST(ZMQ_LIBS)
            ], AC_MSG_RESULT([no valid ZMQ version was found]))
        else
            AC_MSG_WARN([no valid ZMQ installation was found])
        fi

        if test $HAVE_ZMQ = 1; then
            # execute ACTION-IF-FOUND (if present):
            ifelse([$2], , :, [$2])
        else
            # execute ACTION-IF-NOT-FOUND (if present):
            ifelse([$3], , :, [$3])
        fi
    else
        AC_MSG_NOTICE([not checking for ZMQ])
    fi

    AC_DEFINE(HAVE_ZMQ,[1],[define if the ZMQ library is available])
])
