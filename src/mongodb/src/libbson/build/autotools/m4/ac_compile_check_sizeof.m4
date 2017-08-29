AC_DEFUN([AC_COMPILE_CHECK_SIZEOF],
[changequote(&lt;&lt;, &gt;&gt;)dnl
dnl The name to #define.
define(&lt;&lt;AC_TYPE_NAME&gt;&gt;, translit(sizeof_$1, [a-z *], [A-Z_P]))dnl
dnl The cache variable name.
define(&lt;&lt;AC_CV_NAME&gt;&gt;, translit(ac_cv_sizeof_$1, [ *], [_p]))dnl
changequote([, ])dnl
AC_MSG_CHECKING(size of $1)
AC_CACHE_VAL(AC_CV_NAME,
[for ac_size in 4 8 1 2 16 $2 ; do # List sizes in rough order of prevalence.
  AC_TRY_COMPILE([#include "confdefs.h"
#include &lt;sys/types.h&gt;
$2
], [switch (0) case 0: case (sizeof ($1) == $ac_size):;], AC_CV_NAME=$ac_size)
  if test x$AC_CV_NAME != x ; then break; fi
done
])
if test x$AC_CV_NAME = x ; then
  AC_MSG_ERROR([cannot determine a size for $1])
fi
AC_MSG_RESULT($AC_CV_NAME)
AC_DEFINE_UNQUOTED(AC_TYPE_NAME, $AC_CV_NAME, [The number of bytes in type $1])
undefine([AC_TYPE_NAME])dnl
undefine([AC_CV_NAME])dnl
])
