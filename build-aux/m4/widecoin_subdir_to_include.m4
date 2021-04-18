dnl Copyright (c) 2013-2014 The Widecoin Core developers
dnl Distributed under the MIT software license, see the accompanying
dnl file COPYING or http://www.opensource.org/licenses/mit-license.php.

dnl WIDECOIN_SUBDIR_TO_INCLUDE([CPPFLAGS-VARIABLE-NAME],[SUBDIRECTORY-NAME],[HEADER-FILE])
dnl SUBDIRECTORY-NAME must end with a path separator
AC_DEFUN([WIDECOIN_SUBDIR_TO_INCLUDE],[
  if test "x$2" = "x"; then
    AC_MSG_RESULT([default])
  else
    echo "#include <$2$3.h>" >conftest.cpp
    newinclpath=`${CXXCPP} ${CPPFLAGS} -M conftest.cpp 2>/dev/null | [ tr -d '\\n\\r\\\\' | sed -e 's/^.*[[:space:]:]\(\/[^[:space:]]*\)]$3[\.h[[:space:]].*$/\1/' -e t -e d`]
    AC_MSG_RESULT([${newinclpath}])
    if test "x${newinclpath}" != "x"; then
      eval "$1=\"\$$1\"' -I${newinclpath}'"
    fi
  fi
])
