// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txreconciliation.h>
#include <net_processing.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(RegisterPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, INBOUND_FANOUT_DESTINATIONS_FRACTION, OUTBOUND_FANOUT_THRESHOLD);
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
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, INBOUND_FANOUT_DESTINATIONS_FRACTION, OUTBOUND_FANOUT_THRESHOLD);
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
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, INBOUND_FANOUT_DESTINATIONS_FRACTION, OUTBOUND_FANOUT_THRESHOLD);
    NodeId peer_id0 = 0;

    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(AddToSetTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, INBOUND_FANOUT_DESTINATIONS_FRACTION, OUTBOUND_FANOUT_THRESHOLD);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    Wtxid wtxid{Wtxid::FromUint256(frc.rand256())};

    // If the peer is not registered, adding to the set fails
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    auto r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE(!r.m_collision.has_value());

    // As long as the peer is registered, adding a new wtxid to the set should work
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(r.m_succeeded);
    BOOST_REQUIRE(!r.m_collision.has_value());

    // If the peer is dropped, adding wtxids to its set should fail
    tracker.ForgetPeer(peer_id0);
    Wtxid wtxid2{Wtxid::FromUint256(frc.rand256())};
    r = tracker.AddToSet(peer_id0, wtxid2);
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE(!r.m_collision.has_value());

    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id1, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id1));

    // As long as the peer is registered, the transaction is not in the set, and there is no short id
    // collision, adding should work
    size_t added_txs = 0;
    while (added_txs < MAX_RECONSET_SIZE) {
        wtxid = Wtxid::FromUint256(frc.rand256());
        Wtxid collision;

        r = tracker.AddToSet(peer_id1, wtxid);
        if (r.m_succeeded) {
            BOOST_REQUIRE(!r.m_collision.has_value());
            ++added_txs;
        } else {
            BOOST_REQUIRE_EQUAL(r.m_collision.value(), collision);
        }
    }

    // Adding one more item will fail due to the set being full
    r = tracker.AddToSet(peer_id1, Wtxid::FromUint256(frc.rand256()));
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE(!r.m_collision.has_value());

    // Trying to add the same item twice will just bypass
    r = tracker.AddToSet(peer_id1, wtxid);
    BOOST_REQUIRE(r.m_succeeded);
    BOOST_REQUIRE(!r.m_collision.has_value());
}

BOOST_AUTO_TEST_CASE(AddToSetCollisionTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, INBOUND_FANOUT_DESTINATIONS_FRACTION, OUTBOUND_FANOUT_THRESHOLD);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    // Precompute collision
    Wtxid wtxid{Wtxid::FromUint256(uint256("c70d778bccef36a81aed8da0b819d2bd28bd8653e56a5d40903df1a0ade0b876"))};
    Wtxid collision{Wtxid::FromUint256(uint256("ae52a6ecb8733fba1f7af6022a8b9dd327d7825054229fafcad7e03c38ae2a50"))};

    // Register the peer with a predefined salt so we can force the collision
    tracker.PreRegisterPeerWithSalt(peer_id0, 2);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, 1, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    // Once the peer is registered, we can try to add both transactions and check
    BOOST_REQUIRE(tracker.AddToSet(peer_id0, wtxid).m_succeeded);
    auto r = tracker.AddToSet(peer_id0, collision);
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE_EQUAL(r.m_collision.value(), wtxid);
}

// Also tests AddToPeerQueue
BOOST_AUTO_TEST_CASE(IsPeerNextToReconcileWith)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, INBOUND_FANOUT_DESTINATIONS_FRACTION, OUTBOUND_FANOUT_THRESHOLD);
    NodeId peer_id0 = 0;

    // If the peer is not fully registered, the method will return false, doesn't matter the current time
    BOOST_CHECK(!tracker.IsPeerNextToReconcileWith(peer_id0, GetTime<std::chrono::microseconds>()));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_CHECK(!tracker.IsPeerNextToReconcileWith(peer_id0, GetTime<std::chrono::microseconds>()));

    // When the first peer is added to the reconciliation tracker, a full RECON_REQUEST_INTERVAL
    // is given to let transaction pile up, otherwise the node will request an empty reconciliation
    // right away
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, false, 1, 1), ReconciliationRegisterResult::SUCCESS);
    auto current_time = GetTime<std::chrono::microseconds>() + RECON_REQUEST_INTERVAL;
    BOOST_CHECK(tracker.IsPeerNextToReconcileWith(peer_id0, current_time));

    // Not enough time passed.
    current_time += RECON_REQUEST_INTERVAL - 1s;
    BOOST_CHECK(!tracker.IsPeerNextToReconcileWith(peer_id0, current_time));

    // Enough time passed, but the previous reconciliation is still pending.
    current_time += 1s;
    BOOST_CHECK(tracker.IsPeerNextToReconcileWith(peer_id0, current_time));

    // TODO: expand these tests once there is a way to drop the pending reconciliation.

    // Two-peer setup
    tracker.ForgetPeer(peer_id0);
    NodeId peer_id1 = 1;
    NodeId peer_id2 = 2;
    {
        tracker.PreRegisterPeer(peer_id1);
        BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id1, false, 1, 1), ReconciliationRegisterResult::SUCCESS);
        tracker.PreRegisterPeer(peer_id2);
        BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id2, false, 1, 1), ReconciliationRegisterResult::SUCCESS);

        current_time += RECON_REQUEST_INTERVAL;
        bool peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
        bool peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
        BOOST_CHECK(peer1_next && !peer2_next);

        current_time += RECON_REQUEST_INTERVAL/2;
        peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
        peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
        BOOST_CHECK(!peer1_next && peer2_next);

        current_time += RECON_REQUEST_INTERVAL/2;
        peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
        peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
        BOOST_CHECK(peer1_next && !peer2_next);

        // If the peer has pending reconciliation, it doesn't affect the global timer.
        BOOST_REQUIRE(tracker.InitiateReconciliationRequest(peer_id2) != std::nullopt);
        current_time += RECON_REQUEST_INTERVAL/2;
        peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
        peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
        BOOST_CHECK(peer1_next && peer2_next);

        tracker.ForgetPeer(peer_id2);
        current_time += RECON_REQUEST_INTERVAL/2;
        peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
        peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
        BOOST_CHECK(peer1_next && !peer2_next);
    }
}

BOOST_AUTO_TEST_CASE(InitiateReconciliationRequest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION, INBOUND_FANOUT_DESTINATIONS_FRACTION, OUTBOUND_FANOUT_THRESHOLD);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

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
    tracker.AddToSet(peer_id0, Wtxid::FromUint256(frc.rand256()));
    tracker.AddToSet(peer_id0, Wtxid::FromUint256(frc.rand256()));
    tracker.AddToSet(peer_id0, Wtxid::FromUint256(frc.rand256()));
    const auto reconciliation_request_params2 = tracker.InitiateReconciliationRequest(peer_id0);
    BOOST_CHECK(reconciliation_request_params2 != std::nullopt);
    const auto [local_set_size2, local_q_formatted2] = (*reconciliation_request_params2);
    BOOST_CHECK_EQUAL(local_set_size2, 3);
    BOOST_CHECK_EQUAL(local_q_formatted2, uint16_t(32767 * 0.25));
}

BOOST_AUTO_TEST_SUITE_END()
