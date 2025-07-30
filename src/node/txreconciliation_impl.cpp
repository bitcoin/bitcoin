// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/txreconciliation.h>
#include <node/txreconciliation_impl.h>

#include <common/system.h>
#include <util/check.h>

#include <unordered_map>
#include <variant>

namespace node {
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

public:
    explicit TxReconciliationTrackerImpl(uint32_t recon_version) : m_recon_version(recon_version) {}

    uint64_t PreRegisterPeer(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);

        LogPrintLevel(BCLog::TXRECONCILIATION, BCLog::Level::Debug, "Pre-register peer=%d\n", peer_id);
        const uint64_t local_salt{FastRandomContext().rand64()};

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

        auto peer_state = m_states.find(peer_id);
        if (peer_state == m_states.end()) return ReconciliationRegisterResult::NOT_FOUND;
        if (std::holds_alternative<TxReconciliationState>(peer_state->second)) {
            return ReconciliationRegisterResult::ALREADY_REGISTERED;
        }
        uint64_t local_salt = std::get<uint64_t>(peer_state->second);

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
        peer_state->second = TxReconciliationState(!is_peer_inbound, full_salt.GetUint64(0), full_salt.GetUint64(1));
        return ReconciliationRegisterResult::SUCCESS;
    }

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
        if (m_states.erase(peer_id)) {
            LogPrintLevel(BCLog::TXRECONCILIATION, BCLog::Level::Debug, "Forget txreconciliation state of peer=%d\n", peer_id);
        }
    }
};

TxReconciliationTracker::TxReconciliationTracker(uint32_t recon_version) : m_impl{std::make_unique<TxReconciliationTrackerImpl>(recon_version)} {}

TxReconciliationTracker::~TxReconciliationTracker() = default;

uint64_t TxReconciliationTracker::PreRegisterPeer(NodeId peer_id)
{
    return m_impl->PreRegisterPeer(peer_id);
}

ReconciliationRegisterResult TxReconciliationTracker::RegisterPeer(NodeId peer_id, bool is_peer_inbound, uint32_t peer_recon_version, uint64_t remote_salt)
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
} // namespace node
