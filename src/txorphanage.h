// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXORPHANAGE_H
#define BITCOIN_TXORPHANAGE_H

#include <net.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>

#include <map>
#include <set>

/** A class to track orphan transactions (failed on TX_MISSING_INPUTS)
 * Since we cannot distinguish orphans from bad transactions with
 * non-existent inputs, we heavily limit the number of orphans
 * we keep and the duration we keep them for.
 */
class TxOrphanage {
public:
    /** Add a new orphan transaction */
    bool AddTx(const CTransactionRef& tx, NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Check if we already have an orphan transaction */
    bool HaveTx(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Extract a transaction from a peer's work set
     *  Returns nullptr and sets more to false if there are no transactions
     *  to work on. Otherwise returns the transaction reference, removes
     *  the transaction from the work set, and populates its arguments with
     *  the originating peer, and whether there are more orphans for this peer
     *  to work on after this tx.
     */
    CTransactionRef GetTxToReconsider(NodeId peer, NodeId& originator, bool& more) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Get a set of orphan transactions that can be candidates for reconsideration into the mempool */
    std::set<uint256> GetCandidatesForBlock(const CBlock& block) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Erase an orphan by txid */
    int EraseTx(const uint256& txid) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Erase all orphans announced by a peer (eg, after that peer disconnects) */
    void EraseForPeer(NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Erase all orphans included in or invalidated by a new block */
    void EraseForBlock(const CBlock& block) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Limit the orphanage to the given maximum */
    void LimitOrphans(unsigned int max_orphans_size) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Add any orphans that list a particular tx as a parent into a peer's work set */
    void AddChildrenToWorkSet(const CTransaction& tx, NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Return how many entries exist in the orphange */
    size_t Size() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        return m_orphans.size();
    }

protected:
    /** Guards orphan transactions */
    mutable Mutex m_mutex;

    struct OrphanTx {
        CTransactionRef tx;
        NodeId fromPeer;
        int64_t nTimeExpire;
        size_t list_pos;
        size_t nTxSize;
    };

    /** Map from txid to orphan transaction record. Limited by
     *  -maxorphantx/DEFAULT_MAX_ORPHAN_TRANSACTIONS */
    std::map<uint256, OrphanTx> m_orphans GUARDED_BY(m_mutex);

    /** Which peer provided a parent tx of orphans that need to be reconsidered */
    std::map<NodeId, std::set<uint256>> m_peer_work_set GUARDED_BY(m_mutex);

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
    std::map<COutPoint, std::set<OrphanMap::iterator, IteratorComparator>> m_outpoint_to_orphan_it GUARDED_BY(m_mutex);

    /** Orphan transactions in vector for quick random eviction */
    std::vector<OrphanMap::iterator> m_orphan_list GUARDED_BY(m_mutex);

    /** Cumulative size of all transactions in the orphan map */
    size_t m_orphan_tx_size GUARDED_BY(m_mutex){0};

    /** Erase an orphan by txid */
    int _EraseTx(const uint256& txid) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
};

#endif // BITCOIN_TXORPHANAGE_H
