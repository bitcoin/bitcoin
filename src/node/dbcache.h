// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_DBCACHE_H
#define BITCOIN_NODE_DBCACHE_H

#include <util/byte_units.h>

#include <cstddef>
#include <cstdint>
#include <limits>

//! min. -dbcache (bytes)
static constexpr size_t MIN_DB_CACHE{4_MiB};
//! Maximum dbcache size on current architecture.
static constexpr size_t MAX_DBCACHE_BYTES{SIZE_MAX == UINT64_MAX ? std::numeric_limits<size_t>::max() : 1024_MiB};
//! Larger default dbcache on 64-bit systems with enough RAM.
static constexpr size_t HIGH_DEFAULT_DBCACHE{1024_MiB};
//! Minimum detected RAM required for HIGH_DEFAULT_DBCACHE.
static constexpr uint64_t HIGH_DEFAULT_DBCACHE_MIN_TOTAL_RAM{4096ULL << 20};

namespace node {
size_t GetDefaultDBCache() noexcept;

bool ShouldWarnOversizedDbCache(size_t dbcache, size_t total_ram) noexcept;
} // namespace node

#endif // BITCOIN_NODE_DBCACHE_H
