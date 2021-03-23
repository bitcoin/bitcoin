// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txreconciliation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(RegisterPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    const uint64_t salt = 0;

    // Prepare a peer for reconciliation.
    tracker.PreRegisterPeer(0);

    // Invalid version.
    BOOST_CHECK_EQUAL(tracker.RegisterPeer(/*peer_id=*/0, /*is_peer_inbound=*/true,
                                           /*peer_recon_version=*/0, salt),
                      ReconciliationRegisterResult::PROTOCOL_VIOLATION);

    // Valid registration (inbound and outbound peers).
    BOOST_REQUIRE(!tracker.IsPeerRegistered(0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(0, true, 1, salt), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(!tracker.IsPeerRegistered(1));
    tracker.PreRegisterPeer(1);
    BOOST_REQUIRE(tracker.RegisterPeer(1, false, 1, salt) == ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(1));

    // Reconciliation version is higher than ours, should be able to register.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(2));
    tracker.PreRegisterPeer(2);
    BOOST_REQUIRE(tracker.RegisterPeer(2, true, 2, salt) == ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(2));

    // Try registering for the second time.
    BOOST_REQUIRE(tracker.RegisterPeer(1, false, 1, salt) == ReconciliationRegisterResult::ALREADY_REGISTERED);

    // Do not register if there were no pre-registration for the peer.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(100, true, 1, salt), ReconciliationRegisterResult::NOT_FOUND);
    BOOST_CHECK(!tracker.IsPeerRegistered(100));
}

BOOST_AUTO_TEST_CASE(ForgetPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // Removing peer after pre-registring works and does not let to register the peer.
    tracker.PreRegisterPeer(peer_id0);
    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::NOT_FOUND);

    // Removing peer after it is registered works.
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));
    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(IsPeerRegisteredTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(ShouldFanoutToTest)
{
    TxReconciliationTracker tracker(1);
    NodeId peer_id0 = 0;
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);

    // If peer is not registered for reconciliation, it should be always chosen for flooding.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK(tracker.ShouldFanoutTo(GetRandHash(), hasher, peer_id0,
                                           /*inbounds_nonrcncl_tx_relay=*/0, /*outbounds_nonrcncl_tx_relay=*/0));
    }

    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    // Same after pre-registering.
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK(tracker.ShouldFanoutTo(GetRandHash(), hasher, peer_id0,
                                           /*inbounds_nonrcncl_tx_relay=*/0, /*outbounds_nonrcncl_tx_relay=*/0));
    }

    // Once the peer is registered, it should be selected for flooding of some transactions.
    // Since there is only one reconciling peer, it will be selected for all transactions.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/false, 1, 1), ReconciliationRegisterResult::SUCCESS);
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK(tracker.ShouldFanoutTo(GetRandHash(), hasher, peer_id0,
                                           /*inbounds_nonrcncl_tx_relay=*/0, /*outbounds_nonrcncl_tx_relay=*/0));
    }

    // Don't select a fanout target if it was already fanouted sufficiently.
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK(!tracker.ShouldFanoutTo(GetRandHash(), hasher, peer_id0,
                                            /*inbounds_nonrcncl_tx_relay=*/0, /*outbounds_nonrcncl_tx_relay=*/1));
    }

    tracker.ForgetPeer(peer_id0);
    // A forgotten (reconciliation-wise) peer should be always selected for fanout again.
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK(tracker.ShouldFanoutTo(GetRandHash(), hasher, peer_id0,
                                           /*inbounds_nonrcncl_tx_relay=*/0, /*outbounds_nonrcncl_tx_relay=*/0));
    }

    // Now for inbound connections.
    for (int i = 1; i < 31; ++i) {
        tracker.PreRegisterPeer(i);
        BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(i, /*is_peer_inbound=*/true, 1, 1), ReconciliationRegisterResult::SUCCESS);

    }

    // Relay to a fraction of registered inbound peers.
    for (int j = 0; j < 100; ++j) {
        size_t total_fanouted = 0;
        auto wtxid = GetRandHash();
        for (int i = 1; i < 31; ++i) {
            total_fanouted += tracker.ShouldFanoutTo(wtxid, hasher, i,
                                               /*inbounds_nonrcncl_tx_relay=*/0, /*outbounds_nonrcncl_tx_relay=*/0);
        }
        BOOST_CHECK_EQUAL(total_fanouted, 3);
    }

    // // Don't relay if there is sufficient non-reconciling peers
    for (int j = 0; j < 100; ++j) {
        size_t total_fanouted = 0;
        for (int i = 1; i < 31; ++i) {
            total_fanouted += tracker.ShouldFanoutTo(GetRandHash(), hasher, i,
                                               /*inbounds_nonrcncl_tx_relay=*/4, /*outbounds_nonrcncl_tx_relay=*/0);
        }
        BOOST_CHECK_EQUAL(total_fanouted, 0);
    }

}

// Also tests AddToPeerQueue
BOOST_AUTO_TEST_CASE(IsPeerNextToReconcileWith)
{
    TxReconciliationTracker tracker(1);
    NodeId peer_id0 = 0;

    BOOST_CHECK(!tracker.IsPeerNextToReconcileWith(peer_id0, std::chrono::seconds{1 + 8}));

    tracker.PreRegisterPeer(peer_id0);
    BOOST_CHECK(!tracker.IsPeerNextToReconcileWith(peer_id0, std::chrono::seconds{1 + 8}));

    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, false, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerNextToReconcileWith(peer_id0, std::chrono::seconds{1 + 8}));

    // Not enough time passed.
    BOOST_CHECK(!tracker.IsPeerNextToReconcileWith(peer_id0, std::chrono::seconds{1 + 8 + 1}));

    // Enough time passed.
    BOOST_CHECK(tracker.IsPeerNextToReconcileWith(peer_id0, std::chrono::seconds{1 + 8 + 8}));

    // TODO: test once peers' own reconciliation timers are bumped.

    // Two-peer setup
    tracker.ForgetPeer(peer_id0);
    NodeId peer_id1 = 1;
    NodeId peer_id2 = 2;
    {
        tracker.PreRegisterPeer(peer_id1);
        BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id1, false, 1, 1), ReconciliationRegisterResult::SUCCESS);

        tracker.PreRegisterPeer(peer_id2);
        BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id2, false, 1, 1), ReconciliationRegisterResult::SUCCESS);

        // Initial call
        bool peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, std::chrono::seconds{100});
        // Not enough time passed to pick the second peer.
        bool peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, std::chrono::seconds{100 + 1});
        BOOST_CHECK(peer1_next && !peer2_next);

        // Enough time passed to pick the second peer.
        peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, std::chrono::seconds{100 + 3});
        // Not enough time passed to pick the first peer.
        peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, std::chrono::seconds{100 + 3});
        BOOST_CHECK(peer2_next && !peer1_next);

        // TODO: test once peers' own reconciliation timers are bumped.
        // The global timer is ready to pick the first peer, but it's own reconciliation timer
        // has not passed yet. It should be moved to the end of the queue.
        // peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, std::chrono::seconds{100 + 5});
        // BOOST_CHECK(!peer1_next);
        // Peer 2 should be at the front of the queue (from the previous call of this function),
        // and enough time passed at its own reconciliation timer.
        // peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, std::chrono::seconds{100 + 3 + 8 + 1});
        // TODO: also test that once the peer is forgotten, it is also dropped from the queue.
    }
}

BOOST_AUTO_TEST_CASE(InitiateReconciliationRequest)
{
    TxReconciliationTracker tracker(1);
    NodeId peer_id0 = 0;

    BOOST_CHECK(tracker.InitiateReconciliationRequest(peer_id0) == std::nullopt);

    tracker.PreRegisterPeer(peer_id0);
    BOOST_CHECK(tracker.InitiateReconciliationRequest(peer_id0) == std::nullopt);

    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, false, 1, 1), ReconciliationRegisterResult::SUCCESS);

    const auto reconciliation_request_params = tracker.InitiateReconciliationRequest(peer_id0);
    BOOST_CHECK(reconciliation_request_params != std::nullopt);
    const auto [local_set_size, local_q_formatted] = (*reconciliation_request_params);
    BOOST_CHECK_EQUAL(local_set_size, 0);
    BOOST_CHECK_EQUAL(local_q_formatted, uint16_t(32767 * 0.25));

    // Start fresh
    tracker.ForgetPeer(peer_id0);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, false, 1, 1), ReconciliationRegisterResult::SUCCESS);
    tracker.AddToSet(peer_id0, GetRandHash());
    tracker.AddToSet(peer_id0, GetRandHash());
    tracker.AddToSet(peer_id0, GetRandHash());
    const auto reconciliation_request_params2 = tracker.InitiateReconciliationRequest(peer_id0);
    BOOST_CHECK(reconciliation_request_params2 != std::nullopt);
    const auto [local_set_size2, local_q_formatted2] = (*reconciliation_request_params2);
    BOOST_CHECK_EQUAL(local_set_size2, 3);
    BOOST_CHECK_EQUAL(local_q_formatted2, uint16_t(32767 * 0.25));
}

BOOST_AUTO_TEST_SUITE_END()
