m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_PROG_CC_C_O

# OS Conditionals.
AM_CONDITIONAL([OS_WIN32],[test "$os_win32" = "yes"])
AM_CONDITIONAL([OS_UNIX],[test "$os_win32" = "no"])
AM_CONDITIONAL([OS_LINUX],[test "$os_linux" = "yes"])
AM_CONDITIONAL([OS_GNU],[test "$os_gnu" = "yes"])
AM_CONDITIONAL([OS_DARWIN],[test "$os_darwin" = "yes"])
AM_CONDITIONAL([OS_FREEBSD],[test "$os_freebsd" = "yes"])
AM_CONDITIONAL([OS_SOLARIS],[test "$os_solaris" = "yes"])

# Compiler Conditionals.
AM_CONDITIONAL([COMPILER_GCC],[test "$c_compiler" = "gcc" && test "$cxx_compiler" = "g++"])
AM_CONDITIONAL([COMPILER_CLANG],[test "$c_compiler" = "clang" && test "$cxx_compiler" = "clang++"])

# Feature Conditionals
AM_CONDITIONAL([ENABLE_DEBUG],[test "$enable_debug" = "yes"])
AM_CONDITIONAL([ENABLE_STATIC],[test "$enable_static" = "yes"])

# C99 Features
AM_CONDITIONAL([ENABLE_STDBOOL],[test "$enable_stdbool" = "yes"])

# Should we use pthreads
AM_CONDITIONAL([ENABLE_PTHREADS],[test "$enable_pthreads" = "yes"])

# Should we compile the bundled libbson
AM_CONDITIONAL([WITH_LIBBSON],[test "$with_libbson" = "bundled"])

# Should we compile the bundled snappy
AM_CONDITIONAL([WITH_SNAPPY],[test "$with_snappy" = "bundled"])

# Should we compile the bundled zlib
AM_CONDITIONAL([WITH_ZLIB],[test "$with_zlib" = "bundled"])

# Should we avoid extra BSON_LIBS when linking (SunStudio)
AM_CONDITIONAL([EXPLICIT_LIBS],[test "$with_gnu_ld" = "yes"])

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
