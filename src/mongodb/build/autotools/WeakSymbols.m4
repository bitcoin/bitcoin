AC_MSG_CHECKING(if weak symbols are supported)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
__attribute__((weak)) void __dummy(void *x) { }
void f(void *x) { __dummy(x); }
]], [[ ]]
)],
[AC_MSG_RESULT(yes)
AC_SUBST(MONGOC_HAVE_WEAK_SYMBOLS, 1)],
[AC_MSG_RESULT(no)])
