// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <node/blockfetcher.h>

#include <chain.h>
#include <flatfile.h>
#include <kernel/cs_main.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>
#include <util/expected.h>
#include <util/threadpool.h>

#include <memory>
#include <utility>

namespace node {
bool BlockFetcher::ShouldEnqueue(const CBlockIndex* index) { return index && (index->nStatus & BLOCK_HAVE_DATA); }

bool BlockFetcher::Enqueue(const CBlockIndex& index)
{
    if (m_pool.WorkersCount() == 0) m_pool.Start(WORKER_COUNT);
    auto followup{m_pool.Submit([&read_block = m_read_block, hash = index.GetBlockHash(), pos = index.GetBlockPos()]() -> std::shared_ptr<const CBlock> {
        auto block{std::make_shared<CBlock>()};
        if (!read_block(*block, pos, hash)) return nullptr;
        return block;
    })};
    if (followup) m_followup = std::move(*followup);
    return !!followup;
}

std::shared_ptr<const CBlock> BlockFetcher::Load(const uint256& hash)
{
    if (!m_followup.valid()) return nullptr;
    auto block{m_followup.get()};
    return block && block->GetHash() == hash ? block : nullptr;
}

void BlockFetcher::FillQueue(const CBlockIndex& last_index, int next_height)
{
    AssertLockHeld(::cs_main);
    if (m_followup.valid()) return;
    if (auto* next{last_index.GetAncestor(next_height)}; ShouldEnqueue(next)) Enqueue(*next);
}
} // namespace node
