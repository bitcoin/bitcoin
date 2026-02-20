// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_DBCACHE_H
#define BITCOIN_NODE_DBCACHE_H

#include <kernel/caches.h>
#include <util/byte_units.h>

#include <cstddef>

//! min. -dbcache (bytes)
static constexpr size_t MIN_DB_CACHE{4_MiB};
//! -dbcache default (bytes)
static constexpr size_t DEFAULT_DB_CACHE{DEFAULT_KERNEL_CACHE};

namespace node {
size_t GetDefaultDBCache();

constexpr bool ShouldWarnOversizedDbCache(uint64_t dbcache, uint64_t total_ram) noexcept
{
    const uint64_t cap{(total_ram < 2_GiB) ? DEFAULT_DB_CACHE : (total_ram / 100) * 75};
    return dbcache > cap;
}
} // namespace node

#endif // BITCOIN_NODE_DBCACHE_H
