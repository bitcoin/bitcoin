// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_PRIVATE_BROADCAST_H
#define BITCOIN_PRIVATE_BROADCAST_H

#include <net.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>
#include <util/time.h>

#include <optional>
#include <unordered_map>
#include <vector>

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
    bool Add(const CTransactionRef& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    /// Status of a transaction sent to a given node.
    struct SendStatus {
        const NodeId nodeid; /// Node to which the transaction will be sent (or was sent).
        const NodeClock::time_point picked; ///< When was the transaction picked for sending to the node.
        std::optional<NodeClock::time_point> confirmed; ///< When was the transaction reception confirmed by the node (by PONG).

        SendStatus(const NodeId& nodeid, const NodeClock::time_point& picked) : nodeid{nodeid}, picked{picked} {}
    };

    // No need for salted hasher because we are going to store just a bunch of locally originating transactions.

    struct CTransactionRefHash {
        size_t operator()(const CTransactionRef& tx) const
        {
            return static_cast<size_t>(tx->GetWitnessHash().ToUint256().GetUint64(0));
        }
    };

    struct CTransactionRefComp {
        bool operator()(const CTransactionRef& a, const CTransactionRef& b) const
        {
            return a->GetWitnessHash() == b->GetWitnessHash(); // If wtxid equals, then txid also equals.
        }
    };

    mutable Mutex m_mutex;
    std::unordered_map<CTransactionRef, std::vector<SendStatus>, CTransactionRefHash, CTransactionRefComp>
        m_transactions GUARDED_BY(m_mutex);
};

#endif // BITCOIN_PRIVATE_BROADCAST_H
