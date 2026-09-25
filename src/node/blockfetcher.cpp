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
#include <exception>
#include <memory>
#include <utility>

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
    if (m_followups.size()) return;
    const auto* next{last_index->GetAncestor(next_height)};
    if (!next || !(next->nStatus & BLOCK_HAVE_DATA)) return;
    if (auto followup{m_pool.Submit([this, hash = next->GetBlockHash(), pos = next->GetBlockPos()]() -> std::shared_ptr<const CBlock> {
        try {
            if (auto block{std::make_shared<CBlock>()}; m_read_block(*block, pos, hash)) return block;
        } catch (std::exception&) {} // Retry synchronously when needed
        return nullptr;
    })}) m_followups.emplace_back(std::move(*followup));
}
} // namespace node
