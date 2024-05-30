// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
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

    /** Collection of inbound peers selected for fanout. Should get periodically rotated using RotateInboundFanoutTargets. */
    std::unordered_set<NodeId> m_inbound_fanout_targets GUARDED_BY(m_txreconciliation_mutex);

    /** Next time m_inbound_fanout_targets need to be rotated. */
    std::chrono::microseconds GUARDED_BY(m_txreconciliation_mutex) m_next_inbound_peer_rotation_time{0};

    TxReconciliationState* GetRegisteredPeerState(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(m_txreconciliation_mutex)
    {
        AssertLockHeld(m_txreconciliation_mutex);
        auto salt_or_state = m_states.find(peer_id);
        if (salt_or_state == m_states.end()) return nullptr;

        return std::get_if<TxReconciliationState>(&salt_or_state->second);
    }

public:
    explicit TxReconciliationTrackerImpl(uint32_t recon_version) : m_recon_version(recon_version) {}

    uint64_t PreRegisterPeer(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        LogDebug(BCLog::TXRECONCILIATION, "Pre-register peer=%d.\n", peer_id);
        const uint64_t local_salt{FastRandomContext().rand64()};

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

    void ForgetPeer(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        auto peer_state = GetRegisteredPeerState(peer_id);
        if (peer_state && !peer_state->m_we_initiate) {
            Assert(m_inbounds_count > 0);
            --m_inbounds_count;
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

        // TODO: We should compute the short_id here here first and see if there's any collision
        // if so, return AddToSetResult::Collision(wtxid)

        // Transactions which don't make it to the set due to the limit are announced via fanout.
        if (peer_state->m_local_set.size() >= MAX_RECONSET_SIZE) {
            LogDebug(BCLog::TXRECONCILIATION, "Reconciliation set maximum size reached for peer=%d.\n", peer_id);
            return AddToSetError(ReconciliationError::FULL_RECON_SET);
        }

        // The caller currently keeps track of the per-peer transaction announcements, so it
        // should not attempt to add same tx to the set twice. However, if that happens, we will
        // simply ignore it.
        if (peer_state->m_local_set.insert(wtxid).second) {
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
            LogDebug(BCLog::TXRECONCILIATION, "Removed %s from the reconciliation set for peer=%d. Now the set contains %i transactions.\n",
                wtxid.ToString(), peer_id, peer_state->m_local_set.size());
        } else {
            LogDebug(BCLog::TXRECONCILIATION, "Couldn't remove %s from the reconciliation set for peer=%d. Transaction not found.\n",
                wtxid.ToString(), peer_id);
        }

        return removed;
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
    return m_impl->PreRegisterPeer(peer_id);
}

std::optional<ReconciliationError> TxReconciliationTracker::RegisterPeer(NodeId peer_id, bool is_peer_inbound, uint32_t peer_recon_version, uint64_t remote_salt)
{
    return m_impl->RegisterPeer(peer_id, is_peer_inbound, peer_recon_version, remote_salt);
}

bool TxReconciliationTracker::IsPeerRegistered(NodeId peer_id) const
{
    return m_impl->IsPeerRegistered(peer_id);
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
