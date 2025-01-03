// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/caches.h>

#include <common/args.h>
#include <index/txindex.h>
#include <kernel/caches.h>
#include <logging.h>

#include <algorithm>
#include <optional>
#include <string>

// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/bitcoin/bitcoin/pull/8273#issuecomment-229601991
//! Max memory allocated to tx index DB specific cache in MiB.
static constexpr int64_t MAX_TX_INDEX_CACHE{1024};
//! Max memory allocated to all block filter index caches combined in MiB.
static constexpr int64_t MAX_FILTER_INDEX_CACHE{1024};

namespace node {
std::optional<CacheSizes> CalculateCacheSizes(const ArgsManager& args, size_t n_indexes)
{
    int64_t db_cache = args.GetIntArg("-dbcache", DEFAULT_DB_CACHE);
    if (static_cast<uint64_t>(db_cache << 20) > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        LogWarning("Cannot allocate more than %d MiB in total for db caches.", static_cast<double>(std::numeric_limits<size_t>::max()) * (1.0 / 1024 / 1024));
        return std::nullopt;
    }
    size_t total_cache = std::max(MiBToBytes(db_cache), MiBToBytes(MIN_DB_CACHE));

    IndexCacheSizes index_sizes;
    index_sizes.tx_index = std::min(total_cache / 8, args.GetBoolArg("-txindex", DEFAULT_TXINDEX) ? MiBToBytes(MAX_TX_INDEX_CACHE) : 0);
    total_cache -= index_sizes.tx_index;
    index_sizes.filter_index = 0;
    if (n_indexes > 0) {
        int64_t max_cache = std::min(total_cache / 8, MiBToBytes(MAX_FILTER_INDEX_CACHE));
        index_sizes.filter_index = max_cache / n_indexes;
        total_cache -= index_sizes.filter_index * n_indexes;
    }
    return {{index_sizes, kernel::CacheSizes{total_cache}}};
}
} // namespace node
