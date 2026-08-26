// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <node/blockfetcher.h>

#include <chain.h>
#include <kernel/cs_main.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>

#include <memory>
#include <utility>

namespace node {
bool BlockFetcher::ShouldEnqueue(const CBlockIndex* index) { return index && (index->nStatus & BLOCK_HAVE_DATA); }

bool BlockFetcher::Enqueue(const CBlockIndex& index)
{
    auto block{std::make_shared<CBlock>()};
    if (!m_read_block(*block, index.GetBlockPos(), index.GetBlockHash())) return false;
    m_followup = std::move(block);
    return true;
}

std::shared_ptr<const CBlock> BlockFetcher::Load(const uint256& hash)
{
    auto block{std::move(m_followup)};
    return block && block->GetHash() == hash ? block : nullptr;
}

void BlockFetcher::FillQueue(const CBlockIndex& last_index, int next_height)
{
    AssertLockHeld(::cs_main);
    if (m_followup) return;
    if (auto* next{last_index.GetAncestor(next_height)}; ShouldEnqueue(next)) Enqueue(*next);
}
} // namespace node
