# If --with-zlib=auto, determine if there is a system installed zlib
AS_IF([test "x${with_zlib}" = xauto], [
      PKG_CHECK_MODULES(ZLIB, [zlib],
         [with_zlib=system],
         [
            # If we didn't find zlib with pkgconfig, use bundled
            # unless we find it manually
            with_zlib=bundled
            AC_CHECK_LIB([zlib],[compress2],
               [AC_CHECK_HEADER([zlib-c.h],
                  [
                     with_zlib=system
                     ZLIB_LIBS=-lz
                  ],
                  []
               )],
               []
            )
         ]
      )
   ]
)

AS_IF([test "x${ZLIB_LIBS}" = "x" -a "x$with_zlib" = "xsystem"],
      [AC_MSG_ERROR([Cannot find system installed zlib. try --with-zlib=bundled])])

# If we are using the bundled zlib, recurse into its configure.
AS_IF([test "x${with_zlib}" = xbundled],[
   AC_MSG_CHECKING(whether to enable bundled zlib)
   AC_MSG_RESULT(yes)
   ZLIB_LIBS=
   ZLIB_CFLAGS="-Isrc/zlib-1.2.11"
])

if test "x$with_zlib" != "xno"; then
   AC_SUBST(MONGOC_ENABLE_COMPRESSION_ZLIB, 1)
else
   AC_SUBST(MONGOC_ENABLE_COMPRESSION_ZLIB, 0)
fi
AC_SUBST(ZLIB_LIBS)
AC_SUBST(ZLIB_CFLAGS)

