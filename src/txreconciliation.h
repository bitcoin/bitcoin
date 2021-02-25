// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXRECONCILIATION_H
#define BITCOIN_TXRECONCILIATION_H

#include <net.h>
#include <sync.h>

#include <memory>
#include <tuple>

/**
 * Transaction reconciliation is a way for nodes to efficiently announce transactions.
 * This object keeps track of all reconciliation-related communications with the peers.
 * The high-level protocol is:
 * 0. Reconciliation protocol handshake.
 * 1. Once we receive a new transaction, add it to the set instead of announcing immediately
 * 2. When the time comes, a reconciliation initiator requests a sketch from the peer, where a sketch
 *    is a compressed representation of their set
 * 3. Once the initiator received a sketch from the peer, the initiator computes a local sketch,
 *    and combines the two skethes to find the difference in *sets*.
 * 4. Now the initiator knows full symmetrical difference and can request what the initiator is
 *    missing and announce to the peer what the peer is missing. For the former, an extra round is
 *    required because the initiator knows only short IDs of those transactions.
 * 5. Sometimes reconciliation fails if the difference is larger than the parties estimated,
 *    then there is one sketch extension round, in which the initiator requests for extra data.
 * 6. If extension succeeds, go to step 4.
 * 7. If extension fails, the initiator notifies the peer and announces all transactions from the
 *    corresponding set. Once the peer received the failure notification, the peer announces all
 *    transactions from the corresponding set.
 *
 * This is a modification of the Erlay protocol (https://arxiv.org/abs/1905.10518) with two
 * changes (sketch extensions instead of bisections, and an extra INV exchange round), both
 * are motivated in BIP-330.
 */
class TxReconciliationTracker {
    // Avoid littering this header file with implementation details.
    class Impl;
    const std::unique_ptr<Impl> m_impl;

    public:

    explicit TxReconciliationTracker();
    ~TxReconciliationTracker();

    /**
     * Step 0. Generate and pass reconciliation parameters to be sent along with the suggestion
     * to announce transactions via reconciliations.
     * Generates (and stores) a peer-specific salt which will be used for reconciliations.
     * Reconciliation roles are based on inbound/outbound role in the connection.
     * Returns the following values which will be used to invite a peer to reconcile:
     * - whether we want to initiate reconciliation requests (ask for sketches)
     * - whether we agree to respond to reconciliation requests (send our sketches)
     * - reconciliation protocol version
     * - salt used for short ID computation required for reconciliation
     * A peer can't be registered for future reconciliations without this call.
     * This function must be called only once per peer.
     */
    std::tuple<bool, bool, uint32_t, uint64_t> SuggestReconciling(NodeId peer_id, bool inbound);

    /**
     * Step 0. Once the peer agreed to reconcile with us, generate the data structures required
     * to track transactions we are going to announce and reconciliation-related parameters.
     * At this point, we decide whether we want to also flood certain transactions to the peer
     * along with reconciliations.
     * Add the peer to the queue if we are going to be the reconciliation initiator.
     * Should be called only after SuggestReconciling for the same peer and only once.
     * Does nothing and returns false if the peer violates the protocol.
     */
    bool EnableReconciliationSupport(NodeId peer_id, bool inbound,
        bool recon_requestor, bool recon_responder, uint32_t recon_version, uint64_t remote_salt);

    /**
     * Step 1. Add new transactions we want to announce to the peer to the local reconciliation set
     * of the peer, so that those transactions will be reconciled later.
     */
    void AddToReconSet(NodeId peer_id, const std::vector<uint256>& txs_to_reconcile);

    /**
     * Step 2. If a it's time to request a reconciliation from the peer, this function will return
     * the details of our local state, which should be communicated to the peer so that they better
     * know what we need:
     * - size of our reconciliation set for the peer
     * - our q-coefficient with the peer, formatted to be transmitted as integer value
     * If the peer was not previously registered for reconciliations, returns nullopt.
     */
    std::optional<std::pair<uint16_t, uint16_t>> MaybeRequestReconciliation(NodeId peer_id);

    /**
     * Step 2. Record an (expected) reconciliation request with parameters to respond when its time.
     * All initial reconciliation responses will be done not immediately but in batches after
     * a delay, to prevent privacy leaks.
     * If peer seems to violate the protocol, do nothing.
     */
    void HandleReconciliationRequest(NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q);

    /**
     * Step 2. Once it's time to respond to reconciliation requests, we construct a sketch from
     * the local reconciliation set, and send it to the initiator.
     * If the peer was not previously registered for reconciliations or it's not the time yet,
     * returns false.
     */
    bool RespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata);

    /**
     * Step 3. Process a response to our reconciliation request.
     * Returns false if the peer seems to violate the protocol.
     * Populates the vectors so that we know which transactions should be requested and announced,
     * and whether reconciliation succeeded (nullopt if the reconciliation is not over yet and
     * extension should be requested).
     */
    bool HandleSketch(NodeId peer_id, const std::vector<uint8_t>& skdata,
        // returning values
        std::vector<uint32_t>& txs_to_request, std::vector<uint256>& txs_to_announce, std::optional<bool>& result);

    /**
     * Step 5. Peer requesting extension after reconciliation they initiated failed on their side:
     * the sketch we sent to them was not sufficient to find the difference.
     * No privacy leak can happen here because sketch extension is constructed over the snapshot.
     * If the peer seems to violate the protocol, do nothing.
     */
    void HandleExtensionRequest(NodeId peer_id);

    /**
     * Step 4. Once we received a signal of reconciliation finalization with a given result from the
     * initiating peer, announce the following transactions:
     * - in case of a failure, all transactions we had for that peer
     * - in case of a success, transactions the peer asked for by short id (ask_shortids)
     * Return false if the peer seems to violate the protocol.
     */
    bool FinalizeInitByThem(NodeId peer_id, bool recon_result,
        const std::vector<uint32_t>& remote_missing_short_ids, std::vector<uint256>& remote_missing);

    // Helpers

    /**
     * Removes reconciliation-related state of the peer. After this, we won't be able to reconcile
     * with the peer unless it's registered again (see Step 0).
     */
    void RemovePeer(NodeId peer_id);

    /**
     * Check if a peer is registered to reconcile with us.
     */
    bool IsPeerRegistered(NodeId peer_id) const;

    /**
     * Tells whether a given peer might initiate reconciliations.
     * If the peer was not previously registered for reconciliations, returns nullopt.
     */
    std::optional<bool> IsPeerInitiator(NodeId peer_id) const;

    /**
     * Returns the size of the reconciliation set we have locally for the given peer.
     * If the peer was not previously registered for reconciliations, returns nullopt.
     */
    std::optional<size_t> GetPeerSetSize(NodeId peer_id) const;

    /**
     * Returns whether for the given call the peer is chosen as a low-fanout destination.
     * Remove this peer from the destination list, and add a new peer to the list.
     */
    bool ShouldFloodTo(uint256 wtxid, NodeId peer_id, bool inbound) const;

};

#endif // BITCOIN_TXRECONCILIATION_H
