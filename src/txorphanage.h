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

/** Data structure to keep track of orphan transactions
 */
class TxOrphanage {
public:
    /** Add a new orphan transaction */
    bool AddTx(const CTransactionRef& tx, NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Check if we already have an orphan transaction (by txid or wtxid) */
    bool HaveTx(const GenTxid& gtxid) const EXCLUSIVE_LOCKS_REQUIRED(!g_cs_orphans);

    /** Get the details of an orphan transaction (returns nullptr if not found) */
    std::pair<CTransactionRef, NodeId> GetTx(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Erase an orphan by txid */
    int EraseTx(const uint256& txid) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Erase all orphans announced by a peer (eg, after that peer disconnects) */
    void EraseForPeer(NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Erase all orphans included in / invalidated by a new block */
    void EraseForBlock(const CBlock& block) EXCLUSIVE_LOCKS_REQUIRED(!g_cs_orphans);

    /** Limit the orphanage to the given maximum */
    unsigned int LimitOrphans(unsigned int nMaxOrphans) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

    /** Add any orphans that list a particular tx as a parent into a peer's work set
     * (ie orphans that may have found their final missing parent, and so should be reconsidered for the mempool) */
    void AddChildrenToWorkSet(const CTransaction& tx, std::set<uint256>& orphan_work_set) const EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);

protected:
    struct COrphanTx {
        CTransactionRef tx;
        NodeId fromPeer;
        int64_t nTimeExpire;
        size_t list_pos;
    };

    /** Map from txid to orphan transaction record. Limited by
     *  -maxorphantx/DEFAULT_MAX_ORPHAN_TRANSACTIONS */
    std::map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(g_cs_orphans);

    using OrphanMap = decltype(mapOrphanTransactions);

    struct IteratorComparator
    {
        template<typename I>
        bool operator()(const I& a, const I& b) const
        {
            return &(*a) < &(*b);
        }
    };

    /** Index from the parents' COutPoint into the mapOrphanTransactions. Used
     *  to remove orphan transactions from the mapOrphanTransactions */
    std::map<COutPoint, std::set<OrphanMap::iterator, IteratorComparator>> mapOrphanTransactionsByPrev GUARDED_BY(g_cs_orphans);

    /** Orphan transactions in vector for quick random eviction */
    std::vector<OrphanMap::iterator> g_orphan_list GUARDED_BY(g_cs_orphans);

    /** Index from wtxid into the mapOrphanTransactions to lookup orphan
     *  transactions using their witness ids. */
    std::map<uint256, OrphanMap::iterator> g_orphans_by_wtxid GUARDED_BY(g_cs_orphans);
};

#endif // BITCOIN_TXORPHANAGE_H
