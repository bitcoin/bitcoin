// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_PRIVATE_BROADCAST_H
#define BITCOIN_PRIVATE_BROADCAST_H

#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>
#include <util/hasher.h>
#include <util/time.h>

#include <map>
#include <unordered_map>

/**
 * Store a list of transactions to be broadcast privately. Supports the following operations:
 * - Add a new transaction
 */
class PrivateBroadcast
{
public:
    /**
     * Add a transaction to the storage.
     * @param[in] tx The transaction to add.
     * @retval true The transaction was added.
     * @retval false The transaction was already present.
     */
    bool Add(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    struct Priority {
        size_t num_broadcasted{0};
        NodeClock::time_point last_broadcasted{};

        bool operator<(const Priority& other) const;
    };

    struct TxWithPriority {
        CTransactionRef tx;
        Priority priority;
    };

    using ByTxid = std::unordered_map<Txid, TxWithPriority, SaltedTxidHasher>;
    using ByPriority = std::multimap<Priority, Txid>;

    struct Iterators {
        ByTxid::iterator by_txid;
        ByPriority::iterator by_priority;
    };

    mutable Mutex m_mutex;
    ByTxid m_by_txid GUARDED_BY(m_mutex);
    ByPriority m_by_priority GUARDED_BY(m_mutex);
};

#endif // BITCOIN_PRIVATE_BROADCAST_H
