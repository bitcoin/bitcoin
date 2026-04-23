// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/dbcache.h>

#include <common/system_ram.h>
#include <util/byte_units.h>

#include <algorithm>
#include <cstdint>
#include <optional>

//! Larger default dbcache on 64-bit systems with enough RAM.
static constexpr uint64_t HIGH_DEFAULT_DBCACHE{1_GiB};
//! Minimum detected RAM required for HIGH_DEFAULT_DBCACHE.
static constexpr uint64_t HIGH_DEFAULT_DBCACHE_MIN_TOTAL_RAM{4_GiB};

namespace node {
uint64_t GetTotalRam() noexcept { return TryGetTotalRam().value_or(0); }

uint64_t GetDefaultDBCache(std::optional<uint64_t> total_ram)
{
    if (!total_ram) total_ram.emplace(GetTotalRam());
    if constexpr (sizeof(void*) >= 8) {
        if (*total_ram >= HIGH_DEFAULT_DBCACHE_MIN_TOTAL_RAM) {
            return HIGH_DEFAULT_DBCACHE;
        }
    }
    return DEFAULT_DB_CACHE;
}

bool ShouldWarnOversizedDbCache(uint64_t dbcache, uint64_t total_ram) noexcept
{
    const uint64_t headroom{total_ram > RESERVED_RAM ? total_ram - RESERVED_RAM : 0};
    return dbcache > std::max(GetDefaultDBCache(total_ram), (headroom / 4) * 3);
}
} // namespace node
