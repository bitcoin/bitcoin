// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXRECONCILIATION_H
#define BITCOIN_NODE_TXRECONCILIATION_H

#include <net.h>
#include <sync.h>

#include <memory>
#include <tuple>

/** Supported transaction reconciliation protocol version */
static constexpr uint32_t TXRECONCILIATION_VERSION{1};

enum class ReconciliationRegisterResult {
    NOT_FOUND,
    SUCCESS,
    ALREADY_REGISTERED,
    PROTOCOL_VIOLATION,
};

/**
 * Transaction reconciliation is a way for nodes to efficiently announce transactions.
 * This object keeps track of all txreconciliation-related communications with the peers.
 * The high-level protocol is:
 * 0.  Txreconciliation protocol handshake.
 * 1.  Once we receive a new transaction, add it to the set instead of announcing immediately.
 * 2.  At regular intervals, a txreconciliation initiator requests a sketch from a peer, where a
 *     sketch is a compressed representation of short form IDs of the transactions in their set.
 * 3.  Once the initiator received a sketch from the peer, the initiator computes a local sketch,
 *     and combines the two sketches to attempt finding the difference in *sets*.
 * 4a. If the difference was not larger than estimated, see SUCCESS below.
 * 4b. If the difference was larger than estimated, initial txreconciliation fails. The initiator
 *     requests a larger sketch via an extension round (allowed only once).
 *     - If extension succeeds (a larger sketch is sufficient), see SUCCESS below.
 *     - If extension fails (a larger sketch is insufficient), see FAILURE below.
 *
 * SUCCESS. The initiator knows full symmetrical difference and can request what the initiator is
 *          missing and announce to the peer what the peer is missing.
 *
 * FAILURE. The initiator notifies the peer about the failure and announces all transactions from
 *          the corresponding set. Once the peer received the failure notification, the peer
 *          announces all transactions from their set.

 * This is a modification of the Erlay protocol (https://arxiv.org/abs/1905.10518) with two
 * changes (sketch extensions instead of bisections, and an extra INV exchange round), both
 * are motivated in BIP-330.
 */
class TxReconciliationTracker
{
private:
    class Impl;
    const std::unique_ptr<Impl> m_impl;

public:
    explicit TxReconciliationTracker(uint32_t recon_version);
    ~TxReconciliationTracker();

    /**
     * Step 0. Generates initial part of the state (salt) required to reconcile txs with the peer.
     * The salt is used for short ID computation required for txreconciliation.
     * The function returns the salt.
     * A peer can't participate in future txreconciliations without this call.
     * This function must be called only once per peer.
     */
    uint64_t PreRegisterPeer(NodeId peer_id);

    /**
     * Step 0. Once the peer agreed to reconcile txs with us, generate the state required to track
     * ongoing reconciliations. Must be called only after pre-registering the peer and only once.
     */
    ReconciliationRegisterResult RegisterPeer(NodeId peer_id, bool is_peer_inbound,
                                              uint32_t peer_recon_version, uint64_t remote_salt);

    /**
     * Attempts to forget txreconciliation-related state of the peer (if we previously stored any).
     * After this, we won't be able to reconcile transactions with the peer.
     */
    void ForgetPeer(NodeId peer_id);

    /**
     * Check if a peer is registered to reconcile transactions with us.
     */
    bool IsPeerRegistered(NodeId peer_id) const;
};

#endif // BITCOIN_NODE_TXRECONCILIATION_H
