// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txreconciliation.h>

#include <minisketch/include/minisketch.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

namespace {

// Taken verbatim from txreconciliation.cpp.
const std::string RECON_STATIC_SALT = "Tx Relay Salting";
constexpr unsigned int RECON_FIELD_SIZE = 32;
constexpr double RECON_Q = 0.01;
constexpr uint16_t Q_PRECISION{(2 << 14) - 1};
// Converted to int/seconds to be used in SetMockTime().
constexpr int RECON_REQUEST_INTERVAL = 1;
constexpr int RECON_RESPONSE_INTERVAL = 1;

class TxReconciliationTrackerTest
{
    TxReconciliationTracker m_tracker;

    uint64_t m_k0, m_k1;

    NodeId m_peer_id;

    public:

    std::vector<uint256> m_transactions;

    TxReconciliationTrackerTest(bool inbound)
    {
        m_peer_id = 1;
        uint64_t our_salt = 0;
        auto recon_params = m_tracker.SuggestReconciling(m_peer_id, inbound);
        uint64_t node_salt = std::get<3>(recon_params);
        if (inbound) {
            assert(m_tracker.EnableReconciliationSupport(m_peer_id, true, true, false, 1, our_salt));
        } else {
            assert(m_tracker.EnableReconciliationSupport(m_peer_id, false, false, true, 1, our_salt));
        }
        assert(m_tracker.IsPeerRegistered(m_peer_id));

        uint64_t salt1 = 0, salt2 = node_salt;
        if (salt1 > salt2) std::swap(salt1, salt2);
        static const auto RECON_SALT_HASHER = TaggedHash(RECON_STATIC_SALT);
        uint256 full_salt = (CHashWriter(RECON_SALT_HASHER) << salt1 << salt2).GetSHA256();

        m_k0 = full_salt.GetUint64(0);
        m_k1 = full_salt.GetUint64(1);
    }

    void HandleReconciliationRequest(uint16_t set_size=1, uint16_t q=uint16_t(RECON_Q * Q_PRECISION))
    {
        m_tracker.HandleReconciliationRequest(m_peer_id, set_size, q);
    }

    bool RespondToReconciliationRequest(std::vector<uint8_t>& skdata)
    {
        return m_tracker.RespondToReconciliationRequest(m_peer_id, skdata);
    }

    bool FinalizeInitByThem(bool recon_result,
        const std::vector<uint32_t>& remote_missing_short_ids, std::vector<uint256>& remote_missing,
        bool fake_peer=false)
    {
        return m_tracker.FinalizeInitByThem(fake_peer ? 2 : m_peer_id, recon_result, remote_missing_short_ids, remote_missing);
    }

    void HandleExtensionRequest()
    {
        m_tracker.HandleExtensionRequest(m_peer_id);
    }

    bool HandleSketch(const std::vector<uint8_t>& skdata,
        // returning values
        std::vector<uint32_t>& txs_to_request, std::vector<uint256>& txs_to_announce, std::optional<bool>& result,
        bool fake_peer=false)
    {
        return m_tracker.HandleSketch(fake_peer ? 2 : m_peer_id, skdata, txs_to_request, txs_to_announce, result);
    }

    std::optional<std::pair<uint16_t, uint16_t>> MaybeRequestReconciliation()
    {
        return m_tracker.MaybeRequestReconciliation(m_peer_id);
    }

    std::optional<size_t> GetPeerSetSize() const
    {
        return m_tracker.GetPeerSetSize(m_peer_id);
    }

    void AddTransactions(size_t count)
    {
        std::vector<uint256> their_txs;
        for (size_t i = 0; i < count; i++) {
            uint256 wtxid = GetRandHash();
            their_txs.push_back(wtxid);
            m_transactions.push_back(wtxid);
        }
        m_tracker.AddToReconSet(m_peer_id, their_txs);
    }

    uint32_t ComputeShortID(const uint256 wtxid) const
    {
        const uint64_t s = SipHashUint256(m_k0, m_k1, wtxid);
        const uint32_t short_txid = 1 + (s & 0xFFFFFFFF);
        return short_txid;
    }

    Minisketch ComputeSketch(std::vector<uint256> txs, uint16_t capacity)
    {
        Minisketch sketch = Minisketch(RECON_FIELD_SIZE, 0, capacity);
        for (const auto& wtxid: txs) {
            uint32_t short_txid = ComputeShortID(wtxid);
            sketch.Add(short_txid);
        }
        return sketch;
    }
};

}  // namespace

BOOST_AUTO_TEST_CASE(SuggestReconcilingTest)
{
    TxReconciliationTracker tracker;

    auto [we_initiate_recon, we_respond_recon, recon_version, recon_salt] = tracker.SuggestReconciling(0, true);
    assert(!we_initiate_recon);
    assert(we_respond_recon);
    assert(recon_version == 1); // RECON_VERSION in src/txreconciliation.cpp

    std::tie(we_initiate_recon, we_respond_recon, recon_version, recon_salt) = tracker.SuggestReconciling(1, false);
    assert(we_initiate_recon);
    assert(!we_respond_recon);
}

BOOST_AUTO_TEST_CASE(EnableReconciliationSupportTest)
{
    TxReconciliationTracker tracker;
    const uint64_t salt = 0;

    NodeId peer_id0 = 0;
    // Generate salt.
    tracker.SuggestReconciling(peer_id0, true);
    // Both roles are false, don't register.
    assert(!tracker.EnableReconciliationSupport(peer_id0, true, false, false, 1, salt));
    // Invalid roles for the given direction.
    assert(!tracker.EnableReconciliationSupport(peer_id0, true, false, true, 1, salt));
    // Invalid version.
    assert(!tracker.EnableReconciliationSupport(peer_id0, true, true, false, 0, salt));
    // Valid peer.
    assert(!tracker.IsPeerRegistered(peer_id0));
    assert(tracker.EnableReconciliationSupport(peer_id0, true, true, false, 1, salt));
    assert(tracker.IsPeerRegistered(peer_id0));

    NodeId unknown_peer = 100;
    // Do not register if salt is not generated for a given peer.
    assert(!tracker.EnableReconciliationSupport(unknown_peer, true, true, false, 1, salt));
    assert(!tracker.IsPeerRegistered(unknown_peer));
}

BOOST_AUTO_TEST_CASE(AddToReconSetTest)
{
    TxReconciliationTracker tracker;

    NodeId peer_id0 = 0;
    bool inbound = true;
    tracker.SuggestReconciling(peer_id0, inbound);
    assert(tracker.EnableReconciliationSupport(peer_id0, inbound, true, false, 1, 0));

    assert(tracker.GetPeerSetSize(peer_id0) == 0);

    size_t count = 10;
    for (size_t i = 0; i < count; i++) {
        tracker.AddToReconSet(peer_id0, std::vector<uint256>{GetRandHash()});
    }

    assert(tracker.GetPeerSetSize(peer_id0) == count);
}

BOOST_AUTO_TEST_CASE(MaybeRequestReconciliationTest)
{
    TxReconciliationTracker tracker;
    NodeId peer_id0 = 0;
    int64_t start_time = GetTime();
    SetMockTime(start_time);

    // Don't request from a non-registered peer.
    assert(!tracker.MaybeRequestReconciliation(peer_id0));

    tracker.SuggestReconciling(peer_id0, true);
    assert(tracker.EnableReconciliationSupport(peer_id0, true, true, false, 1, 0));

    // Don't request from an inbound peer.
    assert(!tracker.MaybeRequestReconciliation(peer_id0));

    // Make a request.
    NodeId peer_id1 = 1;
    tracker.SuggestReconciling(peer_id1, false);
    assert(tracker.EnableReconciliationSupport(peer_id1, false, false, true, 1, 0));
    auto request_data = tracker.MaybeRequestReconciliation(peer_id1);
    const auto [local_set_size, local_q_formatted] = (*request_data);
    assert(local_set_size == 0);
    assert(local_q_formatted == uint16_t(RECON_Q * Q_PRECISION));
    // Don't request until the first request is finalized/terminated.
    SetMockTime(start_time + RECON_REQUEST_INTERVAL);
    assert(!tracker.MaybeRequestReconciliation(peer_id1));
    tracker.RemovePeer(peer_id1);

    // The next request should happen after some interval.
    NodeId peer_id2 = 2;
    tracker.SuggestReconciling(peer_id2, false);
    assert(tracker.EnableReconciliationSupport(peer_id2, false, false, true, 1, 0));
    // Too soon
    SetMockTime(start_time + RECON_REQUEST_INTERVAL * 2 - 1);
    assert(!tracker.MaybeRequestReconciliation(peer_id2));
    SetMockTime(start_time + RECON_REQUEST_INTERVAL * 2);
    assert(tracker.MaybeRequestReconciliation(peer_id2));
    std::vector<uint32_t> txs_to_request;
    std::vector<uint256> txs_to_announce;
    std::optional<bool> recon_result;
    tracker.HandleSketch(peer_id2, std::vector<uint8_t>(), txs_to_request, txs_to_announce, recon_result);
    assert(recon_result && !*recon_result); // Check it's finalized.

    // Re-add peer 1.
    tracker.SuggestReconciling(peer_id1, false);
    assert(tracker.EnableReconciliationSupport(peer_id1, false, false, true, 1, 0));
    // Second peer is earlier in the queue.
    SetMockTime(start_time + RECON_REQUEST_INTERVAL * 3);
    assert(!tracker.MaybeRequestReconciliation(peer_id1));
    assert(tracker.MaybeRequestReconciliation(peer_id2));
    SetMockTime(start_time + RECON_REQUEST_INTERVAL * 4);
    assert(tracker.MaybeRequestReconciliation(peer_id1));

    // Remove peer 2 from the queue, so peer 1 should be selected again.
    tracker.RemovePeer(peer_id2);
    // Clear the state for peer 1.
    tracker.HandleSketch(peer_id1, std::vector<uint8_t>(), txs_to_request, txs_to_announce, recon_result);
    SetMockTime(start_time + RECON_REQUEST_INTERVAL * 5);
    assert(tracker.MaybeRequestReconciliation(peer_id1));
    tracker.RemovePeer(peer_id1);

    // Check that the request has a correct set size.
    NodeId peer_id3 = 3;
    tracker.SuggestReconciling(peer_id3, false);
    assert(tracker.EnableReconciliationSupport(peer_id3, false, false, true, 1, 0));

    size_t count = 10;
    for (size_t i = 0; i < count; i++) {
        tracker.AddToReconSet(peer_id3, std::vector<uint256>{GetRandHash()});
    }

    SetMockTime(start_time + RECON_REQUEST_INTERVAL * 6);
    request_data = tracker.MaybeRequestReconciliation(peer_id3);
    if (request_data) {
        const auto [set_size, q_formatted] = (*request_data);
        assert(set_size == count);
        assert(q_formatted == uint16_t(RECON_Q * Q_PRECISION));
    }
}

BOOST_AUTO_TEST_CASE(HandleReconciliationRequestTest)
{
    int64_t start_time = GetTime();
    std::vector<uint8_t> skdata;

    // Request from a non-registered peer should not trigger a response.
    TxReconciliationTracker tracker;
    SetMockTime(start_time);
    NodeId peer_id0 = 0;
    tracker.HandleReconciliationRequest(peer_id0, 1, 1);
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    assert(!tracker.RespondToReconciliationRequest(peer_id0, skdata));

    // Request from a non-initiating peer should be ignored.
    TxReconciliationTrackerTest tracker_test1(false);
    SetMockTime(start_time);
    tracker_test1.HandleReconciliationRequest();
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    assert(!tracker_test1.RespondToReconciliationRequest(skdata));

    // The node receives a reconciliation request and should respond with a sketch after a delay.
    TxReconciliationTrackerTest tracker_test2(true);
    SetMockTime(start_time);
    tracker_test2.HandleReconciliationRequest();
    // Too early, do not respond yet.
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL - 1);
    assert(!tracker_test2.RespondToReconciliationRequest(skdata));
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    assert(tracker_test2.RespondToReconciliationRequest(skdata));

    // Request at the wrong reconciliation phase should be ignored.
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL * 10);
    tracker_test2.HandleReconciliationRequest();
    assert(!tracker_test2.RespondToReconciliationRequest(skdata));
}

BOOST_AUTO_TEST_CASE(RespondToReconciliationRequestTest)
{
    int64_t start_time = GetTime();
    std::vector<uint8_t> skdata;
    NodeId peer_id0 = 0;

    // Check that we won't respond if we initiate.
    TxReconciliationTracker tracker;
    SetMockTime(start_time);
    tracker.SuggestReconciling(peer_id0, false);
    assert(tracker.EnableReconciliationSupport(peer_id0, false, false, true, 1, 0));
    tracker.HandleReconciliationRequest(peer_id0, 1, 1);
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    assert(!tracker.RespondToReconciliationRequest(peer_id0, skdata));

    // Check that we won't respond if the peer is removed ("unregistered").
    SetMockTime(start_time);
    TxReconciliationTracker tracker2;
    tracker2.SuggestReconciling(peer_id0, true);
    assert(tracker2.EnableReconciliationSupport(peer_id0, true, true, false, 1, 0));
    tracker2.HandleReconciliationRequest(peer_id0, 1, 1);
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    tracker2.RemovePeer(peer_id0);
    assert(!tracker2.RespondToReconciliationRequest(peer_id0, skdata));

    // The node receives a reconciliation request noting that the node's own local set is empty.
    // The node should terminate reconciliation by sending an empty sketch.
    TxReconciliationTrackerTest tracker_test(true);
    SetMockTime(start_time);
    tracker_test.HandleReconciliationRequest(1);
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    assert(tracker_test.RespondToReconciliationRequest(skdata));
    assert(skdata.size() == 0);

    // The node receives a reconciliation request noting that the initiator has an empty set.
    // The node should terminate reconciliation by sending an empty sketch.
    TxReconciliationTrackerTest tracker_test2(true);
    SetMockTime(start_time);
    tracker_test2.AddTransactions(5);
    tracker_test2.HandleReconciliationRequest(0);
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    assert(tracker_test2.RespondToReconciliationRequest(skdata));
    assert(skdata.size() == 0);

    // The node receives a reconciliation request and should respond with an expected sketch.
    TxReconciliationTrackerTest tracker_test3(true);
    SetMockTime(start_time);
    double q = 1;
    tracker_test3.AddTransactions(10);
    tracker_test3.HandleReconciliationRequest(1, q * Q_PRECISION);
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    assert(tracker_test3.RespondToReconciliationRequest(skdata));
    assert(tracker_test3.GetPeerSetSize() == 0);
    uint32_t expected_capacity = (10 - 1) + q * 1 + 1;
    Minisketch expected_sketch = tracker_test3.ComputeSketch(tracker_test3.m_transactions, expected_capacity);
    assert(skdata == expected_sketch.Serialize());
    // Then respond with an extension sketch.
    tracker_test3.HandleExtensionRequest();
    assert(tracker_test3.RespondToReconciliationRequest(skdata));
    std::vector<uint8_t> extended_sketch = tracker_test3.ComputeSketch(tracker_test3.m_transactions, expected_capacity * 2).Serialize();
    std::vector<uint8_t> sketch_extension(extended_sketch.begin() + extended_sketch.size() / 2, extended_sketch.end());
    assert(skdata == sketch_extension);
}

BOOST_AUTO_TEST_CASE(HandleExtensionRequestTest)
{
    int64_t start_time = GetTime();
    SetMockTime(start_time);
    TxReconciliationTrackerTest tracker_test(true);
    tracker_test.AddTransactions(5);

    // Extension request without initial request does nothing.
    std::vector<uint8_t> skdata;
    tracker_test.HandleExtensionRequest();
    assert(!tracker_test.RespondToReconciliationRequest(skdata));

    tracker_test.HandleReconciliationRequest();
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    assert(tracker_test.RespondToReconciliationRequest(skdata));
    assert(skdata.size() != 0);
    // Then respond with an extension sketch.
    tracker_test.HandleExtensionRequest();
    assert(tracker_test.RespondToReconciliationRequest(skdata));

    TxReconciliationTrackerTest tracker_test2(true);
    SetMockTime(start_time);
    tracker_test2.HandleReconciliationRequest();
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    std::vector<uint8_t> skdata2;
    assert(tracker_test2.RespondToReconciliationRequest(skdata2));
    assert(skdata2.size() == 0);
    // Do not respond if the initial sketch we sent out was empty.
    tracker_test2.HandleExtensionRequest();
    assert(!tracker_test2.RespondToReconciliationRequest(skdata2));
}

BOOST_AUTO_TEST_CASE(HandleInitialSketchTest)
{
    int64_t start_time = GetTime();
    SetMockTime(start_time);

    TxReconciliationTrackerTest tracker_test(false);
    std::vector<uint32_t> txs_to_request;
    std::vector<uint256> txs_to_announce;
    std::optional<bool> recon_result;
    // Protocol violation: wrong reconciliation phase.
    assert(!tracker_test.HandleSketch(std::vector<uint8_t>(), txs_to_request, txs_to_announce, recon_result, false));
    // Check that we won't respond to non-registered peer.
    assert(!tracker_test.HandleSketch(std::vector<uint8_t>(), txs_to_request, txs_to_announce, recon_result, true));

    // Handling valid sketch.
    tracker_test.AddTransactions(5);
    SetMockTime(start_time + RECON_REQUEST_INTERVAL);
    assert(tracker_test.MaybeRequestReconciliation());
    tracker_test.AddTransactions(10);

    std::vector<uint256> txs_to_sketch(tracker_test.m_transactions.begin(), tracker_test.m_transactions.begin() + 12);
    std::vector<uint256> node_will_announce(tracker_test.m_transactions.begin() + 12, tracker_test.m_transactions.end());
    // Add one extra tx.
    uint256 extra_tx = GetRandHash();
    txs_to_sketch.push_back(extra_tx);

    // Sketch should have sufficient capacity to decode the difference.
    Minisketch sketch = tracker_test.ComputeSketch(txs_to_sketch, 10);

    bool sketch_valid = tracker_test.HandleSketch(sketch.Serialize(), txs_to_request, txs_to_announce, recon_result);
    assert(sketch_valid);
    assert(recon_result && *recon_result);
    assert(txs_to_request == std::vector<uint32_t>{tracker_test.ComputeShortID(extra_tx)});
    assert(std::set<uint256>(txs_to_announce.begin(), txs_to_announce.end()) ==
        std::set<uint256>(node_will_announce.begin(), node_will_announce.end()));
    assert(tracker_test.GetPeerSetSize() == 0);

    // Early exit: a received sketch is empty, terminate reconciliation with a failure.
    tracker_test.m_transactions.clear();
    tracker_test.AddTransactions(5);
    SetMockTime(start_time + RECON_REQUEST_INTERVAL * 2);
    assert(tracker_test.MaybeRequestReconciliation());
    tracker_test.AddTransactions(10);

    txs_to_request.clear();
    txs_to_announce.clear();
    recon_result = std::nullopt;

    sketch_valid = tracker_test.HandleSketch(std::vector<uint8_t>(), txs_to_request, txs_to_announce, recon_result);
    assert(sketch_valid);
    assert(recon_result && !*recon_result);
    assert(txs_to_request.size() == 0);
    assert(std::set<uint256>(txs_to_announce.begin(), txs_to_announce.end()) ==
        std::set<uint256>(tracker_test.m_transactions.begin(), tracker_test.m_transactions.end()));
    assert(tracker_test.GetPeerSetSize() == 0);

    // Early exit: a local set is empty, terminate reconciliation with a failure.
    tracker_test.m_transactions.clear();
    SetMockTime(start_time + RECON_REQUEST_INTERVAL * 3);
    assert(tracker_test.MaybeRequestReconciliation());

    txs_to_request.clear();
    txs_to_announce.clear();
    recon_result = std::nullopt;

    std::vector<uint8_t> random_skdata = tracker_test.ComputeSketch(std::vector<uint256>{GetRandHash()}, 1).Serialize();

    sketch_valid = tracker_test.HandleSketch(random_skdata, txs_to_request, txs_to_announce, recon_result);
    assert(sketch_valid);
    assert(recon_result && !*recon_result);
    assert(txs_to_request.size() == 0);
    assert(txs_to_announce.size() == 0);

    // Check the limits: a received sketch could be at most MAX_SKETCH_CAPACITY = 2 << 12.
    tracker_test.m_transactions.clear();
    tracker_test.AddTransactions(5);
    SetMockTime(start_time + RECON_REQUEST_INTERVAL * 4);
    assert(tracker_test.MaybeRequestReconciliation());

    random_skdata = tracker_test.ComputeSketch(std::vector<uint256>{GetRandHash()}, (2 << 12) + 1).Serialize();
    sketch_valid = tracker_test.HandleSketch(random_skdata, txs_to_request, txs_to_announce, recon_result);
    assert(!sketch_valid);
}

BOOST_AUTO_TEST_CASE(HandleExtensionSketchTest)
{
    int64_t start_time = GetTime();
    SetMockTime(start_time);

    TxReconciliationTrackerTest tracker_test(false);

    // Extension success.
    tracker_test.AddTransactions(5);
    assert(tracker_test.MaybeRequestReconciliation());
    tracker_test.AddTransactions(10);

    std::vector<uint32_t> txs_to_request;
    std::vector<uint256> txs_to_announce;
    std::optional<bool> recon_result;

    std::vector<uint256> txs_to_sketch(tracker_test.m_transactions.begin(), tracker_test.m_transactions.begin() + 12);
    std::vector<uint256> node_will_announce(tracker_test.m_transactions.begin() + 12, tracker_test.m_transactions.end());
    // Add one extra tx.
    uint256 extra_tx = GetRandHash();
    txs_to_sketch.push_back(extra_tx);

    // Sketch should have low capacity, so that decoding suceeeds only after extension.
    std::vector<uint8_t> sketch = tracker_test.ComputeSketch(txs_to_sketch, 3).Serialize();

    bool sketch_valid = tracker_test.HandleSketch(sketch, txs_to_request, txs_to_announce, recon_result);
    assert(sketch_valid);
    assert(!recon_result);
    assert(txs_to_request.size() == 0);
    assert(txs_to_announce.size() == 0);

    std::vector<uint8_t> extendned_sketch = tracker_test.ComputeSketch(txs_to_sketch, 6).Serialize();
    std::vector<uint8_t> sketch_extension = std::vector<uint8_t>(extendned_sketch.begin() + extendned_sketch.size() / 2, extendned_sketch.end());

    sketch_valid = tracker_test.HandleSketch(sketch_extension, txs_to_request, txs_to_announce, recon_result);
    assert(sketch_valid);
    assert(recon_result && *recon_result);
    assert(txs_to_request == std::vector<uint32_t>{tracker_test.ComputeShortID(extra_tx)});
    assert(std::set<uint256>(txs_to_announce.begin(), txs_to_announce.end()) ==
        std::set<uint256>(node_will_announce.begin(), node_will_announce.end()));

    // Extension failure.
    tracker_test.m_transactions.clear();
    tracker_test.AddTransactions(5);
    SetMockTime(start_time + RECON_REQUEST_INTERVAL);
    assert(tracker_test.MaybeRequestReconciliation());
    tracker_test.AddTransactions(10);

    txs_to_request.clear();
    txs_to_announce.clear();
    recon_result = std::nullopt;

    txs_to_sketch.assign(tracker_test.m_transactions.begin(), tracker_test.m_transactions.begin() + 12);

    // Add one extra tx.
    extra_tx = GetRandHash();
    txs_to_sketch.push_back(extra_tx);

    // Sketch should have low capacity, so that decoding fails even after extension.
    sketch = tracker_test.ComputeSketch(txs_to_sketch, 1).Serialize();

    sketch_valid = tracker_test.HandleSketch(sketch, txs_to_request, txs_to_announce, recon_result);
    assert(sketch_valid);
    assert(!recon_result);
    assert(txs_to_request.size() == 0);
    assert(txs_to_announce.size() == 0);

    extendned_sketch = tracker_test.ComputeSketch(txs_to_sketch, 2).Serialize();
    sketch_extension = std::vector<uint8_t>(extendned_sketch.begin() + extendned_sketch.size() / 2, extendned_sketch.end());

    sketch_valid = tracker_test.HandleSketch(sketch_extension, txs_to_request, txs_to_announce, recon_result);
    assert(sketch_valid);
    assert(recon_result && !*recon_result);
    assert(txs_to_request.size() == 0);
    assert(std::set<uint256>(txs_to_announce.begin(), txs_to_announce.end()) ==
        std::set<uint256>(tracker_test.m_transactions.begin(), tracker_test.m_transactions.end()));

    // Check the limits: a received sketch extension could be at most MAX_SKETCH_CAPACITY = 2 << 12.
    TxReconciliationTrackerTest tracker_test2(false);
    tracker_test2.AddTransactions(10);
    assert(tracker_test2.MaybeRequestReconciliation());

    std::vector<uint256> too_many_transactions;
    for (size_t i = 0; i < (2 << 12) * 2; i++) {
        too_many_transactions.push_back(GetRandHash());
    }

    std::vector<uint8_t> random_skdata = tracker_test2.ComputeSketch(too_many_transactions, 2 << 12).Serialize();

    recon_result = std::nullopt;
    sketch_valid = tracker_test2.HandleSketch(random_skdata, txs_to_request, txs_to_announce, recon_result);
    assert(sketch_valid);
    assert(!recon_result);
    std::vector<uint8_t> random_extension = tracker_test2.ComputeSketch(too_many_transactions, (2 << 12) + 1).Serialize();
    sketch_valid = tracker_test2.HandleSketch(random_extension, txs_to_request, txs_to_announce, recon_result);
    assert(!sketch_valid);
}

BOOST_AUTO_TEST_CASE(FinalizeInitByThemTest)
{
    int64_t start_time = GetTime();
    SetMockTime(start_time);

    TxReconciliationTrackerTest tracker_test(true);
    std::vector<uint256> remote_missing;

    // Return false if called for an unknown peer.
    assert(!tracker_test.FinalizeInitByThem(false, std::vector<uint32_t>(), remote_missing, true));

    // Return false if called during the wrong reconciliation phase.
    assert(!tracker_test.FinalizeInitByThem(false, std::vector<uint32_t>(), remote_missing));

    tracker_test.HandleReconciliationRequest();

    // Return false if called during the wrong reconciliation phase.
    assert(!tracker_test.FinalizeInitByThem(false, std::vector<uint32_t>(), remote_missing));

    // Return true if called at a proper time. Announce all transactions if reconciliation failed.
    std::vector<uint8_t> skdata;
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL);
    assert(tracker_test.RespondToReconciliationRequest(skdata));
    assert(tracker_test.FinalizeInitByThem(false, std::vector<uint32_t>(), remote_missing));
    assert(remote_missing.size() == 0);

    // Return true if called at a proper time. Announce requested transactions if reconciliation
    // succeeded.
    tracker_test.AddTransactions(10);
    tracker_test.HandleReconciliationRequest();
    tracker_test.AddTransactions(5);
    SetMockTime(start_time + RECON_RESPONSE_INTERVAL * 2);
    assert(tracker_test.RespondToReconciliationRequest(skdata));
    std::vector<uint32_t> remote_missing_by_shortid;
    uint256 known_tx_asked_for = tracker_test.m_transactions[0];
    remote_missing_by_shortid.push_back(tracker_test.ComputeShortID(known_tx_asked_for));
    uint256 extra_tx = GetRandHash(); // An unknown transaction should just be ignored.
    remote_missing_by_shortid.push_back(tracker_test.ComputeShortID(extra_tx));
    assert(tracker_test.FinalizeInitByThem(true, remote_missing_by_shortid, remote_missing));
    assert(remote_missing.size() == 1);
    assert(remote_missing[0] == known_tx_asked_for);

}

BOOST_AUTO_TEST_CASE(RemovePeerTest)
{
    TxReconciliationTracker tracker;
    NodeId peer_id0 = 0;

    // Removing peer after generating salt works and does not let to register the peer.
    tracker.SuggestReconciling(peer_id0, true);
    tracker.RemovePeer(peer_id0);
    assert(!tracker.EnableReconciliationSupport(peer_id0, true, true, false, 1, 1));

    // Removing peer after it is registered works.
    tracker.SuggestReconciling(peer_id0, true);
    assert(!tracker.IsPeerRegistered(peer_id0));
    tracker.EnableReconciliationSupport(peer_id0, true, true, false, 1, 1);
    assert(tracker.IsPeerRegistered(peer_id0));
    tracker.RemovePeer(peer_id0);
    assert(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(IsPeerRegisteredTest)
{
    TxReconciliationTracker tracker;
    NodeId peer_id0 = 0;

    assert(!tracker.IsPeerRegistered(peer_id0));
    tracker.SuggestReconciling(peer_id0, true);
    assert(!tracker.IsPeerRegistered(peer_id0));
    assert(tracker.EnableReconciliationSupport(peer_id0, true, true, false, 1, 1));
    assert(tracker.IsPeerRegistered(peer_id0));
    tracker.RemovePeer(peer_id0);
    assert(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(IsPeerInitiatorTest)
{
    TxReconciliationTracker tracker;

    // Inbound peers are initiators.
    NodeId peer_id0 = 0;
    assert(!tracker.IsPeerInitiator(peer_id0));
    tracker.SuggestReconciling(peer_id0, true);
    assert(!tracker.IsPeerInitiator(peer_id0));
    assert(tracker.EnableReconciliationSupport(peer_id0, true, true, false, 1, 1));
    assert(*tracker.IsPeerInitiator(peer_id0));
    tracker.RemovePeer(peer_id0);
    assert(!tracker.IsPeerInitiator(peer_id0));

    // Outbound peers are not initiators.
    NodeId peer_id1 = 1;
    assert(!tracker.IsPeerInitiator(peer_id1));
    tracker.SuggestReconciling(peer_id1, false);
    assert(!tracker.IsPeerInitiator(peer_id1));
    assert(tracker.EnableReconciliationSupport(peer_id1, false, false, true, 1, 1));
    assert(!*tracker.IsPeerInitiator(peer_id1));
    tracker.RemovePeer(peer_id1);
    assert(!tracker.IsPeerInitiator(peer_id1));
}

BOOST_AUTO_TEST_CASE(GetPeerSetSizeTest)
{
    TxReconciliationTracker tracker;

    NodeId peer_id0 = 0;
    assert(!tracker.GetPeerSetSize(peer_id0));
    tracker.SuggestReconciling(peer_id0, true);
    assert(!tracker.GetPeerSetSize(peer_id0));
    assert(tracker.EnableReconciliationSupport(peer_id0, true, true, false, 1, 1));
    assert(*tracker.GetPeerSetSize(peer_id0) == 0);
    uint256 tx_to_add = GetRandHash();
    tracker.AddToReconSet(peer_id0, std::vector<uint256>{tx_to_add});
    assert(*tracker.GetPeerSetSize(peer_id0) == 1);
    // Adding for the second time should not work.
    tracker.AddToReconSet(peer_id0, std::vector<uint256>{tx_to_add});
    assert(*tracker.GetPeerSetSize(peer_id0) == 1);
    tracker.RemovePeer(peer_id0);
    assert(!tracker.GetPeerSetSize(peer_id0));
}

BOOST_AUTO_TEST_CASE(ShouldFloodToTest)
{
    TxReconciliationTracker tracker;

    NodeId peer_id0 = 0;
    uint256 wtxid = GetRandHash();
    assert(!tracker.ShouldFloodTo(wtxid, peer_id0, true));
    tracker.SuggestReconciling(peer_id0, true);
    assert(tracker.EnableReconciliationSupport(peer_id0, true, true, false, 1, 1));
    assert(tracker.ShouldFloodTo(wtxid, peer_id0, true));
    assert(!tracker.ShouldFloodTo(wtxid, peer_id0, !true));
    tracker.RemovePeer(peer_id0);
    assert(!tracker.ShouldFloodTo(wtxid, peer_id0, true));

    // Add 2 more inbound peers.
    tracker.SuggestReconciling(1, true);
    assert(tracker.EnableReconciliationSupport(1, true, true, false, 1, 1));
    tracker.SuggestReconciling(2, true);
    assert(tracker.EnableReconciliationSupport(2, true, true, false, 1, 1));

    bool flood0 = tracker.ShouldFloodTo(wtxid, peer_id0, true);
    bool flood1 = tracker.ShouldFloodTo(wtxid, 1, true);
    bool flood2 = tracker.ShouldFloodTo(wtxid, 2, true);

    assert(flood0 + flood1 + flood2 == 2);
}


BOOST_AUTO_TEST_SUITE_END()
