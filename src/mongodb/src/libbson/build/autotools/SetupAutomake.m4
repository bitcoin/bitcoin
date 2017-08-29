m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_PROG_CC_C_O

# OS Conditionals.
AM_CONDITIONAL([OS_WIN32],[test "$os_win32" = "yes"])
AM_CONDITIONAL([OS_UNIX],[test "$os_win32" = "no"])
AM_CONDITIONAL([OS_LINUX],[test "$os_linux" = "yes"])
AM_CONDITIONAL([OS_GNU],[test "$os_gnu" = "yes"])
AM_CONDITIONAL([OS_DARWIN],[test "$os_darwin" = "yes"])
AM_CONDITIONAL([OS_FREEBSD],[test "$os_freebsd" = "yes"])

# Compiler Conditionals.
AM_CONDITIONAL([COMPILER_GCC],[test "$c_compiler" = "gcc" && test "$cxx_compiler" = "g++"])
AM_CONDITIONAL([COMPILER_CLANG],[test "$c_compiler" = "clang" && test "$cxx_compiler" = "clang++"])

# Feature Conditionals
AM_CONDITIONAL([ENABLE_DEBUG],[test "$enable_debug" = "yes"])
AM_CONDITIONAL([ENABLE_STATIC],[test "$enable_static" = "yes"])

# C99 Features
AM_CONDITIONAL([ENABLE_STDBOOL],[test "$enable_stdbool" = "yes"])

# Should we use pthreads
AM_CONDITIONAL([ENABLE_PTHREADS], test "$enable_pthreads" = "yes")

# Should we build the examples.
AM_CONDITIONAL([ENABLE_EXAMPLES],[test "$enable_examples" = "yes"])

# Should we build the tests.
AM_CONDITIONAL([ENABLE_TESTS],[test "$enable_tests" = "yes"])

# Should we build man pages
AM_CONDITIONAL([ENABLE_MAN_PAGES],[test "$enable_man_pages" = "yes"])
AS_IF([test "$enable_man_pages" = "yes" && test -z "$SPHINX_BUILD"],
      [AC_MSG_ERROR([The Sphinx Python package must be installed to generate man pages.])])

# Should we build HTML documentation
AM_CONDITIONAL([ENABLE_HTML_DOCS],[test "$enable_html_docs" = "yes"])
AS_IF([test "$enable_html_docs" = "yes" && test -z "$SPHINX_BUILD"],
      [AC_MSG_ERROR([The Sphinx Python package must be installed to generate HTML documentation.])])
