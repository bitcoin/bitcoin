dnl BITCOIN_SUBDIR_TO_INCLUDE([CPPFLAGS-VARIABLE-NAME],[SUBDIRECTORY-NAME],[HEADER-FILE])
dnl SUBDIRECTORY-NAME must end with a path separator
AC_DEFUN([BITCOIN_SUBDIR_TO_INCLUDE],[
  if test "x$2" = "x"; then
    AC_MSG_RESULT([default])
  else
    echo "#include <$2$3.h>" >conftest.cpp
    newinclpath=$(${CXXCPP} ${CPPFLAGS} -M conftest.cpp 2>/dev/null | sed [-E -e ':a' -e '/\\$/!b b' -e N -e 's/\\\n/ /' -e 't a' -e ':b' -e 's/^[^:]*:[[:space:]]*(([^[:space:]\]|\\.)*[[:space:]])*(([^[:space:]\]|\\.)*)]$3\.h[([[:space:]].*)?$/\3/' -e 't' -e d])
    AC_MSG_RESULT([${newinclpath}])
    if test "x${newinclpath}" != "x"; then
      eval "$1=\"\$$1\"' -I${newinclpath}'"
    fi
  fi
])
