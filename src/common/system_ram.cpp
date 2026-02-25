// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system_ram.h>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <cstdint>
#include <limits>

std::optional<size_t> GetTotalRAM()
{
    [[maybe_unused]] auto clamp{[](uint64_t v) { return size_t(std::min(v, uint64_t{std::numeric_limits<size_t>::max()})); }};
#ifdef WIN32
    if (MEMORYSTATUSEX m{}; (m.dwLength = sizeof(m), GlobalMemoryStatusEx(&m))) return clamp(m.ullTotalPhys);
#elif defined(__APPLE__) || \
      defined(__FreeBSD__) || \
      defined(__NetBSD__) || \
      defined(__OpenBSD__) || \
      defined(__illumos__) || \
      defined(__linux__)
    if (long p{sysconf(_SC_PHYS_PAGES)}, s{sysconf(_SC_PAGESIZE)}; p > 0 && s > 0) return clamp(1ULL * p * s);
#endif
    return std::nullopt;
}
