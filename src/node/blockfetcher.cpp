// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <node/blockfetcher.h>

#include <chain.h>
#include <flatfile.h>
#include <kernel/chainstatemanager_opts.h>
#include <kernel/cs_main.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>
#include <util/expected.h>
#include <util/threadpool.h>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace node {
BlockFetcher::BlockFetcher(ReadBlockFn read_block, int32_t thread_count)
    : m_read_block{std::move(read_block)}, m_thread_count{std::clamp(thread_count, 0, MAX_BLOCK_READ_AHEAD_THREADS)}
{
    if (m_thread_count > 0) m_pool.Start(m_thread_count);
}

void BlockFetcher::Clear()
{
    AssertLockHeld(::cs_main);
    m_followups.clear();
}

std::shared_ptr<const CBlock> BlockFetcher::Load(const uint256& hash)
{
    AssertLockHeld(::cs_main);
    if (m_followups.size()) {
        auto followup{std::move(m_followups.front())};
        m_followups.pop_front();
        if (auto block{followup.get()}; block && block->GetHash() == hash) return block;
    }
    Clear();
    return nullptr;
}

void BlockFetcher::FillQueue(const CBlockIndex* last_index, int next_height)
{
    AssertLockHeld(::cs_main);
    if (!last_index || next_height > last_index->nHeight) {
        Clear();
        return;
    }
    std::vector<ReadTask> tasks;
    for (size_t i{m_followups.size()}; i < m_thread_count * BLOCKS_PER_READ_AHEAD_THREAD; ++i) {
        const auto* next{last_index->GetAncestor(next_height + i)};
        if (!next || !(next->nStatus & BLOCK_HAVE_DATA)) break;
        tasks.emplace_back([this, hash = next->GetBlockHash(), pos = next->GetBlockPos()] {
            try {
                if (auto block{std::make_shared<CBlock>()}; m_read_block(*block, pos, hash)) return block;
            } catch (std::exception&) {} // Retry synchronously when needed
            return std::shared_ptr<CBlock>{};
        });
    }
    if (auto followups{m_pool.Submit(std::move(tasks))}) std::ranges::move(*followups, std::back_inserter(m_followups));
}
} // namespace node
