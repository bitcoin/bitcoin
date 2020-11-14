// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>
#include <minisketch/include/minisketch.h>

#include <chrono>
#include <cmath>
#include <set>
#include <string>
#include <vector>

/** Static component of the salt used to compute short txids for transaction reconciliation. */
static const std::string RECON_STATIC_SALT = "Tx Relay Salting";
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
 * Interval between responding to peers' reconciliation requests.
 * We don't respond to reconciliation requests right away because that would enable monitoring
 * when we receive transactions (privacy leak).
 */
static constexpr std::chrono::microseconds RECON_RESPONSE_INTERVAL{2s};
/** The size of the field, used to compute sketches to reconcile transactions (see BIP-330). */
static constexpr unsigned int RECON_FIELD_SIZE = 32;
/** Limit sketch capacity to avoid DoS. */
static constexpr uint16_t MAX_SKETCH_CAPACITY = 2 << 12;
/**
* It is possible that if sketch encodes more elements than the capacity, or
* if it is constructed of random bytes, sketch decoding may "succeed",
* but the result will be nonsense (false-positive decoding).
* Given this coef, a false positive probability will be of 1 in 2**coef.
*/
static constexpr unsigned int RECON_FALSE_POSITIVE_COEF = 16;
static_assert(RECON_FALSE_POSITIVE_COEF <= 256,
    "Reducing reconciliation false positives beyond 1 in 2**256 is not supported");

/**
 * Used to keep track of the current reconciliation round with a peer.
 * Used for both inbound (responded) and outgoing (requested/initiated) reconciliations.
 */
enum ReconPhase {
    RECON_NONE,
    RECON_INIT_REQUESTED,
    RECON_INIT_RESPONDED,
    RECON_EXT_REQUESTED,
    RECON_EXT_RESPONDED,
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
class ReconState {
    /** Default coefficient used to estimate set difference for tx reconciliation. */
    static constexpr double DEFAULT_RECON_Q = 0.02;

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

    /**
     * Reconciliation sketches are computed over short transaction IDs.
     * This is a cache of these IDs enabling faster lookups of full wtxids,
     * useful when peer will ask for missing transactions by short IDs
     * at the end of a reconciliation round.
     */
    std::map<uint32_t, uint256> m_local_short_id_mapping;

    /**
     * A reconciliation request comes from a peer with a reconciliation set size from their side,
     * which is supposed to help us to estimate set difference size. The value is stored here until
     * we respond to that request with a sketch.
     */
    uint16_t m_remote_set_size;

    /**
     * The use of q coefficients is described above (see local_q comment).
     * The value transmitted from the peer with a reconciliation requests is stored here until
     * we respond to that request with a sketch.
     */
    double m_remote_q;

    /**
     * When a reconciliation request is received, instead of responding to it right away,
     * we schedule a response for later, so that a spy can’t monitor our reconciliation sets.
     */
    std::chrono::microseconds m_next_recon_respond{0};

    /** Keep track of reconciliations with the peer. */
    ReconPhase m_outgoing_recon{RECON_NONE};
    ReconPhase m_incoming_recon{RECON_NONE};

    /**
     * Reconciliation sketches are computed over short transaction IDs.
     * Short IDs are salted with a link-specific constant value.
     */
    uint32_t ComputeShortID(const uint256 wtxid) const
    {
        uint64_t s = SipHashUint256(m_k0, m_k1, wtxid);
        uint32_t short_txid = 1 + (s & 0xFFFFFFFF);
        return short_txid;
    }

public:

    ReconState(bool requestor, bool responder, bool flood_to, uint64_t k0, uint64_t k1) :
        m_requestor(requestor), m_responder(responder), m_flood_to(flood_to),
        m_k0(k0), m_k1(k1), m_local_q(DEFAULT_RECON_Q) {}

    bool IsChosenForFlooding() const
    {
        return m_flood_to;
    }

    bool IsRequestor() const
    {
        return m_requestor;
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

    ReconPhase GetIncomingPhase() const
    {
        return m_incoming_recon;
    }

    ReconPhase GetOutgoingPhase() const
    {
        return m_outgoing_recon;
    }

    void UpdateIncomingPhase(ReconPhase phase)
    {
        m_incoming_recon = phase;
        if (phase == RECON_INIT_RESPONDED) m_local_set.clear();
    }

    void UpdateOutgoingPhase(ReconPhase phase)
    {
        m_outgoing_recon = phase;
    }

    std::vector<uint256> AddToReconSet(std::vector<uint256> txs_to_reconcile, uint32_t limit)
    {
        std::vector<uint256> remaining_txs;
        int32_t recon_set_overflow = m_local_set.size() + txs_to_reconcile.size() - limit;
        if (recon_set_overflow > 0) {
            remaining_txs = std::vector<uint256>(txs_to_reconcile.end() - recon_set_overflow, txs_to_reconcile.end());
            txs_to_reconcile.resize(txs_to_reconcile.size() - recon_set_overflow);
        }

        for (const auto& wtxid: txs_to_reconcile) {
            m_local_set.insert(wtxid);
        }

        return remaining_txs;
    }

    bool UpdateRemoteState(uint16_t remote_set_size, uint16_t remote_q)
    {
        double updated_q = double(remote_q * Q_PRECISION);
        if (updated_q < 0 || updated_q > 2) return false;
        m_remote_q = updated_q;
        m_remote_set_size = remote_set_size;
        return true;
    }

    std::chrono::microseconds GetNextRespond()
    {
        return m_next_recon_respond;
    }

    void UpdateNextReconRespond(std::chrono::microseconds next_recon_respond)
    {
        m_next_recon_respond = next_recon_respond;
    }

    /**
     * Estimate a capacity of a sketch we will send or use locally (to find set difference)
     * based on the local set size.
     */
    uint16_t EstimateSketchCapacity() const
    {
        uint16_t set_size_diff = std::abs(uint16_t(m_local_set.size()) - m_remote_set_size);
        uint16_t min_size = std::min(uint16_t(m_local_set.size()), m_remote_set_size);
        uint16_t weighted_min_size = m_remote_q * min_size;
        uint16_t estimated_diff = 1 + weighted_min_size + set_size_diff;
        return minisketch_compute_capacity(RECON_FIELD_SIZE, estimated_diff, RECON_FALSE_POSITIVE_COEF);
    }

    /**
     * Reconciliation involves computing a space-efficient representation of transaction identifiers
     * (a sketch). A sketch has a capacity meaning it allows reconciling at most a certain number
     * of elements (see BIP-330). Considering whether we are going to send a sketch to a peer or use
     * locally, we estimate the set difference.
     */
    Minisketch ComputeSketch(uint16_t& capacity)
    {
        Minisketch sketch;
        // Avoid serializing/sending an empty sketch.
        if (m_local_set.size() == 0 || capacity == 0) return sketch;

        std::vector<uint32_t> short_ids;
        for (const auto& wtxid: m_local_set) {
            uint32_t short_txid = ComputeShortID(wtxid);
            short_ids.push_back(short_txid);
            m_local_short_id_mapping.emplace(short_txid, wtxid);
        }

        capacity = std::min(capacity, MAX_SKETCH_CAPACITY);
        sketch = Minisketch(RECON_FIELD_SIZE, 0, capacity);
        if (sketch) {
            for (const uint32_t short_id: short_ids) {
                sketch.Add(short_id);
            }
        }
        return sketch;
    }
};
