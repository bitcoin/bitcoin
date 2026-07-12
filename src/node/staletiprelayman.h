// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_STALETIPRELAYMAN_H
#define BITCOIN_NODE_STALETIPRELAYMAN_H

#include <arith_uint256.h>
#include <kernel/cs_main.h>
#include <net.h>
#include <node/staletiptracker.h>
#include <staletipmessage.h>
#include <sync.h>
#include <util/chaintype.h>
#include <util/expected.h>

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <vector>

class CBlockIndex;
class CChain;

namespace node {
class BlockManager;

/** Maximum number of stale tips announced by a peer that are remembered, so
 *  they are not announced back to it. Leaves room for a peer's
 *  MAX_ADVERTISED_STALETIPS previously known tips plus new ones. */
inline constexpr size_t MAX_STALETIPS_FROM_PEER{2 * MAX_ADVERTISED_STALETIPS};

/** Why ValidateStaleTipAnnouncement() rejected an announcement. */
enum class StaleTipAnnouncementError {
    //! The announced tip is more than the recency window below the active
    //! tip.
    OUTSIDE_RECENT_WINDOW,
    //! The announced branch has less work than the anti-DoS threshold.
    LOW_WORK,
};

/** Check an announced stale branch of `num_headers` headers, forking off at
 *  `fork_point` and claiming `claimed_work`, against the active tip, the
 *  recency window and the anti-DoS `work_threshold`. A null `active_tip`
 *  skips the recency check. */
util::Expected<void, StaleTipAnnouncementError> ValidateStaleTipAnnouncement(const CBlockIndex& fork_point, size_t num_headers, const arith_uint256& claimed_work,
                                                                             const CBlockIndex* active_tip, const arith_uint256& work_threshold,
                                                                             int recent_window = STALETIP_RECENT_WINDOW);

struct StaleTipRelayOptions {
    ChainType chain_type{ChainType::MAIN};
};

/** Build the `staletip` message announcing `fork`, compressing the headers
 *  between `fork.fork_point` (exclusive) and `fork.tip` (inclusive). `fork`
 *  must be one the tracker returned, so it fits in one message. */
StaleTipMessage MakeStaleTipMessage(const StaleFork& fork, bool have_block) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

/**
 * Class responsible for deciding which stale tips to track and announce for
 * BIP 332 stale-tip relay. Its data structures include the StaleTipTracker
 * and, per peer, the stale tips the peer announced to us and the ones we
 * announced to it.
 *
 * It decides but does not act: PeerManager parses and validates `staletip`
 * messages, sends them, and serves blocks. What it knows about a peer is
 * passed in as callbacks that are only used during the call.
 *
 * Caller needs to interact with StaleTipRelayManager:
 * - ValidationInterface callbacks.
 * - When a peer connects or disconnects.
 * - When a peer announces a stale tip.
 * - To get the stale tips to announce.
 *
 * This class is not thread-safe. Callers must hold cs_main.
 */
class StaleTipRelayManager
{
public:
    using PeerHasHeaderFn = std::function<bool(const CBlockIndex&)>;
    using CanServeBlockFn = std::function<bool(const CBlockIndex&)>;

    explicit StaleTipRelayManager(const StaleTipRelayOptions& opts);

    /** Seed the tracker with the eligible stale tips in the block index. */
    void Initialize(const BlockManager& blockman, const CChain& chain) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    // Responses to chain events. StaleTipRelayManager is not an actual client
    // of ValidationInterface, these are called through PeerManager.
    /** @return Whether a new stale tip is tracked, so it should be relayed. */
    bool BlockDisconnected(const CChain& chain, const CBlockIndex& block) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    /** @return Whether a new stale tip is tracked, so it should be relayed. */
    bool AcceptedStaleTip(const CChain& chain, const CBlockIndex& tip) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void ConnectedPeer(NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void DisconnectedPeer(NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Remember a stale tip `peer` announced, and track it if it is eligible.
     *  The peer's offer of the branch's blocks is kept once made.
     *
     * @return Whether a new stale tip is tracked, so it should be relayed.
     */
    bool ReceivedStaleTip(NodeId peer, const CChain& chain, const CBlockIndex& tip, bool have_block) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Whether `block` is on a stale branch `peer` announced with have_block
     *  set. The branch is the part of the announced tip's ancestry that is not
     *  on `chain`. A block known to be invalid is still included, as it may
     *  have turned out invalid after the peer announced it. */
    bool IsAnnouncedStaleBranchBlock(NodeId peer, const CChain& chain, const CBlockIndex& block) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Build the `staletip` messages to send to `peer` now, for tracked stale
     *  tips in the order they were tracked, and remember the tips as
     *  announced.
     *
     * A tip is not announced twice, nor back to a peer that announced it to
     * us, nor to a peer that lacks its fork point or already has the tip.
     *
     * @param[in] can_serve_block Whether our serving limits let the peer fetch
     *                            a block, if we have it.
     * @param[in] peer_has_header Whether the peer is known to have a header.
     */
    std::vector<StaleTipMessage> GetStaleTipsToAnnounce(
        NodeId peer, const CChain& chain,
        const CanServeBlockFn& can_serve_block,
        const PeerHasHeaderFn& peer_has_header) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Whether `block` is stored on a tracked, still-eligible stale branch and
     *  may be served to a peer that negotiated stale-tip relay. */
    bool CanServeStaleBranchBlock(const CChain& chain, const CBlockIndex& block) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Get the tracked tips that are eligible for relay, with their fork
     *  points, in the order they were added. */
    std::vector<StaleFork> GetStaleTips(const CChain& chain) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    /** Whether to ask peers to announce stale tips once they can serve the
     *  blocks. Always on signet, where a tip is only tracked once its block
     *  is validated. */
    bool PrefersBlocks() const;

    /** Check that no per-peer state remains. */
    void CheckIsEmpty() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

private:
    /** A stale tip a peer announced to us. */
    struct StaleTipAnnouncement {
        const CBlockIndex* tip;
        //! Whether the peer offered to serve the blocks on the tip's branch. It
        //! may not have connected them, so their scripts may be unchecked.
        bool have_block;
    };

    struct PeerInfo {
        /** Stale tips this peer announced to us, oldest first. Kept
         *  independently of our tracker, so tips we don't track (yet) are
         *  remembered too. */
        std::deque<StaleTipAnnouncement> m_staletips_from_peer;
        /** Tracked stale tips we announced to this peer. */
        std::set<const CBlockIndex*> m_staletips_to_peer;
    };

    const StaleTipRelayOptions m_opts;
    StaleTipTracker m_tracker GUARDED_BY(::cs_main);
    std::map<NodeId, PeerInfo> m_peers GUARDED_BY(::cs_main);
};

} // namespace node

#endif // BITCOIN_NODE_STALETIPRELAYMAN_H
