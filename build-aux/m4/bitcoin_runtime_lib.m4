# On some platforms clang builtin implementations
# require compiler-rt as a runtime library to use.
#
# See:
# - https://bugs.llvm.org/show_bug.cgi?id=28629

m4_define([_CHECK_RUNTIME_testbody], [[
  #if defined(__clang__) && defined(__has_builtin)
  #if __has_builtin(__builtin_mul_overflow)
  bool f(long long x, long long y, long long* p)
  {
    return __builtin_mul_overflow(x, y, p);
  }
  #endif
  #endif

  int main() { return 0; }
]])

AC_DEFUN([CHECK_RUNTIME_LIB], [

  AC_LANG_PUSH(C++)

  AC_MSG_CHECKING([whether __builtin_mul_overflow can be used without compiler-rt])

  AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_RUNTIME_testbody])], [
      AC_MSG_RESULT([yes])
    ], [
      AC_MSG_RESULT([no])
      AX_CHECK_LINK_FLAG([--rtlib=compiler-rt -lgcc_s],
                         [RUNTIME_LDFLAGS="--rtlib=compiler-rt -lgcc_s"],
                         [AC_MSG_FAILURE([cannot figure out how to use __builtin_mul_overflow])],
                         [$LDFLAG_WERROR],
                         [AC_LANG_SOURCE([_CHECK_RUNTIME_testbody])])
    ])

  AC_LANG_POP
  AC_SUBST(RUNTIME_LDFLAGS)
])
