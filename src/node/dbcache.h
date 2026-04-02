// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_DBCACHE_H
#define BITCOIN_NODE_DBCACHE_H

#include <util/byte_units.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

//! min. -dbcache (bytes)
static constexpr size_t MIN_DB_CACHE{4_MiB};
//! Automatic -dbcache floor (bytes)
static constexpr size_t MIN_DEFAULT_DBCACHE{100_MiB};
//! Automatic -dbcache cap (bytes)
static constexpr size_t MAX_DEFAULT_DBCACHE{2048_MiB};
//! Assumed total RAM when we cannot determine it.
static constexpr size_t FALLBACK_RAM_BYTES{SIZE_MAX == UINT64_MAX ? 4096_MiB : 2048_MiB};
//! Reserved non-dbcache memory usage.
static constexpr size_t RESERVED_RAM{2048_MiB};
//! Maximum dbcache size on current architecture.
static constexpr size_t MAX_DBCACHE_BYTES{SIZE_MAX == UINT64_MAX ? std::numeric_limits<size_t>::max() : 1024_MiB};

namespace node {
size_t GetTotalRam() noexcept;
size_t GetDefaultDBCache(std::optional<size_t> total_ram = {}) noexcept;

bool ShouldWarnOversizedDbCache(size_t dbcache, size_t total_ram) noexcept;
} // namespace node

#endif // BITCOIN_NODE_DBCACHE_H
