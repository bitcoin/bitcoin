// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/caches.h>

#include <common/args.h>
#include <common/system.h>
#include <index/txindex.h>
#include <kernel/caches.h>
#include <logging.h>
#include <node/interface_ui.h>
#include <tinyformat.h>
#include <util/byte_units.h>

#include <algorithm>
#include <string>

// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/bitcoin/bitcoin/pull/8273#issuecomment-229601991
//! Max memory allocated to tx index DB specific cache in bytes.
static constexpr size_t MAX_TX_INDEX_CACHE{1024_MiB};
//! Max memory allocated to all block filter index caches combined in bytes.
static constexpr size_t MAX_FILTER_INDEX_CACHE{1024_MiB};
//! Maximum dbcache size on 32-bit systems.
static constexpr size_t MAX_32BIT_DBCACHE{1024_MiB};

namespace node {
size_t CalculateDbCacheBytes(const ArgsManager& args)
{
    if (auto db_cache{args.GetIntArg("-dbcache")}) {
        if (*db_cache < 0) db_cache = 0;
        const uint64_t db_cache_bytes{SaturatingLeftShift<uint64_t>(*db_cache, 20)};
        constexpr auto max_db_cache{sizeof(void*) == 4 ? MAX_32BIT_DBCACHE : std::numeric_limits<size_t>::max()};
        return std::max<size_t>(MIN_DB_CACHE, std::min<uint64_t>(db_cache_bytes, max_db_cache));
    }
    return DEFAULT_DB_CACHE;
}

CacheSizes CalculateCacheSizes(const ArgsManager& args, size_t n_indexes)
{
    size_t total_cache{CalculateDbCacheBytes(args)};

    IndexCacheSizes index_sizes;
    index_sizes.tx_index = std::min(total_cache / 8, args.GetBoolArg("-txindex", DEFAULT_TXINDEX) ? MAX_TX_INDEX_CACHE : 0);
    total_cache -= index_sizes.tx_index;
    if (n_indexes > 0) {
        size_t max_cache = std::min(total_cache / 8, MAX_FILTER_INDEX_CACHE);
        index_sizes.filter_index = max_cache / n_indexes;
        total_cache -= index_sizes.filter_index * n_indexes;
    }
    return {index_sizes, kernel::CacheSizes{total_cache}};
}

void LogOversizedDbCache(const ArgsManager& args) noexcept
{
    if (const auto total_ram{GetTotalRAM()}) {
        const size_t db_cache{CalculateDbCacheBytes(args)};
        if (ShouldWarnOversizedDbCache(db_cache, *total_ram)) {
            InitWarning(bilingual_str{tfm::format(_("A %zu MiB dbcache may be too large for a system memory of only %zu MiB."),
                        db_cache >> 20, *total_ram >> 20)});
        }
    }
}
} // namespace node
