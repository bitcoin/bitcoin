# SYNOPSIS
#
#   AX_BOOST_PROCESS
#
# DESCRIPTION
#
#   Test for Process library from the Boost C++ libraries. The macro requires
#   a preceding call to AX_BOOST_BASE. Further documentation is available at
#   <http://randspringer.de/boost/index.html>.
#
#   This macro calls:
#
#     AC_SUBST(BOOST_PROCESS_LIB)
#
#   And sets:
#
#     HAVE_BOOST_PROCESS
#
# LICENSE
#
#   Copyright (c) 2012 Xiyue Deng <manphiz@gmail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 4

AC_DEFUN([AX_BOOST_PROCESS],
[
	AC_ARG_WITH([boost-process],
	AS_HELP_STRING([--with-boost-process@<:@=special-lib@:>@],
                   [use the Process library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-process=boost_process-gcc-mt ]),
        [
        if test "$withval" = "no"; then
			want_boost="no"
        elif test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_process_lib=""
        else
		    want_boost="yes"
		ax_boost_user_process_lib="$withval"
		fi
        ],
        [want_boost="yes"]
	)

	if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
        AC_REQUIRE([AC_CANONICAL_BUILD])
		CPPFLAGS_SAVED="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
		export CPPFLAGS

		LDFLAGS_SAVED="$LDFLAGS"
		LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
		export LDFLAGS

        AC_CACHE_CHECK(whether the Boost::Process library is available,
					   ax_cv_boost_process,
        [AC_LANG_PUSH([C++])
			 CXXFLAGS_SAVE=$CXXFLAGS

			 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <boost/process.hpp>]],
                                   [[boost::process::child* child = new boost::process::child; delete child;]])],
                   ax_cv_boost_process=yes, ax_cv_boost_process=no)
			 CXXFLAGS=$CXXFLAGS_SAVE
             AC_LANG_POP([C++])
		])
		if test "x$ax_cv_boost_process" = "xyes"; then
			AC_SUBST(BOOST_CPPFLAGS)

			AC_DEFINE(HAVE_BOOST_PROCESS,,[define if the Boost::Process library is available])
            BOOSTLIBDIR=`echo $BOOST_LDFLAGS | sed -e 's/@<:@^\/@:>@*//'`

			LDFLAGS_SAVE=$LDFLAGS
            if test "x$ax_boost_user_process_lib" = "x"; then
                for libextension in `ls $BOOSTLIBDIR/libboost_process*.so* $BOOSTLIBDIR/libboost_process*.dylib* $BOOSTLIBDIR/libboost_process*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^lib\(boost_process.*\)\.so.*$;\1;' -e 's;^lib\(boost_process.*\)\.dylib.*$;\1;' -e 's;^lib\(boost_process.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [BOOST_PROCESS_LIB="-l$ax_lib"; AC_SUBST(BOOST_PROCESS_LIB) link_process="yes"; break],
                                 [link_process="no"])
				done
                if test "x$link_process" != "xyes"; then
                for libextension in `ls $BOOSTLIBDIR/boost_process*.dll* $BOOSTLIBDIR/boost_process*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^\(boost_process.*\)\.dll.*$;\1;' -e 's;^\(boost_process.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [BOOST_PROCESS_LIB="-l$ax_lib"; AC_SUBST(BOOST_PROCESS_LIB) link_process="yes"; break],
                                 [link_process="no"])
				done
                fi

            else
               for ax_lib in $ax_boost_user_process_lib boost_process-$ax_boost_user_process_lib; do
				      AC_CHECK_LIB($ax_lib, exit,
                                   [BOOST_PROCESS_LIB="-l$ax_lib"; AC_SUBST(BOOST_PROCESS_LIB) link_process="yes"; break],
                                   [link_process="no"])
                  done

            fi
            if test "x$ax_lib" = "x"; then
                AC_MSG_ERROR(Could not find a version of the library!)
            fi
			if test "x$link_process" = "xno"; then
				AC_MSG_ERROR(Could not link against $ax_lib !)
			fi
		fi

		CPPFLAGS="$CPPFLAGS_SAVED"
	LDFLAGS="$LDFLAGS_SAVED"
	fi
])
