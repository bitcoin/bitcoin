// Copyright (c) 2023-present The Bitcoin Knots developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <util/mempressure.h>

#include <logging.h>
#include <util/byte_units.h>

#ifdef HAVE_LINUX_SYSINFO
#include <sys/sysinfo.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

#include <cstddef>
#include <cstdint>

size_t g_low_memory_threshold{64_MiB};

bool SystemNeedsMemoryReleased()
{
    if (g_low_memory_threshold <= 0) {
        // Intentionally bypass other metrics when disabled entirely
        return false;
    }
#ifdef WIN32
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    if (GlobalMemoryStatusEx(&mem_status)) {
        if (mem_status.dwMemoryLoad >= 99 ||
            mem_status.ullAvailPhys < g_low_memory_threshold ||
            mem_status.ullAvailVirtual < g_low_memory_threshold) {
            LogPrintf("%s: YES: %s%% memory load; %s available physical memory; %s available virtual memory\n", __func__, int(mem_status.dwMemoryLoad), size_t(mem_status.ullAvailPhys), size_t(mem_status.ullAvailVirtual));
            return true;
        }
    }
#endif
#ifdef HAVE_LINUX_SYSINFO
    struct sysinfo sys_info;
    if (!sysinfo(&sys_info)) {
        // Explicitly 64-bit in case of 32-bit userspace on 64-bit kernel
        const uint64_t free_ram = uint64_t(sys_info.freeram) * sys_info.mem_unit;
        const uint64_t buffer_ram = uint64_t(sys_info.bufferram) * sys_info.mem_unit;
        if (free_ram + buffer_ram < g_low_memory_threshold) {
            LogPrintf("%s: YES: %s free RAM + %s buffer RAM\n", __func__, free_ram, buffer_ram);
            return true;
        }
    }
#endif
    // NOTE: sysconf(_SC_AVPHYS_PAGES) doesn't account for caches on at least Linux, so not safe to use here
    return false;
}
