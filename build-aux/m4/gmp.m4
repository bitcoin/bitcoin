dnl Configure paths for GMP
dnl Based on macros by Owen Taylor.
dnl Hans Petter Jansson     2001-04-29
dnl Modified slightly by Allin Cottrell, April 2003
dnl modified by Jerome Benoit <calculus@rezozer.net> 2013/11/26

dnl AM_PATH_GMP([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for GMP, and define GMP_CFLAGS and GMP_LIBS.
dnl
AC_DEFUN([AM_PATH_GMP],
[dnl

		AC_ARG_WITH([gmp],
		[AS_HELP_STRING([--without-gmp],[Disable support for GMP])],
		[],[with_gmp=yes])

	AC_MSG_CHECKING(whether GMP is disabled)
	if test x"$with_gmp" != xno ; then

		AC_MSG_RESULT(no)

		AC_ARG_WITH(gmp-prefix,[  --with-gmp-prefix=PFX   Prefix where GMP is installed (optional)],
            gmp_config_prefix="$withval", gmp_config_prefix="")

  if test x$gmp_config_prefix != x ; then
     gmp_config_args="$gmp_config_args --prefix=$gmp_config_prefix"
  fi

  min_gmp_version=ifelse([$1], ,1.0.0,$1)

  AC_MSG_CHECKING(for GMP - version >= $min_gmp_version)

  GMP_CFLAGS="-I$gmp_config_prefix/include"
  GMP_LIBS="-L$gmp_config_prefix/lib -lgmp"

  ac_save_CFLAGS="$CFLAGS"
  ac_save_LIBS="$LIBS"
  CFLAGS="$CFLAGS $GMP_CFLAGS"
  LIBS="$GMP_LIBS $LIBS"

dnl
dnl Now check if the installed GMP is sufficiently new.
dnl
  rm -f conf.gmptest
  AC_TRY_RUN([
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main ()
{
  int gmp_major_version = 0, gmp_minor_version = 0, gmp_micro_version = 0;
  int major, minor, micro;
  char *tmp_version;
  mpz_t a, b, c;

  mpz_init (a);
  mpz_init (b);
  mpz_init (c);
  mpz_mul (c, a, b);

  system ("touch conf.gmptest");

#ifdef __GNU_MP_VERSION
  gmp_major_version = __GNU_MP_VERSION;
#endif

#ifdef __GNU_MP_VERSION_MINOR
  gmp_minor_version = __GNU_MP_VERSION_MINOR;
#endif

#ifdef __GNU_MP_VERSION_PATCHLEVEL
  gmp_micro_version = __GNU_MP_VERSION_PATCHLEVEL;
#endif

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = strdup("$min_gmp_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gmp_version");
     exit(1);
  }

  if ((gmp_major_version > major) ||
     ((gmp_major_version == major) && (gmp_minor_version > minor)) ||
     ((gmp_major_version == major) && (gmp_minor_version == minor) && (gmp_micro_version >= micro)))
  {
    return 0;
  }
  else
  {
    printf("\n*** An old version of GNU MP (%d.%d.%d) was found.\n",
           gmp_major_version, gmp_minor_version, gmp_micro_version);
    printf("*** You need a version of GNU MP newer than %d.%d.%d. The latest version of\n",
           major, minor, micro);

    printf("*** GNU MP is always available from http://gmplib.org.\n");
    printf("***\n");
  }

  return 1;
}
],, no_gmp=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
  CFLAGS="$ac_save_CFLAGS"
  LIBS="$ac_save_LIBS"

  if test "x$no_gmp" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test -f conf.gmptest ; then
       :
     else
       echo "*** Could not run GNU MP test program, checking why..."
       CFLAGS="$CFLAGS $GMP_CFLAGS"
       LIBS="$LIBS $GMP_LIBS"
       AC_TRY_LINK([
#include <gmp.h>
#include <stdio.h>
],     [ return (1); ],
       [ echo "*** The test program compiled, but did not run. This usually means"
         echo "*** that the run-time linker is not finding GNU MP or finding the wrong"
         echo "*** version of GNU MP. If it is not finding GNU MP, you'll need to set your"
         echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
         echo "*** to the installed location. Also, make sure you have run ldconfig if that"
         echo "*** is required on your system"
         echo "***"
         echo "*** If you have an old version installed, it is best to remove it, although"
         echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
         echo "***" ],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
         echo "*** exact error that occured. This usually means GNU MP was incorrectly installed"
         echo "*** or that you have moved GNU MP since it was installed." ])
         CFLAGS="$ac_save_CFLAGS"
         LIBS="$ac_save_LIBS"
     fi
     GMP_CFLAGS=""
     GMP_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  if test "$GMP_CFLAGS" = "-I/include" ; then
     GMP_CFLAGS=""
  fi
  if test "$GMP_LIBS" = "-L/lib -lgmp" ; then
     GMP_LIBS="-lgmp"
  fi

  rm -f conf.gmptest

	else

		AC_MSG_RESULT(yes)

		AC_DEFINE([WITHOUT_GMP],[1],[GMP is disabled])

		GMP_CFLAGS=""
    GMP_LIBS=""

	fi

	AC_SUBST(GMP_CFLAGS)
	AC_SUBST(GMP_LIBS)

	AM_CONDITIONAL([WITH_GMP_IS_YES],[test x"$with_gmp" != xno])

])