# If --with-libbson=auto, determine if there is a system installed libbson
# greater than our required version.
AS_IF([test "x${with_libbson}" = xauto],
      [PKG_CHECK_MODULES(BSON, [libbson-1.0 >= libbson_required_version],
                         [with_libbson=system],
                         [with_libbson=bundled])])

# If we are to use the system, check for libbson enforcing success.
AS_IF([test "x${with_libbson}" = xsystem],
      [PKG_CHECK_MODULES(BSON,
                         [libbson-1.0 >= libbson_required_version],
                         [],
                         [AC_MSG_ERROR([

  --------------------------------------
   The libbson-1.0 library could not be
   found on your system. Please install
   libbson-1.0  development  package or
   set --with-libbson=auto.
  --------------------------------------
])])])

# If we are using the bundled libbson, recurse into its configure.
AS_IF([test "x${with_libbson}" = xbundled],[
   AC_CONFIG_SUBDIRS([src/libbson])
   AC_SUBST(BSON_CFLAGS, "-I${srcdir}/src/libbson/src/bson -Isrc/libbson/src/bson")
   AC_SUBST(BSON_LIBS, "src/libbson/libbson-1.0.la")
])
