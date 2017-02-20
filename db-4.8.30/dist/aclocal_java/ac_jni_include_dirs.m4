dnl @synopsis AC_JNI_INCLUDE_DIR
dnl
dnl AC_JNI_INCLUDE_DIR finds include directories needed
dnl for compiling programs using the JNI interface.
dnl
dnl JNI include directories are usually in the java distribution
dnl This is deduced from the value of JAVAC.  When this macro
dnl completes, a list of directories is left in the variable
dnl JNI_INCLUDE_DIRS.
dnl
dnl Example usage follows:
dnl
dnl 	AC_JNI_INCLUDE_DIR
dnl
dnl	for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
dnl	do
dnl		CPPFLAGS="$CPPFLAGS -I$JNI_INCLUDE_DIR"
dnl	done
dnl
dnl If you want to force a specific compiler:
dnl
dnl - at the configure.in level, set JAVAC=yourcompiler before calling
dnl AC_JNI_INCLUDE_DIR
dnl
dnl - at the configure level, setenv JAVAC
dnl
dnl Note: This macro can work with the autoconf M4 macros for Java programs.
dnl This particular macro is not part of the original set of macros.
dnl
dnl @author Don Anderson
dnl @version $Id$
dnl
AC_DEFUN(AC_JNI_INCLUDE_DIR,[

JNI_INCLUDE_DIRS=""

test "x$JAVAC" = x && AC_MSG_ERROR(['$JAVAC' undefined])
AC_PATH_PROG(_ACJNI_JAVAC, $JAVAC, $JAVAC)
test ! -x "$_ACJNI_JAVAC" && AC_MSG_ERROR([$JAVAC could not be found in path])
AC_MSG_CHECKING(absolute path of $JAVAC)
case "$_ACJNI_JAVAC" in
/*)	AC_MSG_RESULT($_ACJNI_JAVAC);;
*)	AC_MSG_ERROR([$_ACJNI_JAVAC is not an absolute path name]);;
esac

_ACJNI_FOLLOW_SYMLINKS("$_ACJNI_JAVAC")
_JTOPDIR=`echo "$_ACJNI_FOLLOWED" | sed -e 's://*:/:g' -e 's:/[[^/]]*$::'`
case "$host_os" in
	darwin*)	_JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
			_JINC="$_JTOPDIR/Headers";;
	*)		_JINC="$_JTOPDIR/include";;
esac

# If we find jni.h in /usr/include, then it's not a java-only tree, so
# don't add /usr/include or subdirectories to the list of includes.
# An extra -I/usr/include can foul things up with newer gcc's.
#
# If we don't find jni.h, just keep going.  Hopefully javac knows where
# to find its include files, even if we can't.
if test -r "$_JINC/jni.h"; then
	if test "$_JINC" != "/usr/include"; then
		JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JINC"
	fi
else
	_JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
	if test -r "$_JTOPDIR/include/jni.h"; then
		if test "$_JTOPDIR" != "/usr"; then
			JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include"
		fi
	fi
fi

# get the likely subdirectories for system specific java includes
if test "$_JTOPDIR" != "/usr"; then
	case "$host_os" in
	aix*)		_JNI_INC_SUBDIRS="aix";;
	bsdi*)		_JNI_INC_SUBDIRS="bsdos";;
	cygwin*)	_JNI_INC_SUBDIRS="win32";;
	freebsd*)	_JNI_INC_SUBDIRS="freebsd";;
	hp*)		_JNI_INC_SUBDIRS="hp-ux";;
	linux*)		_JNI_INC_SUBDIRS="linux genunix";;
	osf*)		_JNI_INC_SUBDIRS="alpha";;
	solaris*)	_JNI_INC_SUBDIRS="solaris";;
	*)		_JNI_INC_SUBDIRS="genunix";;
	esac
fi

# add any subdirectories that are present
for _JINCSUBDIR in $_JNI_INC_SUBDIRS
do
	if test -d "$_JTOPDIR/include/$_JINCSUBDIR"; then
		JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include/$_JINCSUBDIR"
	fi
done
])

# _ACJNI_FOLLOW_SYMLINKS <path>
# Follows symbolic links on <path>,
# finally setting variable _ACJNI_FOLLOWED
# --------------------
AC_DEFUN(_ACJNI_FOLLOW_SYMLINKS,[
# find the include directory relative to the javac executable
_cur="$1"
while ls -ld "$_cur" 2>/dev/null | grep " -> " >/dev/null; do
	AC_MSG_CHECKING(symlink for $_cur)
	_slink=`ls -ld "$_cur" | sed 's/.* -> //'`
	case "$_slink" in
	/*) _cur="$_slink";;
	# 'X' avoids triggering unwanted echo options.
	*) _cur=`echo "X$_cur" | sed -e 's/^X//' -e 's:[[^/]]*$::'`"$_slink";;
	esac
	AC_MSG_RESULT($_cur)
done
_ACJNI_FOLLOWED="$_cur"
])# _ACJNI
