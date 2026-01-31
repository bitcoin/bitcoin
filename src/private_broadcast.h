// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_PRIVATE_BROADCAST_H
#define BITCOIN_PRIVATE_BROADCAST_H

#include <net.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <sync.h>
#include <threadsafety.h>
#include <util/expected.h>
#include <util/time.h>

#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

/**
 * Store a list of transactions to be broadcast privately. Supports the following operations:
 * - Add a new transaction
 * - Stop broadcasting a transaction
 * - Pick a transaction for sending to one recipient
 * - Query which transaction has been picked for sending to a given recipient node
 * - Mark that a given recipient node has confirmed receipt of a transaction
 * - Query whether a given recipient node has confirmed reception
 * - Query whether any transactions that need sending are currently on the list
 */
class PrivateBroadcast
{
public:
    struct FinalState {
        util::Expected<CService, std::string> result;
        NodeClock::time_point time;
    };

    struct PeerSendInfo {
        CService address;
        NodeClock::time_point sent;
        std::optional<NodeClock::time_point> received;
    };

    struct TxBroadcastInfo {
        CTransactionRef tx;
        std::vector<PeerSendInfo> peers;
        std::optional<FinalState> final_state;
    };

    /**
     * Add a transaction to the storage.
     * @param[in] tx The transaction to add.
     * @retval true The transaction was added, or was previously stopped and is now scheduled again.
     * @retval false The transaction was already present and pending.
     */
    bool Add(const CTransactionRef& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Stop broadcasting a transaction and mark as received back from the network.
     * Updates a previously aborted transaction with the received_from address.
     *
     * @param[in] tx Transaction to stop broadcasting.
     * @param[in] received_from The peer address from which the transaction was received.
     * @retval !nullopt The number of times the transaction was sent and confirmed
     * by the recipient (if the transaction existed and broadcasting was stopped).
     * @retval nullopt The transaction was not in the storage, or was already stopped.
     */
    std::optional<size_t> StopBroadcasting(const CTransactionRef& tx, const CService& received_from)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        return StopBroadcasting(tx, util::Expected<CService, std::string>{received_from});
    }

    /**
     * Stop broadcasting a transaction and mark as aborted.
     *
     * @param[in] tx Transaction to stop broadcasting.
     * @param[in] aborted Message describing why private broadcasting was aborted.
     * @retval !nullopt The number of times the transaction was sent and confirmed
     * by the recipient (if the transaction existed and broadcasting was stopped).
     * @retval nullopt The transaction was not in the storage, or was already stopped.
     */
    std::optional<size_t> StopBroadcasting(const CTransactionRef& tx, std::string aborted)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        return StopBroadcasting(tx, util::Unexpected<std::string>{std::move(aborted)});
    }

    /**
     * Pick the transaction with the fewest send attempts, and confirmations,
     * and oldest send/confirm times.
     * @param[in] will_send_to_nodeid Will remember that the returned transaction
     * was picked for sending to this node.
     * @param[in] will_send_to_address Address of the peer to which this transaction
     * will be sent.
     * @return Most urgent transaction or nullopt if there are no transactions.
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
    /**
     * Stop broadcasting a transaction. Shared implementation for the public overloads.
     *
     * @param[in] tx Transaction to stop broadcasting.
     * @param[in] final_state The final state of the transaction. If it has a value, the
     *                        transaction was received back from the network (and this is the
     *                        peer address it was received from). Otherwise it was aborted and
     *                        the error contains the details.
     * @retval !nullopt The number of times the transaction was sent and confirmed
     * by the recipient (if the transaction existed and broadcasting was stopped).
     * @retval nullopt The transaction was not in the storage, or was already stopped.
     */
    std::optional<size_t> StopBroadcasting(const CTransactionRef& tx, util::Expected<CService, std::string> final_state)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /// Status of a transaction sent to a given node.
    struct SendStatus {
        const NodeId nodeid; /// Node to which the transaction will be sent (or was sent).
        const CService address; /// Address of the node.
        const NodeClock::time_point picked; ///< When was the transaction picked for sending to the node.
        std::optional<NodeClock::time_point> confirmed; ///< When was the transaction reception confirmed by the node (by PONG).

        SendStatus(const NodeId& nodeid, const CService& address, const NodeClock::time_point& picked) : nodeid{nodeid}, address{address}, picked{picked} {}
    };

    struct TxState {
        std::optional<FinalState> final_state;
        std::vector<SendStatus> sent_to;
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

    mutable Mutex m_mutex;
    std::unordered_map<CTransactionRef, TxState, CTransactionRefHash, CTransactionRefComp>
        m_transactions GUARDED_BY(m_mutex);
};

#endif // BITCOIN_PRIVATE_BROADCAST_H
