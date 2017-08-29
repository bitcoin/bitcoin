dnl as-compiler-flag.m4 0.1.0

dnl autostars m4 macro for detection of compiler flags

dnl David Schleef <ds@schleef.org>

dnl $Id: as-compiler-flag.m4,v 1.1 2005/12/15 23:35:19 ds Exp $

dnl AS_COMPILER_FLAG(CFLAGS, ACTION-IF-ACCEPTED, [ACTION-IF-NOT-ACCEPTED])
dnl Tries to compile with the given CFLAGS.
dnl Runs ACTION-IF-ACCEPTED if the compiler can compile with the flags,
dnl and ACTION-IF-NOT-ACCEPTED otherwise.

AC_DEFUN([AS_COMPILER_FLAG],
[
  AC_MSG_CHECKING([to see if compiler understands $1])

  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"

  AC_TRY_COMPILE([ ], [], [flag_ok=yes], [flag_ok=no])
  CFLAGS="$save_CFLAGS"

  if test "X$flag_ok" = Xyes ; then
    m4_ifvaln([$2],[$2])
    true
  else
    m4_ifvaln([$3],[$3])
    true
  fi
  AC_MSG_RESULT([$flag_ok])
])

dnl AS_COMPILER_FLAGS(VAR, FLAGS)
dnl Tries to compile with the given CFLAGS.

AC_DEFUN([AS_COMPILER_FLAGS],
[
  list=$2
  flags_supported=""
  flags_unsupported=""
  AC_MSG_CHECKING([for supported compiler flags])
  for each in $list
  do
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $each"
    AC_TRY_COMPILE([ ], [], [flag_ok=yes], [flag_ok=no])
    CFLAGS="$save_CFLAGS"

    if test "X$flag_ok" = Xyes ; then
      flags_supported="$flags_supported $each"
    else
      flags_unsupported="$flags_unsupported $each"
    fi
  done
  AC_MSG_RESULT([$flags_supported])
  if test "X$flags_unsupported" != X ; then
    AC_MSG_WARN([unsupported compiler flags: $flags_unsupported])
  fi
  $1="$$1 $flags_supported"
])

