// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txreconciliation.h>
#include <node/txreconciliation_impl.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace node;

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(RegisterPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    const uint64_t salt = 0;

    // Prepare a peer for reconciliation.
    tracker.PreRegisterPeer(0);

    // Invalid version.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(/*peer_id=*/0, /*is_peer_inbound=*/true,
                                           /*peer_recon_version=*/0, salt),
                      ReconciliationRegisterResult::PROTOCOL_VIOLATION);

    // Valid registration (inbound and outbound peers).
    BOOST_REQUIRE(!tracker.IsPeerRegistered(0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(0, true, TXRECONCILIATION_VERSION, salt), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(!tracker.IsPeerRegistered(1));
    tracker.PreRegisterPeer(1);
    BOOST_REQUIRE(tracker.RegisterPeer(1, false, TXRECONCILIATION_VERSION, salt) == ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(tracker.IsPeerRegistered(1));

    // Reconciliation version is higher than ours, should be able to register.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(2));
    tracker.PreRegisterPeer(2);
    BOOST_REQUIRE(tracker.RegisterPeer(2, true, TXRECONCILIATION_VERSION+1, salt) == ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(tracker.IsPeerRegistered(2));

    // Try registering for the second time.
    BOOST_REQUIRE(tracker.RegisterPeer(1, false, TXRECONCILIATION_VERSION, salt) == ReconciliationRegisterResult::ALREADY_REGISTERED);

    // Do not register if there were no pre-registration for the peer.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(100, true, TXRECONCILIATION_VERSION, salt), ReconciliationRegisterResult::NOT_FOUND);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(100));
}

BOOST_AUTO_TEST_CASE(IsPeerRegisteredTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // Non-registered of simply pre-registered peers not count a registered.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    // Registered peer do.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));

    // Forgotten peers do not count either.
    tracker.ForgetPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(IsPeerNextToReconcileWithTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // If the peer is not fully registered, the method will return false, doesn't matter the current time
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.IsPeerNextToReconcileWith(peer_id0, GetTime<std::chrono::microseconds>()));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerNextToReconcileWith(peer_id0, GetTime<std::chrono::microseconds>()));

    // When the first peer is added to the reconciliation tracker, a full RECON_REQUEST_INTERVAL
    // is given to let transaction pile up, otherwise the node will request an empty reconciliation
    // right away
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, false, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    auto current_time = GetTime<std::chrono::microseconds>() + RECON_REQUEST_INTERVAL;
    BOOST_REQUIRE(tracker.IsPeerNextToReconcileWith(peer_id0, current_time));

    // Not enough time passed.
    current_time += RECON_REQUEST_INTERVAL - 1s;
    BOOST_REQUIRE(!tracker.IsPeerNextToReconcileWith(peer_id0, current_time));

    // Enough time passed, but the previous reconciliation is still pending.
    current_time += 1s;
    BOOST_REQUIRE(tracker.IsPeerNextToReconcileWith(peer_id0, current_time));

    // Two-peer setup
    tracker.ForgetPeer(peer_id0);
    NodeId peer_id1 = 1;
    NodeId peer_id2 = 2;

    // Partially register, check how they are not flagged as next
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id1, false, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    tracker.PreRegisterPeer(peer_id2);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id2, false, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);

    // Only one peer can be flagged as next at a time (in insertion order)
    current_time += RECON_REQUEST_INTERVAL;
    bool peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    bool peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    BOOST_REQUIRE(peer1_next && !peer2_next);

    // The frequency depends on the number of peers on re-sampling, we have two
    // so each one is picked every RECON_REQUEST_INTERVAL/2
    current_time += RECON_REQUEST_INTERVAL/2;
    peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    BOOST_REQUIRE(!peer1_next && peer2_next);

    current_time += RECON_REQUEST_INTERVAL/2;
    peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    BOOST_REQUIRE(peer1_next && !peer2_next);

    // If the peer has pending reconciliation, it doesn't affect the global timer.
    // Here it's peer2's turn, but we are currently reconciling with him, so his turn
    // is immediately passed to the next peer (peer1).
    BOOST_REQUIRE(tracker.InitiateReconciliationRequest(peer_id2).has_value());
    current_time += RECON_REQUEST_INTERVAL/2;
    bool peer2_skipped = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    peer2_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    BOOST_REQUIRE(peer2_skipped && peer1_next && !peer2_next);

    // A removed peer cannot be next
    tracker.ForgetPeer(peer_id2);
    current_time += RECON_REQUEST_INTERVAL/2;
    peer1_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    // After removing and resampling, the frequency changes given we have less peers now
    current_time += RECON_REQUEST_INTERVAL;
    bool peer1_next_next = tracker.IsPeerNextToReconcileWith(peer_id1, current_time);
    BOOST_REQUIRE(peer1_next && peer1_next_next);

    // It doesn't matter for how long we keep checking
    bool peer2_next_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time);
    bool peer2_next_next_next = tracker.IsPeerNextToReconcileWith(peer_id2, current_time + RECON_REQUEST_INTERVAL);
    BOOST_REQUIRE(!peer2_next_next && !peer2_next_next_next);
}

BOOST_AUTO_TEST_CASE(ForgetPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // Removing peer after pre-registering works and does not let to register the peer.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    tracker.ForgetPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::NOT_FOUND);

    // Removing peer after it is registered works.
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));
    tracker.ForgetPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    // Removing a non-existing peer does nothing
    NodeId not_a_peer_id = 1;
    BOOST_REQUIRE(!tracker.IsPeerRegistered(not_a_peer_id));
    tracker.ForgetPeer(not_a_peer_id);
}

BOOST_AUTO_TEST_CASE(AddToSetTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    Wtxid wtxid{Wtxid::FromUint256(frc.rand256())};

    // If the peer is not registered, adding to the set fails
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    auto r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE(!r.m_collision.has_value());

    // This holds even if the peer is just pre-registered
    tracker.PreRegisterPeer(peer_id0);
    r = tracker.AddToSet(peer_id0, wtxid);
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE(!r.m_collision.has_value());

    // As long as the peer is registered, adding a new wtxid to the set should work
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));

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
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id1, true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id1));

    // As long as the peer is registered, the transaction is not in the set, and there is no short id
    // collision, adding should work
    size_t added_txs = 0;
    while (added_txs < MAX_RECONSET_SIZE) {
        wtxid = Wtxid::FromUint256(frc.rand256());

        r = tracker.AddToSet(peer_id1, wtxid);
        if (r.m_succeeded) {
            BOOST_REQUIRE(!r.m_collision.has_value());
            ++added_txs;
        } else {
            BOOST_REQUIRE(r.m_collision.has_value());
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
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // Precompute collision
    Wtxid wtxid{Wtxid::FromUint256(uint256("c70d778bccef36a81aed8da0b819d2bd28bd8653e56a5d40903df1a0ade0b876"))};
    Wtxid collision{Wtxid::FromUint256(uint256("ae52a6ecb8733fba1f7af6022a8b9dd327d7825054229fafcad7e03c38ae2a50"))};

    // Register the peer with a predefined salt so we can force the collision
    tracker.PreRegisterPeerWithSalt(peer_id0, 2);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));

    // Once the peer is registered, we can try to add both transactions and check
    BOOST_REQUIRE(tracker.AddToSet(peer_id0, wtxid).m_succeeded);
    auto r = tracker.AddToSet(peer_id0, collision);
    BOOST_REQUIRE(!r.m_succeeded);
    BOOST_REQUIRE_EQUAL(r.m_collision.value(), wtxid);
}

BOOST_AUTO_TEST_CASE(TryRemovingFromSetTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    Wtxid wtxid{Wtxid::FromUint256(uint256("c70d778bccef36a81aed8da0b819d2bd28bd8653e56a5d40903df1a0ade0b876"))};
    Wtxid collision{Wtxid::FromUint256(uint256("ae52a6ecb8733fba1f7af6022a8b9dd327d7825054229fafcad7e03c38ae2a50"))};

    // If the peer is not registered, removing will fail
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // This holds even if the peer is just pre-registered (register specific salt so we can also check collisions)
    tracker.PreRegisterPeerWithSalt(peer_id0, 2);
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // If the peer is registered but the transaction is not part of the set, this will fail too
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // Only if the transaction is found the method will return true
    BOOST_REQUIRE(tracker.AddToSet(peer_id0, wtxid).m_succeeded);
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));
    // Removing twice won't work
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // Adding a transaction but removing a collision won't work
    BOOST_REQUIRE(tracker.AddToSet(peer_id0, wtxid).m_succeeded);
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, collision));
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));

    // And removing after forgetting the peer won't work either
    BOOST_REQUIRE(tracker.AddToSet(peer_id0, wtxid).m_succeeded);
    tracker.ForgetPeer(peer_id0);
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));
}

BOOST_AUTO_TEST_CASE(InitiateReconciliationRequestTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    // A reconciliation request cannot be initiated with a non-fully registered peer
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.InitiateReconciliationRequest(peer_id0).has_value());
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.InitiateReconciliationRequest(peer_id0).has_value());

    // For a registered peer with no pending transactions, we expect the set size to be zero, and the
    // reconciliation params to match the default
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    auto reconciliation_request_params = tracker.InitiateReconciliationRequest(peer_id0);
    BOOST_REQUIRE(reconciliation_request_params.has_value());
    auto [local_set_size, local_q_formatted] = reconciliation_request_params.value();
    BOOST_REQUIRE_EQUAL(local_set_size, 0);
    BOOST_REQUIRE_EQUAL(local_q_formatted, uint16_t(Q * Q_PRECISION));

    // We only initiate reconciliation with outbound peers. For inbounds, we expect them to initiate
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id1, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(!tracker.InitiateReconciliationRequest(peer_id1).has_value());

    // Start fresh
    tracker.ForgetPeer(peer_id0);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);

    // After adding some transactions to a registered peer we expect its set to contain that amount of transactions
    int n_added_txs = 0;
    int n_target_txs = frc.randrange(42) + 1;
    while (n_added_txs < n_target_txs) {
        if (tracker.AddToSet(peer_id0, Wtxid::FromUint256(frc.rand256())).m_succeeded) {
            ++n_added_txs;
        }
    }
    reconciliation_request_params = tracker.InitiateReconciliationRequest(peer_id0);
    BOOST_REQUIRE(reconciliation_request_params.has_value());
    std::tie(local_set_size, local_q_formatted) = reconciliation_request_params.value();
    BOOST_REQUIRE_EQUAL(local_set_size, n_added_txs);
    BOOST_REQUIRE_EQUAL(local_q_formatted, uint16_t(Q * Q_PRECISION));

    // Once we are reconciling with a peer, a new reconciliation cannot be initiated until the previous is completed
    BOOST_REQUIRE(!tracker.InitiateReconciliationRequest(peer_id0).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
