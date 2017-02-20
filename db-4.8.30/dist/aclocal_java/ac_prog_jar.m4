dnl @synopsis AC_PROG_JAR
dnl
dnl AC_PROG_JAR tests for an existing jar program. It uses the environment
dnl variable JAR then tests in sequence various common jar programs.
dnl
dnl If you want to force a specific compiler:
dnl
dnl - at the configure.in level, set JAR=yourcompiler before calling
dnl AC_PROG_JAR
dnl
dnl - at the configure level, setenv JAR
dnl
dnl You can use the JAR variable in your Makefile.in, with @JAR@.
dnl
dnl Note: This macro depends on the autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download that whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl
dnl The general documentation of those macros, as well as the sample
dnl configure.in, is included in the AC_PROG_JAVA macro.
dnl
dnl @author Egon Willighagen <egonw@sci.kun.nl>
dnl @version $Id$
dnl
AC_DEFUN([AC_PROG_JAR],[
AC_REQUIRE([AC_EXEEXT])dnl
if test "x$JAVAPREFIX" = x; then
        test "x$JAR" = x && AC_CHECK_PROGS(JAR, jar$EXEEXT)
else
        test "x$JAR" = x && AC_CHECK_PROGS(JAR, jar, $JAVAPREFIX)
fi
test "x$JAR" = x && AC_MSG_ERROR([no acceptable jar program found in \$PATH])
AC_PROVIDE([$0])dnl
])
