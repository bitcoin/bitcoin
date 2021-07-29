// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txreconciliation.h>

#include <common/system.h>
#include <logging.h>
#include <node/minisketchwrapper.h>
#include <util/check.h>
#include <util/hasher.h>

#include <cmath>
#include <unordered_map>
#include <variant>


namespace {

/** Static salt component used to compute short txids for sketch construction, see BIP-330. */
const std::string RECON_STATIC_SALT = "Tx Relay Salting";
const HashWriter RECON_SALT_HASHER = TaggedHash(RECON_STATIC_SALT);
/**
 * Announce transactions via full wtxid to a limited number of inbound and outbound peers.
 * Justification for these values are provided here:
 * https://github.com/naumenkogs/txrelaysim/issues/7#issuecomment-902165806 */
constexpr double INBOUND_FANOUT_DESTINATIONS_FRACTION = 0.1;
constexpr size_t OUTBOUND_FANOUT_DESTINATIONS = 1;
constexpr size_t MAX_SET_SIZE = 3000;
/** The size of the field, used to compute sketches to reconcile transactions (see BIP-330). */
constexpr unsigned int RECON_FIELD_SIZE = 32;
/**
 * Allows to infer capacity of a reconciliation sketch based on it's char[] representation,
 * which is necessary to deserealize a received sketch.
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
static_assert(RECON_FALSE_POSITIVE_COEF <= 256,
              "Reducing reconciliation false positives beyond 1 in 2**256 is not supported");
/**
 * A floating point coefficient q for estimating reconciliation set difference, and
 * the value used to convert it to integer for transmission purposes, as specified in BIP-330.
 */
constexpr double Q = 0.25;
constexpr uint16_t Q_PRECISION{(2 << 14) - 1};
/**
 * Interval between initiating reconciliations with peers.
 * This value allows to reconcile ~(7 tx/s * 8s) transactions during normal operation.
 * More frequent reconciliations would cause significant constant bandwidth overhead
 * due to reconciliation metadata (sketch sizes etc.), which would nullify the efficiency.
 * Less frequent reconciliations would introduce high transaction relay latency.
 */
constexpr std::chrono::microseconds RECON_REQUEST_INTERVAL{8s};
/**
 * We should keep an interval between responding to reconciliation requests from the same peer,
 * to reduce potential DoS surface.
 */
constexpr std::chrono::microseconds RECON_RESPONSE_INTERVAL{1s};

/**
 * Represents phase of the current reconciliation round with a peer.
 */
enum class Phase {
    NONE,
    INIT_REQUESTED,
    INIT_RESPONDED,
    EXT_REQUESTED
};

/**
 * Salt (specified by BIP-330) constructed from contributions from both peers. It is used
 * to compute transaction short IDs, which are then used to construct a sketch representing a set
 * of transactions we want to announce to the peer.
 */
uint256 ComputeSalt(uint64_t salt1, uint64_t salt2)
{
    // According to BIP-330, salts should be combined in ascending order.
    return (HashWriter(RECON_SALT_HASHER) << std::min(salt1, salt2) << std::max(salt1, salt2)).GetSHA256();
}

/**
 * Keeps track of txreconciliation-related per-peer state.
 */
class TxReconciliationState
{
public:
    /**
     * These values are used to salt short IDs, which is necessary for transaction reconciliations.
     */
    uint64_t m_k0, m_k1;

    /**
     * A reconciliation round may involve an extension, in which case we should remember
     * a capacity of the sketch sent out initially, so that a sketch extension is of the same size.
     */
    uint16_t m_capacity_snapshot{0};

    /**
     * TODO: This field is public to ignore -Wunused-private-field. Make private once used in
     * the following commits.
     *
     * Reconciliation protocol assumes using one role consistently: either a reconciliation
     * initiator (requesting sketches), or responder (sending sketches). This defines our role,
     * based on the direction of the p2p connection.
     *
     */
    bool m_we_initiate;

    /**
     * Store all wtxids which we would announce to the peer (policy checks passed, etc.)
     * in this set instead of announcing them right away. When reconciliation time comes, we will
     * compute a compressed representation of this set ("sketch") and use it to efficiently
     * reconcile this set with a set on the peer's side.
     */
    std::set<uint256> m_local_set;

    /**
     * Reconciliation sketches are computed over short transaction IDs.
     * This is a cache of these IDs enabling faster lookups of full wtxids,
     * useful when peer will ask for missing transactions by short IDs
     * at the end of a reconciliation round.
     */
    std::map<uint32_t, uint256> m_short_id_mapping;

    /**
     * A reconciliation round may involve an extension, which is an extra exchange of messages.
     * Since it may happen after a delay (at least network latency), new transactions may come
     * during that time. To avoid mixing old and new transactions, those which are subject for
     * extension of a current reconciliation round are moved to a reconciliation set snapshot
     * after an initial (non-extended) sketch is sent.
     * New transactions are kept in the regular reconciliation set.
     */
    std::set<uint256> m_local_set_snapshot;
    /** Same as non-snapshot set above */
    std::map<uint32_t, uint256> m_snapshot_short_id_mapping;

    /**
     * A peer could announce a transaction to us during reconciliation and after we snapshoted
     * the initial set. We can't remove this new transaction from the snapshot, because
     * then we won't be able to compute a valid extension (for the sketch already transmitted).
     * Instead, we just remember those transaction, and not announce them when we announce
     * stuff from the snapshot.
     */
    std::set<uint256> m_announced_during_extension;

    /** Keep track of the reconciliation phase with the peer. */
    Phase m_phase{Phase::NONE};

    TxReconciliationState(bool we_initiate, uint64_t k0, uint64_t k1) : m_k0(k0), m_k1(k1), m_we_initiate(we_initiate) {}

    /**
     * Reconciliation sketches are computed over short transaction IDs.
     * Short IDs are salted with a link-specific constant value.
     */
    uint32_t ComputeShortID(const uint256 wtxid) const
    {
        const uint64_t s = SipHashUint256(m_k0, m_k1, wtxid);
        const uint32_t short_txid = 1 + (s & 0xFFFFFFFF);
        return short_txid;
    }

    /**
     * Estimate a capacity of a sketch we will send or use locally (to find set difference)
     * based on the local set size.
     */
    uint32_t EstimateSketchCapacity(size_t local_set_size) const
    {
        const uint16_t set_size_diff = std::abs(uint16_t(local_set_size) - m_remote_set_size);
        const uint16_t min_size = std::min(uint16_t(local_set_size), m_remote_set_size);
        const uint16_t weighted_min_size = m_remote_q * min_size;
        const uint32_t estimated_diff = 1 + weighted_min_size + set_size_diff;
        return minisketch_compute_capacity(RECON_FIELD_SIZE, estimated_diff, RECON_FALSE_POSITIVE_COEF);
    }

    /**
     * Reconciliation involves computing a space-efficient representation of transaction identifiers
     * (a sketch). A sketch has a capacity meaning it allows reconciling at most a certain number
     * of elements (see BIP-330).
     */
    Minisketch ComputeBaseSketch(uint32_t& capacity)
    {
        Minisketch sketch;
        // Avoid serializing/sending an empty sketch.
        if (capacity == 0) return sketch;

        // To be used for sketch extension of the exact same size.
        m_capacity_snapshot = capacity;

        capacity = std::min(capacity, MAX_SKETCH_CAPACITY);
        sketch = node::MakeMinisketch32(capacity);

        for (const auto& wtxid : m_local_set) {
            uint32_t short_txid = ComputeShortID(wtxid);
            sketch.Add(short_txid);
            m_short_id_mapping.emplace(short_txid, wtxid);
        }

        return sketch;
    }

    /**
     * When our peer tells us that our sketch was insufficient to reconcile transactions because
     * of the low capacity, we compute an extended sketch with the double capacity, and then send
     * only the part the peer is missing to that peer.
     */
    Minisketch ComputeExtendedSketch(uint32_t extended_capacity)
    {
        assert(extended_capacity > 0);
        // This can't happen because we should have terminated reconciliation early.
        assert(m_local_set_snapshot.size() > 0);

        // For now, compute a sketch of twice the capacity were computed originally.
        // TODO: optimize by computing the extension *on top* of the existent sketch
        // instead of computing the lower order elements again.
        Minisketch sketch = Minisketch(RECON_FIELD_SIZE, 0, extended_capacity);

        // We don't have to recompute short IDs here.
        for (const auto& shortid_to_wtxid : m_snapshot_short_id_mapping) {
            sketch.Add(shortid_to_wtxid.first);
        }
        return sketch;
    }

    std::vector<uint256> GetAllTransactions() const
    {
        return std::vector<uint256>(m_local_set.begin(), m_local_set.end());
    }

    /**
     * Once we are fully done with the reconciliation we initiated, prepare the state for the
     * following reconciliations we initiate.
     */
    void FinalizeInitByUs(bool clear_local_set)
    {
        assert(m_we_initiate);
        if (clear_local_set) {
            m_short_id_mapping.clear();
            m_local_set.clear();
        }
        m_local_set_snapshot.clear();
        m_announced_during_extension.clear();
        // This is currently belt-and-suspenders, as the code should work even without these calls.
        m_capacity_snapshot = 0;
        m_remote_sketch_snapshot.clear();
        m_snapshot_short_id_mapping.clear();
    }

    /**
     * When during reconciliation we find a set difference successfully (by combining sketches),
     * we want to find which transactions are missing on our and on their side.
     * For those missing on our side, we may only find short IDs.
     */
    void GetRelevantIDsFromShortIDs(const std::vector<uint64_t>& diff,
        // returning values
        std::vector<uint32_t>& local_missing, std::vector<uint256>& remote_missing) const
    {
        for (const auto& diff_short_id: diff) {
            const auto local_tx = m_short_id_mapping.find(diff_short_id);
            if (local_tx != m_short_id_mapping.end()) {
                remote_missing.push_back(local_tx->second);
            } else {
                local_missing.push_back(diff_short_id);
            }
        }
    }

    /**
     * In a reconciliation round initiated by us, if we asked for an extension, we want to store
     * the sketch computed/transmitted in the initial step, so that we can use it when
     * sketch extension arrives.
     */
    std::vector<uint8_t> m_remote_sketch_snapshot;

    /**
     * The following fields are specific to only reconciliations initiated by the peer.
     */

    /**
     * The use of q coefficients is described above (see local_q comment).
     * The value transmitted from the peer with a reconciliation requests is stored here until
     * we respond to that request with a sketch.
     */
    double m_remote_q{Q};

    /**
     * A reconciliation request comes from a peer with a reconciliation set size from their side,
     * which is supposed to help us to estimate set difference size. The value is stored here until
     * we respond to that request with a sketch.
     */
    uint16_t m_remote_set_size;

    /**
     * We track when was the last time we responded to a reconciliation request by the peer,
     * so that we don't respond to them too often. This helps to reduce DoS surface.
     */
    std::chrono::microseconds m_last_init_recon_respond{0};
    /**
     * Returns whether at this time it's not too early to respond to a reconciliation request by
     * the peer, and, if so, bumps the time we last responded to allow further checks.
     */
    bool ConsiderInitResponseAndTrack()
    {
        auto current_time = GetTime<std::chrono::seconds>();
        if (m_last_init_recon_respond <= current_time - RECON_RESPONSE_INTERVAL) {
            m_last_init_recon_respond = current_time;
            return true;
        }
        return false;
    }

    /**
     * Be ready to respond to extension request, to compute the extended sketch over
     * the same initial set (without transactions received during the reconciliation).
     * Allow to store new transactions separately in the original set.
     */
    void PrepareForExtensionRequest(uint16_t sketch_capacity)
    {
        assert(!m_we_initiate);
        m_capacity_snapshot = sketch_capacity;
        m_local_set_snapshot = m_local_set;
        m_snapshot_short_id_mapping = m_short_id_mapping;
        m_local_set.clear();
        m_short_id_mapping.clear();
    }

    /**
     * To be efficient in transmitting extended sketch, we store a snapshot of the sketch
     * received in the initial reconciliation step, so that only the necessary extension data
     * has to be transmitted.
     * We also store a snapshot of our local reconciliation set, to better keep track of
     * transactions arriving during this reconciliation (they will be added to the cleared
     * original reconciliation set, to be reconciled next time).
     */
    void PrepareForExtensionResponse(uint16_t sketch_capacity, const std::vector<uint8_t>& remote_sketch)
    {
        assert(m_we_initiate);
        m_capacity_snapshot = sketch_capacity;
        m_remote_sketch_snapshot = remote_sketch;
        m_local_set_snapshot = m_local_set;
        m_snapshot_short_id_mapping = m_short_id_mapping;
        m_local_set.clear();
        m_short_id_mapping.clear();
    }
};
} // namespace

/** Actual implementation for TxReconciliationTracker's data structure. */
class TxReconciliationTracker::Impl
{
private:
    mutable Mutex m_txreconciliation_mutex;

    /**
     * ReconciliationTracker-wide randomness to choose fanout targets for a given txid.
     */
    const SaltedTxidHasher m_txid_hasher;

    // Local protocol version
    uint32_t m_recon_version;

    /**
     * Keeps track of txreconciliation states of eligible peers.
     * For pre-registered peers, the locally generated salt is stored.
     * For registered peers, the locally generated salt is forgotten, and the state (including
     * "full" salt) is stored instead.
     */
    std::unordered_map<NodeId, std::variant<uint64_t, TxReconciliationState>> m_states GUARDED_BY(m_txreconciliation_mutex);

    /**
     * Maintains a queue of reconciliations we should initiate. To achieve higher bandwidth
     * conservation and avoid overflows, we should reconcile in the same order, because then it’s
     * easier to estimate set difference size.
     */
    std::deque<NodeId> m_queue GUARDED_BY(m_txreconciliation_mutex);

    /**
     * Make reconciliation requests periodically to make reconciliations efficient.
     */
    std::chrono::microseconds m_next_recon_request GUARDED_BY(m_txreconciliation_mutex){0};
    void UpdateNextReconRequest(std::chrono::microseconds now) EXCLUSIVE_LOCKS_REQUIRED(m_txreconciliation_mutex)
    {
        // We have one timer for the entire queue. This is safe because we initiate reconciliations
        // with outbound connections, which are unlikely to game this timer in a serious way.
        size_t we_initiate_to_count = std::count_if(m_states.begin(), m_states.end(),
                                                    [](std::pair<NodeId, std::variant<uint64_t, TxReconciliationState>> indexed_state) {
                                                        const auto* cur_state = std::get_if<TxReconciliationState>(&indexed_state.second);
                                                        if (cur_state) return cur_state->m_we_initiate;
                                                        return false;
                                                    });

        Assert(we_initiate_to_count != 0);
        m_next_recon_request = now + (RECON_REQUEST_INTERVAL / we_initiate_to_count);
    }

    bool HandleInitialSketch(TxReconciliationState& recon_state, const NodeId peer_id,
                             const std::vector<uint8_t>& skdata,
                             // returning values
                             std::vector<uint32_t>& txs_to_request, std::vector<uint256>& txs_to_announce, std::optional<bool>& result)
    {
        assert(recon_state.m_we_initiate);
        assert(recon_state.m_phase == Phase::INIT_REQUESTED);

        uint32_t remote_sketch_capacity = uint32_t(skdata.size() / BYTES_PER_SKETCH_CAPACITY);
        // Protocol violation: our peer exceeded the sketch capacity, or sent a malformed sketch.
        if (remote_sketch_capacity > MAX_SKETCH_CAPACITY) {
            return false;
        }

        Minisketch local_sketch, remote_sketch;
        if (recon_state.m_local_set.size() > 0) {
            local_sketch = recon_state.ComputeBaseSketch(remote_sketch_capacity);
        }
        if (remote_sketch_capacity != 0) {
            remote_sketch = node::MakeMinisketch32(remote_sketch_capacity).Deserialize(skdata);
        }

        // Remote sketch is empty in two cases per which reconciliation is pointless:
        // 1. the peer has no transactions for us
        // 2. we told the peer we have no transactions for them while initiating reconciliation.
        // In case (2), local sketch is also empty.
        if (remote_sketch_capacity == 0 || !remote_sketch || !local_sketch) {
            // Announce all transactions we have.
            txs_to_announce = recon_state.GetAllTransactions();

            // Update local reconciliation state for the peer.
            recon_state.FinalizeInitByUs(true);
            recon_state.m_phase = Phase::NONE;

            result = false;

            LogPrint(BCLog::NET, "Reconciliation we initiated with peer=%d terminated due to empty sketch. " /* Continued */
                                 "Announcing all %i transactions from the local set.\n",
                     peer_id, txs_to_announce.size());

            return true;
        }

        assert(remote_sketch);
        assert(local_sketch);
        // Attempt to decode the set difference
        size_t max_elements = minisketch_compute_max_elements(RECON_FIELD_SIZE, remote_sketch_capacity, RECON_FALSE_POSITIVE_COEF);
        std::vector<uint64_t> differences(max_elements);
        if (local_sketch.Merge(remote_sketch).Decode(differences)) {
            // Initial reconciliation step succeeded.

            // Identify locally/remotely missing transactions.
            recon_state.GetRelevantIDsFromShortIDs(differences, txs_to_request, txs_to_announce);

            // Update local reconciliation state for the peer.
            recon_state.FinalizeInitByUs(true);
            recon_state.m_phase = Phase::NONE;

            result = true;
            LogPrint(BCLog::NET, "Reconciliation we initiated with peer=%d has succeeded at initial step, " /* Continued */
                                 "request %i txs, announce %i txs.\n",
                     peer_id, txs_to_request.size(), txs_to_announce.size());
        } else {
            // Initial reconciliation step failed.

            // Update local reconciliation state for the peer.
            recon_state.PrepareForExtensionResponse(remote_sketch_capacity, skdata);
            recon_state.m_phase = Phase::EXT_REQUESTED;

            result = std::nullopt;

            LogPrint(BCLog::NET, "Reconciliation we initiated with peer=%d has failed at initial step, " /* Continued */
                "request sketch extension.\n", peer_id);
        }
        return true;
    }

    public:
    explicit Impl(uint32_t recon_version) : m_recon_version(recon_version) {}

    uint64_t PreRegisterPeer(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        LogPrintLevel(BCLog::TXRECONCILIATION, BCLog::Level::Debug, "Pre-register peer=%d\n", peer_id);
        const uint64_t local_salt{GetRand(UINT64_MAX)};

        // We do this exactly once per peer (which are unique by NodeId, see GetNewNodeId) so it's
        // safe to assume we don't have this record yet.
        Assume(m_states.emplace(peer_id, local_salt).second);
        return local_salt;
    }

    ReconciliationRegisterResult RegisterPeer(NodeId peer_id, bool is_peer_inbound, uint32_t peer_recon_version,
                                              uint64_t remote_salt) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        auto recon_state = m_states.find(peer_id);

        if (recon_state == m_states.end()) return ReconciliationRegisterResult::NOT_FOUND;

        if (std::holds_alternative<TxReconciliationState>(recon_state->second)) {
            return ReconciliationRegisterResult::ALREADY_REGISTERED;
        }

        uint64_t local_salt = *std::get_if<uint64_t>(&recon_state->second);

        // If the peer supports the version which is lower than ours, we downgrade to the version
        // it supports. For now, this only guarantees that nodes with future reconciliation
        // versions have the choice of reconciling with this current version. However, they also
        // have the choice to refuse supporting reconciliations if the common version is not
        // satisfactory (e.g. too low).
        const uint32_t recon_version{std::min(peer_recon_version, m_recon_version)};
        // v1 is the lowest version, so suggesting something below must be a protocol violation.
        if (recon_version < 1) return ReconciliationRegisterResult::PROTOCOL_VIOLATION;

        LogPrintLevel(BCLog::TXRECONCILIATION, BCLog::Level::Debug, "Register peer=%d (inbound=%i)\n",
                      peer_id, is_peer_inbound);

        const uint256 full_salt{ComputeSalt(local_salt, remote_salt)};
        recon_state->second = TxReconciliationState(!is_peer_inbound, full_salt.GetUint64(0), full_salt.GetUint64(1));
        if (!is_peer_inbound) m_queue.push_back(peer_id);
        return ReconciliationRegisterResult::SUCCESS;
    }

    bool AddToSet(NodeId peer_id, const uint256& wtxid) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        if (!IsPeerRegistered(peer_id)) return false;
        auto& recon_state = std::get<TxReconciliationState>(m_states.find(peer_id)->second);

        // Check if reconciliation set is not at capacity for two reasons:
        // - limit sizes of reconciliation sets and short id mappings;
        // - limit CPU use for sketch computations.
        //
        // Since we reconcile frequently, reaching capacity either means:
        // (1) a peer for some reason does not request reconciliations from us for a long while, or
        // (2) really a lot of valid fee-paying transactions were dumped on us at once.
        // We don't care about a laggy peer (1) because we probably can't help them even if we fanout transactions.
        // However, exploiting (2) should not prevent us from relaying certain transactions.
        //
        // Transactions which don't make it to the set due to the limit are announced via fan-out.
        if (recon_state.m_local_set.size() >= MAX_SET_SIZE) return false;

        Assume(recon_state.m_local_set.insert(wtxid).second);
        LogPrintLevel(BCLog::TXRECONCILIATION, BCLog::Level::Debug, "Added %s to the reconciliation set for peer=%d. " /* Continued */
                                                                    "Now the set contains %i transactions.\n",
                      wtxid.ToString(), peer_id, recon_state.m_local_set.size());
        return true;
    }

    bool TryRemovingFromSet(NodeId peer_id, const uint256& wtxid_to_remove) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        if (!IsPeerRegistered(peer_id)) return false;
        auto& recon_state = std::get<TxReconciliationState>(m_states.find(peer_id)->second);

        const bool removed = recon_state.m_local_set.erase(wtxid_to_remove) > 0;
        if (recon_state.m_local_set_snapshot.find(wtxid_to_remove) !=
            recon_state.m_local_set_snapshot.end()) {
                recon_state.m_announced_during_extension.insert(wtxid_to_remove);
            }
        return removed;
    }

    bool IsPeerNextToReconcileWith(NodeId peer_id, std::chrono::microseconds now) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        if (!IsPeerRegistered(peer_id)) return false;
        if (m_queue.empty()) return false;

        const auto& recon_state = std::get<TxReconciliationState>(m_states.find(peer_id)->second);

        if (m_next_recon_request <= now && m_queue.front() == peer_id) {
            Assume(recon_state.m_we_initiate);
            m_queue.pop_front();
            m_queue.push_back(peer_id);

            // If the phase is not NONE, the peer hasn't responded to the previous reconciliation.
            // A laggy peer should not affect other peers.
            //
            // This doesn't prevent from a malicious peer gaming this by staying in this state
            // all the time somehow.
            if (recon_state.m_phase == Phase::NONE) UpdateNextReconRequest(now);
            return true;
        }

        return false;
    }

    std::optional<std::pair<uint16_t, uint16_t>> InitiateReconciliationRequest(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        if (!IsPeerRegistered(peer_id)) return std::nullopt;

        auto& recon_state = std::get<TxReconciliationState>(m_states.find(peer_id)->second);
        if (!recon_state.m_we_initiate) return std::nullopt;

        if (recon_state.m_phase != Phase::NONE) return std::nullopt;
        recon_state.m_phase = Phase::INIT_REQUESTED;

        size_t local_set_size = recon_state.m_local_set.size();

        LogPrintLevel(BCLog::TXRECONCILIATION, BCLog::Level::Debug, "Initiate reconciliation with peer=%d with the following params: " /* Continued */
                                                                    "local_set_size=%i\n",
                      peer_id, local_set_size);

        // In future, Q could be recomputed after every reconciliation based on the
        // set differences. For now, it provides good enough results without recompute
        // complexity, but we communicate it here to allow backward compatibility if
        // the value is changed or made dynamic.
        return std::make_pair(local_set_size, Q * Q_PRECISION);
    }

    void HandleReconciliationRequest(NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        if (!IsPeerRegistered(peer_id)) return;
        auto& recon_state = std::get<TxReconciliationState>(m_states.find(peer_id)->second);
        if (recon_state.m_we_initiate) return;
        if (recon_state.m_phase != Phase::NONE) return;

        double peer_q_converted = peer_q * 1.0 / Q_PRECISION;
        recon_state.m_remote_q = peer_q_converted;
        recon_state.m_remote_set_size = peer_recon_set_size;
        recon_state.m_phase = Phase::INIT_REQUESTED;

        LogPrint(BCLog::NET, "Reconciliation initiated by peer=%d with the following params: " /* Continued */
                             "remote_q=%d, remote_set_size=%i.\n",
                 peer_id, peer_q_converted, peer_recon_set_size);
    }

    bool RespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        if (!IsPeerRegistered(peer_id)) return false;
        auto& recon_state = std::get<TxReconciliationState>(m_states.find(peer_id)->second);
        if (recon_state.m_we_initiate) return false;

        Phase incoming_phase = recon_state.m_phase;

        // For initial requests we have an extra check to avoid short intervals between responses
        // to the same peer (see comments in the check function for justification).
        bool respond_to_initial_request = incoming_phase == Phase::INIT_REQUESTED &&
                                          recon_state.ConsiderInitResponseAndTrack();
        if (!respond_to_initial_request) {
            return false;
        }

        // Compute a sketch over the local reconciliation set.
        uint32_t sketch_capacity = 0;

        // We send an empty vector at initial request in the following 2 cases because
        // reconciliation can't help:
        // - if we have nothing on our side
        // - if they have nothing on their side
        // Then, they will terminate reconciliation early and force flooding-style announcement.
        if (recon_state.m_remote_set_size > 0 && recon_state.m_local_set.size() > 0) {

            sketch_capacity = recon_state.EstimateSketchCapacity(
                recon_state.m_local_set.size());
            Minisketch sketch = recon_state.ComputeBaseSketch(sketch_capacity);
            if (sketch) skdata = sketch.Serialize();
        }

        recon_state.m_phase = Phase::INIT_RESPONDED;
        recon_state.PrepareForExtensionRequest(sketch_capacity);

        LogPrint(BCLog::NET, "Responding with a sketch to reconciliation initiated by peer=%d: " /* Continued */
                             "sending sketch of capacity=%i.\n",
                 peer_id, sketch_capacity);

        return true;
    }

    bool HandleSketch(NodeId peer_id, const std::vector<uint8_t>& skdata,
                      // returning values
                      std::vector<uint32_t>& txs_to_request, std::vector<uint256>& txs_to_announce, std::optional<bool>& result) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        if (!IsPeerRegistered(peer_id)) return false;
        auto& recon_state = std::get<TxReconciliationState>(m_states.find(peer_id)->second);
        // We only may receive a sketch from reconciliation responder, not initiator.
        if (!recon_state.m_we_initiate) return false;

        Phase cur_phase = recon_state.m_phase;
        if (cur_phase == Phase::INIT_REQUESTED) {
            return HandleInitialSketch(recon_state, peer_id, skdata, txs_to_request, txs_to_announce, result);
        } else {
            LogPrint(BCLog::NET, "Received sketch from peer=%d in wrong reconciliation phase=%i.\n", peer_id, static_cast<int>(cur_phase));
            return false;
        }
    }

    void ForgetPeer(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        if (m_states.erase(peer_id)) {
            m_queue.erase(std::remove(m_queue.begin(), m_queue.end(), peer_id), m_queue.end());
            LogPrintLevel(BCLog::TXRECONCILIATION, BCLog::Level::Debug, "Forget txreconciliation state of peer=%d\n", peer_id);
        }
    }

    bool IsPeerRegistered(NodeId peer_id) const EXCLUSIVE_LOCKS_REQUIRED(m_txreconciliation_mutex)
    {
        AssertLockHeld(m_txreconciliation_mutex);
        auto recon_state = m_states.find(peer_id);
        return (recon_state != m_states.end() &&
                std::holds_alternative<TxReconciliationState>(recon_state->second));
    }

    bool IsPeerRegisteredExternal(NodeId peer_id) const EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        return IsPeerRegistered(peer_id);
    }

    std::vector<NodeId> GetFanoutTargets(CSipHasher& deterministic_randomizer_with_wtxid,
                                         bool we_initiate, double limit) const EXCLUSIVE_LOCKS_REQUIRED(m_txreconciliation_mutex)
    {
        // The algorithm works as follows. We iterate through the peers (of a given direction)
        // hashing them with the given wtxid, and sort them by this hash.
        // We then consider top `limit` peers to be low-fanout flood targets.
        // The randomness should be seeded with wtxid to return consistent results for every call.

        // To handle fractional values, we add one peer optimistically and then probabilistically
        // drop it later.
        double integer_part;
        double fractional_peer = std::modf(limit, &integer_part);
        const size_t targets = size_t(integer_part) + 1;
        const bool drop_peer_if_extra = deterministic_randomizer_with_wtxid.Finalize() > fractional_peer * double(UINT64_MAX);

        std::vector<std::pair<uint64_t, NodeId>> best_peers;
        for (size_t i = 0; i < targets; ++i) {
            best_peers.push_back(std::make_pair(0, 0));
        }

        auto try_fanout_candidate = [&best_peers, &deterministic_randomizer_with_wtxid, targets](
                                        const TxReconciliationState& candidate_state, const NodeId& candidate_id) {
            uint64_t hash_key = deterministic_randomizer_with_wtxid.Write(candidate_state.m_k0).Finalize();

            for (size_t i = 0; i < targets; ++i) {
                if (hash_key > best_peers[i].first) {
                    best_peers.insert(best_peers.begin() + i, std::make_pair(hash_key, candidate_id));
                    break;
                }
            }
        };

        for (auto indexed_state : m_states) {
            const auto cur_state = std::get_if<TxReconciliationState>(&indexed_state.second);
            if (cur_state && cur_state->m_we_initiate == we_initiate) {
                try_fanout_candidate(*cur_state, indexed_state.first);
            }
        }

        best_peers.erase(best_peers.begin() + targets, best_peers.end());

        std::vector<NodeId> result;
        std::for_each(best_peers.begin(), best_peers.end(),
                      [&result](auto best_peer) {
                          if (best_peer.first != 0) result.push_back(best_peer.second);
                      });

        if (result.size() > limit && drop_peer_if_extra) {
            result.pop_back();
        }

        return result;
    }

    bool ShouldFanoutTo(const uint256& wtxid, CSipHasher deterministic_randomizer, NodeId peer_id,
                        size_t inbounds_nonrcncl_tx_relay, size_t outbounds_nonrcncl_tx_relay)
                        const EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        if (!IsPeerRegistered(peer_id)) return true;
        // We use the pre-determined randomness to give a consistent result per transaction,
        // thus making sure that no transaction gets "unlucky" if every per-peer roll fails.
        deterministic_randomizer.Write(wtxid.GetUint64(0));
        const auto& recon_state = std::get<TxReconciliationState>(m_states.find(peer_id)->second);
        // We decide whether a particular peer is a low-fanout flood target differently
        // based on its connection direction:
        // - for outbounds we have a fixed number of flood destinations;
        // - for inbounds we use a fraction of all inbound peers supporting tx relay.
        //
        // We first decide how many reconciling peers of a given direction we want to flood to,
        // and then generate a list of peers of that size for a given transaction. We then see
        // whether the given peer falls into this list.
        double destinations;
        if (recon_state.m_we_initiate) {
            destinations = OUTBOUND_FANOUT_DESTINATIONS - outbounds_nonrcncl_tx_relay;
        } else {
            const size_t inbound_rcncl_peers = std::count_if(m_states.begin(), m_states.end(),
                                                        [](std::pair<NodeId, std::variant<uint64_t, TxReconciliationState>> indexed_state) {
                                                            const auto* cur_state = std::get_if<TxReconciliationState>(&indexed_state.second);
                                                            if (cur_state) return !cur_state->m_we_initiate;
                                                            return false;
                                                        });

            // Since we use the fraction for inbound peers, we first need to compute the total
            // number of inbound targets.
            const double inbound_targets = (inbounds_nonrcncl_tx_relay + inbound_rcncl_peers) * INBOUND_FANOUT_DESTINATIONS_FRACTION;
            destinations = inbound_targets - inbounds_nonrcncl_tx_relay;
        }

        if (destinations < 0.01) {
            return false;
        }

        auto fanout_candidates = GetFanoutTargets(deterministic_randomizer, recon_state.m_we_initiate, destinations);
        return std::count(fanout_candidates.begin(), fanout_candidates.end(), peer_id);
    }
};

TxReconciliationTracker::TxReconciliationTracker(uint32_t recon_version) : m_impl{std::make_unique<TxReconciliationTracker::Impl>(recon_version)} {}

TxReconciliationTracker::~TxReconciliationTracker() = default;

uint64_t TxReconciliationTracker::PreRegisterPeer(NodeId peer_id)
{
    return m_impl->PreRegisterPeer(peer_id);
}

ReconciliationRegisterResult TxReconciliationTracker::RegisterPeer(NodeId peer_id, bool is_peer_inbound,
                                                                   uint32_t peer_recon_version, uint64_t remote_salt)
{
    return m_impl->RegisterPeer(peer_id, is_peer_inbound, peer_recon_version, remote_salt);
}

bool TxReconciliationTracker::AddToSet(NodeId peer_id, const uint256& wtxid)
{
    return m_impl->AddToSet(peer_id, wtxid);
}

bool TxReconciliationTracker::TryRemovingFromSet(NodeId peer_id, const uint256& wtxid_to_remove)
{
    return m_impl->TryRemovingFromSet(peer_id, wtxid_to_remove);
}

bool TxReconciliationTracker::IsPeerNextToReconcileWith(NodeId peer_id, std::chrono::microseconds now)
{
    return m_impl->IsPeerNextToReconcileWith(peer_id, now);
}

std::optional<std::pair<uint16_t, uint16_t>> TxReconciliationTracker::InitiateReconciliationRequest(NodeId peer_id)
{
    return m_impl->InitiateReconciliationRequest(peer_id);
}

void TxReconciliationTracker::HandleReconciliationRequest(NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q)
{
    m_impl->HandleReconciliationRequest(peer_id, peer_recon_set_size, peer_q);
}

bool TxReconciliationTracker::RespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata)
{
    return m_impl->RespondToReconciliationRequest(peer_id, skdata);
}

bool TxReconciliationTracker::HandleSketch(NodeId peer_id, const std::vector<uint8_t>& skdata,
    std::vector<uint32_t>& txs_to_request, std::vector<uint256>& txs_to_announce, std::optional<bool>& result)
{
    return m_impl->HandleSketch(peer_id, skdata, txs_to_request, txs_to_announce, result);
}

void TxReconciliationTracker::ForgetPeer(NodeId peer_id)
{
    m_impl->ForgetPeer(peer_id);
}

bool TxReconciliationTracker::IsPeerRegistered(NodeId peer_id) const
{
    return m_impl->IsPeerRegisteredExternal(peer_id);
}

bool TxReconciliationTracker::ShouldFanoutTo(const uint256& wtxid, CSipHasher deterministic_randomizer, NodeId peer_id,
                                             size_t inbounds_nonrcncl_tx_relay, size_t outbounds_nonrcncl_tx_relay) const
{
    return m_impl->ShouldFanoutTo(wtxid, deterministic_randomizer, peer_id,
                                  inbounds_nonrcncl_tx_relay, outbounds_nonrcncl_tx_relay);
}
