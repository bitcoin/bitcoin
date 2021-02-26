// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txreconciliation.h>

/** Static component of the salt used to compute short txids for transaction reconciliation. */
static const std::string RECON_STATIC_SALT = "Tx Relay Salting";
/**
 * When considering whether we should flood to an outbound connection supporting reconciliation,
 * see how many outbound connections are already used for flooding. Flood only if the limit is not reached.
 * It helps to save bandwidth and reduce the privacy leak.
 */
static constexpr uint32_t MAX_OUTBOUND_FLOOD_TO = 8;

static uint256 ComputeSalt(uint64_t local_salt, uint64_t remote_salt)
{
    uint64_t salt1 = local_salt, salt2 = remote_salt;
    if (salt1 > salt2) std::swap(salt1, salt2);
    static const auto RECON_SALT_HASHER = TaggedHash(RECON_STATIC_SALT);
    return (CHashWriter(RECON_SALT_HASHER) << salt1 << salt2).GetSHA256();
}


/**
* After a reconciliation round is over, the local q coefficient may be adjusted to enable
* better accuracy of future set difference estimations.
* Recompute q in case of full reconciliation success (both initially or after extension).
* In case reconciliation completely failed (initial and extension), fallback to the default q,
* set to cause an overestimation, but should converge to the reasonable q in the next round.
* Note that accurate recompute in case of complete failure is difficult,
* because it requires waiting for GETDATA/INV the peer would send to us, and find
* the actual difference from there (also may be inaccurate due to the latencies).
*/
static double RecomputeQ(uint8_t local_set_size, uint8_t actual_local_missing, uint8_t actual_remote_missing)
{
    const uint8_t remote_set_size = local_set_size + actual_local_missing - actual_remote_missing;
    const uint8_t set_size_diff = std::abs(local_set_size - remote_set_size);
    const uint8_t min_size = std::min(local_set_size, remote_set_size);
    const uint8_t actual_difference = actual_local_missing + actual_remote_missing;
    if (min_size != 0) {
        double result = double(actual_difference - set_size_diff) / min_size;
        assert(result >= 0 && result <= 2);
        return result;
    }
    return DEFAULT_RECON_Q;
}

static Optional<std::pair<Minisketch, uint16_t>> ParseRemoteSketch(const ReconciliationState& recon_state,
    const std::vector<uint8_t>& skdata)
{
    uint16_t remote_sketch_capacity = uint16_t(skdata.size() / BYTES_PER_SKETCH_CAPACITY);
    if (remote_sketch_capacity != 0) {
        Minisketch remote_sketch = Minisketch(RECON_FIELD_SIZE, 0, remote_sketch_capacity).Deserialize(skdata);
        return std::make_pair(remote_sketch, remote_sketch_capacity);
    } else {
        return nullopt;
    }
}

std::tuple<bool, bool, uint32_t, uint64_t> TxReconciliationTracker::SuggestReconciling(const NodeId peer_id, bool inbound)
{
    bool be_recon_requestor, be_recon_responder;
    // Currently reconciliation requests flow only in one direction inbound->outbound.
    if (inbound) {
        be_recon_requestor = false;
        be_recon_responder = true;
    } else {
        be_recon_requestor = true;
        be_recon_responder = false;
    }

    uint32_t recon_version = 1;
    uint64_t m_local_recon_salt(GetRand(UINT64_MAX));
    WITH_LOCK(m_local_salts_mutex, m_local_salts.emplace(peer_id, m_local_recon_salt));

    return std::make_tuple(be_recon_requestor, be_recon_responder, recon_version, m_local_recon_salt);
}

bool TxReconciliationTracker::EnableReconciliationSupport(const NodeId peer_id, bool inbound,
    bool recon_requestor, bool recon_responder, uint32_t recon_version, uint64_t remote_salt,
    size_t outbound_flooders)
{
    // Do not support reconciliation salt/version updates
    LOCK(m_states_mutex);
    auto recon_state = m_states.find(peer_id);
    if (recon_state != m_states.end()) return false;

    if (recon_version != 1) return false;

    // Do not flood through inbound connections which support reconciliation to save bandwidth.
    // Flood only through a limited number of outbound connections.
    bool flood_to = false;
    if (inbound) {
        // We currently don't support reconciliations with inbound peers which
        // don't want to be reconciliation senders (request our sketches),
        // or want to be reconciliation responders (send us their sketches).
        // Just ignore SENDRECON and use normal flooding for transaction relay with them.
        if (!recon_requestor) return false;
        if (recon_responder) return false;
    } else {
        // We currently don't support reconciliations with outbound peers which
        // don't want to be reconciliation responders (send us their sketches),
        // or want to be reconciliation senders (request our sketches).
        // Just ignore SENDRECON and use normal flooding for transaction relay with them.
        if (recon_requestor) return false;
        if (!recon_responder) return false;
        // TODO: Flood only through a limited number of outbound connections.
        flood_to = outbound_flooders < MAX_OUTBOUND_FLOOD_TO;
    }

    // Reconcile with all outbound peers supporting reconciliation (even if we flood to them),
    // to not miss transactions they have for us but won't flood.
    if (recon_responder) {
        LOCK(m_queue_mutex);
        m_queue.push_back(peer_id);
    }

    uint64_t local_peer_salt = WITH_LOCK(m_local_salts_mutex, return m_local_salts.at(peer_id));
    uint256 full_salt = ComputeSalt(local_peer_salt, remote_salt);

    m_states.emplace(peer_id, ReconciliationState(recon_requestor, recon_responder,
                        flood_to, full_salt.GetUint64(0), full_salt.GetUint64(1)));
    return true;
}

Optional<std::pair<uint16_t, uint16_t>> TxReconciliationTracker::MaybeRequestReconciliation(const NodeId peer_id)
{
    LOCK(m_states_mutex);
    auto recon_state = m_states.find(peer_id);
    if (recon_state == m_states.end()) return nullopt;
    if (recon_state->second.GetOutgoingPhase() != RECON_NONE) return nullopt;

    LOCK(m_queue_mutex);
    if (m_queue.size() > 0) {
        // Request transaction reconciliation periodically to efficiently exchange transactions.
        // To make reconciliation predictable and efficient, we reconcile with peers in order based on the queue,
        // and with a delay between requests.
        auto current_time = GetTime<std::chrono::seconds>();
        if (m_next_recon_request < current_time && m_queue.back() == peer_id) {
            recon_state->second.UpdateOutgoingPhase(RECON_INIT_REQUESTED);
            m_queue.pop_back();
            m_queue.push_front(peer_id);
            UpdateNextReconRequest(current_time);
            return std::make_pair(recon_state->second.GetLocalSetSize(), recon_state->second.GetLocalQ());
        }
    }
    return nullopt;
}

void TxReconciliationTracker::HandleReconciliationRequest(const NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q)
{
    double peer_q_converted = double(peer_q * Q_PRECISION);
    if (peer_q_converted < 0 || peer_q_converted > 2) return;

    LOCK(m_states_mutex);
    auto recon_state = m_states.find(peer_id);
    if (recon_state == m_states.end()) return;
    if (recon_state->second.GetIncomingPhase() != RECON_NONE) return;
    if (!recon_state->second.IsRequestor()) return;

    recon_state->second.PrepareIncoming(peer_recon_set_size, peer_q_converted, NextReconRespond());
    recon_state->second.UpdateIncomingPhase(RECON_INIT_REQUESTED);
}

Optional<std::vector<uint8_t>> TxReconciliationTracker::MaybeRespondToReconciliationRequest(const NodeId peer_id)
{
    LOCK(m_states_mutex);
    auto recon_state = m_states.find(peer_id);
    if (recon_state == m_states.end()) return nullopt;
    if (!recon_state->second.IsRequestor()) return nullopt;
    // Respond to a requested reconciliation to enable efficient transaction exchange.
    // For initial requests, respond only periodically to a) limit CPU usage for sketch computation,
    // and, b) limit transaction possession privacy leak.
    // It's safe to respond to extension request without a delay because they are already limited by initial requests.

    auto current_time = GetTime<std::chrono::microseconds>();

    auto incoming_phase = recon_state->second.GetIncomingPhase();
    bool timely_initial_request = incoming_phase == RECON_INIT_REQUESTED && current_time > recon_state->second.GetNextRespond();
    bool extension_request = incoming_phase == RECON_EXT_REQUESTED;
    if (!timely_initial_request && !extension_request) {
        return nullopt;
    }

    std::vector<unsigned char> response_skdata;
    Minisketch sketch;
    if (timely_initial_request) {
        uint16_t sketch_capacity = recon_state->second.EstimateSketchCapacity();
        sketch = recon_state->second.GetLocalBaseSketch(sketch_capacity);
        recon_state->second.UpdateIncomingPhase(RECON_INIT_RESPONDED);
        recon_state->second.PrepareForExtensionRequest(sketch_capacity);
        if (sketch) response_skdata = sketch.Serialize();
    } else {
        sketch = recon_state->second.GetLocalExtendedSketch();
        recon_state->second.UpdateIncomingPhase(RECON_EXT_RESPONDED);
        // Local extension sketch can be null only if initial sketch or initial capacity was 0,
        // in which case we would have terminated reconciliation already.
        assert(sketch);
        response_skdata = sketch.Serialize();
        // For the sketch extension, send only the higher sketch elements.
        size_t lower_bytes_to_drop = recon_state->second.GetCapacitySnapshot() * BYTES_PER_SKETCH_CAPACITY;
        // Extended sketch is twice the size of the initial sketch (which is m_capacity_snapshot).
        assert(lower_bytes_to_drop <= response_skdata.size());
        response_skdata.erase(response_skdata.begin(), response_skdata.begin() + lower_bytes_to_drop);
    }
    return response_skdata;
}

std::vector<uint256> TxReconciliationTracker::FinalizeIncomingReconciliation(const NodeId peer_id,
    bool recon_result, const std::vector<uint32_t>& ask_shortids)
{
    std::vector<uint256> remote_missing;
    LOCK(m_states_mutex);
    auto recon_state = m_states.find(peer_id);
    if (recon_state == m_states.end()) return remote_missing;

    assert(recon_state->second.IsRequestor());
    const auto incoming_phase = recon_state->second.GetIncomingPhase();
    const bool phase_init_responded = incoming_phase == RECON_INIT_RESPONDED;

    if (!phase_init_responded) return remote_missing;

    if (recon_result) {
        remote_missing = recon_state->second.GetWTXIDsFromShortIDs(ask_shortids);
    } else {
        remote_missing = recon_state->second.GetLocalSet();
    }
    recon_state->second.FinalizeIncomingReconciliation();
    recon_state->second.UpdateIncomingPhase(RECON_NONE);
    return remote_missing;
}

Optional<std::tuple<bool, bool, std::vector<uint32_t>, std::vector<uint256>>> TxReconciliationTracker::HandleSketch(
    const NodeId peer_id, int common_version, std::vector<uint8_t>& skdata)
{
    if (skdata.size() / BYTES_PER_SKETCH_CAPACITY > MAX_SKETCH_CAPACITY) {
        return nullopt;
    }

    LOCK(m_states_mutex);
    auto recon_state = m_states.find(peer_id);
    if (recon_state == m_states.end()) return nullopt;

    assert(recon_state->second.IsResponder());

    const auto outgoing_phase = recon_state->second.GetOutgoingPhase();
    const bool phase_init_requested = outgoing_phase == RECON_INIT_REQUESTED;

    if (!phase_init_requested) return nullopt;

    Minisketch remote_sketch;
    uint16_t remote_sketch_capacity = 0;
    auto parsed_remote_sketch = ParseRemoteSketch(recon_state->second, skdata);
    if (parsed_remote_sketch) {
        remote_sketch = (*parsed_remote_sketch).first;
        remote_sketch_capacity = (*parsed_remote_sketch).second;
    }

    Minisketch local_sketch = recon_state->second.GetLocalBaseSketch(remote_sketch_capacity);

    if (remote_sketch_capacity == 0 || !remote_sketch || !local_sketch) {
        LogPrint(BCLog::NET, "Outgoing reconciliation failed due to %s \n",
            remote_sketch_capacity == 0 ? "empty sketch" : "minisketch API failure");
        std::vector<uint256> remote_missing = recon_state->second.GetLocalSet();
        recon_state->second.FinalizeOutgoingReconciliation(true, DEFAULT_RECON_Q);
        recon_state->second.UpdateOutgoingPhase(RECON_NONE);
        return std::make_tuple(true, false, std::vector<uint32_t>(), remote_missing);
    }

    assert(remote_sketch);
    assert(local_sketch);
    // Attempt to decode the set difference
    std::vector<uint64_t> differences(remote_sketch_capacity);
    if (local_sketch.Merge(remote_sketch).Decode(differences)) {
        // Reconciliation over the current working sketch succeeded
        LogPrint(BCLog::NET, "Outgoing reconciliation succeeded\n");
        auto missing_txs = recon_state->second.GetRelevantIDsFromShortIDs(differences);
        std::vector<uint32_t> local_missing = missing_txs.first;
        std::vector<uint256> remote_missing = missing_txs.second;

        size_t local_set_size = recon_state->second.GetLocalSetSize();
        recon_state->second.FinalizeOutgoingReconciliation(true, RecomputeQ(local_set_size, local_missing.size(), remote_missing.size()));
        recon_state->second.UpdateOutgoingPhase(RECON_NONE);
        return std::make_tuple(true, true, local_missing, remote_missing);
    } else {
        // Initial reconciliation failed.
        // Store the received sketch and the local sketch, request extension.
        LogPrint(BCLog::NET, "Outgoing reconciliation initially failed, requesting extension sketch\n");
        recon_state->second.UpdateOutgoingPhase(RECON_EXT_REQUESTED);
        recon_state->second.PrepareForExtensionResponse(remote_sketch_capacity, skdata);
        return std::make_tuple(false, false, std::vector<uint32_t>(), std::vector<uint256>());
    }
}

void TxReconciliationTracker::HandleIncomingExtensionRequest(const NodeId peer_id)
{
    LOCK(m_states_mutex);
    auto recon_state = m_states.find(peer_id);
    if (recon_state == m_states.end()) return;
    if (recon_state->second.GetIncomingPhase() != RECON_INIT_RESPONDED) return;
    recon_state->second.UpdateIncomingPhase(RECON_EXT_REQUESTED);
}
