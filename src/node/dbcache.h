// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_DBCACHE_H
#define BITCOIN_NODE_DBCACHE_H

#include <util/byte_units.h>

#include <cstdint>
#include <limits>
#include <optional>

//! min. -dbcache (bytes)
static constexpr uint64_t MIN_DBCACHE_BYTES{4_MiB};
//! Automatic -dbcache floor (bytes)
static constexpr uint64_t MIN_DEFAULT_DBCACHE{100_MiB};
//! Automatic -dbcache cap (bytes)
static constexpr uint64_t MAX_DEFAULT_DBCACHE{2_GiB};
//! Assumed total RAM when we cannot determine it.
static constexpr uint64_t FALLBACK_RAM_BYTES{SIZE_MAX == UINT64_MAX ? 4_GiB : 2_GiB};
//! Reserved non-dbcache memory usage.
static constexpr uint64_t RESERVED_RAM{2_GiB};
//! Maximum dbcache size on current architecture.
static constexpr uint64_t MAX_DBCACHE_BYTES{SIZE_MAX == UINT64_MAX ? std::numeric_limits<uint64_t>::max() : 1_GiB};

namespace node {
uint64_t GetTotalRam() noexcept;
uint64_t GetDefaultDBCache(std::optional<uint64_t> total_ram = {}) noexcept;

bool ShouldWarnOversizedDbCache(uint64_t dbcache, uint64_t total_ram) noexcept;
} // namespace node

#endif // BITCOIN_NODE_DBCACHE_H
