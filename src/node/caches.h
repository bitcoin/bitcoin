// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CACHES_H
#define BITCOIN_NODE_CACHES_H

#include <kernel/caches.h>
#include <util/byte_units.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

class ArgsManager;

//! min. -dbcache (bytes)
static constexpr uint64_t MIN_DB_CACHE{4_MiB};
//! -dbcache default (bytes)
static constexpr uint64_t DEFAULT_DB_CACHE{DEFAULT_KERNEL_CACHE};
//! Reserved non-dbcache memory usage.
static constexpr uint64_t DBCACHE_WARNING_RESERVED_RAM{2_GiB};

namespace node {
uint64_t GetDefaultDBCache();
struct IndexCacheSizes {
    uint64_t tx_index{0};
    uint64_t filter_index{0};
    uint64_t txospender_index{0};
};
struct CacheSizes {
    IndexCacheSizes index;
    kernel::CacheSizes kernel;
};
CacheSizes CalculateCacheSizes(const ArgsManager& args, size_t n_indexes = 0);
constexpr bool ShouldWarnOversizedDbCache(uint64_t dbcache, uint64_t total_ram) noexcept
{
    const uint64_t available_ram{total_ram > DBCACHE_WARNING_RESERVED_RAM ? total_ram - DBCACHE_WARNING_RESERVED_RAM : 0};
    const uint64_t cap{std::max<uint64_t>(DEFAULT_DB_CACHE, (available_ram / 4) * 3)};
    return dbcache > cap;
}

void LogOversizedDbCache(const ArgsManager& args) noexcept;
} // namespace node

#endif // BITCOIN_NODE_CACHES_H
