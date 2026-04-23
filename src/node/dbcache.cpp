// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/dbcache.h>

#include <common/system_ram.h>
#include <util/byte_units.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

//! Larger default dbcache on 64-bit systems with enough RAM.
static constexpr size_t HIGH_DEFAULT_DBCACHE{1_GiB};
//! Minimum detected RAM required for HIGH_DEFAULT_DBCACHE.
static constexpr uint64_t HIGH_DEFAULT_DBCACHE_MIN_TOTAL_RAM{4_GiB};

namespace node {
size_t GetDefaultDBCache()
{
    if constexpr (sizeof(void*) >= 8) {
        if (TryGetTotalRam().value_or(0) >= HIGH_DEFAULT_DBCACHE_MIN_TOTAL_RAM) {
            return HIGH_DEFAULT_DBCACHE;
        }
    }
    return DEFAULT_DBCACHE_BYTES;
}

bool ShouldWarnOversizedDbCache(uint64_t dbcache, uint64_t total_ram) noexcept
{
    const uint64_t headroom{total_ram > RESERVED_RAM ? total_ram - RESERVED_RAM : 0};
    return dbcache > std::max<uint64_t>(DEFAULT_DBCACHE_BYTES, (headroom / 4) * 3);
}
} // namespace node
