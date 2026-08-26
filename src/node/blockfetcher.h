// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCKFETCHER_H
#define BITCOIN_NODE_BLOCKFETCHER_H

#include <kernel/cs_main.h>
#include <sync.h>
#include <util/threadpool.h>

#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>

class CBlock;
class CBlockIndex;
struct FlatFilePos;
class uint256;

namespace node {
/** Supplies blocks to validation. Destruction waits for any queued reads. */
class BlockFetcher
{
    using ReadBlockFn = std::function<bool(CBlock&, const FlatFilePos&, const uint256&)>;

    static constexpr uint32_t WORKER_COUNT{1};

    const ReadBlockFn m_read_block;
    ThreadPool m_pool{"blockread"};
    std::future<std::shared_ptr<const CBlock>> m_followup GUARDED_BY(::cs_main);

    static bool ShouldEnqueue(const CBlockIndex* index) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool Enqueue(const CBlockIndex& index) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

public:
    explicit BlockFetcher(ReadBlockFn read_block) : m_read_block{std::move(read_block)} {}

    std::shared_ptr<const CBlock> Load(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void FillQueue(const CBlockIndex& last_index, int next_height) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};
} // namespace node

#endif // BITCOIN_NODE_BLOCKFETCHER_H
