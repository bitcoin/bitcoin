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

#include <cstddef>
#include <memory>
#include <utility>

namespace node {
bool BlockFetcher::ShouldEnqueue(const CBlockIndex* index) { return index && (index->nStatus & BLOCK_HAVE_DATA); }

std::shared_ptr<const CBlock> BlockFetcher::PopFollowup()
{
    if (m_followups.empty()) return nullptr;
    auto followup{std::move(m_followups[0])};
    m_followups.pop_front();
    return followup.get();
}

bool BlockFetcher::Enqueue(const CBlockIndex& index)
{
    auto followup{m_pool.Submit([&read_block = m_read_block, hash = index.GetBlockHash(), pos = index.GetBlockPos()]() -> std::shared_ptr<const CBlock> {
        auto block{std::make_shared<CBlock>()};
        if (!read_block(*block, pos, hash)) return nullptr;
        return block;
    })};
    if (followup) m_followups.emplace_back(std::move(*followup));
    return !!followup;
}

std::shared_ptr<const CBlock> BlockFetcher::Load(const uint256& hash)
{
    if (auto block{PopFollowup()}; block && block->GetHash() == hash) return block;
    return nullptr;
}

void BlockFetcher::FillQueue(const CBlockIndex& last_index, int next_height)
{
    AssertLockHeld(::cs_main);
    for (size_t i{m_followups.size()}; i < m_queue_size; ++i) {
        const auto* next{last_index.GetAncestor(next_height + i)};
        if (!ShouldEnqueue(next) || !Enqueue(*next)) break;
    }
}
} // namespace node
