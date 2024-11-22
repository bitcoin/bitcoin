// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CACHES_H
#define BITCOIN_KERNEL_CACHES_H

#include <txdb.h>

#include <algorithm>
#include <cstdint>

namespace kernel {
struct CacheSizes {
    int64_t block_tree_db;
    int64_t coins_db;
    int64_t coins;

    CacheSizes(int64_t total_cache)
    {
        block_tree_db = std::min(total_cache / 8, nMaxBlockDBCache << 20);
        total_cache -= block_tree_db;
        // use 25%-50% of the remainder for disk cache, capped by the maximum.
        coins_db = std::min({total_cache / 2, (total_cache / 4) + (1 << 23), nMaxCoinsDBCache << 20});
        total_cache -= coins_db;
        coins = total_cache; // the rest goes to the coins cache
    }
};
} // namespace kernel

#endif // BITCOIN_KERNEL_CACHES_H
