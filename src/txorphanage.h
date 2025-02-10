// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXORPHANAGE_H
#define BITCOIN_TXORPHANAGE_H

#include <consensus/validation.h>
#include <net.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <util/time.h>

#include <map>
#include <set>

/** Expiration time for orphan transactions */
static constexpr auto ORPHAN_TX_EXPIRE_TIME{20min};
/** Minimum time between orphan transactions expire time checks */
static constexpr auto ORPHAN_TX_EXPIRE_INTERVAL{5min};

/** A class to track orphan transactions (failed on TX_MISSING_INPUTS)
 * Since we cannot distinguish orphans from bad transactions with
 * non-existent inputs, we heavily limit the number of orphans
 * we keep and the duration we keep them for.
 * Not thread-safe. Requires external synchronization.
 */
class TxOrphanage {
public:
    /** Add a new orphan transaction */
    bool AddTx(const CTransactionRef& tx, NodeId peer);

    /** Add an additional announcer to an orphan if it exists. Otherwise, do nothing. */
    bool AddAnnouncer(const Wtxid& wtxid, NodeId peer);

    CTransactionRef GetTx(const Wtxid& wtxid) const;

    /** Check if we already have an orphan transaction (by wtxid only) */
    bool HaveTx(const Wtxid& wtxid) const;

    /** Check if a {tx, peer} exists in the orphanage.*/
    bool HaveTxFromPeer(const Wtxid& wtxid, NodeId peer) const;

    /** Extract a transaction from a peer's work set
     *  Returns nullptr if there are no transactions to work on.
     *  Otherwise returns the transaction reference, and removes
     *  it from the work set.
     */
    CTransactionRef GetTxToReconsider(NodeId peer);

    /** Erase an orphan by wtxid */
    int EraseTx(const Wtxid& wtxid);

    /** Maybe erase all orphans announced by a peer (eg, after that peer disconnects). If an orphan
     * has been announced by another peer, don't erase, just remove this peer from the list of announcers. */
    void EraseForPeer(NodeId peer);

    /** Erase all orphans included in or invalidated by a new block */
    void EraseForBlock(const CBlock& block);

    /** Limit the orphanage to the given maximum */
    void LimitOrphans(unsigned int max_orphans, FastRandomContext& rng);

    /** Add any orphans that list a particular tx as a parent into the from peer's work set */
    void AddChildrenToWorkSet(const CTransaction& tx, FastRandomContext& rng);

    /** Does this peer have any work to do? */
    bool HaveTxToReconsider(NodeId peer);

    /** Get all children that spend from this tx and were received from nodeid. Sorted from most
     * recent to least recent. */
    std::vector<CTransactionRef> GetChildrenFromSamePeer(const CTransactionRef& parent, NodeId nodeid) const;

    /** Return how many entries exist in the orphange */
    size_t Size() const
    {
        return m_orphans.size();
    }

    /** Allows providing orphan information externally */
    struct OrphanTxBase {
        CTransactionRef tx;
        /** Peers added with AddTx or AddAnnouncer. */
        std::set<NodeId> announcers;
        NodeSeconds nTimeExpire;

        /** Get the weight of this transaction, an approximation of its memory usage. */
        unsigned int GetUsage() const {
            return GetTransactionWeight(*tx);
        }
    };

    std::vector<OrphanTxBase> GetOrphanTransactions() const;

    /** Get the total usage (weight) of all orphans. If an orphan has multiple announcers, its usage is
     * only counted once within this total. */
    unsigned int TotalOrphanUsage() const { return m_total_orphan_usage; }

    /** Total usage (weight) of orphans for which this peer is an announcer. If an orphan has multiple
     * announcers, its weight will be accounted for in each PeerOrphanInfo, so the total of all
     * peers' UsageByPeer() may be larger than TotalOrphanBytes(). */
    unsigned int UsageByPeer(NodeId peer) const {
        auto peer_it = m_peer_orphanage_info.find(peer);
        return peer_it == m_peer_orphanage_info.end() ? 0 : peer_it->second.m_total_usage;
    }

    /** Check consistency between PeerOrphanInfo and m_orphans. Recalculate counters and ensure they
     * match what is cached. */
    void SanityCheck() const;

protected:
    struct OrphanTx : public OrphanTxBase {
        size_t list_pos;
    };

    /** Total usage (weight) of all entries in m_orphans. */
    unsigned int m_total_orphan_usage{0};

    /** Total number of <peer, tx> pairs. Can be larger than m_orphans.size() because multiple peers
     * may have announced the same orphan. */
    unsigned int m_total_announcements{0};

    /** Map from wtxid to orphan transaction record. Limited by
     *  -maxorphantx/DEFAULT_MAX_ORPHAN_TRANSACTIONS */
    std::map<Wtxid, OrphanTx> m_orphans;

    struct PeerOrphanInfo {
        /** List of transactions that should be reconsidered: added to in AddChildrenToWorkSet,
         * removed from one-by-one with each call to GetTxToReconsider. The wtxids may refer to
         * transactions that are no longer present in orphanage; these are lazily removed in
         * GetTxToReconsider. */
        std::set<Wtxid> m_work_set;

        /** Total weight of orphans for which this peer is an announcer.
         * If orphans are provided by different peers, its weight will be accounted for in each
         * PeerOrphanInfo, so the total of all peers' m_total_usage may be larger than
         * m_total_orphan_size. If a peer is removed as an announcer, even if the orphan still
         * remains in the orphanage, this number will be decremented. */
        unsigned int m_total_usage{0};
    };
    std::map<NodeId, PeerOrphanInfo> m_peer_orphanage_info;

    using OrphanMap = decltype(m_orphans);

    struct IteratorComparator
    {
        template<typename I>
        bool operator()(const I& a, const I& b) const
        {
            return a->first < b->first;
        }
    };

    /** Index from the parents' COutPoint into the m_orphans. Used
     *  to remove orphan transactions from the m_orphans */
    std::map<COutPoint, std::set<OrphanMap::iterator, IteratorComparator>> m_outpoint_to_orphan_it;

    /** Orphan transactions in vector for quick random eviction */
    std::vector<OrphanMap::iterator> m_orphan_list;

    /** Timestamp for the next scheduled sweep of expired orphans */
    NodeSeconds m_next_sweep{0s};
};

#endif // BITCOIN_TXORPHANAGE_H
