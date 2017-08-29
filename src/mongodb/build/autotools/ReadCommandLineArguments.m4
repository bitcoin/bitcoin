AC_MSG_CHECKING([whether to do a debug build])
AC_ARG_ENABLE(debug, 
    AC_HELP_STRING([--enable-debug], [disable optimizations @<:@default=no@:>@]),
    [],[enable_debug="no"])
AC_MSG_RESULT([$enable_debug])

AC_MSG_CHECKING([whether to enable tracing])
AC_ARG_ENABLE(tracing, 
    AC_HELP_STRING([--enable-tracing], [very verbose debug output @<:@default=no@:>@]),
    [],[enable_tracing="no"])
AC_MSG_RESULT([$enable_tracing])

AC_MSG_CHECKING([whether to automatic init and cleanup])
AC_ARG_ENABLE(automatic-init-and-cleanup,
    AC_HELP_STRING([--enable-automatic-init-and-cleanup],
    [call mongoc_init() and mongoc_cleanup() automatically - DEPRECATED @<:@default=yes@:>@]),
    [],[enable_automatic_init_and_cleanup="yes"])
AC_MSG_RESULT([$enable_automatic_init_and_cleanup])

AC_MSG_CHECKING([whether to enable optimized builds])
AC_ARG_ENABLE(optimizations, 
    AC_HELP_STRING([--enable-optimizations], [turn on build-time optimizations @<:@default=yes@:>@]),
    [enable_optimizations=$enableval],
    [
        if test "$enable_debug" = "yes"; then
            enable_optimizations="no";
        else
            enable_optimizations="yes";
        fi
    ])
AC_MSG_RESULT([$enable_optimizations])

AC_MSG_CHECKING([whether to enable shared memory performance counters])
AC_ARG_ENABLE(shm_counters,
    AC_HELP_STRING([--enable-shm-counters], [turn on shared memory performance counters @<:@default=yes@:>@]),
    [],[enable_shm_counters="yes"])
AC_MSG_RESULT([$enable_shm_counters])

AC_MSG_CHECKING([whether to enable code coverage support])
AC_ARG_ENABLE(coverage,
    AC_HELP_STRING([--enable-coverage], [enable code coverage support @<:@default=no@:>@]),
    [],
    [enable_coverage="no"])
AC_MSG_RESULT([$enable_coverage])

AC_MSG_CHECKING([whether to enable debug symbols])
AC_ARG_ENABLE(debug_symbols,
    AC_HELP_STRING([--enable-debug-symbols=yes|no|min],
                   [enable debug symbols @<:@default=yes for debug builds, otherwise no@:>@]),
    [
        case "$enable_debug_symbols" in
            yes) enable_debug_symbols="full" ;;
            no|min|full) ;;
            *) AC_MSG_ERROR([Invalid debug symbols option: must be yes, no, or min.]) ;;
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

AC_ARG_ENABLE([rdtscp],
              [AS_HELP_STRING([--enable-rdtscp=@<:@no/yes@:>@],
                              [fast performance counters on Intel using the RDTSCP instruction @<:@default=no@:>@])],
              [],
              [enable_rdtscp=no])

# use strict compiler flags only on development releases
AS_IF([test "x$MONGOC_PRERELEASE_VERSION" != "x"],
      [maintainer_flags_default=yes],
      [maintainer_flags_default=no])

AC_ARG_ENABLE([maintainer-flags],
              [AS_HELP_STRING([--enable-maintainer-flags=@<:@no/yes@:>@],
                              [use strict compiler checks @<:@default=no for release builds, yes for prereleases@:>@])],
              [],
              enable_maintainer_flags=$maintainer_flags_default)

# Check if we should use the bundled (git submodule) libbson
AC_ARG_WITH(libbson,
    AC_HELP_STRING([--with-libbson=@<:@auto/system/bundled@:>@],
                   [use system installed libbson or bundled libbson. default=auto]),
    [],
    [with_libbson=auto])
AS_IF([test "x$with_libbson" != xbundled && test "x$with_libbson" != xsystem],
      [with_libbson=auto])

AC_ARG_WITH(snappy,
    AC_HELP_STRING([--with-snappy=@<:@auto/system/bundled/no@:>@],
                   [use system installed snappy or bundled snappy. default=auto]),
    [],
    [with_snappy=auto])
AS_IF([test "x$with_snappy" != xbundled && test "x$with_snappy" != xsystem && test "x$with_snappy" != xno],
      [with_snappy=auto])

AC_ARG_WITH(zlib,
    AC_HELP_STRING([--with-zlib=@<:@auto/system/bundled/no@:>@],
                   [use system installed zlib or bundled zlib. default=auto]),
    [],
    [with_zlib=auto])
AS_IF([test "x$with_zlib" != xbundled && test "x$with_zlib" != xsystem && test "x$with_zlib" != xno],
      [with_zlib=auto])

AC_ARG_ENABLE([html-docs],
              [AS_HELP_STRING([--enable-html-docs=@<:@yes/no@:>@],
                              [build HTML documentation @<:@default=no@:>@])],
              [],
              [enable_html_docs=no])

AC_ARG_ENABLE([man-pages],
              [AS_HELP_STRING([--enable-man-pages=@<:@yes/no@:>@],
                              [build and install man pages @<:@default=no@:>@])],
              [],
              [enable_man_pages=no])

AC_ARG_ENABLE([examples],
              [AS_HELP_STRING([--enable-examples=@<:@yes/no@:>@],
                              [build MongoDB C Driver example programs])],
              [],
              [enable_examples=yes])

AC_ARG_ENABLE([tests],
              [AS_HELP_STRING([--enable-tests=@<:@yes/no@:>@],
                              [build MongoDB C Driver tests])],
              [],
              [enable_tests=yes])
