AS_IF([test "$enable_automatic_init_and_cleanup" != "no"], [
   AC_SUBST(MONGOC_NO_AUTOMATIC_GLOBALS, 0)
], [
   AC_SUBST(MONGOC_NO_AUTOMATIC_GLOBALS, 1)
])

