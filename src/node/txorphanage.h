// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXORPHANAGE_H
#define BITCOIN_NODE_TXORPHANAGE_H

#include <consensus/validation.h>
#include <net.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <util/time.h>

#include <map>
#include <set>

namespace node {
/** Default value for TxOrphanage::m_reserved_usage_per_peer. Helps limit the total amount of memory used by the orphanage. */
static constexpr int64_t DEFAULT_RESERVED_ORPHAN_WEIGHT_PER_PEER{404'000};
/** Default value for TxOrphanage::m_max_global_latency_score. Helps limit the maximum latency for operations like
 * EraseForBlock and LimitOrphans. */
static constexpr unsigned int DEFAULT_MAX_ORPHANAGE_LATENCY_SCORE{3000};

/** A class to track orphan transactions (failed on TX_MISSING_INPUTS)
 * Since we cannot distinguish orphans from bad transactions with non-existent inputs, we heavily limit the amount of
 * announcements (unique (NodeId, wtxid) pairs), the number of inputs, and size of the orphans stored (both individual
 * and summed). We also try to prevent adversaries from churning this data structure: once global limits are reached, we
 * continuously evict the oldest announcement (sorting non-reconsiderable orphans before reconsiderable ones) from the
 * most resource-intensive peer until we are back within limits.
 * - Peers can exceed their individual limits (e.g. because they are very useful transaction relay peers) as long as the
 *   global limits are not exceeded.
 * - As long as the orphan has 1 announcer, it remains in the orphanage.
 * - No peer can trigger the eviction of another peer's orphans.
 * - Peers' orphans are effectively protected from eviction as long as they don't exceed their limits.
 * Not thread-safe. Requires external synchronization.
 */
class TxOrphanage {
public:
    using Usage = int64_t;
    using Count = unsigned int;

    /** Allows providing orphan information externally */
    struct OrphanInfo {
        CTransactionRef tx;
        /** Peers added with AddTx or AddAnnouncer. */
        std::set<NodeId> announcers;

        // Constructor with moved announcers
        OrphanInfo(CTransactionRef tx, std::set<NodeId>&& announcers) :
            tx(std::move(tx)),
            announcers(std::move(announcers))
        {}
    };

    virtual ~TxOrphanage() = default;

    /** Add a new orphan transaction */
    virtual bool AddTx(const CTransactionRef& tx, NodeId peer) = 0;

    /** Add an additional announcer to an orphan if it exists. Otherwise, do nothing. */
    virtual bool AddAnnouncer(const Wtxid& wtxid, NodeId peer) = 0;

    /** Get a transaction by its witness txid */
    virtual CTransactionRef GetTx(const Wtxid& wtxid) const = 0;

    /** Check if we already have an orphan transaction (by wtxid only) */
    virtual bool HaveTx(const Wtxid& wtxid) const = 0;

    /** Check if a {tx, peer} exists in the orphanage.*/
    virtual bool HaveTxFromPeer(const Wtxid& wtxid, NodeId peer) const = 0;

    /** Extract a transaction from a peer's work set, and flip it back to non-reconsiderable.
     *  Returns nullptr if there are no transactions to work on.
     *  Otherwise returns the transaction reference, and removes
     *  it from the work set.
     */
    virtual CTransactionRef GetTxToReconsider(NodeId peer) = 0;

    /** Erase an orphan by wtxid, including all announcements if there are multiple.
     * Returns true if an orphan was erased, false if no tx with this wtxid exists. */
    virtual bool EraseTx(const Wtxid& wtxid) = 0;

    /** Maybe erase all orphans announced by a peer (eg, after that peer disconnects). If an orphan
     * has been announced by another peer, don't erase, just remove this peer from the list of announcers. */
    virtual void EraseForPeer(NodeId peer) = 0;

    /** Erase all orphans included in or invalidated by a new block */
    virtual void EraseForBlock(const CBlock& block) = 0;

    /** Add any orphans that list a particular tx as a parent into the from peer's work set */
    virtual std::vector<std::pair<Wtxid, NodeId>> AddChildrenToWorkSet(const CTransaction& tx, FastRandomContext& rng) = 0;

    /** Does this peer have any work to do? */
    virtual bool HaveTxToReconsider(NodeId peer) = 0;

    /** Get all children that spend from this tx and were received from nodeid. Sorted
     * reconsiderable before non-reconsiderable, then from most recent to least recent. */
    virtual std::vector<CTransactionRef> GetChildrenFromSamePeer(const CTransactionRef& parent, NodeId nodeid) const = 0;

    /** Get all orphan transactions */
    virtual std::vector<OrphanInfo> GetOrphanTransactions() const = 0;

    /** Get the total usage (weight) of all orphans. If an orphan has multiple announcers, its usage is
     * only counted once within this total. */
    virtual Usage TotalOrphanUsage() const = 0;

    /** Total usage (weight) of orphans for which this peer is an announcer. If an orphan has multiple
     * announcers, its weight will be accounted for in each PeerOrphanInfo, so the total of all
     * peers' UsageByPeer() may be larger than TotalOrphanUsage(). Similarly, UsageByPeer() may be far higher than
     * ReservedPeerUsage(), particularly if many peers have provided the same orphans. */
    virtual Usage UsageByPeer(NodeId peer) const = 0;

    /** Check consistency between PeerOrphanInfo and m_orphans. Recalculate counters and ensure they
     * match what is cached. */
    virtual void SanityCheck() const = 0;

    /** Number of announcements, i.e. total size of m_orphans. Ones for the same wtxid are not de-duplicated.
     * Not the same as TotalLatencyScore(). */
    virtual Count CountAnnouncements() const = 0;

    /** Number of unique orphans (by wtxid). */
    virtual Count CountUniqueOrphans() const = 0;

    /** Number of orphans stored from this peer. */
    virtual Count AnnouncementsFromPeer(NodeId peer) const = 0;

    /** Latency score of transactions announced by this peer. */
    virtual Count LatencyScoreFromPeer(NodeId peer) const = 0;

    /** Get the maximum global latency score allowed */
    virtual Count MaxGlobalLatencyScore() const = 0;

    /** Get the total latency score of all orphans */
    virtual Count TotalLatencyScore() const = 0;

    /** Get the reserved usage per peer */
    virtual Usage ReservedPeerUsage() const = 0;

    /** Get the maximum latency score allowed per peer */
    virtual Count MaxPeerLatencyScore() const = 0;

    /** Get the maximum global usage allowed */
    virtual Usage MaxGlobalUsage() const = 0;
};

/** Create a new TxOrphanage instance */
std::unique_ptr<TxOrphanage> MakeTxOrphanage() noexcept;
std::unique_ptr<TxOrphanage> MakeTxOrphanage(TxOrphanage::Count max_global_latency_score, TxOrphanage::Usage reserved_peer_usage) noexcept;
} // namespace node
#endif // BITCOIN_NODE_TXORPHANAGE_H
