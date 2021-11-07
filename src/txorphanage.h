// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXORPHANAGE_H
#define BITCOIN_TXORPHANAGE_H

#include <net.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>

/** Guards orphan transactions and extra txs for compact blocks */
extern RecursiveMutex g_cs_orphans;

/** A class to track orphan transactions (failed on TX_MISSING_INPUTS)
 * Since we cannot distinguish orphans from bad transactions with
 * non-existent inputs, we heavily limit the number of orphans
 * we keep and the duration we keep them for.
 */
class TxOrphanage {
public:
    /** Add a new orphan transaction */
    bool AddTx(const CTransactionRef& tx, NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Check if we already have an orphan transaction (by txid or wtxid) */
    bool HaveTx(const GenTxid& gtxid) const LOCKS_EXCLUDED(::g_cs_orphans);

    /** Get an orphan transaction and its originating peer
     * (Transaction ref will be nullptr if not found)
     */
    std::pair<CTransactionRef, NodeId> GetTx(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Erase an orphan by txid */
    int EraseTx(const uint256& txid) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Erase all orphans announced by a peer (eg, after that peer disconnects) */
    void EraseForPeer(NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Erase all orphans included in or invalidated by a new block */
    void EraseForBlock(const CBlock& block) LOCKS_EXCLUDED(::g_cs_orphans);

    /** Limit the orphanage to the given maximum */
    unsigned int LimitOrphans(unsigned int max_orphans) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Add any orphans that list a particular tx as a parent into a peer's work set
     * (ie orphans that may have found their final missing parent, and so should be reconsidered for the mempool) */
    void AddChildrenToWorkSet(const CTransaction& tx, std::set<uint256>& orphan_work_set) const EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

protected:
    struct OrphanTx {
        CTransactionRef tx;
        NodeId fromPeer;
        int64_t nTimeExpire;
        size_t list_pos;
    };

    /** Map from txid to orphan transaction record. Limited by
     *  -maxorphantx/DEFAULT_MAX_ORPHAN_TRANSACTIONS */
    std::map<uint256, OrphanTx> m_orphans GUARDED_BY(g_cs_orphans);

    using OrphanMap = decltype(m_orphans);

    struct IteratorComparator
    {
        template<typename I>
        bool operator()(const I& a, const I& b) const
        {
            return &(*a) < &(*b);
        }
    };

    /** Index from the parents' COutPoint into the m_orphans. Used
     *  to remove orphan transactions from the m_orphans */
    std::map<COutPoint, std::set<OrphanMap::iterator, IteratorComparator>> m_outpoint_to_orphan_it GUARDED_BY(g_cs_orphans);

    /** Orphan transactions in vector for quick random eviction */
    std::vector<OrphanMap::iterator> m_orphan_list GUARDED_BY(g_cs_orphans);

    /** Index from wtxid into the m_orphans to lookup orphan
     *  transactions using their witness ids. */
    std::map<uint256, OrphanMap::iterator> m_wtxid_to_orphan_it GUARDED_BY(g_cs_orphans);
};

#endif // BITCOIN_TXORPHANAGE_H
