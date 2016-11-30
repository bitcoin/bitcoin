# Copyright (c) 2025-present The Bitcoin Knots developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(CheckCXXSourceCompiles)

check_cxx_source_compiles([[
  #include <sys/resource.h>

  int main()
  {
    int x = getiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD);
    setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD, x);
    return x;
  }
  ]] HAVE_IOPOLICY
)

check_cxx_source_compiles([[
  #define _GNU_SOURCE
  #include <unistd.h>
  #include <sys/syscall.h>

  int main()
  {
    int x = syscall(SYS_ioprio_get, 1, 0);
    syscall(SYS_ioprio_set, 1, 0, x);
    return x;
  }
  ]] HAVE_IOPRIO_SYSCALL
)
