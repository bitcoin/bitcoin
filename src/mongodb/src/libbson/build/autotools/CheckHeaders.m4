AC_HEADER_STDBOOL
AC_SUBST(BSON_HAVE_STDBOOL_H, 0)
if test "$ac_cv_header_stdbool_h" = "yes"; then
	AC_SUBST(BSON_HAVE_STDBOOL_H, 1)
fi

AC_CREATE_STDINT_H([src/bson/bson-stdint.h])

AC_CHECK_HEADERS_ONCE([strings.h])
