// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system_ram.h>
#include <node/dbcache.h>

#include <algorithm>
#include <cstddef>
#include <optional>

namespace node {
size_t GetTotalRam() noexcept { return TryGetTotalRam().value_or(FALLBACK_RAM_BYTES); }

size_t GetDefaultDBCache(std::optional<size_t> total_ram) noexcept
{
    if (!total_ram) total_ram.emplace(GetTotalRam());
    const size_t usable{*total_ram > RESERVED_RAM ? *total_ram - RESERVED_RAM : 0};
    return std::clamp(usable / 4, MIN_DEFAULT_DBCACHE, std::min(MAX_DEFAULT_DBCACHE, MAX_DBCACHE_BYTES));
}

bool ShouldWarnOversizedDbCache(size_t dbcache, size_t total_ram) noexcept
{
    return (total_ram < FALLBACK_RAM_BYTES) ? dbcache > GetDefaultDBCache(total_ram)
                                            : dbcache > (total_ram / 100) * 75;
}
} // namespace node
