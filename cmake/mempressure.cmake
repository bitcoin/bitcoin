# Copyright (c) 2025-present The Bitcoin Knots developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(CheckCXXSourceCompiles)

check_cxx_source_compiles([[
  #include <sys/sysinfo.h>

  int main()
  {
    struct sysinfo info;
    int rv = sysinfo(&info);
    unsigned long test = info.freeram + info.bufferram + info.mem_unit;
    return test + rv;
  }
  ]] HAVE_LINUX_SYSINFO
)
