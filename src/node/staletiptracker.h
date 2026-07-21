// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_STALETIPTRACKER_H
#define BITCOIN_NODE_STALETIPTRACKER_H

#include <chain.h>
#include <kernel/cs_main.h>

#include <cstddef>
#include <vector>

/** Maximum number of blocks a tracked stale branch may have after its fork
 *  point, unless StaleTipTracker is constructed with another limit. From
 *  BIP 332. */
inline constexpr size_t MAX_STALETIP_HEADERS{20};

namespace node {

/** Default number of blocks below the active tip within which a stale tip is
 *  still considered recent enough to be tracked. From BIP 332. */
inline constexpr int STALETIP_RECENT_WINDOW{1000};

/** Maximum number of stale tips StaleTipTracker retains. From BIP 332, which
 *  limits how many previously known stale tips are advertised to a peer. */
inline constexpr size_t MAX_ADVERTISED_STALETIPS{10};

/** A stale branch of the block tree, described by its tip and the point where
 *  it forks off the active chain. */
struct StaleFork {
    //! Last common ancestor of the stale tip and the active chain. Never null.
    const CBlockIndex* fork_point;
    //! Tip of the stale branch. Never null.
    const CBlockIndex* tip;
};

/** Tracker of recently seen stale tips: tips of branches of the block tree
 *  that are not part of the active chain and not known to be invalid.
 *
 * At most MAX_ADVERTISED_STALETIPS tips are tracked for later relay to
 * peers. A tracked tip is only relayed while it is eligible:
 * - its branch forks off the active chain by at most `m_max_headers` blocks,
 * - its height is within `m_recent_window` blocks of the active tip,
 * - it has no more work than the active tip, and
 * - it is not known to be invalid.
 *
 * A tip that is ineligible only for now, for example because a reorg changed
 * its fork point, stays tracked. A tip that has expired (see IsExpired()) is
 * dropped the next time an eligible tip is added.
 */
class StaleTipTracker
{
private:
    //! Tracked stale tips in the order they were added, oldest first.
    //! Never holds more than MAX_ADVERTISED_STALETIPS entries.
    std::vector<const CBlockIndex*> m_tips;
    //! Tips more than this many blocks below the active tip are not tracked.
    const int m_recent_window{STALETIP_RECENT_WINDOW};
    //! Maximum stale branch length to track.
    const size_t m_max_headers{MAX_STALETIP_HEADERS};

    /** Find the fork point of `stale_tip` with the active chain, checking that
     *  the tip is eligible.
     *
     * @return The fork point, or nullptr if the tip is not eligible.
     */
    const CBlockIndex* GetEligibleForkPoint(const CChain& active_chain, const CBlockIndex& stale_tip) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    /** Whether `stale_tip` is known to be invalid or too far below the active
     *  tip. Such a tip is unlikely to become eligible again, so it is not
     *  worth keeping. */
    bool IsExpired(const CChain& active_chain, const CBlockIndex& stale_tip) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    /** Drop tracked tips that have expired (see IsExpired()). */
    void PruneExpired(const CChain& active_chain) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    /** Insert `stale_tip` into the tracker:
     *  - It is not added if it or one of its descendants is already tracked.
     *  - It replaces any tracked tips it descends from.
     *  - If the tracker is still full, the tip with the least work is evicted
     *    (the oldest among equals), but only if it has less work than
     *    `stale_tip`. Otherwise `stale_tip` is not added, so an evicted tip
     *    cannot be readmitted while the tracker stays full.
     *
     * @return Whether `stale_tip` was added.
     */
    bool Add(const CBlockIndex& stale_tip) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

public:
    /** Construct a tracker, with optional non-default policy parameters for
     *  tests. */
    explicit StaleTipTracker(int recent_window = STALETIP_RECENT_WINDOW, size_t max_headers = MAX_STALETIP_HEADERS)
        : m_recent_window{recent_window}, m_max_headers{max_headers}
    {
    }

    /** Track `stale_tip` if it is eligible (see GetEligibleForkPoint()) and
     *  Add() accepts it.
     *
     * @return Whether the tracker was updated to track `stale_tip`.
     */
    bool AddStaleTip(const CChain& active_chain, const CBlockIndex& stale_tip) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    /** Get the tracked tips that are currently eligible, with their fork
     *  points, in the order they were added. */
    std::vector<StaleFork> GetStaleTips(const CChain& active_chain) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};

} // namespace node

#endif // BITCOIN_NODE_STALETIPTRACKER_H
