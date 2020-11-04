dnl Copyright (c) 2015 Tim Kosse <tim.kosse@filezilla-project.org>
dnl Copying and distribution of this file, with or without modification, are
dnl permitted in any medium without royalty provided the copyright notice
dnl and this notice are preserved. This file is offered as-is, without any
dnl warranty.

# Some versions of gcc/libstdc++ require linking with -latomic if
# using the C++ atomic library.
#
# Sourced from http://bugs.debian.org/797228

m4_define([_CHECK_ATOMIC_testbody], [[
  #include <atomic>
  #include <cstdint>

  template <typename T>
  void test_atomic()
  {
      std::atomic<T> a;
      a.store(T{});
      T i = a.load();
  }

  struct uint128_type
  {
      std::uint64_t left;
      std::uint64_t right;
  };

  int main()
  {
      std::atomic<uint128_type> f1({0,0});
      uint128_type f2 = {0,0};
      uint128_type f3 = {1,1};
      std::atomic_flag af = ATOMIC_FLAG_INIT;
      if (af.test_and_set())
          af.clear();

      f1.compare_exchange_strong(f2, f3);

      test_atomic<int>();
      test_atomic<std::uint8_t>();
      test_atomic<std::uint16_t>();
      test_atomic<std::uint32_t>();
      test_atomic<std::uint64_t>();
      test_atomic<uint128_type>();

      std::memory_order mo;
      mo = std::memory_order_relaxed;
      mo = std::memory_order_acquire;
      mo = std::memory_order_release;
      mo = std::memory_order_acq_rel;
      mo = std::memory_order_seq_cst;
  }
]])

AC_DEFUN([CHECK_ATOMIC], [

  AC_LANG_PUSH(C++)

  AC_MSG_CHECKING([whether std::atomic can be used without link library])

  AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_ATOMIC_testbody])],[
      AC_MSG_RESULT([yes])
    ],[
      AC_MSG_RESULT([no])
      LIBS="$LIBS -latomic"
      AC_MSG_CHECKING([whether std::atomic needs -latomic])
      AC_LINK_IFELSE([AC_LANG_SOURCE([_CHECK_ATOMIC_testbody])],[
          AC_MSG_RESULT([yes])
        ],[
          AC_MSG_RESULT([no])
          AC_MSG_FAILURE([cannot figure out how to use std::atomic])
        ])
    ])

  AC_LANG_POP
])
