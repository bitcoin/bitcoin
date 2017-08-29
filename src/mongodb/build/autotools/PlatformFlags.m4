dnl Ignore OpenSSL deprecation warnings on OSX
AS_IF([test "$os_darwin" = "yes"],
      [AX_CHECK_COMPILE_FLAG([-Wno-deprecated-declarations], [CFLAGS="$CFLAGS -Wno-deprecated-declarations"])])
dnl We know there are some cast-align issues on OSX
AS_IF([test "$os_darwin" = "yes"],
      [AX_CHECK_COMPILE_FLAG([-Wno-cast-align], [CFLAGS="$CFLAGS -Wno-cast-align"])])
AS_IF([test "$os_darwin" = "yes"],
      [AX_CHECK_COMPILE_FLAG([-Wno-unneeded-internal-declaration], [CFLAGS="$CFLAGS -Wno-unneeded-internal-declaration"])])
AS_IF([test "$os_darwin" = "yes"],
      [AX_CHECK_COMPILE_FLAG([-Wno-error=unused-command-line-argument], [CFLAGS="$CFLAGS -Wno-error=unused-command-line-argument"])])
dnl We know there are some cast-align issues on Solaris
AS_IF([test "$os_solaris" = "yes"],
      [AX_CHECK_COMPILE_FLAG([-Wno-cast-align], [CFLAGS="$CFLAGS -Wno-cast-align"])])


