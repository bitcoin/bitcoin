AC_PATH_PROG(PERL, perl)
if test -z "$PERL"; then
    AC_MSG_ERROR([You need 'perl' to compile libbson])
fi

AC_PATH_PROG(MV, mv)
if test -z "$MV"; then
    AC_MSG_ERROR([You need 'mv' to compile libbson])
fi

AC_PATH_PROG(GREP, grep)
if test -z "$GREP"; then
    AC_MSG_ERROR([You need 'grep' to compile libbson])
fi

# Optional for documentation
AC_PATH_PROG(SPHINX_BUILD, sphinx-build)

AC_PROG_INSTALL
