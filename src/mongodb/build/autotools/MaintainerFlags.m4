AS_IF(
   [test "x$enable_maintainer_flags" = "xyes" && test "x$GCC" = "xyes"],
   [
    # clang only warns if it doesn't support a warning option, turn it into an
    # error so we really know if whether it supports it.
    AX_CHECK_COMPILE_FLAG(
       "-Werror=unknown-warning-option",
       [WERROR_UNKNOWN_OPTION="-Werror=unknown-warning-option"],
       [WERROR_UNKNOWN_OPTION=""])

    # Read maintainer-flags.txt and apply each flag that the compiler supports.
    m4_foreach([MAINTAINER_FLAG], m4_split(m4_normalize(m4_esyscmd(cat build/maintainer-flags.txt))), [
        AX_CHECK_COMPILE_FLAG(
           MAINTAINER_FLAG,
           [MAINTAINER_CFLAGS="$MAINTAINER_CFLAGS MAINTAINER_FLAG"],
           [],
           [$WERROR_UNKNOWN_OPTION])
    ])
   ]

   AC_SUBST(MAINTAINER_CFLAGS)
)
