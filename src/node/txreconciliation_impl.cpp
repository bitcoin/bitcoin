// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/minisketchwrapper.h>
#include <node/txreconciliation.h>
#include <node/txreconciliation_impl.h>

#include <common/system.h>
#include <util/check.h>
#include <util/log.h>

#include <cmath>
#include <unordered_map>
#include <variant>

namespace node {
uint32_t TxReconciliationState::ComputeShortID(const Wtxid& wtxid) const
{
    const uint32_t short_txid = 1 + (m_hasher(wtxid.ToUint256()) & 0xFFFFFFFF);
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
    const auto set_size_diff = static_cast<uint16_t>(std::abs(int(local_set_size) - int(m_remote_set_size)));
    const uint16_t min_size = std::min(uint16_t(local_set_size), m_remote_set_size);
    // Round up to match the bias the sender applied to q (check BIP-330) and to err on the side of a larger sketch
    // (an under-estimate forces an extension round-trip).
    const auto weighted_min_size = static_cast<uint32_t>(std::ceil(m_remote_q * min_size));
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

    /**
     * Maintains a queue of reconciliations we should initiate. To achieve higher bandwidth conservation and avoid overflows,
     * we should reconcile in the same order, because then it’s easier to estimate set difference size.
     */
    std::deque<NodeId> m_queue GUARDED_BY(m_txreconciliation_mutex);

    /** Make reconciliation requests periodically to make reconciliations efficient. */
    std::chrono::microseconds m_next_recon_request GUARDED_BY(m_txreconciliation_mutex){0};

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
        // FIXME: This is not great. If two nodes get added back to back, the second peer would have to
        // wait for 3*RECON_REQUEST_INTERVAL/2 instead of RECON_REQUEST_INTERVAL/2 seconds to reconcile.
        Assume(!m_queue.empty());
        m_next_recon_request = now + (RECON_REQUEST_INTERVAL / m_queue.size());
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
        peer_state->second.emplace<TxReconciliationState>(!is_peer_inbound, full_salt.GetUint64(0), full_salt.GetUint64(1));

        if (!is_peer_inbound) {
            // If this is the first outbound peer registered for reconciliation, don't bother instantly requesting reconciliation.
            // Set the next request one RECON_REQUEST_INTERVAL in the future so we have time to gather some transactions
            m_queue.push_back(peer_id);
            if (m_queue.size() == 1) {
                ScheduleNextReconRequest(GetTime<std::chrono::microseconds>());
            }
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

    bool ForgetPeer(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (peer_state && peer_state->m_we_initiate) {
            Assert(m_queue.size() > 0);
            m_queue.erase(std::remove(m_queue.begin(), m_queue.end(), peer_id), m_queue.end());
        }

        if (m_states.erase(peer_id)) {
            LogDebug(BCLog::TXRECONCILIATION, "Forget txreconciliation state of peer=%d.\n", peer_id);
            return true;
        }
        return false;
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
        // q is rounded up as per BIP-330.
        return std::make_pair(local_set_size, static_cast<uint16_t>(std::ceil(Q * Q_PRECISION)));
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

        // A BIP-330 compliant peer rounds up when narrowing q to the wire format, so the
        // formatted q is at most Q_PRECISION. Reject anything larger.
        if (peer_q > Q_PRECISION) return ReconciliationError::PROTOCOL_VIOLATION;
        const double peer_q_converted = static_cast<double>(peer_q) / Q_PRECISION;
        peer_state->m_remote_q = peer_q_converted;
        peer_state->m_remote_set_size = peer_recon_set_size;
        peer_state->m_phase = ReconciliationPhase::INIT_REQUESTED;

        LogDebug(BCLog::TXRECONCILIATION, "Reconciliation initiated by peer=%d with the following params: remote_q=%d, remote_set_size=%i.\n",
            peer_id, peer_q_converted, peer_recon_set_size);

        return std::nullopt;
    }

    bool ShouldRespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return false;
        if (peer_state->m_we_initiate) return false;

        // Return if there is nothing to respond to
        if (peer_state->m_phase != ReconciliationPhase::INIT_REQUESTED) {
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

bool TxReconciliationTracker::ForgetPeer(NodeId peer_id)
{
    return m_impl->ForgetPeer(peer_id);
}

std::optional<AddToSetError> TxReconciliationTracker::AddToSet(NodeId peer_id, const Wtxid& wtxid)
{
    return m_impl->AddToSet(peer_id, wtxid);
}

bool TxReconciliationTracker::TryRemovingFromSet(NodeId peer_id, const Wtxid& wtxid)
{
    return m_impl->TryRemovingFromSet(peer_id, wtxid);
}

std::variant<ReconCoefficients, ReconciliationError> TxReconciliationTracker::InitiateReconciliationRequest(NodeId peer_id)
{
    return m_impl->InitiateReconciliationRequest(peer_id);
}

std::optional<ReconciliationError> TxReconciliationTracker::HandleReconciliationRequest(NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q)
{
    return m_impl->HandleReconciliationRequest(peer_id, peer_recon_set_size, peer_q);
}

bool TxReconciliationTracker::ShouldRespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata)
{
    return m_impl->ShouldRespondToReconciliationRequest(peer_id, skdata);
}
} // namespace node
