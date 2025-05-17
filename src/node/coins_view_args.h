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
    return std::clamp(
        (dbcache_bytes / DEFAULT_KERNEL_CACHE) * DEFAULT_DB_CACHE_BATCH,
        /*lo=*/DEFAULT_DB_CACHE_BATCH,
        /*hi=*/256_MiB
    );
}

void ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options);
} // namespace node

#endif // BITCOIN_NODE_COINS_VIEW_ARGS_H
