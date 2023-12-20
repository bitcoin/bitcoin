// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txreconciliation.h>

#include <common/system.h>
#include <logging.h>
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
/**
 * Maximum number of wtxids stored in a peer local set, bounded to protect the memory use of
 * reconciliation sets and short ids mappings, and CPU used for sketch computation.
 */
constexpr size_t MAX_SET_SIZE = 3000;

/**
 * Maximum number of transactions for which we store assigned fanout targets.
 */
constexpr size_t FANOUT_TARGETS_PER_TX_CACHE_SIZE = 3000;
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
     * Reconciliation protocol assumes using one role consistently: either a reconciliation
     * initiator (requesting sketches), or responder (sending sketches). This defines our role,
     * based on the direction of the p2p connection.
     *
     */
    bool m_we_initiate;

    /**
     * These values are used to salt short IDs, which is necessary for transaction reconciliations.
     */
    uint64_t m_k0, m_k1;

    /**
     * Store all wtxids which we would announce to the peer (policy checks passed, etc.)
     * in this set instead of announcing them right away. When reconciliation time comes, we will
     * compute a compressed representation of this set ("sketch") and use it to efficiently
     * reconcile this set with a set on the peer's side.
     */
    std::unordered_set<Wtxid, SaltedTxidHasher> m_local_set;

    TxReconciliationState(bool we_initiate, uint64_t k0, uint64_t k1) : m_we_initiate(we_initiate), m_k0(k0), m_k1(k1) {}
};

} // namespace

/** Actual implementation for TxReconciliationTracker's data structure. */
class TxReconciliationTracker::Impl
{
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

    /*
     * A least-recently-added cache tracking which peers we should fanout a transaction to.
     *
     * Since the time between cache accesses is on the order of seconds, returning an outdated
     * set of peers is not a concern (especially since we fanout to outbound peers, which should
     * be hard to manipulate).
     *
     * No need to use LRU (bump transaction order upon access) because in most cases
     * transactions are processed almost-sequentially.
     */
    std::deque<Wtxid> m_tx_fanout_targets_cache_order;
    std::map<Wtxid, std::vector<NodeId>> m_tx_fanout_targets_cache_data GUARDED_BY(m_txreconciliation_mutex);
    // Used for randomly choosing fanout targets and determines the corresponding cache.
    CSipHasher m_deterministic_randomizer;

    TxReconciliationState* GetRegisteredPeerState(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(m_txreconciliation_mutex)
    {
        AssertLockHeld(m_txreconciliation_mutex);
        auto salt_or_state = m_states.find(peer_id);
        if (salt_or_state == m_states.end()) return nullptr;

        auto* state = std::get_if<TxReconciliationState>(&salt_or_state->second);
        return state;
    }

public:
    explicit Impl(uint32_t recon_version, CSipHasher hasher) : m_recon_version(recon_version), m_deterministic_randomizer(std::move(hasher)) {}

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

        auto new_state = TxReconciliationState(!is_peer_inbound, full_salt.GetUint64(0), full_salt.GetUint64(1));;
        m_states.erase(recon_state);
        m_states.emplace(peer_id, std::move(new_state));

        return ReconciliationRegisterResult::SUCCESS;
    }

    bool AddToSet(NodeId peer_id, const Wtxid& wtxid) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return false;

        // Check if the reconciliation set is not at capacity for two reasons:
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
        if (peer_state->m_local_set.size() >= MAX_SET_SIZE) return false;

        // The caller currently keeps track of the per-peer transaction announcements, so it
        // should not attempt to add same tx to the set twice. However, if that happens, we will
        // simply ignore it.
        if (peer_state->m_local_set.insert(wtxid).second) {
            LogPrintLevel(BCLog::TXRECONCILIATION, BCLog::Level::Debug, "Added %s to the reconciliation set for peer=%d. " /* Continued */
                                                                        "Now the set contains %i transactions.\n",
                          wtxid.ToString(), peer_id, peer_state->m_local_set.size());
        }
        return true;
    }

    bool TryRemovingFromSet(NodeId peer_id, const Wtxid& wtxid_to_remove) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return false;

        return peer_state->m_local_set.erase(wtxid_to_remove) > 0;
    }

    void ForgetPeer(NodeId peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        if (m_states.erase(peer_id)) {
            LogPrintLevel(BCLog::TXRECONCILIATION, BCLog::Level::Debug, "Forget txreconciliation state of peer=%d\n", peer_id);
        }
    }

    /**
     * For calls within this class use GetRegisteredPeerState instead.
     */
    bool IsPeerRegistered(NodeId peer_id) const EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        auto recon_state = m_states.find(peer_id);
        return (recon_state != m_states.end() &&
                std::holds_alternative<TxReconciliationState>(recon_state->second));
    }

    // Not const because of caching.
    bool IsFanoutTarget(const Wtxid& wtxid, NodeId peer_id, bool we_initiate, double limit) EXCLUSIVE_LOCKS_REQUIRED(m_txreconciliation_mutex)
    {
        auto fanout_candidates = m_tx_fanout_targets_cache_data.find(wtxid);
        if (fanout_candidates != m_tx_fanout_targets_cache_data.end()) {
            return std::binary_search(fanout_candidates->second.begin(), fanout_candidates->second.end(), peer_id);
        }

        // We use the pre-determined randomness to give a consistent result per transaction,
        // thus making sure that no transaction gets "unlucky" if every per-peer roll fails.
        CSipHasher deterministic_randomizer{m_deterministic_randomizer};
        deterministic_randomizer.Write(wtxid.ToUint256());

        // The algorithm works as follows. We iterate through the peers (of a given direction)
        // hashing them with the given wtxid, and sort them by this hash.
        // We then consider top `limit` peers to be low-fanout flood targets.
        // The randomness should be seeded with wtxid to return consistent results for every call.

        // The integral part of `limit` is accounted in the higher 32 bits of the second element.
        // The fractional part of `limit` is stored in the lower 32 bits, and then we check
        // whether adding a random lower 32-bit value (first element) would end up modifying
        // the higher bits.
        const size_t targets_size = ((deterministic_randomizer.Finalize() & 0xFFFFFFFF) + uint64_t(limit * 0x100000000)) >> 32;

        std::vector<std::pair<uint64_t, NodeId>> best_peers;
        best_peers.reserve(m_states.size());

        for (const auto& indexed_state : m_states) {
            const auto cur_state = std::get_if<TxReconciliationState>(&indexed_state.second);
            if (cur_state && cur_state->m_we_initiate == we_initiate) {
                uint64_t hash_key = CSipHasher(deterministic_randomizer).Write(cur_state->m_k0).Finalize();
                best_peers.emplace_back(hash_key, indexed_state.first);
            }
        }

        // Sort by the assigned key.
        std::sort(best_peers.begin(), best_peers.end());
        best_peers.resize(targets_size);

        std::vector<NodeId> new_fanout_candidates;
        new_fanout_candidates.reserve(targets_size);
        for_each(best_peers.begin(), best_peers.end(),
                 [&new_fanout_candidates](auto& keyed_peer) { new_fanout_candidates.push_back(keyed_peer.second); });

        // Sort by NodeId.
        std::sort(new_fanout_candidates.begin(), new_fanout_candidates.end());
        bool found = std::binary_search(new_fanout_candidates.begin(), new_fanout_candidates.end(), peer_id);

        // If the cache is full, make room for the new entry.
        if (m_tx_fanout_targets_cache_order.size() == FANOUT_TARGETS_PER_TX_CACHE_SIZE) {
            auto expired_tx = m_tx_fanout_targets_cache_order.front();
            m_tx_fanout_targets_cache_data.erase(expired_tx);
            m_tx_fanout_targets_cache_order.pop_front();
        }
        m_tx_fanout_targets_cache_data.emplace(wtxid, std::move(new_fanout_candidates));
        m_tx_fanout_targets_cache_order.push_back(wtxid);

        return found;
    }

    bool ShouldFanoutTo(const Wtxid& wtxid, NodeId peer_id,
                        size_t inbounds_fanout_tx_relay, size_t outbounds_fanout_tx_relay)
        EXCLUSIVE_LOCKS_REQUIRED(!m_txreconciliation_mutex)
    {
        AssertLockNotHeld(m_txreconciliation_mutex);
        LOCK(m_txreconciliation_mutex);
        auto peer_state = GetRegisteredPeerState(peer_id);
        if (!peer_state) return true;
        // We decide whether a particular peer is a low-fanout flood target differently
        // based on its connection direction:
        // - for outbounds we have a fixed number of flood destinations;
        // - for inbounds we use a fraction of all inbound peers supporting tx relay.
        //
        // We first decide how many reconciling peers of a given direction we want to flood to,
        // and then generate a list of peers of that size for a given transaction. We then see
        // whether the given peer falls into this list.
        double destinations;
        if (peer_state->m_we_initiate) {
            destinations = OUTBOUND_FANOUT_DESTINATIONS - outbounds_fanout_tx_relay;
        } else {
            const size_t inbound_rcncl_peers = std::count_if(m_states.begin(), m_states.end(),
                                                             [](const auto& indexed_state) {
                                                                 const auto* cur_state = std::get_if<TxReconciliationState>(&indexed_state.second);
                                                                 if (cur_state) return !cur_state->m_we_initiate;
                                                                 return false;
                                                             });

            // Since we use the fraction for inbound peers, we first need to compute the total
            // number of inbound targets.
            const double inbound_targets = (inbounds_fanout_tx_relay + inbound_rcncl_peers) * INBOUND_FANOUT_DESTINATIONS_FRACTION;
            destinations = inbound_targets - inbounds_fanout_tx_relay;
        }

        // Pure optimization to avoid going through the peers when the odds of picking one are
        // too low.
        if (destinations < 0.001) {
            return false;
        }

        return IsFanoutTarget(wtxid, peer_id, peer_state->m_we_initiate, destinations);
    }
};

TxReconciliationTracker::TxReconciliationTracker(uint32_t recon_version, CSipHasher hasher) : m_impl{std::make_unique<TxReconciliationTracker::Impl>(recon_version, hasher)} {}

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

bool TxReconciliationTracker::AddToSet(NodeId peer_id, const Wtxid& wtxid)
{
    return m_impl->AddToSet(peer_id, wtxid);
}

bool TxReconciliationTracker::TryRemovingFromSet(NodeId peer_id, const Wtxid& wtxid_to_remove)
{
    return m_impl->TryRemovingFromSet(peer_id, wtxid_to_remove);
}

void TxReconciliationTracker::ForgetPeer(NodeId peer_id)
{
    m_impl->ForgetPeer(peer_id);
}

bool TxReconciliationTracker::IsPeerRegistered(NodeId peer_id) const
{
    return m_impl->IsPeerRegistered(peer_id);
}

bool TxReconciliationTracker::ShouldFanoutTo(const Wtxid& wtxid, NodeId peer_id,
                                             size_t inbounds_fanout_tx_relay, size_t outbounds_fanout_tx_relay)
{
    return m_impl->ShouldFanoutTo(wtxid, peer_id,
                                  inbounds_fanout_tx_relay, outbounds_fanout_tx_relay);
}
