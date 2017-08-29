dnl @synopsis AC_CHECK_TYPEDEF_(TYPEDEF, HEADER [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]])
dnl
dnl check if the given typedef-name is recognized as a type. The trick
dnl is to use a sizeof(TYPEDEF) and see if the compiler is happy with
dnl that.
dnl
dnl this can be thought of as a mixture of
dnl AC_CHECK_TYPE(TYPEDEF,DEFAULT) and
dnl AC_CHECK_LIB(LIBRARY,FUNCTION,ACTION-IF-FOUND,ACTION-IF-NOT-FOUND)
dnl
dnl a convenience macro AC_CHECK_TYPEDEF_ is provided that will not
dnl emit any message to the user - it just executes one of the actions.
dnl
dnl @category C
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2006-10-13
dnl @license GPLWithACException

AC_DEFUN([AC_CHECK_TYPEDEF_],
[dnl
ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(ac_cv_lib_$ac_lib_var,
[ eval "ac_cv_type_$ac_lib_var='not-found'"
  ac_cv_check_typedef_header=`echo ifelse([$2], , stddef.h, $2)`
  AC_TRY_COMPILE( [#include <$ac_cv_check_typedef_header>],
	[int x = sizeof($1); x = x;],
        eval "ac_cv_type_$ac_lib_var=yes" ,
        eval "ac_cv_type_$ac_lib_var=no" )
  if test `eval echo '$ac_cv_type_'$ac_lib_var` = "no" ; then
     ifelse([$4], , :, $4)
  else
     ifelse([$3], , :, $3)
  fi
])])

dnl AC_CHECK_TYPEDEF(TYPEDEF, HEADER [, ACTION-IF-FOUND,
dnl    [, ACTION-IF-NOT-FOUND ]])
AC_DEFUN([AC_CHECK_TYPEDEF],
[dnl
 AC_MSG_CHECKING([for $1 in $2])
 AC_CHECK_TYPEDEF_($1,$2,AC_MSG_RESULT(yes),AC_MSG_RESULT(no))dnl
])
