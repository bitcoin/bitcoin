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
static constexpr size_t MIN_DBCACHE_BYTES{4_MiB};
//! -dbcache default (bytes)
static constexpr size_t DEFAULT_DBCACHE_BYTES{450_MiB};
//! Reserved non-dbcache memory usage.
static constexpr uint64_t RESERVED_RAM{2_GiB};
//! Maximum dbcache size on current architecture.
static constexpr uint64_t MAX_DBCACHE_BYTES{SIZE_MAX == UINT64_MAX ? std::numeric_limits<uint64_t>::max() : 1_GiB};

namespace node {
size_t GetDefaultDBCache();

bool ShouldWarnOversizedDbCache(uint64_t dbcache, uint64_t total_ram) noexcept;
} // namespace node

#endif // BITCOIN_NODE_DBCACHE_H
