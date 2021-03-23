// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXRECONCILIATION_H
#define BITCOIN_NODE_TXRECONCILIATION_H

#include <net.h>
#include <util/hasher.h>

#include <node/minisketchwrapper.h>

namespace node {
/**
 * A floating point coefficient q for estimating reconciliation set difference, and
 * the value used to convert it to integer for transmission purposes, as specified in BIP-330.
 */
constexpr double Q = 0.25;
constexpr uint16_t Q_PRECISION{(2 << 14) - 1};

/** The size of the field, used to compute sketches to reconcile transactions (see BIP-330). */
constexpr unsigned int RECON_FIELD_SIZE = 32;

/** Static salt component used to compute short txids for sketch construction, see BIP-330. */
const std::string RECON_STATIC_SALT = "Tx Relay Salting";
const HashWriter RECON_SALT_HASHER = TaggedHash(RECON_STATIC_SALT);

/** Represents phase of the current reconciliation round with a peer. */
enum class ReconciliationPhase
{
    NONE,
    INIT_REQUESTED,
    INIT_RESPONDED,
    EXT_REQUESTED,
};

/**
 * Salt (specified by BIP-330) constructed from contributions from both peers. It is used
 * to compute transaction short IDs, which are then used to construct a sketch representing a set
 * of transactions we want to announce to the peer.
 */
inline uint256 ComputeSalt(uint64_t salt1, uint64_t salt2)
{
    // According to BIP-330, salts should be combined in ascending order.
    return (HashWriter(RECON_SALT_HASHER) << std::min(salt1, salt2) << std::max(salt1, salt2)).GetSHA256();
}

/** Keeps track of txreconciliation-related per-peer state. */
class TxReconciliationState
{
public:
    /**
     * Reconciliation protocol assumes using one role consistently: either a reconciliation
     * initiator (requesting sketches), or responder (sending sketches). This defines our role,
     * based on the direction of the p2p connection.
     */
    bool m_we_initiate;

    /** Keep track of the reconciliation phase with the peer. */
    ReconciliationPhase m_phase{ReconciliationPhase::NONE};

    /**
     * Store all wtxids that we would announce to the peer (policy checks passed, etc.)
     * in this set instead of announcing them right away. When reconciliation time comes, we will
     * compute a compressed representation of this set (a "sketch") and use it to efficiently
     * reconcile this set with a set on the peer's side.
     */
    std::unordered_set<Wtxid, SaltedWtxidHasher> m_local_set;

    /**
     * Reconciliation sketches are computed over short transaction IDs.
     * This is a cache of these IDs enabling faster lookups of full wtxids,
     * useful when the peer asks for missing transactions by short IDs
     * at the end of a reconciliation round.
     * We also use this to keep track of short ID collisions. In case of a
     * collision, both transactions should be fanout.
     */
    std::map<uint32_t, Wtxid> m_short_id_mapping;

    /**
     * A reconciliation round may involve an extension, which is an extra exchange of messages.
     * Since it may happen after a delay (at least network latency), new transactions may come
     * during that time. To avoid mixing old and new transactions, those which are subject for
     * extension of a current reconciliation round are moved to a reconciliation set snapshot
     * after an initial (non-extended) sketch is sent.
     * New transactions are kept in the regular reconciliation set.
     */
    std::unordered_set<Wtxid, SaltedWtxidHasher> m_local_set_snapshot;

    /** Same as non-snapshot set above, but for the transactions in the snapshot. */
    std::map<uint32_t, Wtxid> m_short_id_mapping_snapshot;

    /**
     * A reconciliation round may involve an extension, in which case we should remember
     * a capacity of the sketch sent out initially, so that a sketch extension is of the same size.
     */
    uint16_t m_capacity_snapshot{0};

    /**
     * In a reconciliation round initiated by us, if we asked for an extension, we want to store
     * the sketch computed/transmitted in the initial step, so that we can use it when sketch extension arrives.
     */
    std::vector<uint8_t> m_remote_sketch_snapshot;

    /**
     * The following fields are specific to only reconciliations initiated by the peer.
     */

    /**
     * The value transmitted from the peer with a reconciliation requests is stored here until
     * we respond to that request with a sketch.
     */
    double m_remote_q;

    /**
     * A reconciliation request comes from a peer with a reconciliation set size from their side,
     * which is supposed to help us to estimate set difference size. The value is stored here until
     * we respond to that request with a sketch.
     */
    uint16_t m_remote_set_size;

    TxReconciliationState(bool we_initiate, uint64_t k0, uint64_t k1) : m_we_initiate(we_initiate), m_k0(k0), m_k1(k1) {}

    /**
     * Reconciliation sketches are computed over short transaction IDs.
     * Short IDs are salted with a link-specific constant value.
     */
    uint32_t ComputeShortID(const Wtxid& wtxid) const;

    /**
     * Check whether a given wtxid has a short ID collision with an existing transaction in the peer's reconciliation state.
     * If a collision is found, sets collision to the wtxid of the conflicting transaction.
    */
    bool HasCollision(const Wtxid& wtxid, Wtxid& collision, uint32_t &short_id);

    /**
     * Estimate a capacity of a sketch we will send or use locally (to find set difference) based on the local set size.
     */
    uint32_t EstimateSketchCapacity(size_t local_set_size) const;

    /**
     * Reconciliation involves computing a space-efficient representation of transaction identifiers (a sketch).
     * A sketch has a capacity meaning it allows reconciling at most a certain number of elements (see BIP-330).
     */
    Minisketch ComputeBaseSketch(uint32_t& capacity);

    /**
     * When our peer tells us that our sketch was insufficient to reconcile transactions because
     * of the low capacity, we compute an extended sketch with the double capacity, and then send
     * only missing part to that peer.
     */
    Minisketch ComputeExtendedSketch(uint32_t extended_capacity);

    /**
     * Creates a snapshot of the peer local set (m_local_set and m_short_id_mapping) and clears
     * it. This is useful for both sides of the reconciliation when preparing for an extension
     * (request or response) so the current data can be persisted, and any additional data that
     * enters the sets is not lost after the current reconciliation flow is concluded.
     */
    void SnapshotLocalSet();

    /**
     * To be efficient in transmitting extended sketch, we store a snapshot of the sketch
     * received in the initial reconciliation step, so that only the necessary extension data
     * has to be transmitted.
     * We also store a snapshot of our local reconciliation set, to better keep track of
     * transactions arriving during this reconciliation (they will be added to the cleared
     * original reconciliation set, to be reconciled next time).
     */
    void PrepareForExtensionResponse(const std::vector<uint8_t>& remote_sketch);

    /**
     * Get all the transactions we have stored for this peer.
     */
    std::vector<Wtxid> GetAllTransactions() const;

    /**
     * When during reconciliation we find a set difference successfully (by combining sketches),
     * we want to find which transactions are missing on our and on their side.
     * For those missing on our side, we may only find short IDs.
     */
    std::pair<std::vector<uint32_t>, std::vector<Wtxid>> GetRelevantIDsFromShortIDs(const std::vector<uint64_t>& diff) const;

private:
    /** These values are used to salt short IDs, which is necessary for transaction reconciliations. */
    uint64_t m_k0, m_k1;
};
} // namespace node
#endif // BITCOIN_NODE_TXRECONCILIATION_H
