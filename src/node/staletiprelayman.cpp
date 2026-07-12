// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/staletiprelayman.h>

#include <chain.h>
#include <util/check.h>

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace node {

StaleTipMessage MakeStaleTipMessage(const StaleFork& fork, bool have_block)
{
    AssertLockHeld(::cs_main);
    const CBlockIndex& tip{*fork.tip};
    Assert(HasAncestor(tip, *fork.fork_point));

    const int fork_length{tip.nHeight - fork.fork_point->nHeight};
    Assert(fork_length > 0);
    Assert(static_cast<size_t>(fork_length) <= MAX_STALETIP_HEADERS);

    StaleTipMessage msg{
        .fork_point = fork.fork_point->GetBlockHash(),
        .headers = std::vector<StaleTipCompressedHeader>(fork_length),
        .have_block = have_block,
    };
    const CBlockIndex* pindex{&tip};
    for (int i{fork_length - 1}; i >= 0; --i) {
        msg.headers.at(i) = {
            .version = pindex->nVersion,
            .merkle_root = pindex->hashMerkleRoot,
            .time = pindex->nTime,
            .bits = pindex->nBits,
            .nonce = pindex->nNonce,
        };
        pindex = pindex->pprev;
    }
    return msg;
}

util::Expected<void, StaleTipAnnouncementError> ValidateStaleTipAnnouncement(const CBlockIndex& fork_point, size_t num_headers, const arith_uint256& claimed_work,
                                                                             const CBlockIndex* active_tip, const arith_uint256& work_threshold,
                                                                             int recent_window)
{
    if (active_tip != nullptr && fork_point.nHeight + static_cast<int>(num_headers) < active_tip->nHeight - recent_window) {
        return util::Unexpected{StaleTipAnnouncementError::OUTSIDE_RECENT_WINDOW};
    }
    if (fork_point.nChainWork + claimed_work < work_threshold) return util::Unexpected{StaleTipAnnouncementError::LOW_WORK};
    return {};
}

StaleTipRelayManager::StaleTipRelayManager(const StaleTipRelayOptions& opts)
    : m_opts{opts}, m_tracker{opts.chain_type}
{
}

void StaleTipRelayManager::Initialize(const BlockManager& blockman, const CChain& chain)
{
    AssertLockHeld(::cs_main);
    m_tracker.SeedFromBlockIndex(blockman, chain);
}

bool StaleTipRelayManager::BlockDisconnected(const CChain& chain, const CBlockIndex& block)
{
    AssertLockHeld(::cs_main);
    return m_tracker.AddStaleTip(chain, block);
}

bool StaleTipRelayManager::AcceptedStaleTip(const CChain& chain, const CBlockIndex& tip)
{
    AssertLockHeld(::cs_main);
    return m_tracker.AddStaleTip(chain, tip);
}

void StaleTipRelayManager::ConnectedPeer(NodeId peer)
{
    AssertLockHeld(::cs_main);
    const bool inserted{m_peers.try_emplace(peer).second};
    Assume(inserted);
}

void StaleTipRelayManager::DisconnectedPeer(NodeId peer)
{
    AssertLockHeld(::cs_main);
    m_peers.erase(peer);
}

bool StaleTipRelayManager::ReceivedStaleTip(NodeId peer, const CChain& chain, const CBlockIndex& tip, bool have_block)
{
    AssertLockHeld(::cs_main);
    const auto it{m_peers.find(peer)};
    if (!Assume(it != m_peers.end())) return false;
    auto& from_peer{it->second.m_staletips_from_peer};
    if (auto announced{std::ranges::find(from_peer, &tip, &StaleTipAnnouncement::tip)}; announced != from_peer.end()) {
        announced->have_block |= have_block;
    } else {
        if (from_peer.size() >= MAX_STALETIPS_FROM_PEER) from_peer.pop_front();
        from_peer.push_back({.tip = &tip, .have_block = have_block});
    }
    return m_tracker.AddStaleTip(chain, tip);
}

bool StaleTipRelayManager::IsAnnouncedStaleBranchBlock(NodeId peer, const CChain& chain, const CBlockIndex& block) const
{
    AssertLockHeld(::cs_main);
    if (chain.Contains(block)) return false;
    const auto it{m_peers.find(peer)};
    if (it == m_peers.end()) return false;
    return std::ranges::any_of(it->second.m_staletips_from_peer, [&](const StaleTipAnnouncement& announced) {
        return announced.have_block && HasAncestor(*announced.tip, block);
    });
}

std::vector<StaleTipMessage> StaleTipRelayManager::GetStaleTipsToAnnounce(
    NodeId peer, const CChain& chain,
    const CanServeBlockFn& can_serve_block,
    const PeerHasHeaderFn& peer_has_header)
{
    AssertLockHeld(::cs_main);
    const auto it{m_peers.find(peer)};
    if (!Assume(it != m_peers.end())) return {};
    PeerInfo& info{it->second};

    std::vector<StaleTipMessage> announcements;
    for (const StaleFork& fork : m_tracker.GetStaleTips(chain)) {
        const CBlockIndex& stale_tip{*fork.tip};
        // Don't announce a tip twice.
        if (info.m_staletips_to_peer.contains(&stale_tip)) continue;
        // Don't announce a tip back to the peer that announced it to us.
        if (std::ranges::find(info.m_staletips_from_peer, &stale_tip, &StaleTipAnnouncement::tip) != info.m_staletips_from_peer.end()) continue;
        // The compressed payload reconstructs headers from the fork point, so
        // only announce tips whose fork point the peer is known to have.
        if (!peer_has_header(*fork.fork_point)) continue;
        // Don't announce a tip to a peer that already has it (for example, as
        // its active tip).
        if (peer_has_header(stale_tip)) continue;

        // BIP 332: have_block promises the stale tip's block. The peer then
        // fetches the branch oldest first. A request for a block we lack is
        // ignored, but one that our serving limits refuse disconnects the
        // peer, so every block on the branch must be within them. Block
        // times need not increase, so each block is checked. A tracked
        // branch may be served whatever its age (see
        // CanServeStaleBranchBlock()).
        const bool have_tip_data{(stale_tip.nStatus & BLOCK_HAVE_DATA) != 0};
        bool within_serve_limits{true};
        for (const CBlockIndex* block{&stale_tip}; block != fork.fork_point; block = block->pprev) {
            if (!can_serve_block(*block)) {
                within_serve_limits = false;
                break;
            }
        }
        const bool have_block{have_tip_data && within_serve_limits};
        // Signet stale tips can only be validated with the block data, and
        // peers may disconnect for announcements without it.
        if (!have_block && m_opts.chain_type == ChainType::SIGNET) continue;
        info.m_staletips_to_peer.insert(&stale_tip);
        announcements.push_back(MakeStaleTipMessage(fork, have_block));
    }
    // Forget tips only once they are dropped from the tracker. Tips that are
    // merely ineligible at the moment (for example, reorged onto the active
    // chain) are kept, so they are not re-announced to the same peer if they
    // become eligible again.
    std::erase_if(info.m_staletips_to_peer, [&](const CBlockIndex* tip) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) { return !m_tracker.IsTracked(*tip); });
    return announcements;
}

bool StaleTipRelayManager::CanServeStaleBranchBlock(const CChain& chain, const CBlockIndex& block) const
{
    AssertLockHeld(::cs_main);
    return m_tracker.CanServeStaleBranchBlock(chain, block);
}

std::vector<StaleFork> StaleTipRelayManager::GetStaleTips(const CChain& chain) const
{
    AssertLockHeld(::cs_main);
    return m_tracker.GetStaleTips(chain);
}

bool StaleTipRelayManager::PrefersBlocks() const
{
    return m_opts.chain_type == ChainType::SIGNET;
}

void StaleTipRelayManager::CheckIsEmpty() const
{
    AssertLockHeld(::cs_main);
    assert(m_peers.empty());
}

} // namespace node
