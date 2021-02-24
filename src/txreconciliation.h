// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXRECONCILIATION_H
#define BITCOIN_TXRECONCILIATION_H

#include <net.h>
#include <sync.h>

#include <tuple>
#include <unordered_map>

/** Default coefficient used to estimate set difference for tx reconciliation. */
static constexpr double DEFAULT_RECON_Q = 0.02;
/** Used to convert a floating point reconciliation coefficient q to an int for transmission.
  * Specified by BIP-330.
  */
static constexpr uint16_t Q_PRECISION{(2 << 14) - 1};
/**
 * Interval between sending reconciliation request to the same peer.
 * This value allows to reconcile ~100 transactions (7 tx/s * 16s) during normal system operation
 * at capacity. More frequent reconciliations would cause significant constant bandwidth overhead
 * due to reconciliation metadata (sketch sizes etc.), which would nullify the efficiency.
 * Less frequent reconciliations would introduce high transaction relay latency.
 */
static constexpr std::chrono::microseconds RECON_REQUEST_INTERVAL{16s};

/**
 * Represents phase of the current reconciliation round with a peer.
 */
enum ReconciliationPhase {
    RECON_NONE,
    RECON_INIT_REQUESTED,
};

/**
 * This struct is used to keep track of the reconciliations with a given peer,
 * and also short transaction IDs for the next reconciliation round.
 * Transaction reconciliation means an efficient synchronization of the known
 * transactions between a pair of peers.
 * One reconciliation round consists of a sequence of messages. The sequence is
 * asymmetrical, there is always a requestor and a responder. At the end of the
 * sequence, nodes are supposed to exchange transactions, so that both of them
 * have all relevant transactions. For more protocol details, refer to BIP-0330.
 */
class ReconciliationState {
    /** Whether this peer will send reconciliation requests. */
    bool m_requestor;

    /** Whether this peer will respond to reconciliation requests. */
    bool m_responder;

    /**
     * Since reconciliation-only approach makes transaction relay
     * significantly slower, we also announce some of the transactions
     * (currently, transactions received from inbound links)
     * to some of the peers:
     * - all pre-reconciliation peers supporting transaction relay;
     * - a limited number of outbound reconciling peers *for which this flag is enabled*.
     * We enable this flag based on whether we have a
     * sufficient number of outbound transaction relay peers.
     * This flooding makes transaction relay across the network faster
     * without introducing high the bandwidth overhead.
     * Transactions announced via flooding should not be added to
     * the reconciliation set.
     */
    bool m_flood_to;

    /**
     * Reconciliation involves computing and transmitting sketches,
     * which is a bandwidth-efficient representation of transaction IDs.
     * Since computing sketches over full txID is too CPU-expensive,
     * they will be computed over shortened IDs instead.
     * These short IDs will be salted so that they are not the same
     * across all pairs of peers, because otherwise it would enable network-wide
     * collisions which may (intentionally or not) halt relay of certain transactions.
     * Both of the peers contribute to the salt.
     */
    const uint64_t m_k0, m_k1;

    /**
     * Computing a set reconciliation sketch involves estimating the difference
     * between sets of transactions on two sides of the connection. More specifically,
     * a sketch capacity is computed as
     * |set_size - local_set_size| + q * (set_size + local_set_size) + c,
     * where c is a small constant, and q is a node+connection-specific coefficient.
     * This coefficient is recomputed by every node based on its previous reconciliations,
     * to better predict future set size differences.
     */
    double m_local_q;

    /**
     * Store all transactions which we would relay to the peer (policy checks passed, etc.)
     * in this set instead of announcing them right away. When reconciliation time comes, we will
     * compute an efficient representation of this set ("sketch") and use it to efficient reconcile
     * this set with a similar set on the other side of the connection.
     */
    std::set<uint256> m_local_set;

    /** Keep track of reconciliations with the peer. */
    ReconciliationPhase m_outgoing_recon{RECON_NONE};

public:

    ReconciliationState(bool requestor, bool responder, bool flood_to, uint64_t k0, uint64_t k1) :
        m_requestor(requestor), m_responder(responder), m_flood_to(flood_to),
        m_k0(k0), m_k1(k1), m_local_q(DEFAULT_RECON_Q) {}

    bool IsChosenForFlooding() const
    {
        return m_flood_to;
    }

    bool IsResponder() const
    {
        return m_responder;
    }

    uint16_t GetLocalSetSize() const
    {
        return m_local_set.size();
    }

    uint16_t GetLocalQ() const
    {
        return m_local_q * Q_PRECISION;
    }

    ReconciliationPhase GetOutgoingPhase() const
    {
        return m_outgoing_recon;
    }

    void AddToReconSet(const std::vector<uint256>& txs_to_reconcile)
    {
        for (const auto& wtxid: txs_to_reconcile) {
            m_local_set.insert(wtxid);
        }
    }

    void UpdateOutgoingPhase(ReconciliationPhase phase)
    {
        assert(m_responder);
        m_outgoing_recon = phase;
    }
};

/**
 * Used to track reconciliations across all peers.
 */
class TxReconciliationTracker {
    /**
     * Salt used to compute short IDs during transaction reconciliation.
     * Salt is generated randomly per-connection to prevent linking of
     * connections belonging to the same physical node.
     * Also, salts should be different per-connection to prevent halting
     * of relay of particular transactions due to collisions in short IDs.
     */
    Mutex m_local_salts_mutex;
    std::unordered_map<NodeId, uint64_t> m_local_salts GUARDED_BY(m_local_salts_mutex);

    /**
     * Used to keep track of ongoing reconciliations (or lack of them) per peer.
     */
    Mutex m_states_mutex;
    std::unordered_map<NodeId, ReconciliationState> m_states GUARDED_BY(m_states_mutex);

    /**
     * Reconciliation should happen with peers in the same order, because the efficiency gain is the
     * highest when reconciliation set difference is predictable. This queue is used to maintain the
     * order of peers chosen for reconciliation.
     */
    Mutex m_queue_mutex;
    std::deque<NodeId> m_queue GUARDED_BY(m_queue_mutex);

    /**
     * Reconciliations are requested periodically:
     * every RECON_REQUEST_INTERVAL seconds we pick a peer from the queue.
     */
    std::chrono::microseconds m_next_recon_request{0};
    void UpdateNextReconRequest(std::chrono::microseconds now) EXCLUSIVE_LOCKS_REQUIRED(m_queue_mutex)
    {
        m_next_recon_request = now + RECON_REQUEST_INTERVAL / m_queue.size();
    }

    public:

    TxReconciliationTracker() {};

    /**
     * TODO: document
     */
    std::tuple<bool, bool, uint32_t, uint64_t> SuggestReconciling(const NodeId peer_id, bool inbound);

    /**
     * If a peer was previously initiated for reconciliations, get its current reconciliation state.
     * Note that the returned instance is read-only and modifying it won't alter the actual state.
     */
    bool EnableReconciliationSupport(const NodeId peer_id, bool inbound,
        bool recon_requestor, bool recon_responder, uint32_t recon_version, uint64_t remote_salt,
        size_t outbound_flooders);

    /**
     * If a it's time to request a reconciliation from the peer, this function will return the
     * details of our local state, which should be communicated to the peer so that they better
     * know what we need.
     */
    Optional<std::pair<uint16_t, uint16_t>> MaybeRequestReconciliation(const NodeId peer_id);

    Optional<ReconciliationState> GetPeerState(const NodeId peer_id) const
    {
        // This does not compile if this function is marked const. Not sure how to fix this.
        // LOCK(m_states_mutex);
        auto recon_state = m_states.find(peer_id);
        if (recon_state != m_states.end()) {
            return recon_state->second;
        } else {
            return nullopt;
        }
    }

    void StoreTxsToAnnounce(const NodeId peer_id, const std::vector<uint256>& txs_to_reconcile)
    {
        LOCK(m_states_mutex);
        auto recon_state = m_states.find(peer_id);
        assert(recon_state != m_states.end());
        recon_state->second.AddToReconSet(txs_to_reconcile);
    }

    void RemovePeer(const NodeId peer_id)
    {
        LOCK(m_queue_mutex);
        m_queue.erase(std::remove(m_queue.begin(), m_queue.end(), peer_id), m_queue.end());
        LOCK(m_local_salts_mutex);
        m_local_salts.erase(peer_id);
        LOCK(m_states_mutex);
        m_states.erase(peer_id);
    }
};

#endif // BITCOIN_TXRECONCILIATION_H
