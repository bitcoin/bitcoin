// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <node/blockfetcher.h>

#include <chain.h>
#include <kernel/cs_main.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>

#include <exception>
#include <memory>
#include <utility>

namespace node {
BlockFetcher::BlockFetcher(ReadBlockFn read_block) : m_read_block{std::move(read_block)} {}

void BlockFetcher::Clear()
{
    AssertLockHeld(::cs_main);
    m_followup.reset();
}

std::shared_ptr<const CBlock> BlockFetcher::Load(const uint256& hash)
{
    AssertLockHeld(::cs_main);
    auto block{std::move(m_followup)};
    return block && block->GetHash() == hash ? block : nullptr;
}

void BlockFetcher::FillQueue(const CBlockIndex* last_index, int next_height)
{
    AssertLockHeld(::cs_main);
    if (!last_index || next_height > last_index->nHeight) {
        Clear();
        return;
    }
    if (m_followup) return;
    const auto* next{last_index->GetAncestor(next_height)};
    if (!next || !(next->nStatus & BLOCK_HAVE_DATA)) return;
    try {
        auto block{std::make_shared<CBlock>()};
        if (m_read_block(*block, next->GetBlockPos(), next->GetBlockHash())) m_followup = std::move(block);
    } catch (std::exception&) {} // Retry synchronously when needed
}
} // namespace node
