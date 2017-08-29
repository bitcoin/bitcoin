enable_bigendian=no
AC_C_BIGENDIAN
AC_SUBST(BSON_BYTE_ORDER, 1234)
if test "x$ac_cv_c_bigendian" = "xyes"; then
    AC_SUBST(BSON_BYTE_ORDER, 4321)
    enable_bigendian=yes
fi
