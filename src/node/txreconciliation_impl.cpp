// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/minisketchwrapper.h>
#include <node/txreconciliation.h>
#include <node/txreconciliation_impl.h>

#include <common/system.h>
#include <util/check.h>

#include <cmath>
#include <unordered_map>
#include <variant>

namespace node {
uint32_t TxReconciliationState::ComputeShortID(const Wtxid& wtxid) const
{
    const uint64_t s = SipHashUint256(m_k0, m_k1, wtxid.ToUint256());
    const uint32_t short_txid = 1 + (s & 0xFFFFFFFF);
    return short_txid;
}

bool TxReconciliationState::HasCollision(const Wtxid& wtxid, Wtxid& collision, uint32_t &short_id)
{
    short_id = TxReconciliationState::ComputeShortID(wtxid);
    const auto iter = m_short_id_mapping.find(short_id);

    if (iter != m_short_id_mapping.end()) {
        collision = iter->second;
        return true;
    }

    return false;
}

uint32_t TxReconciliationState::EstimateSketchCapacity(size_t local_set_size) const
{
    const uint16_t set_size_diff = std::abs(uint16_t(local_set_size) - m_remote_set_size);
    const uint16_t min_size = std::min(uint16_t(local_set_size), m_remote_set_size);
    // TODO: This rounding by casting. Should we be more careful about how we want to round(up, down) this?
    const uint16_t weighted_min_size = m_remote_q * min_size;
    const uint32_t estimated_diff = 1 + weighted_min_size + set_size_diff;
    return minisketch_compute_capacity(RECON_FIELD_SIZE, estimated_diff, RECON_FALSE_POSITIVE_COEF);
}

Minisketch TxReconciliationState::ComputeSketch(uint32_t& capacity)
{
    // Avoid serializing/sending an empty sketch.
    Assume(capacity > 0);

    capacity = std::min(capacity, MAX_SKETCH_CAPACITY);
    Minisketch sketch = node::MakeMinisketch32(capacity);

    for (const auto& wtxid : m_local_set) {
        uint32_t short_txid = ComputeShortID(wtxid);
        sketch.Add(short_txid);
        m_short_id_mapping.emplace(short_txid, wtxid);
    }

    return sketch;
}

std::vector<Wtxid> TxReconciliationState::GetAllTransactions() const
{
    return std::vector<Wtxid>(m_local_set.begin(), m_local_set.end());
}

std::pair<std::vector<uint32_t>, std::vector<Wtxid>> TxReconciliationState::GetRelevantIDsFromShortIDs(const std::vector<uint64_t>& diff) const
{
    std::vector<Wtxid> remote_missing{};
    std::vector<uint32_t> local_missing{};
    // diff elements are 64-bit since Minisketch::Decode works with 64-bit elements, no matter what the field element size is.
    // In our case, elements are always 32-bit long, so we can safely cast them down.
    for (const auto& diff_short_id : diff) {
        const auto local_tx = m_short_id_mapping.find(diff_short_id);
        if (local_tx != m_short_id_mapping.end()) {
            remote_missing.push_back(local_tx->second);
        } else {
            local_missing.push_back(diff_short_id);
        }
    }

    return std::make_pair(local_missing, remote_missing);
}

/** Actual implementation for TxReconciliationTracker's data structure. */
class TxReconciliationTrackerImpl {
private:
    mutable Mutex m_txreconciliation_mutex;

    // Local protocol version
    uint32_t m_recon_version;

    /**
     * Keeps track of txreconciliation states of eligible peers.
     * For pre-registered peers, the locally generated salt is stored.
     * For registered peers, the locally generated salt is forgotten, and the state (including
     * "full" salt) is stored instead.
     */
    std::unordered_map<NodeId, std::variant<uint64_t, TxReconciliationState>> m_states GUARDED_BY(m_txreconciliation_mutex);

    /** Keeps track of how many of the registered peers are inbound. Updated on registering or forgetting peers. */
    size_t m_inbounds_count GUARDED_BY(m_txreconciliation_mutex){0};

    /**
     * Maintains a queue of reconciliations we should initiate. To achieve higher bandwidth conservation and avoid overflows,
     * we should reconcile in the same order, because then it’s easier to estimate set difference size.
     */
    std::deque<NodeId> m_queue GUARDED_BY(m_txreconciliation_mutex);

    /** Make reconciliation requests periodically to make reconciliations efficient. */
    std::chrono::microseconds m_next_recon_request GUARDED_BY(m_txreconciliation_mutex){0};

    /** Collection of inbound peers selected for fanout. Should get periodically rotated using RotateInboundFanoutTargets. */
    std::unordered_set<NodeId> m_inbound_fanout_targets GUARDED_BY(m_txreconciliation_mutex);

    /** Next time m_inbound_fanout_targets need to be rotated. */
    std::chrono::microseconds GUARDED_BY(m_txreconciliation_mutex) m_next_inbound_peer_rotation_time{0};

    /**
     * Filter of recently requested transactions (via RECONCILDIFF).
     * Used to check whether a transaction was received via fanout or reconciliation.
     * TODO: This can likely be made smaller, given we only care about the really recent ones
     */
    mutable CRollingBloomFilter m_recently_requested_short_ids GUARDED_BY(m_txreconciliation_mutex){50000, 0.000001};

    TxReconciliationState* GetRegisteredPeerState(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(m_txreconciliation_mutex)
    {
        AssertLockHeld(m_txreconciliation_mutex);
        auto salt_or_state = m_states.find(peer_id);
        if (salt_or_state == m_states.end()) return nullptr;

        return std::get_if<TxReconciliationState>(&salt_or_state->second);
    }

    /**
     * Schedules the a reconciliation request with the next outbound peer in our reconciliation queue.
     * The next time depends on the number of peers on our queue, so we can reconcile with all out outbound neighborhood
     * once every RECON_REQUEST_INTERVAL.
     */
    void ScheduleNextReconRequest(std::chrono::microseconds now) EXCLUSIVE_LOCKS_REQUIRED(m_txreconciliation_mutex)
    {
        // We have one timer for the entire queue. This is safe because we initiate reconciliations
        // with outbound connections, which are unlikely to game this timer in a serious way.
        Assume( m_states.size() > m_inbounds_count);
        size_t we_initiate_to_count = m_states.size() - m_inbounds_count;
        m_next_recon_request = now + (RECON_REQUEST_INTERVAL / we_initiate_to_count);
    }

    std::variant<HandleSketchResult, ReconciliationError> HandleInitialSketch(TxReconciliationState& peer_state, const NodeId peer_id, const std::vector<uint8_t>& skdata)
    {
        Assume(peer_state.m_we_initiate);
        Assume(peer_state.m_phase == ReconciliationPhase::INIT_REQUESTED);

        // The serialized remote sketch size needs to be a multiple of the sketch element size, otherwise we received a malformed sketch.
        if (skdata.size() % BYTES_PER_SKETCH_CAPACITY != 0) return ReconciliationError::PROTOCOL_VIOLATION;

        uint32_t remote_sketch_capacity = uint32_t(skdata.size() / BYTES_PER_SKETCH_CAPACITY);
        // Protocol violation: our peer exceeded the sketch capacity, or sent a malformed sketch.
        if (remote_sketch_capacity > MAX_SKETCH_CAPACITY) return ReconciliationError::PROTOCOL_VIOLATION;

        std::optional<Minisketch> local_sketch, remote_sketch;
        if (remote_sketch_capacity != 0) {
            remote_sketch = node::MakeMinisketch32(remote_sketch_capacity).Deserialize(skdata);

            if (!peer_state.m_local_set.empty()) {
                local_sketch = peer_state.ComputeSketch(remote_sketch_capacity);
            }
        }

        std::vector<uint32_t> txs_to_request;
        std::vector<Wtxid> txs_to_announce;
        std::optional<bool> result;

        // Remote sketch is empty in two cases per which reconciliation is pointless:
        // 1. the peer has no transactions for us
        // 2. we told the peer we have no transactions for them while initiating reconciliation.
        // In case (2), local sketch is also empty.
        if (!remote_sketch.has_value() || !local_sketch.has_value()) {
            // Announce all transactions we have.
            txs_to_announce = peer_state.GetAllTransactions();
            // Update local reconciliation state for the peer.
            peer_state.m_local_set.clear();
            peer_state.m_phase = ReconciliationPhase::NONE;

            result = false;
            LogDebug(BCLog::TXRECONCILIATION, "Reconciliation we initiated with peer=%d terminated due to empty sketch. " /* Continued */
                                              "Announcing all %i transactions from the local set.\n", peer_id, txs_to_announce.size());
        } else {
            Assume(remote_sketch.has_value());
            Assume(local_sketch.has_value());
            // Attempt to decode the set difference
            size_t max_elements = minisketch_compute_max_elements(RECON_FIELD_SIZE, remote_sketch_capacity, RECON_FALSE_POSITIVE_COEF);
            std::vector<uint64_t> differences(max_elements);
            if (local_sketch.value().Merge(remote_sketch.value()).Decode(differences)) {
                // Initial reconciliation step succeeded.
                // Identify locally/remotely missing transactions.
                std::tie(txs_to_request, txs_to_announce) = peer_state.GetRelevantIDsFromShortIDs(differences);
                // Update local reconciliation state for the peer.
                peer_state.m_local_set.clear();
                peer_state.m_phase = ReconciliationPhase::NONE;

                result = true;
                LogDebug(BCLog::TXRECONCILIATION, "Reconciliation we initiated with peer=%d has succeeded at initial step, request %i txs, announce %i txs.\n",
                    peer_id, txs_to_request.size(), txs_to_announce.size());
            } else {
                // Initial reconciliation step failed.
                // Update local reconciliation state for the peer.
                peer_state.m_phase = ReconciliationPhase::EXT_REQUESTED;

                result = std::nullopt;
                LogDebug(BCLog::TXRECONCILIATION, "Reconciliation we initiated with peer=%d has failed at initial step, request sketch extension.\n", peer_id);
            }
        }

        return HandleSketchResult{txs_to_request, txs_to_announce, result};
    }

public:
    explicit TxReconciliationTrackerImpl(uint32_t recon_version) : m_recon_version(recon_version) {}

    uint64_t PreRegisterPeer(NodeId peer_id, uint64_t local_salt) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        LogDebug(BCLog::TXRECONCILIATION, "Pre-register peer=%d.\n", peer_id);

        // We do this exactly once per peer (which are unique by NodeId, see GetNewNodeId) so it's
        // safe to assume we don't have this record yet.
        Assume(m_states.emplace(peer_id, local_salt).second);
        return local_salt;
    }

    std::optional<ReconciliationError> RegisterPeer(NodeId peer_id, bool is_peer_inbound, uint32_t peer_recon_version,
                                                    uint64_t remote_salt) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = m_states.find(peer_id);
        if (peer_state == m_states.end()) return ReconciliationError::NOT_FOUND;
        if (std::holds_alternative<TxReconciliationState>(peer_state->second)) {
            return ReconciliationError::ALREADY_REGISTERED;
        }
        uint64_t local_salt = std::get<uint64_t>(peer_state->second);

        // If the peer supports the version which is lower than ours, we downgrade to the version
        // it supports. For now, this only guarantees that nodes with future reconciliation
        // versions have the choice of reconciling with this current version. However, they also
        // have the choice to refuse supporting reconciliations if the common version is not
        // satisfactory (e.g. too low).
        const uint32_t recon_version{std::min(peer_recon_version, m_recon_version)};
        // v1 is the lowest version, so suggesting something below must be a protocol violation.
        if (recon_version < 1) return ReconciliationError::PROTOCOL_VIOLATION;

        LogDebug(BCLog::TXRECONCILIATION, "Register peer=%d (inbound=%i).\n", peer_id, is_peer_inbound);

        const uint256 full_salt{ComputeSalt(local_salt, remote_salt)};

        auto new_state = TxReconciliationState(!is_peer_inbound, full_salt.GetUint64(0), full_salt.GetUint64(1));
        m_states.erase(peer_state);
        bool emplaced = m_states.emplace(peer_id, std::move(new_state)).second;
        Assume(emplaced);

        if (is_peer_inbound && m_inbounds_count < std::numeric_limits<size_t>::max()) {
            ++m_inbounds_count;

            // Scale up fanout targets as we get more connections. Targets will be rotated periodically via RotateInboundFanoutTargets
            if (FastRandomContext().randrange(10) < INBOUND_FANOUT_DESTINATIONS_FRACTION * 10) {
                m_inbound_fanout_targets.insert(peer_id);
            }
        }
        if (!is_peer_inbound) {
            // If this is the first outbound peer registered for reconciliation, don't bother instantly requesting reconciliation.
            // Set the next request one RECON_REQUEST_INTERVAL in the future so we have time to gather some transactions
            if (m_queue.empty()) {
                m_next_recon_request = GetTime<std::chrono::microseconds>() + RECON_REQUEST_INTERVAL;
            }
            m_queue.push_back(peer_id);
        }
        return std::nullopt;
    }

    /** For calls within this class use GetRegisteredPeerState instead. */
    bool IsPeerRegistered(NodeId peer_id) const EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        auto peer_state = m_states.find(peer_id);
        return (peer_state != m_states.end() &&
                std::holds_alternative<TxReconciliationState>(peer_state->second));
    }

    bool IsPeerNextToReconcileWith(NodeId peer_id, std::chrono::microseconds now) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return false;
        if (m_queue.empty()) return false;

        if (m_next_recon_request <= now && m_queue.front() == peer_id) {
            Assume(peer_state->m_we_initiate);
            m_queue.pop_front();
            m_queue.push_back(peer_id);

            // If the phase is not NONE, the peer hasn't concluded the previous reconciliation cycle.
            // We won't be updating the shared reconciliation timer, to let the next peer on the queue take
            // its place without waiting. Moreover, we won't send another reconciliation request to this peer
            // until the previous one is completed (InitiateReconciliationRequest will shortcut)
            if (peer_state->m_phase == ReconciliationPhase::NONE) ScheduleNextReconRequest(now);
            return true;
        }

        return false;
    }

    void ForgetPeer(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (peer_state) {
            if (peer_state->m_we_initiate) {
                m_queue.erase(std::remove(m_queue.begin(), m_queue.end(), peer_id), m_queue.end());
            } else {
                Assume(m_inbounds_count > 0);
                --m_inbounds_count;
                m_inbound_fanout_targets.erase(peer_id);
            }
        }

        if (m_states.erase(peer_id)) {
            LogDebug(BCLog::TXRECONCILIATION, "Forget txreconciliation state of peer=%d.\n", peer_id);
        }
    }

    std::optional<AddToSetError> AddToSet(NodeId peer_id, const Wtxid& wtxid) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return AddToSetError(ReconciliationError::NOT_FOUND);

        // Bypass if the wtxid is already in the set
        if (peer_state->m_local_set.contains(wtxid)) {
            LogDebug(BCLog::TXRECONCILIATION, "%s already in reconciliation set for peer=%d. Bypassing.\n", wtxid.ToString(), peer_id);
            return std::nullopt;
        }

        // Make sure there is no short id collision between the wtxid we are trying to add
        // and any existing one in the reconciliation set
        uint32_t short_id;
        Wtxid collision;
        if (peer_state->HasCollision(wtxid, collision, short_id)) {
            return AddToSetError(ReconciliationError::SHORTID_COLLISION, collision);
        }

        // Transactions which don't make it to the set due to the limit are announced via fanout.
        if (peer_state->m_local_set.size() >= MAX_RECONSET_SIZE) {
            LogDebug(BCLog::TXRECONCILIATION, "Reconciliation set maximum size reached for peer=%d.\n", peer_id);
            return AddToSetError(ReconciliationError::FULL_RECON_SET);
        }

        if (peer_state->m_local_set.insert(wtxid).second) {
            peer_state->m_short_id_mapping.emplace(short_id, wtxid);
            LogDebug(BCLog::TXRECONCILIATION, "Added %s to the reconciliation set for peer=%d. Now the set contains %i transactions.\n",
                wtxid.ToString(), peer_id, peer_state->m_local_set.size());
        }
        return std::nullopt;
    }

    bool TryRemovingFromSet(NodeId peer_id, const Wtxid& wtxid) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return false;

        const bool removed = peer_state->m_local_set.erase(wtxid) > 0;
        if (removed) {
            peer_state->m_short_id_mapping.erase(peer_state->ComputeShortID(wtxid));
            LogDebug(BCLog::TXRECONCILIATION, "Removed %s from the reconciliation set for peer=%d. Now the set contains %i transactions.\n",
                wtxid.ToString(), peer_id, peer_state->m_local_set.size());
        } else {
            LogDebug(BCLog::TXRECONCILIATION, "Couldn't remove %s from the reconciliation set for peer=%d. Transaction not found.\n",
                wtxid.ToString(), peer_id);
        }

        return removed;
    }

    void TrackRecentlyRequestedTransactions(std::vector<uint32_t>& requested_txs) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        std::vector<unsigned char> data(4);
        for (auto& short_id : requested_txs) {
            WriteLE32(data.data(), short_id);
            m_recently_requested_short_ids.insert(data);
        }
    }

    bool WasTransactionRecentlyRequested(const Wtxid& wtxid) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        std::vector<unsigned char> data(4);
        // FIXME: I'm not sure this may be the best way of doing this (as opposed to having a filter per peer). But it'll do for now
        for (auto& [peer_id, opt_peer_state]: m_states) {
            auto peer_state = std::get_if<TxReconciliationState>(&opt_peer_state);
            if (!peer_state) continue;
            // We only care about outbound peers
            if (!peer_state->m_we_initiate) continue;
            auto short_id = peer_state->ComputeShortID(wtxid);
            WriteLE32(data.data(), short_id);
            if (m_recently_requested_short_ids.contains(data)) return true;
        }

        return false;
    }

    std::variant<ReconCoefficients, ReconciliationError> InitiateReconciliationRequest(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return ReconciliationError::NOT_FOUND;

        if (!peer_state->m_we_initiate) return ReconciliationError::WRONG_ROLE;

        // Shortcut if the peer hasn't completed the previous reconciliation cycle
        if (peer_state->m_phase != ReconciliationPhase::NONE) return ReconciliationError::WRONG_PHASE;
        peer_state->m_phase = ReconciliationPhase::INIT_REQUESTED;

        size_t local_set_size = peer_state->m_local_set.size();

        LogDebug(BCLog::TXRECONCILIATION, "Initiate reconciliation with peer=%d with the following params: local_set_size=%i.\n",
            peer_id, local_set_size);

        // In future, Q could be recomputed after every reconciliation based on the
        // set differences. For now, it provides good enough results without recompute
        // complexity, but we communicate it here to allow backward compatibility if
        // the value is changed or made dynamic.
        return std::make_pair(local_set_size, Q * Q_PRECISION);
    }

    std::optional<ReconciliationError> HandleReconciliationRequest(NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return ReconciliationError::NOT_FOUND;

        // We only respond to reconciliation requests if the peer is the initiator and we are not
        // in the middle of another reconciliation cycle with him
        if (peer_state->m_we_initiate) return ReconciliationError::WRONG_ROLE;
        if (peer_state->m_phase != ReconciliationPhase::NONE) return ReconciliationError::WRONG_PHASE;

        double peer_q_converted = peer_q * 1.0 / Q_PRECISION;
        peer_state->m_remote_q = peer_q_converted;
        peer_state->m_remote_set_size = peer_recon_set_size;
        peer_state->m_phase = ReconciliationPhase::INIT_REQUESTED;

        LogDebug(BCLog::TXRECONCILIATION, "Reconciliation initiated by peer=%d with the following params: remote_q=%d, remote_set_size=%i.\n",
            peer_id, peer_q_converted, peer_recon_set_size);

        return std::nullopt;
    }

    bool ShouldRespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata, bool send_trickle) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return false;
        if (peer_state->m_we_initiate) return false;

        // Return if there is nothing to respond to
        if (peer_state->m_phase != ReconciliationPhase::INIT_REQUESTED || !send_trickle) {
            return false;
        }

        // Compute a sketch over the local reconciliation set.
        uint32_t sketch_capacity = 0;

        // We send an empty vector at initial request in the following 2 cases because
        // reconciliation can't help:
        // - if we have nothing on our side
        // - if they have nothing on their side
        // Then, they will terminate reconciliation early and force flooding-style announcement.
        if (peer_state->m_remote_set_size > 0 && peer_state->m_local_set.size() > 0) {
            if (sketch_capacity = peer_state->EstimateSketchCapacity(peer_state->m_local_set.size()); sketch_capacity > 0) {
                Minisketch sketch = peer_state->ComputeSketch(sketch_capacity);
                skdata = sketch.Serialize();
            }
        }

        peer_state->m_phase = ReconciliationPhase::INIT_RESPONDED;

        LogDebug(BCLog::TXRECONCILIATION, "Responding with a sketch to reconciliation initiated by peer=%d: sending sketch of capacity=%i.\n",
            peer_id, sketch_capacity);

        return true;
    }

    std::variant<HandleSketchResult, ReconciliationError> HandleSketch(NodeId peer_id, const std::vector<uint8_t>& skdata) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return ReconciliationError::NOT_FOUND;
        // We only may receive a sketch from reconciliation responder, not initiator.
        if (!peer_state->m_we_initiate) return ReconciliationError::WRONG_ROLE;

        ReconciliationPhase cur_phase = peer_state->m_phase;
        if (cur_phase == ReconciliationPhase::INIT_REQUESTED) {
            return HandleInitialSketch(*peer_state, peer_id, skdata);
        } else {
            LogDebug(BCLog::TXRECONCILIATION, "Received sketch from peer=%d in wrong reconciliation phase=%i.\n", peer_id, static_cast<int>(cur_phase));
            return ReconciliationError::WRONG_PHASE;
        }
    }

    bool IsInboundFanoutTarget(NodeId peer_id) const EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        return m_inbound_fanout_targets.contains(peer_id);
    }

    std::chrono::microseconds GetNextInboundPeerRotationTime() EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        return m_next_inbound_peer_rotation_time;
    }

    void SetNextInboundPeerRotationTime(std::chrono::microseconds next_time) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        Assume(next_time >= m_next_inbound_peer_rotation_time);
        m_next_inbound_peer_rotation_time = next_time;
    }

    void RotateInboundFanoutTargets() EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto targets_size = std::floor(m_inbounds_count * INBOUND_FANOUT_DESTINATIONS_FRACTION);
        if (targets_size == 0) {
            return;
        }

        std::vector<NodeId> inbound_recon_peers;
        inbound_recon_peers.reserve(m_inbounds_count);

        // Collect all inbound reconciling peers ids in a vector and shuffle it
        for (const auto& [peer_id, op_peer_state]: m_states) {
            const auto peer_state = std::get_if<TxReconciliationState>(&op_peer_state);
            if (peer_state && !peer_state->m_we_initiate) {
                inbound_recon_peers.push_back(peer_id);
            }
        }
        std::shuffle(inbound_recon_peers.begin(), inbound_recon_peers.end(), FastRandomContext());

        // Pick the new selection of inbound fanout peers
        Assume(inbound_recon_peers.size() > targets_size);
        m_inbound_fanout_targets.clear();
        m_inbound_fanout_targets.reserve(targets_size);
        m_inbound_fanout_targets.insert(inbound_recon_peers.begin(), inbound_recon_peers.begin() + targets_size);
    }
};

TxReconciliationTracker::TxReconciliationTracker(uint32_t recon_version) : m_impl{std::make_unique<TxReconciliationTrackerImpl>(recon_version)} {}

TxReconciliationTracker::~TxReconciliationTracker() = default;

uint64_t TxReconciliationTracker::PreRegisterPeer(NodeId peer_id)
{
    const uint64_t local_salt{FastRandomContext().rand64()};
    return m_impl->PreRegisterPeer(peer_id, local_salt);
}

void TxReconciliationTracker::PreRegisterPeerWithSalt(NodeId peer_id, uint64_t local_salt)
{
    m_impl->PreRegisterPeer(peer_id, local_salt);
}

std::optional<ReconciliationError> TxReconciliationTracker::RegisterPeer(NodeId peer_id, bool is_peer_inbound, uint32_t peer_recon_version, uint64_t remote_salt)
{
    return m_impl->RegisterPeer(peer_id, is_peer_inbound, peer_recon_version, remote_salt);
}

bool TxReconciliationTracker::IsPeerRegistered(NodeId peer_id) const
{
    return m_impl->IsPeerRegistered(peer_id);
}

bool TxReconciliationTracker::IsPeerNextToReconcileWith(NodeId peer_id, std::chrono::microseconds now)
{
    return m_impl->IsPeerNextToReconcileWith(peer_id, now);
}

void TxReconciliationTracker::ForgetPeer(NodeId peer_id)
{
    m_impl->ForgetPeer(peer_id);
}

std::optional<AddToSetError> TxReconciliationTracker::AddToSet(NodeId peer_id, const Wtxid& wtxid)
{
    return m_impl->AddToSet(peer_id, wtxid);
}

bool TxReconciliationTracker::TryRemovingFromSet(NodeId peer_id, const Wtxid& wtxid)
{
    return m_impl->TryRemovingFromSet(peer_id, wtxid);
}

void TxReconciliationTracker::TrackRecentlyRequestedTransactions(std::vector<uint32_t>& requested_txs)
{
    return m_impl->TrackRecentlyRequestedTransactions(requested_txs);
}

bool TxReconciliationTracker::WasTransactionRecentlyRequested(const Wtxid& wtxid)
{
    return m_impl->WasTransactionRecentlyRequested(wtxid);
}

std::variant<ReconCoefficients, ReconciliationError> TxReconciliationTracker::InitiateReconciliationRequest(NodeId peer_id)
{
    return m_impl->InitiateReconciliationRequest(peer_id);
}

std::optional<ReconciliationError> TxReconciliationTracker::HandleReconciliationRequest(NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q)
{
    return m_impl->HandleReconciliationRequest(peer_id, peer_recon_set_size, peer_q);
}

bool TxReconciliationTracker::ShouldRespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata, bool send_trickle)
{
    return m_impl->ShouldRespondToReconciliationRequest(peer_id, skdata, send_trickle);
}

std::variant<HandleSketchResult, ReconciliationError> TxReconciliationTracker::HandleSketch(NodeId peer_id, const std::vector<uint8_t>& skdata)
{
    return m_impl->HandleSketch(peer_id, skdata);
}

bool TxReconciliationTracker::IsInboundFanoutTarget(NodeId peer_id)
{
    return m_impl->IsInboundFanoutTarget(peer_id);
}

std::chrono::microseconds TxReconciliationTracker::GetNextInboundPeerRotationTime()
{
    return m_impl->GetNextInboundPeerRotationTime();
}

void TxReconciliationTracker::SetNextInboundPeerRotationTime(std::chrono::microseconds next_time)
{
    return m_impl->SetNextInboundPeerRotationTime(next_time);
}

void TxReconciliationTracker::RotateInboundFanoutTargets()
{
    return m_impl->RotateInboundFanoutTargets();
}
} // namespace node
