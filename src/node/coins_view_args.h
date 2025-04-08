// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_COINS_VIEW_ARGS_H
#define BITCOIN_NODE_COINS_VIEW_ARGS_H

#include <kernel/caches.h>

class ArgsManager;
struct CoinsViewOptions;

namespace node
{
    static constexpr size_t GetDbBatchSize(const size_t dbcache_bytes)
    {
        const auto target{(dbcache_bytes / DEFAULT_KERNEL_CACHE) * DEFAULT_DB_CACHE_BATCH};
        return std::max<size_t>(MIN_DB_CACHE_BATCH, std::min<size_t>(MAX_DB_CACHE_BATCH, target));
    }

    void ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options);
} // namespace node

#endif // BITCOIN_NODE_COINS_VIEW_ARGS_H
