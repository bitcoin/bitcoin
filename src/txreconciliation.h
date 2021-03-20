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

    // Helpers

    /**
     * Removes reconciliation-related state of the peer. After this, we won't be able to reconcile
     * with the peer unless it's registered again (see Step 0).
     */
    void RemovePeer(NodeId peer_id);
};

#endif // BITCOIN_TXRECONCILIATION_H
