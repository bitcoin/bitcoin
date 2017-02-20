# Version 2.96 of gcc (shipped with RedHat Linux 7.[01] and Mandrake) had
# serious problems.
AC_DEFUN(AC_GCC_CONFIG1, [
AC_CACHE_CHECK([whether we are using gcc version 2.96],
db_cv_gcc_2_96, [
db_cv_gcc_2_96=no
if test "$GCC" = "yes"; then
	GCC_VERSION=`${MAKEFILE_CC} --version`
	case ${GCC_VERSION} in
	2.96*)
		db_cv_gcc_2_96=yes;;
	esac
fi])
if test "$db_cv_gcc_2_96" = "yes"; then
	CFLAGS=`echo "$CFLAGS" | sed 's/-O2/-O/'`
	CXXFLAGS=`echo "$CXXFLAGS" | sed 's/-O2/-O/'`
	AC_MSG_WARN([INSTALLED GCC COMPILER HAS SERIOUS BUGS; PLEASE UPGRADE.])
	AC_MSG_WARN([GCC OPTIMIZATION LEVEL SET TO -O.])
fi])
