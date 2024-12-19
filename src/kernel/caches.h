// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CACHES_H
#define BITCOIN_KERNEL_CACHES_H

#include <algorithm>
#include <cstdint>

//! Suggested default amount of cache reserved for the kernel (MiB)
static constexpr int64_t DEFAULT_KERNEL_CACHE{450};
//! Max memory allocated to block tree DB specific cache (MiB)
static constexpr int64_t MAX_BLOCK_DB_CACHE{2};
//! Max memory allocated to coin DB specific cache (MiB)
static constexpr int64_t MAX_COINS_DB_CACHE{8};

namespace kernel {
struct CacheSizes {
    size_t block_tree_db;
    size_t coins_db;
    size_t coins;

    CacheSizes(int64_t total_cache)
    {
        block_tree_db = std::min(total_cache / 8, MAX_BLOCK_DB_CACHE << 20);
        total_cache -= block_tree_db;
        coins_db = std::min(total_cache / 2, MAX_COINS_DB_CACHE << 20);
        total_cache -= coins_db;
        coins = total_cache; // the rest goes to the coins cache
    }
};
} // namespace kernel

#endif // BITCOIN_KERNEL_CACHES_H
