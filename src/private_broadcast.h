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
 * - Query whether a given recipient node has confirmed reception
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

    /// Maximum number of send attempts for a transaction. Once this limit is
    /// reached, the transaction remains tracked but is not sent again unless
    /// explicitly re-added.
    static constexpr size_t MAX_SEND_ATTEMPTS{1'000};

    /// @param[in] max_transactions Cap on the number of simultaneously tracked
    /// transactions. Defaults to MAX_TRANSACTIONS.
    /// @param[in] max_send_attempts Cap on the number of send attempts per
    /// transaction. Defaults to MAX_SEND_ATTEMPTS.
    explicit PrivateBroadcast(size_t max_transactions = MAX_TRANSACTIONS,
                              size_t max_send_attempts = MAX_SEND_ATTEMPTS)
        : m_max_transactions{max_transactions}, m_max_send_attempts{max_send_attempts} {}

    struct PeerSendInfo {
        CService address;
        NodeClock::time_point sent;
        std::optional<NodeClock::time_point> received;
    };

    struct TxBroadcastInfo {
        CTransactionRef tx;
        NodeClock::time_point time_added;
        //! The earliest time the transaction may be sent (same as time_added if not delayed).
        NodeClock::time_point release_time;
        /// Number of additional send attempts allowed for this transaction (0 if exhausted).
        size_t attempts_remaining;
        std::vector<PeerSendInfo> peers;
    };

    /// Outcome of Add().
    enum class AddResult {
        //! The transaction was newly added or reset after exhausting its send attempts.
        Added,
        //! The transaction was already present with send attempts remaining; no change.
        AlreadyPresent,
        //! Rejected: the queue is already at MAX_TRANSACTIONS.
        QueueFull,
    };

    /// Outcome of a successful Remove().
    struct RemoveResult {
        //! The number of times the transaction was sent and confirmed by the recipient.
        size_t num_confirmed;
        //! Whether the transaction had been released for sending (see ReleaseDue()).
        bool released;
    };

    /**
     * Add a transaction to the storage, or reset an exhausted transaction so it
     * can be broadcast again.
     * @param[in] tx The transaction to add.
     * @param[in] release_time If set and in the future, the transaction is held back
     * and not considered for sending until it is released by ReleaseDue() at or after
     * this time. Otherwise the transaction is released immediately.
     * @return Whether the transaction was newly added or reset, was already
     * present with send attempts remaining, or was rejected because the queue is
     * full (see AddResult). An already present transaction keeps its release time.
     */
    [[nodiscard]] AddResult Add(const CTransactionRef& tx,
                                std::optional<NodeClock::time_point> release_time = std::nullopt)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Forget a transaction.
     * @param[in] tx Transaction to forget.
     * @retval !nullopt The transaction existed and was removed, see RemoveResult.
     * @retval nullopt The transaction was not in the storage.
     */
    std::optional<RemoveResult> Remove(const CTransactionRef& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Release the delayed transactions whose release time has been reached, so
     * that they become eligible for sending. Each transaction is returned at most
     * once. Transactions added without a delay are released at Add() and are
     * never returned.
     * @return The newly released transactions.
     */
    std::vector<CTransactionRef> ReleaseDue()
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
     * @return Most urgent transaction or nullopt if there are no released
     * transactions with send attempts remaining.
     */
    std::optional<CTransactionRef> PickTxForSend(const NodeId& will_send_to_nodeid, const CService& will_send_to_address)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get the transaction that was picked for sending to a given node by PickTxForSend().
     * @param[in] nodeid Node to which a transaction is being (or was) sent.
     * @return Transaction or nullopt if the nodeid is unknown.
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
     * Check if the node has confirmed reception of the transaction.
     * @retval true Node has confirmed, `NodeConfirmedReception()` has been called.
     * @retval false Node has not confirmed, `NodeConfirmedReception()` has not been called.
     */
    bool DidNodeConfirmReception(const NodeId& nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Check if there are released transactions with send attempts remaining.
     */
    bool HavePendingTransactions()
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get the released transactions that have not been broadcast recently and have
     * send attempts remaining. For a transaction not yet confirmed by any recipient,
     * staleness is measured from its release time.
     */
    std::vector<CTransactionRef> GetStale() const
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Get stats about all transactions currently being privately broadcast.
     */
    std::vector<TxBroadcastInfo> GetBroadcastInfo() const
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    /// Status of a transaction sent to a given node.
    struct SendStatus {
        /// Node to which the transaction will be sent (or was sent).
        const NodeId nodeid;
        /// Address of the node.
        const CService address;
        /// When was the transaction picked for sending to the node.
        const NodeClock::time_point picked;
        /// When was the transaction reception confirmed by the node (by PONG).
        std::optional<NodeClock::time_point> confirmed;

        SendStatus(const NodeId& nodeid, const CService& address, const NodeClock::time_point& picked) : nodeid{nodeid}, address{address}, picked{picked} {}
    };

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

    /// A pair of a transaction and a sent status for a given node. Convenience return type of GetSendStatusByNode().
    struct TxAndSendStatusForNode {
        const CTransactionRef& tx;
        SendStatus& send_status;
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

    /**
     * Derive the sending priority of a transaction.
     * @param[in] sent_to List of nodes that the transaction has been sent to.
     */
    static Priority DerivePriority(const std::vector<SendStatus>& sent_to);

    /**
     * Find which transaction we sent to a given node (marked by PickTxForSend()).
     * @return That transaction together with the send status or nullopt if we did not
     * send any transaction to the given node.
     */
    std::optional<TxAndSendStatusForNode> GetSendStatusByNode(const NodeId& nodeid)
        EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    struct TxSendStatus {
        NodeClock::time_point time_added{NodeClock::now()};
        //! The earliest time the transaction may be sent, never earlier than time_added.
        NodeClock::time_point release_time{time_added};
        //! Whether the transaction is eligible for sending (see ReleaseDue()).
        bool released{true};
        std::vector<SendStatus> send_statuses;
    };
    /// Whether the transaction has send attempts remaining.
    bool IsPending(const TxSendStatus& status) const;
    /// Whether the transaction is released and has send attempts remaining.
    bool IsSendable(const TxSendStatus& status) const;
    /// Cap on the number of simultaneously tracked transactions (see Add()).
    const size_t m_max_transactions;
    /// Cap on the number of send attempts per transaction (see PickTxForSend()).
    const size_t m_max_send_attempts;
    mutable Mutex m_mutex;
    std::unordered_map<CTransactionRef, TxSendStatus, CTransactionRefHash, CTransactionRefComp>
        m_transactions GUARDED_BY(m_mutex);
};

#endif // BITCOIN_PRIVATE_BROADCAST_H
