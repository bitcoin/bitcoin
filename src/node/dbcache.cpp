// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system_ram.h>
#include <node/dbcache.h>

#include <algorithm>
#include <cstdint>
#include <optional>

namespace node {
uint64_t GetTotalRam() noexcept { return TryGetTotalRam().value_or(FALLBACK_RAM_BYTES); }

uint64_t GetDefaultDBCache(std::optional<uint64_t> total_ram) noexcept
{
    if (!total_ram) total_ram.emplace(GetTotalRam());
    const uint64_t usable{*total_ram > RESERVED_RAM ? *total_ram - RESERVED_RAM : 0};
    return std::max(MIN_DEFAULT_DBCACHE, std::min(usable / 4, std::min(MAX_DEFAULT_DBCACHE, MAX_DBCACHE_BYTES)));
}

bool ShouldWarnOversizedDbCache(uint64_t dbcache, uint64_t total_ram) noexcept
{
    const uint64_t headroom{total_ram > RESERVED_RAM ? total_ram - RESERVED_RAM : 0};
    return dbcache > std::max(GetDefaultDBCache(total_ram), (headroom / 4) * 3);
}
} // namespace node
