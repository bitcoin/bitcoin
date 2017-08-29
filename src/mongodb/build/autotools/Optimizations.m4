OPTIMIZE_CFLAGS=""
OPTIMIZE_LDFLAGS=""

AC_DEFUN([check_link_flag],
    [AX_CHECK_LINK_FLAG([$1], [$2], [$3], [-Werror $4])])

dnl Check if we should use -Bsymbolic
AS_IF([test "$enable_optimizations" != "no"], [
    check_link_flag([-Wl,-Bsymbolic], [OPTIMIZE_LDFLAGS="$OPTIMIZE_LDFLAGS -Wl,-Bsymbolic"])

    dnl Add the appropriate 'O' level for optimized builds.
    CFLAGS="$CFLAGS -O2"
])

AC_SUBST(OPTIMIZE_CFLAGS)
AC_SUBST(OPTIMIZE_LDFLAGS)


# Add '-g' flag to gcc to build with debug symbols.
if test "$enable_debug_symbols" = "min"; then
    CFLAGS="$CFLAGS -g1"
elif test "$enable_debug_symbols" != "no"; then
    CFLAGS="$CFLAGS -g"
fi


AS_IF([test "$enable_tracing" = "yes"],
      [CPPFLAGS="$CPPFLAGS -DMONGOC_TRACE"])
