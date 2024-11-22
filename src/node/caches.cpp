// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/caches.h>

#include <common/args.h>
#include <index/txindex.h>
#include <kernel/caches.h>
#include <txdb.h>

#include <algorithm>
#include <string>

// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/bitcoin/bitcoin/pull/8273#issuecomment-229601991
//! Max memory allocated to tx index DB specific cache in MiB.
static constexpr int64_t MAX_TX_INDEX_CACHE{1024};
//! Max memory allocated to all block filter index caches combined in MiB.
static constexpr int64_t MAX_FILTER_INDEX_CACHE{1024};

namespace node {
CacheSizes CalculateCacheSizes(const ArgsManager& args, size_t n_indexes)
{
    int64_t nTotalCache = (args.GetIntArg("-dbcache", nDefaultDbCache) << 20);
    nTotalCache = std::max(nTotalCache, nMinDbCache << 20); // total cache cannot be less than nMinDbCache
    IndexCacheSizes sizes;
    sizes.tx_index = std::min(nTotalCache / 8, args.GetBoolArg("-txindex", DEFAULT_TXINDEX) ? MAX_TX_INDEX_CACHE << 20 : 0);
    nTotalCache -= sizes.tx_index;
    if (n_indexes > 0) {
        int64_t max_cache = std::min(nTotalCache / 8, MAX_FILTER_INDEX_CACHE << 20);
        sizes.filter_index = max_cache / n_indexes;
        nTotalCache -= sizes.filter_index * n_indexes;
    }
    return {sizes, kernel::CacheSizes{static_cast<size_t>(nTotalCache)}};
}
} // namespace node
