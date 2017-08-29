AC_MSG_CHECKING([whether to do a debug build])
AC_ARG_ENABLE(debug, 
    AC_HELP_STRING([--enable-debug], [turn on debugging [default=no]]),
    [],[enable_debug="no"])
AC_MSG_RESULT([$enable_debug])

AC_MSG_CHECKING([whether to enable optimized builds])
AC_ARG_ENABLE(optimizations, 
    AC_HELP_STRING([--enable-optimizations], [turn on build-time optimizations [default=yes]]),
    [enable_optimizations=$enableval],
    [
        if test "$enable_debug" = "yes"; then
            enable_optimizations="no";
        else
            enable_optimizations="yes";
        fi
    ])
AC_MSG_RESULT([$enable_optimizations])

AC_MSG_CHECKING([whether to enable extra alignment of types])
AC_ARG_ENABLE(extra_align,
    AC_HELP_STRING([--enable-extra-align], [turn on extra alignment of types.  This is required for the 1.0 ABI [default=yes]]),
    [enable_extra_align=$enableval],
    [enable_extra_align="yes"])
AC_MSG_RESULT([$enable_extra_align])

AS_IF([test "$enable_extra_align" = "yes"],
      [AC_SUBST(BSON_EXTRA_ALIGN, 1)],
      [AC_SUBST(BSON_EXTRA_ALIGN, 0)])

AC_ARG_ENABLE(lto,
              AC_HELP_STRING([--enable-lto], [turn on link time optimizations [default=no]]),
              [enable_lto=$enableval],
              [enable_lto=no])

AC_MSG_CHECKING([whether to enable code coverage support])
AC_ARG_ENABLE(coverage,
    AC_HELP_STRING([--enable-coverage], [enable code coverage support [default=no]]),
    [],
    [enable_coverage="no"])
AC_MSG_RESULT([$enable_coverage])

AC_MSG_CHECKING([whether to enable debug symbols])
AC_ARG_ENABLE(debug_symbols,
    AC_HELP_STRING([--enable-debug-symbols=yes|no|min|full], [enable debug symbols default=no, default=yes for debug builds]),
    [
        case "$enable_debug_symbols" in
            yes) enable_debug_symbols="full" ;;
            no|min|full) ;;
            *) AC_MSG_ERROR([Invalid debug symbols option: must be yes, no, min or full.]) ;;
        esac
    ],
    [
         if test "$enable_debug" = "yes"; then
             enable_debug_symbols="yes";
         else
             enable_debug_symbols="no";
         fi
    ])
AC_MSG_RESULT([$enable_debug_symbols])

# use strict compiler flags only on development releases
m4_define([maintainer_flags_default], [m4_ifset([BSON_PRERELEASE_VERSION], [yes], [no])])
AC_ARG_ENABLE([maintainer-flags],
              [AS_HELP_STRING([--enable-maintainer-flags=@<:@no/yes@:>@],
                              [Use strict compiler flags @<:@default=]maintainer_flags_default[@:>@])],
              [],
              [enable_maintainer_flags=maintainer_flags_default])

AC_ARG_ENABLE([html-docs],
              [AS_HELP_STRING([--enable-html-docs=@<:@yes/no@:>@],
                              [Build HTML documentation.])],
              [],
              [enable_html_docs=no])

AC_ARG_ENABLE([man-pages],
              [AS_HELP_STRING([--enable-man-pages=@<:@yes/no@:>@],
                              [Build and install man-pages.])],
              [],
              [enable_man_pages=no])

AC_ARG_ENABLE([examples],
              [AS_HELP_STRING([--enable-examples=@<:@yes/no@:>@],
                              [Build libbson examples.])],
              [],
              [enable_examples=yes])

AC_ARG_ENABLE([tests],
              [AS_HELP_STRING([--enable-tests=@<:@yes/no@:>@],
                              [Build libbson tests.])],
              [],
              [enable_tests=yes])
