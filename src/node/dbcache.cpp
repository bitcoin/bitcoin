// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/dbcache.h>
#include <cstddef>
#include <common/system_ram.h>
#include <algorithm>

namespace node {
size_t GetDefaultDBCache() noexcept
{
    if constexpr (sizeof(void*) >= 8) {
        if (TryGetTotalRam().value_or(0) >= HIGH_DEFAULT_DBCACHE_MIN_TOTAL_RAM) {
            return HIGH_DEFAULT_DBCACHE;
        }
    }
    return 450_MiB;
}

bool ShouldWarnOversizedDbCache(size_t dbcache, size_t total_ram) noexcept
{
    return (total_ram < 2048_MiB) ? dbcache > GetDefaultDBCache()
                                  : dbcache > (total_ram / 100) * 75;
}
} // namespace node
