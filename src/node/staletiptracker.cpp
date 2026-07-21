// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/staletiptracker.h>

#include <chain.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace node {

namespace {

/** Whether `ancestor` is an ancestor of `block`, or `block` itself. */
bool HasAncestor(const CBlockIndex& block, const CBlockIndex& ancestor)
{
    return block.nHeight >= ancestor.nHeight && block.GetAncestor(ancestor.nHeight) == &ancestor;
}

} // namespace

const CBlockIndex* StaleTipTracker::GetEligibleForkPoint(const CChain& active_chain, const CBlockIndex& stale_tip) const
{
    const CBlockIndex* active_tip{active_chain.Tip()};
    if (active_tip == nullptr) return nullptr;
    if (active_chain.Contains(stale_tip)) return nullptr;
    if (IsExpired(active_chain, stale_tip)) return nullptr;
    if (stale_tip.nChainWork > active_tip->nChainWork) return nullptr;

    // Walk back at most m_max_headers blocks, rather than to the fork point
    // however far back it is.
    const CBlockIndex* fork_point{&stale_tip};
    for (size_t steps{0}; !active_chain.Contains(*fork_point); ++steps) {
        if (steps == m_max_headers || fork_point->pprev == nullptr) return nullptr;
        fork_point = fork_point->pprev;
    }
    return fork_point;
}

bool StaleTipTracker::IsExpired(const CChain& active_chain, const CBlockIndex& stale_tip) const
{
    if (stale_tip.nStatus & BLOCK_FAILED_VALID) return true;
    const CBlockIndex* active_tip{active_chain.Tip()};
    return active_tip != nullptr && stale_tip.nHeight < active_tip->nHeight - m_recent_window;
}

void StaleTipTracker::PruneExpired(const CChain& active_chain)
{
    AssertLockHeld(::cs_main);
    std::erase_if(m_tips, [&](const CBlockIndex* tip) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return IsExpired(active_chain, *tip);
    });
}

bool StaleTipTracker::Add(const CBlockIndex& stale_tip)
{
    AssertLockHeld(::cs_main);

    // Nothing to do if the tip, or one of its descendants, is tracked.
    for (const CBlockIndex* tip : m_tips) {
        if (HasAncestor(*tip, stale_tip)) return false;
    }

    // The tip replaces any tracked ancestors.
    std::erase_if(m_tips, [&](const CBlockIndex* tip) { return HasAncestor(stale_tip, *tip); });

    if (m_tips.size() == MAX_ADVERTISED_STALETIPS) {
        // Evict the tip with the least work, if it has less than the new
        // tip. min_element returns the first, so the oldest, of equals.
        const auto evict{std::ranges::min_element(m_tips, {}, [](const CBlockIndex* tip) { return tip->nChainWork; })};
        if ((*evict)->nChainWork >= stale_tip.nChainWork) return false;
        m_tips.erase(evict);
    }

    m_tips.push_back(&stale_tip);
    return true;
}

bool StaleTipTracker::AddStaleTip(const CChain& active_chain, const CBlockIndex& stale_tip)
{
    AssertLockHeld(::cs_main);
    if (GetEligibleForkPoint(active_chain, stale_tip) == nullptr) return false;

    PruneExpired(active_chain);
    return Add(stale_tip);
}

std::vector<StaleFork> StaleTipTracker::GetStaleTips(const CChain& active_chain) const
{
    AssertLockHeld(::cs_main);

    std::vector<StaleFork> tips;
    tips.reserve(m_tips.size());
    for (const CBlockIndex* tip : m_tips) {
        const CBlockIndex* fork_point{GetEligibleForkPoint(active_chain, *tip)};
        if (fork_point == nullptr) continue;
        tips.push_back({.fork_point = fork_point, .tip = tip});
    }

    return tips;
}

} // namespace node
