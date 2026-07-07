// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_PRIVATE_BROADCAST_H
#define BITCOIN_PRIVATE_BROADCAST_H

#include <net.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <sync.h>
#include <util/time.h>

#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

/**
 * Store a list of transactions to be broadcast privately. Supports the following operations:
 * - Add a new transaction
 * - Remove a transaction
 * - Pick a transaction for sending to one recipient
 * - Query which transaction has been picked for sending to a given recipient node
 * - Mark that a given recipient node has confirmed receipt of a transaction
 * - Mark that a given recipient node has disconnected, dropping its per-node state
 * - Query whether any transactions that need sending are currently on the list
 */
class PrivateBroadcast
{
public:

    /// If a transaction is not sent to any peer for this duration,
    /// then we consider it stale / for rebroadcasting.
    static constexpr auto INITIAL_STALE_DURATION{5min};

    /// If a transaction is not received back from the network for this duration
    /// after it is broadcast, then we consider it stale / for rebroadcasting.
    static constexpr auto STALE_DURATION{1min};

    /// Maximum number of transactions tracked simultaneously.
    /// Additions that would exceed this are rejected (see Add()).
    static constexpr size_t MAX_TRANSACTIONS{10'000};

    /// @param[in] max_transactions Cap on the number of simultaneously tracked
    /// transactions. Defaults to MAX_TRANSACTIONS.
    explicit PrivateBroadcast(size_t max_transactions = MAX_TRANSACTIONS)
        : m_max_transactions{max_transactions} {}

    struct PeerSendInfo {
        CService address;
        NodeClock::time_point sent;
        std::optional<NodeClock::time_point> received;
    };

    struct TxBroadcastInfo {
        CTransactionRef tx;
        NodeClock::time_point time_added;
        /// Total number of times the transaction was sent to a peer.
        size_t num_sent{0};
        /// Total number of peers that confirmed reception (by PONG).
        size_t num_confirmed{0};
        /// Send details for currently connected peers only.
        std::vector<PeerSendInfo> peers;
    };

    /// Outcome of Add().
    enum class AddResult {
        //! The transaction was newly added.
        Added,
        //! The transaction was already present; no change.
        AlreadyPresent,
        //! Rejected: the queue is already at MAX_TRANSACTIONS.
        QueueFull,
    };

    /**
     * Add a transaction to the storage.
     * @param[in] tx The transaction to add.
     * @return Whether the transaction was newly added, was already present, or
     * was rejected because the queue is full (see AddResult).
     */
    [[nodiscard]] AddResult Add(const CTransactionRef& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Forget a transaction.
     * @param[in] tx Transaction to forget.
     * @retval !nullopt The number of times the transaction was sent and confirmed
     * by the recipient (if the transaction existed and was removed).
     * @retval nullopt The transaction was not in the storage.
     */
    std::optional<size_t> Remove(const CTransactionRef& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Pick the transaction with the fewest send attempts, and confirmations,
     * and oldest send/confirm times.
     * @param[in] will_send_to_nodeid Will remember that the returned transaction
     * was picked for sending to this node. Calling this method more than once with
     * the same `will_send_to_nodeid` is not allowed because sending more than one
     * transaction to one node would be a privacy leak.
     * @param[in] will_send_to_address Address of the peer to which this transaction
     * will be sent.
     * @return Most urgent transaction or nullopt if there are no transactions.
     */
    std::optional<CTransactionRef> PickTxForSend(const NodeId& will_send_to_nodeid, const CService& will_send_to_address)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get the transaction that was picked for sending to a given node by PickTxForSend().
     * @param[in] nodeid Node to which a transaction is being sent.
     * @return Transaction or nullopt if the nodeid is unknown (or has disconnected).
     */
    std::optional<CTransactionRef> GetTxForNode(const NodeId& nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Mark that the node has confirmed reception of the transaction we sent it by
     * responding with `PONG` to our `PING` message.
     * @param[in] nodeid Node that we sent a transaction to.
     */
    void NodeConfirmedReception(const NodeId& nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Mark that the node has disconnected and drop its per-node state (if any).
     * The transaction's cumulative send/confirm counters are unaffected.
     * @param[in] nodeid Node that disconnected.
     * @retval true The node had confirmed reception, `NodeConfirmedReception()` has been called.
     * @retval false The node had not confirmed reception (or is unknown).
     */
    bool NodeDisconnected(const NodeId& nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Check if there are transactions that need to be broadcast.
     */
    bool HavePendingTransactions()
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get the transactions that have not been broadcast recently.
     */
    std::vector<CTransactionRef> GetStale() const
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get stats about all transactions currently being privately broadcast.
     */
    std::vector<TxBroadcastInfo> GetBroadcastInfo() const
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    /// Cumulative stats from all the send attempts for a transaction. Used to prioritize transactions.
    struct Priority {
        size_t num_picked{0}; ///< Number of times the transaction was picked for sending.
        NodeClock::time_point last_picked{}; ///< The most recent time when the transaction was picked for sending.
        size_t num_confirmed{0}; ///< Number of nodes that have confirmed reception of a transaction (by PONG).
        NodeClock::time_point last_confirmed{}; ///< The most recent time when the transaction was confirmed.

        auto operator<=>(const Priority& other) const
        {
            // Invert `other` and `this` in the comparison because smaller num_picked, num_confirmed or
            // earlier times mean greater priority. In other words, if this.num_picked < other.num_picked
            // then this > other.
            return std::tie(other.num_picked, other.num_confirmed, other.last_picked, other.last_confirmed) <=>
                   std::tie(num_picked, num_confirmed, last_picked, last_confirmed);
        }
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

    /// Per-transaction state: when it was added and cumulative send stats.
    struct TxState {
        const NodeClock::time_point time_added{NodeClock::now()};
        Priority priority;
    };

    /// Status of a send to a currently connected node.
    struct NodeSend {
        /// Transaction that will be sent (or was sent) to the node. Always present in m_transactions.
        CTransactionRef tx;
        /// Address of the node.
        CService address;
        /// When was the transaction picked for sending to the node.
        NodeClock::time_point sent;
        /// When was the transaction reception confirmed by the node (by PONG).
        std::optional<NodeClock::time_point> confirmed;
    };

    /// Cap on the number of simultaneously tracked transactions (see Add()).
    const size_t m_max_transactions;
    mutable Mutex m_mutex;
    std::unordered_map<CTransactionRef, TxState, CTransactionRefHash, CTransactionRefComp>
        m_transactions GUARDED_BY(m_mutex);
    /// Sends to currently connected nodes. Bounded
    /// by the number of live private broadcast connections.
    std::unordered_map<NodeId, NodeSend> m_by_node GUARDED_BY(m_mutex);
};

#endif // BITCOIN_PRIVATE_BROADCAST_H
