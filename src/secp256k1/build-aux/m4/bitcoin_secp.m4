dnl escape "$0x" below using the m4 quadrigaph @S|@, and escape it again with a \ for the shell.
AC_DEFUN([SECP_X86_64_ASM_CHECK],[
AC_MSG_CHECKING(for x86_64 assembly availability)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
  #include <stdint.h>]],[[
  uint64_t a = 11, tmp;
  __asm__ __volatile__("movq \@S|@0x100000000,%1; mulq %%rsi" : "+a"(a) : "S"(tmp) : "cc", "%rdx");
  ]])], [has_x86_64_asm=yes], [has_x86_64_asm=no])
AC_MSG_RESULT([$has_x86_64_asm])
])

AC_DEFUN([SECP_ARM32_ASM_CHECK], [
  AC_MSG_CHECKING(for ARM32 assembly availability)
  SECP_ARM32_ASM_CHECK_CFLAGS_saved_CFLAGS="$CFLAGS"
  CFLAGS="-x assembler"
  AC_LINK_IFELSE([AC_LANG_SOURCE([[
    .syntax unified
    .eabi_attribute 24, 1
    .eabi_attribute 25, 1
    .text
    .global main
    main:
      ldr r0, =0x002A
      mov r7, #1
      swi 0   
    ]])], [has_arm32_asm=yes], [has_arm32_asm=no])
  AC_MSG_RESULT([$has_arm32_asm])
  CFLAGS="$SECP_ARM32_ASM_CHECK_CFLAGS_saved_CFLAGS"
])

AC_DEFUN([SECP_VALGRIND_CHECK],[
AC_MSG_CHECKING([for valgrind support])
if test x"$has_valgrind" != x"yes"; then
  CPPFLAGS_TEMP="$CPPFLAGS"
  CPPFLAGS="$VALGRIND_CPPFLAGS $CPPFLAGS"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <valgrind/memcheck.h>
  ]], [[
    #if defined(NVALGRIND)
    #  error "Valgrind does not support this platform."
    #endif
  ]])], [has_valgrind=yes])
  CPPFLAGS="$CPPFLAGS_TEMP"
fi
AC_MSG_RESULT($has_valgrind)
])

dnl SECP_TRY_APPEND_CFLAGS(flags, VAR)
dnl Append flags to VAR if CC accepts them.
AC_DEFUN([SECP_TRY_APPEND_CFLAGS], [
  AC_MSG_CHECKING([if ${CC} supports $1])
  SECP_TRY_APPEND_CFLAGS_saved_CFLAGS="$CFLAGS"
  CFLAGS="$1 $CFLAGS"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])], [flag_works=yes], [flag_works=no])
  AC_MSG_RESULT($flag_works)
  CFLAGS="$SECP_TRY_APPEND_CFLAGS_saved_CFLAGS"
  if test x"$flag_works" = x"yes"; then
    $2="$$2 $1"
  fi
  unset flag_works
  AC_SUBST($2)
])

dnl SECP_SET_DEFAULT(VAR, default, default-dev-mode)
dnl Set VAR to default or default-dev-mode, depending on whether dev mode is enabled
AC_DEFUN([SECP_SET_DEFAULT], [
  if test "${enable_dev_mode+set}" != set; then
    AC_MSG_ERROR([[Set enable_dev_mode before calling SECP_SET_DEFAULT]])
  fi
  if test x"$enable_dev_mode" = x"yes"; then
    $1="$3"
  else
    $1="$2"
  fi
])
