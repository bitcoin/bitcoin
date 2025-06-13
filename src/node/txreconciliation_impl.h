// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXRECONCILIATION_IMPL_H
#define BITCOIN_NODE_TXRECONCILIATION_IMPL_H

#include <node/txreconciliation.h>

namespace node {
class TxReconciliationTrackerImpl;

/** Supported transaction reconciliation protocol version */
static constexpr uint32_t TXRECONCILIATION_VERSION{1};

/**
 * Allows to infer capacity of a reconciliation sketch based on it's char[] representation,
 * which is necessary to deserialize a received sketch.
 */
constexpr unsigned int BYTES_PER_SKETCH_CAPACITY = RECON_FIELD_SIZE / 8;

/**
 * Limit sketch capacity to avoid DoS. This applies only to the original sketches,
 * and implies that extended sketches could be at most twice the size.
 */
constexpr uint32_t MAX_SKETCH_CAPACITY = 2 << 12;

/**
 * It is possible that if sketch encodes more elements than the capacity, or
 * if it is constructed of random bytes, sketch decoding may "succeed",
 * but the result will be nonsense (false-positive decoding).
 * Given this coef, a false positive probability will be of 1 in 2**coef.
 */
constexpr unsigned int RECON_FALSE_POSITIVE_COEF = 16;
static_assert(RECON_FALSE_POSITIVE_COEF <= 256, "Reducing reconciliation false positives beyond 1 in 2**256 is not supported");

/**
 * Maximum number of wtxids stored in a peer local set, bounded to protect the memory use of
 * reconciliation sets and short ids mappings, and CPU used for sketch computation.
 */
constexpr size_t MAX_RECONSET_SIZE = 3000;

/**
 * Announce transactions via full wtxid to a limited number of inbound and outbound peers.
 * Justification for these values are provided here:
 * TODO: ADD link to justification based on simulation results */
constexpr double INBOUND_FANOUT_DESTINATIONS_FRACTION = 0.1;
constexpr size_t OUTBOUND_FANOUT_THRESHOLD = 4;

/** Interval for inbound peer fanout selection. The subset is rotated on a timer. */
static constexpr auto INBOUND_FANOUT_ROTATION_INTERVAL{10min};

/**
 * Interval between initiating reconciliations with peers.
 * This value allows to reconcile ~(7 tx/s * 8s) transactions during normal operation.
 * More frequent reconciliations would cause significant constant bandwidth overhead
 * due to reconciliation metadata (sketch sizes etc.), which would nullify the efficiency.
 * Less frequent reconciliations would introduce high transaction relay latency.
 */
constexpr std::chrono::microseconds RECON_REQUEST_INTERVAL{8s};

enum class ReconciliationError
{
    NOT_FOUND,
    ALREADY_REGISTERED,
    WRONG_PHASE,
    WRONG_ROLE,
    FULL_RECON_SET,
    SHORTID_COLLISION,
    PROTOCOL_VIOLATION,
};

class AddToSetError {
    public:
        ReconciliationError m_error;
        std::optional<Wtxid> m_collision;

        Wtxid GetCollision() const
        {
            Assume(m_collision.has_value());
            return m_collision.value();
        }

        explicit AddToSetError(ReconciliationError error): m_error(error), m_collision(std::nullopt) {}
        explicit AddToSetError(ReconciliationError error, Wtxid collision): m_error(error), m_collision(std::optional(collision)) {}
};

class HandleSketchResult
{
    public:
        std::vector<uint32_t> m_txs_to_request;
        std::vector<Wtxid> m_txs_to_announce;
        std::optional<bool> m_succeeded;

        explicit HandleSketchResult(std::vector<uint32_t> txs_to_request, std::vector<Wtxid> txs_to_announce, std::optional<bool> succeeded): m_txs_to_request(txs_to_request), m_txs_to_announce(txs_to_announce), m_succeeded(succeeded) {};
};

using ReconCoefficients = std::pair<uint16_t, uint16_t>;

/**
 * Transaction reconciliation is a way for nodes to efficiently announce transactions.
 * This class keeps track of all txreconciliation-related communications with the peers.
 * The high-level protocol is:
 * 0.  txreconciliation protocol handshake.
 * 1.  Once we receive a new transaction, add it to the set instead of announcing immediately.
 * 2.  At regular intervals, a txreconciliation initiator requests a sketch from a peer, where a
 *     sketch is a compressed representation of short form IDs of the transactions in their set.
 * 3.  Once the initiator received a sketch from the peer, the initiator computes a local sketch,
 *     and combines the two sketches to attempt finding the difference in *sets*.
 * 4.  If the difference was not larger than estimated, see SUCCESS below.
 * 5.  If the difference was larger than estimated, initial txreconciliation fails. The initiator
 *     requests a larger sketch via an extension round (allowed only once).
 *      - If extension succeeds (a larger sketch is sufficient), see SUCCESS below.
 *      - If extension fails (a larger sketch is insufficient), see FAILURE below.
 *
 * SUCCESS. The initiator knows the full symmetrical difference and can request what the initiator
 *          is missing and announces to the peer what the peer is missing.
 *
 * FAILURE. The initiator notifies the peer about the failure and announces all transactions from
 *          the corresponding set. Once the peer received the failure notification, the peer
 *          announces all transactions from their set.

 * This is a modification of the Erlay protocol (https://arxiv.org/abs/1905.10518) with two
 * changes (sketch extensions instead of bisections, and an extra INV exchange round), both
 * are motivated in BIP-330.
 */
class TxReconciliationTracker {
    const std::unique_ptr<TxReconciliationTrackerImpl> m_impl;

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

    /** For testing purposes only. This SHOULD NEVER be used in production. */
    void PreRegisterPeerWithSalt(NodeId peer_id, uint64_t local_salt);

    /**
     * Step 0. Once the peer agreed to reconcile txs with us, generate the state required to track
     * ongoing reconciliations. Must be called only after pre-registering.
     * Returns a error if the registering proccess fails for any reason, nullopt otherwise.
     */
    std::optional<ReconciliationError> RegisterPeer(NodeId peer_id, bool is_peer_inbound, uint32_t peer_recon_version, uint64_t remote_salt);

    /** Check if a peer is registered to reconcile transactions with us. */
    bool IsPeerRegistered(NodeId peer_id) const;

    /**
     * Returns whether it's time to initiate reconciliation (Step 2) with a given peer, based on:
     * - time passed since the last reconciliation
     * - reconciliation queue
     * - whether previous reconciliations for the given peer were finalized
     */
    bool IsPeerNextToReconcileWith(NodeId peer_id, std::chrono::microseconds now);

    /**
     * Attempts to forget txreconciliation-related state of the peer (if we previously stored any).
     * After this, we won't be able to reconcile transactions with the peer.
     */
    void ForgetPeer(NodeId peer_id);

    /**
     * Step 1. Add a to-be-announced transaction to the local reconciliation set of the target peer
     * so that it can be reconciled later.
     * Returns a error if the wtxid cannot be added to the set, nullopt otherwise.
     */
    std::optional<AddToSetError> AddToSet(NodeId peer_id, const Wtxid& wtxid);

    /**
     * Before Step 2, we might want to remove a wtxid from the reconciliation set, for example if
     * the peer just announced the transaction to us. Returns whether the wtxid was removed.
     */
    bool TryRemovingFromSet(NodeId peer_id, const Wtxid& wtxid);

    /**
     * Adds a collection of transactions (identified by short_id) to m_recently_requested_short_ids.
     * This should be called with the short IDs of the transaction being requested to a peer when sending out a RECONCILDIFF.
     */
    void TrackRecentlyRequestedTransactions(std::vector<uint32_t>& requested_txs);

    /**  Checks whether a given transaction was requested by us to any of our Erlay outbound peers (during RECONCILDIFF). */
    bool WasTransactionRecentlyRequested(const Wtxid& wtxid);

    /**
     * Step 2. Unless the peer hasn't finished a previous reconciliation round, this function will
     * return the details of our local state, which should be communicated to the peer so that they
     * better know what we need:
     * - size of our reconciliation set for the peer
     * - our q-coefficient with the peer, formatted to be transmitted as integer value
     */
    std::variant<ReconCoefficients, ReconciliationError> InitiateReconciliationRequest(NodeId peer_id);

    /**
     * Step 2. Record a reconciliation request with parameters to respond when its time.
     * If peer violates the protocol, retuns an error so we can disconnect.
     */
    std::optional<ReconciliationError> HandleReconciliationRequest(NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q);

    /**
     * Step 2. Once it's time to respond to reconciliation requests, we construct a sketch from
     * the local reconciliation set, and send it to the initiator.
     * If the peer was not previously registered for reconciliations or the peers didn't request
     * to reconcile with us, return false.
     * For the initial reconciliation response (no extension phase), we only respond if the timer for
     * this peer has ticked (send_trickle is true) to prevent announcing things too early.
     */
    bool ShouldRespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata, bool send_trickle);

    /**
     * Step 3. Process a response to our reconciliation request.
     * Returns an error if the peer seems to violate the protocol.
     * Returns a stucture containing data to request and announce, and a flag signaling
     * whether reconciliation succeeded, failed, or we need to request an extension.
     */
    std::variant<HandleSketchResult, ReconciliationError> HandleSketch(NodeId peer_id, const std::vector<uint8_t>& skdata);

    /** Whether a given inbound peer is currently flagged for fanout. */
    bool IsInboundFanoutTarget(NodeId peer_id);

    /** Picks a different subset of inbound peers to fanout to. */
    void RotateInboundFanoutTargets();

    /** Get the next time the inbound peer subset should be rotated. */
    std::chrono::microseconds GetNextInboundPeerRotationTime();

    /** Update the next inbound peer rotation time. */
    void SetNextInboundPeerRotationTime(std::chrono::microseconds next_time);
};
} // namespace node
#endif // BITCOIN_NODE_TXRECONCILIATION_IMPL_H
