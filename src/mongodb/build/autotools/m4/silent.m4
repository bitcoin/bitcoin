dnl Use GNU make's -s when available
dnl
dnl Copyright (c) 2010, Damien Lespiau <damien.lespiau@gmail.com>
dnl
dnl            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
dnl   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
dnl
dnl  0. You just DO WHAT THE FUCK YOU WANT TO.
dnl
dnl The above copyright notice and this permission notice shall be
dnl included in all copies or substantial portions of the Software.
dnl
dnl Simply put this file in your m4 macro directory (as everyone should be
dnl using AC_CONFIG_MACRO_DIR by now!) and add AS_AM_REALLY_SILENT somewhere
dnl in your configure.ac

AC_DEFUN([AS_AM_REALLY_SILENT],
[
  AC_MSG_CHECKING([whether ${MAKE-make} can be made more silent])
  dnl And we even cache that FFS!
  AC_CACHE_VAL(
    [as_cv_prog_make_can_be_really_silent],
    [cat >confstfu.make <<\_WTF
SHELL = /bin/sh
all:
	@echo '@@@%%%clutter rocks@@@%%%'
_WTF
as_cv_prog_make_can_be_really_silent=no
case `${MAKE-make} -f confstfu.make -s --no-print-directory 2>/dev/null` in
  *'@@@%%%clutter rocks@@@%%%'*)
    test $? = 0 && as_cv_prog_make_can_be_really_silent=yes
esac
rm -f confstfu.make])
if test $as_cv_prog_make_can_be_really_silent = yes; then
  AC_MSG_RESULT([yes])
  make_flags='`\
    if  test "x$(AM_DEFAULT_VERBOSITY)" = x0; then \
       test -z "$V" -o "x$V" = x0 && echo -s; \
    else \
       test "x$V" = x0 && echo -s; \
    fi`'
  AC_SUBST([AM_MAKEFLAGS], [$make_flags])
else
  AC_MSG_RESULT([no])
fi
])
