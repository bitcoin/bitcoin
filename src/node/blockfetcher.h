// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCKFETCHER_H
#define BITCOIN_NODE_BLOCKFETCHER_H

#include <kernel/cs_main.h>
#include <sync.h>
#include <util/threadpool.h>

#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <string>

class CBlock;
class CBlockIndex;
struct FlatFilePos;
class uint256;

namespace node {
/** Reads and deserializes blocks in parallel while scanning a chain */
class BlockFetcher
{
    using ReadBlockFn = std::function<bool(CBlock&, const FlatFilePos&, const uint256&)>;

    const ReadBlockFn m_read_block;
    const int32_t m_thread_count;
    ThreadPool m_pool{"blockread"};
    std::deque<std::future<std::shared_ptr<const CBlock>>> m_followups GUARDED_BY(::cs_main);

public:
    BlockFetcher(ReadBlockFn read_block, int32_t thread_count);

    //! Discard retained results without cancelling submitted reads
    void Clear() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    std::shared_ptr<const CBlock> Load(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void FillQueue(const CBlockIndex* last_index, int next_height) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};
} // namespace node

#endif // BITCOIN_NODE_BLOCKFETCHER_H
