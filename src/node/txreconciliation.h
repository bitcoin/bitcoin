// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXRECONCILIATION_H
#define BITCOIN_NODE_TXRECONCILIATION_H

#include <net.h>
#include <util/hasher.h>

namespace node {
/** Static salt component used to compute short txids for sketch construction, see BIP-330. */
const std::string RECON_STATIC_SALT = "Tx Relay Salting";
const HashWriter RECON_SALT_HASHER = TaggedHash(RECON_STATIC_SALT);

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

private:
    /** These values are used to salt short IDs, which is necessary for transaction reconciliations. */
    uint64_t m_k0, m_k1;
};
} // namespace node
#endif // BITCOIN_NODE_TXRECONCILIATION_H
