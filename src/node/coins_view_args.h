// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_COINS_VIEW_ARGS_H
#define BITCOIN_NODE_COINS_VIEW_ARGS_H

#include <cassert>
#include <kernel/caches.h>

class ArgsManager;
struct CoinsViewOptions;

namespace node
{
static constexpr size_t GetDbBatchSize(const size_t dbcache_bytes)
{
    const auto [min, max]{std::pair{DEFAULT_DB_CACHE_BATCH, 256_MiB}};
    assert(min < max); // std::clamp is undefined if `lo` is greater than `hi`
    return std::clamp(
        (dbcache_bytes / DEFAULT_KERNEL_CACHE) * DEFAULT_DB_CACHE_BATCH,
        min, // Memory usage seems the same for smaller batches while slowdown is significant
        max // The gains are barely measurable for bigger batches
    );
}

void ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options);
} // namespace node

#endif // BITCOIN_NODE_COINS_VIEW_ARGS_H
