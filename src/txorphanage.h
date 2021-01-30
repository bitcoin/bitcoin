// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXORPHANAGE_H
#define BITCOIN_TXORPHANAGE_H

#include <net.h>
#include <primitives/transaction.h>
#include <sync.h>

/** Expiration time for orphan transactions in seconds */
static constexpr int64_t ORPHAN_TX_EXPIRE_TIME = 20 * 60;

/** Guards orphan transactions and extra txs for compact blocks */
extern RecursiveMutex g_cs_orphans;

struct COrphanTx {
    // When modifying, adapt the copy of this definition in tests/DoS_tests.
    CTransactionRef tx;
    NodeId fromPeer;
    int64_t nTimeExpire;
    size_t list_pos;
};

int EraseOrphanTx(uint256 hash) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);
void EraseOrphansFor(NodeId peer);
unsigned int LimitOrphanTxSize(unsigned int nMaxOrphans);

/** Map from txid to orphan transaction record. Limited by
 *  -maxorphantx/DEFAULT_MAX_ORPHAN_TRANSACTIONS */
extern std::map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(g_cs_orphans);

/** Index from wtxid into the mapOrphanTransactions to lookup orphan
 *  transactions using their witness ids. */
extern std::map<uint256, std::map<uint256, COrphanTx>::iterator> g_orphans_by_wtxid GUARDED_BY(g_cs_orphans);

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
    extern std::map<COutPoint, std::set<std::map<uint256, COrphanTx>::iterator, IteratorComparator>> mapOrphanTransactionsByPrev GUARDED_BY(g_cs_orphans);

    /** Orphan transactions in vector for quick random eviction */
    extern std::vector<std::map<uint256, COrphanTx>::iterator> g_orphan_list GUARDED_BY(g_cs_orphans);

#endif // BITCOIN_TXORPHANAGE_H
