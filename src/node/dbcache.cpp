// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/dbcache.h>

#include <common/system_ram.h>
#include <util/byte_units.h>

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
    return DEFAULT_DB_CACHE;
}
} // namespace node
