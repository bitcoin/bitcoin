// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXORPHANAGE_H
#define BITCOIN_TXORPHANAGE_H

#include <consensus/validation.h>
#include <net.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <util/feefrac.h>
#include <util/time.h>

#include <algorithm>
#include <map>
#include <set>

/** Expiration time for orphan transactions */
static constexpr auto ORPHAN_TX_EXPIRE_TIME{20min};
/** Minimum time between orphan transactions expire time checks */
static constexpr auto ORPHAN_TX_EXPIRE_INTERVAL{5min};
/** Default value for TxOrphanage::m_reserved_weight_per_peer. */
static constexpr unsigned int DEFAULT_RESERVED_ORPHAN_WEIGHT_PER_PEER{404'000};
/** Default value for TxOrphanage::m_max_global_announcements. */
static constexpr unsigned int DEFAULT_MAX_ORPHAN_ANNOUNCEMENTS{3000};

/** A class to track orphan transactions (failed on TX_MISSING_INPUTS)
 * Since we cannot distinguish orphans from bad transactions with
 * non-existent inputs, we heavily limit the number of orphans
 * we keep and the duration we keep them for.
 * Not thread-safe. Requires external synchronization.
 */
class TxOrphanage {
    /** The usage (weight) reserved for each peer, representing the amount of memory we are willing
     * to allocate for orphanage space. Note that this number is a reservation, not a limit: peers
     * are allowed to exceed this reservation until the global limit is reached, and peers are
     * effectively guaranteed this amount of space. Reservation is per-peer, so the global upper
     * bound on memory usage scales up with more peers. */
    unsigned int m_reserved_weight_per_peer{DEFAULT_RESERVED_ORPHAN_WEIGHT_PER_PEER};

    /** The maximum number of announcements across all peers, representing a computational upper bound,
     * i.e. the maximum number of evictions we might do at a time. There is no per-peer announcement
     * limit until the global limit is reached. Also, this limit is constant regardless of how many
     * peers we have: if we only have 1 peer, this is the number of orphans they may provide before
     * triggering eviction. As more peers are added, each peer's protected allocation is reduced. */
    unsigned int m_max_global_announcements{DEFAULT_MAX_ORPHAN_ANNOUNCEMENTS};
public:
    TxOrphanage() = default;
    explicit TxOrphanage(unsigned int res_weight_per_peer, unsigned int max_announcements) :
        m_reserved_weight_per_peer{res_weight_per_peer},
        m_max_global_announcements{max_announcements}
    {}

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
        /** Peers added with AddTx or AddAnnouncer, mapping to the position in that
         * peer's PeerOrphanInfo::m_iter_list. */
        std::map<NodeId, size_t> announcers;
        NodeSeconds nTimeExpire;

        /** Get the weight of this transaction, an approximation of its memory usage. */
        unsigned int GetUsage() const {
            return GetTransactionWeight(*tx);
        }
    };

    std::vector<OrphanTxBase> GetOrphanTransactions() const;

    /** Get the total usage (weight) of all orphans. If an orphan has multiple announcers, its usage is
     * only counted once within this total. */
    int64_t TotalOrphanUsage() const { return m_total_orphan_usage; }

    /** Total usage (weight) of orphans for which this peer is an announcer. If an orphan has multiple
     * announcers, its weight will be accounted for in each PeerOrphanInfo, so the total of all
     * peers' UsageByPeer() may be larger than TotalOrphanBytes(). */
    int64_t UsageByPeer(NodeId peer) const {
        auto peer_it = m_peer_orphanage_info.find(peer);
        return peer_it == m_peer_orphanage_info.end() ? 0 : peer_it->second.m_total_usage;
    }

    /** Number of orphans for which this peer is an announcer. If an orphan has multiple announcers,
     * it is accounted for in each PeerOrphanInfo, so the total of all peers' AnnouncementsByPeer()
     * may be larger than Size().. */
    unsigned int AnnouncementsByPeer(NodeId peer) const {
        auto peer_it = m_peer_orphanage_info.find(peer);
        return peer_it == m_peer_orphanage_info.end() ? 0 : peer_it->second.m_iter_list.size();
    }

    /** Check consistency between PeerOrphanInfo and m_orphans. Recalculate counters and ensure they
     * match what is cached. */
    void SanityCheck() const;

protected:
    struct OrphanTx : public OrphanTxBase {
    };

    /** Total usage (weight) of all entries in m_orphans. */
    int64_t m_total_orphan_usage{0};

    /** Total number of <peer, tx> pairs. Can be larger than m_orphans.size() because multiple peers
     * may have announced the same orphan. */
    unsigned int m_total_announcements{0};

    /** Map from wtxid to orphan transaction record. Limited by
     *  -maxorphantx/DEFAULT_MAX_ORPHAN_TRANSACTIONS */
    std::map<Wtxid, OrphanTx> m_orphans;

    using OrphanMap = decltype(m_orphans);

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
        int64_t m_total_usage{0};

        /** Orphan transactions announced by this peer. Also used for quick random eviction. */
        std::vector<OrphanMap::iterator> m_iter_list;

        /** There are 2 DoS scores:
         * - CPU score (ratio of num announcements / max allowed announcements)
         * - Memory score (ratio of total usage / max allowed usage).
         *
         * If the peer is using more than the allowed for either resource, its DoS score is > 1.
         * A peer having a DoS score > 1 does not necessarily mean that something is wrong, since we
         * do not trim unless the orphanage exceeds global limits, but it means that this peer will
         * be selected for trimming sooner. If the global announcement or global memory usage
         * limits are exceeded, it must be that there is a peer whose DoS score > 1. */
        FeeFrac GetDoSScore(unsigned int peer_max_ann, unsigned int peer_max_mem) {
            FeeFrac cpu_score(m_iter_list.size(), peer_max_ann);
            FeeFrac mem_score(m_total_usage, peer_max_mem);
            return std::max<FeeFrac>(cpu_score, mem_score);
        }
    };
    std::map<NodeId, PeerOrphanInfo> m_peer_orphanage_info;

    using PeerMap = decltype(m_peer_orphanage_info);

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

    /** Timestamp for the next scheduled sweep of expired orphans */
    NodeSeconds m_next_sweep{0s};

    /** If ORPHAN_TX_EXPIRE_INTERVAL has elapsed since the last sweep, expire orphans older than
     * ORPHAN_TX_EXPIRE_TIME. Called within LimitOrphans. */
    unsigned int MaybeExpireOrphans();

    /** If any of the following conditions are met, trim orphans until none are true:
     * 1. The global memory usage exceeds the maximum allowed.
     * 2. The global number of announcements exceeds the maximum allowed.
     * 3. The total number of orphans exceeds max_orphans.
     *
     * The trimming process sorts peers by their DoS score, only removing announcements /  orphans
     * of the peer with the worst DoS score. We use a heap to sort the peers, pop the worst one off,
     * and then re-add it if the peer still has transactions. The loop can run a maximum of
     * m_max_global_announcements times before there cannot be any more transactions to evict.
     * Bounds: O(p) to build the heap, O(n log(p)) for subsequent heap operations.
     *   p = number of peers
     *   n = number of announcements
     * */
    unsigned int MaybeTrimOrphans(unsigned int max_orphans, FastRandomContext& rng);

    /** Remove the element at list_pos in m_iter_list in O(1) time by swapping the last element
     * with the one at list_pos and popping the back if there are multiple elements. */
    void RemoveIterAt(NodeId peer, size_t list_pos);

    int64_t GetPerPeerMaxUsage() const {
        return m_reserved_weight_per_peer;
    }

    int64_t GetGlobalMaxUsage() const {
        return std::max<int64_t>(int64_t(m_peer_orphanage_info.size()) * m_reserved_weight_per_peer, 1);
    }

    unsigned int GetPerPeerMaxAnnouncements() const {
        if (m_peer_orphanage_info.empty()) return m_max_global_announcements;
        return m_max_global_announcements / m_peer_orphanage_info.size();
    }

    unsigned int GetGlobalMaxAnnouncements() const {
        return m_max_global_announcements;
    }

    /** Returns whether the global announcement or memory limits have been reached. */
    bool NeedsTrim(unsigned int max_orphans) const;
};

#endif // BITCOIN_TXORPHANAGE_H
