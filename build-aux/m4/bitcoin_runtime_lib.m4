# On some platforms clang builtin implementations
# require compiler-rt as a runtime library to use.
#
# See:
# - https://bugs.llvm.org/show_bug.cgi?id=28629

m4_define([_CHECK_RUNTIME_testbody], [[
  bool f(long long x, long long y, long long* p)
  {
    return __builtin_mul_overflow(x, y, p);
  }
  int main() { return 0; }
]])

AC_DEFUN([CHECK_RUNTIME_LIB], [

  AC_LANG_PUSH([C++])

  AC_MSG_CHECKING([for __builtin_mul_overflow])
  AC_LINK_IFELSE(
    [AC_LANG_SOURCE([_CHECK_RUNTIME_testbody])],
    [
      AC_MSG_RESULT([yes])
      AC_DEFINE([HAVE_BUILTIN_MUL_OVERFLOW], [1], [Define if you have a working __builtin_mul_overflow])
    ],
    [
      ax_check_save_flags="$LDFLAGS"
      LDFLAGS="$LDFLAGS --rtlib=compiler-rt -lgcc_s"
      AC_LINK_IFELSE(
        [AC_LANG_SOURCE([_CHECK_RUNTIME_testbody])],
        [
          AC_MSG_RESULT([yes, with additional linker flags])
          RUNTIME_LDFLAGS="--rtlib=compiler-rt -lgcc_s"
          AC_DEFINE([HAVE_BUILTIN_MUL_OVERFLOW], [1], [Define if you have a working __builtin_mul_overflow])
        ],
        [AC_MSG_RESULT([no])])
      LDFLAGS="$ax_check_save_flags"
    ])

  AC_LANG_POP
  AC_SUBST([RUNTIME_LDFLAGS])
])
