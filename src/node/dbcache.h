// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_DBCACHE_H
#define BITCOIN_NODE_DBCACHE_H

#include <util/byte_units.h>

#include <cstddef>
#include <cstdint>

//! min. -dbcache (bytes)
static constexpr size_t MIN_DB_CACHE{4_MiB};
//! -dbcache default (bytes)
static constexpr size_t DEFAULT_DB_CACHE{450_MiB};
//! Reserved non-dbcache memory usage.
static constexpr uint64_t RESERVED_RAM{2_GiB};

namespace node {
size_t GetDefaultDBCache();

bool ShouldWarnOversizedDbCache(uint64_t dbcache, uint64_t total_ram) noexcept;
} // namespace node

#endif // BITCOIN_NODE_DBCACHE_H
