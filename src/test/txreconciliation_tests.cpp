// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cmath>

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
                                           /*peer_recon_version=*/0, salt).value(),
                      ReconciliationError::PROTOCOL_VIOLATION);

    // Valid registration (inbound and outbound peers).
    BOOST_REQUIRE(!tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(!tracker.RegisterPeer(0, true, TXRECONCILIATION_VERSION, salt).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(!tracker.IsPeerRegistered(1));
    tracker.PreRegisterPeer(1);
    BOOST_REQUIRE(!tracker.RegisterPeer(1, false, TXRECONCILIATION_VERSION, salt).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(1));

    // Reconciliation version is higher than ours, should be able to register.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(2));
    tracker.PreRegisterPeer(2);
    BOOST_REQUIRE(!tracker.RegisterPeer(2, true, TXRECONCILIATION_VERSION+1, salt).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(2));

    // Try registering for the second time.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(1, false, TXRECONCILIATION_VERSION, salt).value(), ReconciliationError::ALREADY_REGISTERED);

    // Do not register if there were no pre-registration for the peer.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(100, true, TXRECONCILIATION_VERSION, salt).value(), ReconciliationError::NOT_FOUND);
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

    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1).has_value());
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
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/false, TXRECONCILIATION_VERSION, 1).has_value());
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
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound=*/false, TXRECONCILIATION_VERSION, 1).has_value());
    tracker.PreRegisterPeer(peer_id2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id2, /*is_peer_inbound=*/false, TXRECONCILIATION_VERSION, 1).has_value());

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
    BOOST_REQUIRE(std::holds_alternative<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id2)));
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
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1).value(), ReconciliationError::NOT_FOUND);

    // Removing peer after it is registered works.
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1).has_value());
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
    auto error = tracker.AddToSet(peer_id0, wtxid).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::NOT_FOUND);
    BOOST_REQUIRE(!error.m_collision.has_value());

    // This holds even if the peer is just pre-registered
    tracker.PreRegisterPeer(peer_id0);
    error = tracker.AddToSet(peer_id0, wtxid).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::NOT_FOUND);
    BOOST_REQUIRE(!error.m_collision.has_value());

    // As long as the peer is registered, adding a new wtxid to the set should work
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());

    // If the peer is dropped, adding wtxids to its set should fail
    tracker.ForgetPeer(peer_id0);
    Wtxid wtxid2{Wtxid::FromUint256(frc.rand256())};
    error = tracker.AddToSet(peer_id0, wtxid2).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::NOT_FOUND);
    BOOST_REQUIRE(!error.m_collision.has_value());

    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id1));

    // As long as the peer is registered, the transaction is not in the set, and there is no short id
    // collision, adding should work
    size_t added_txs = 0;
    while (added_txs < MAX_RECONSET_SIZE) {
        wtxid = Wtxid::FromUint256(frc.rand256());
        auto r = tracker.AddToSet(peer_id1, wtxid);
        if (!r.has_value()) {
            ++added_txs;
        } else {
            BOOST_REQUIRE_EQUAL(r.value().m_error, ReconciliationError::SHORTID_COLLISION);
            BOOST_REQUIRE(r.value().m_collision.has_value());
        }
    }

    // Adding one item over the limit should fail
    error = tracker.AddToSet(peer_id1, Wtxid::FromUint256(frc.rand256())).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::FULL_RECON_SET);
    BOOST_REQUIRE(!error.m_collision.has_value());

    // Trying to add the same item twice will just bypass
    BOOST_REQUIRE(!tracker.AddToSet(peer_id1, wtxid).has_value());
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
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));

    // Once the peer is registered, we can try to add both transactions and check
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
    auto error = tracker.AddToSet(peer_id0, collision);
    BOOST_REQUIRE_EQUAL(error.value().m_error, ReconciliationError::SHORTID_COLLISION);
    BOOST_REQUIRE_EQUAL(error.value().GetCollision(), wtxid);
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
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // Only if the transaction is found the method will return true
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));
    // Removing twice won't work
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // Adding a transaction but removing a collision won't work
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, collision));
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));

    // And removing after forgetting the peer won't work either
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
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
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id0)), ReconciliationError::NOT_FOUND);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id0)), ReconciliationError::NOT_FOUND);

    // For a registered peer with no pending transactions, we expect the set size to be zero, and the
    // reconciliation params to match the default
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, 1).has_value());
    auto [local_set_size, local_q_formatted] = std::get<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0));
    BOOST_REQUIRE_EQUAL(local_set_size, 0);
    BOOST_REQUIRE_EQUAL(local_q_formatted, uint16_t(Q * Q_PRECISION));

    // We only initiate reconciliation with outbound peers. For inbounds, we expect them to initiate
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id1)), ReconciliationError::WRONG_ROLE);

    // Start fresh
    tracker.ForgetPeer(peer_id0);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, 1).has_value());

    // After adding some transactions to a registered peer we expect its set to contain that amount of transactions
    int n_added_txs = 0;
    int n_target_txs = frc.randrange(42) + 1;
    while (n_added_txs < n_target_txs) {
        auto wtxid = Wtxid::FromUint256(frc.rand256());
        if (!tracker.AddToSet(peer_id0, wtxid).has_value()) ++n_added_txs;
    }
    std::tie(local_set_size, local_q_formatted) = std::get<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0));
    BOOST_REQUIRE_EQUAL(local_set_size, n_added_txs);
    BOOST_REQUIRE_EQUAL(local_q_formatted, uint16_t(Q * Q_PRECISION));

    // Once we are reconciling with a peer, a new reconciliation cannot be initiated until the previous is completed
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id0)), ReconciliationError::WRONG_PHASE);
}

BOOST_AUTO_TEST_CASE(HandleReconciliationRequestTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;

    // A reconciliation request cannot be initiated with a non-fully registered peer
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id0, 0, Q).value(), ReconciliationError::NOT_FOUND);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id0, 0, Q).value(), ReconciliationError::NOT_FOUND);

    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, 0, Q).has_value());

    // If a reconciliation flow is ongoing for a given peer, we won't start another with him.
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id0, 0, Q).value(), ReconciliationError::WRONG_PHASE);

    // Other peers can request reconciliation as long as we are not currently reconciling with them.
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id1, 0, Q).has_value());
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id1, 0, Q).value(), ReconciliationError::WRONG_PHASE);

    // We only respond to reconciliation requests if they are the initiator (peer is inbound)
    NodeId peer_id2 = 2;
    tracker.PreRegisterPeer(peer_id2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id2, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE_EQUAL(tracker.HandleReconciliationRequest(peer_id2, 0, Q).value(), ReconciliationError::WRONG_ROLE);
}

BOOST_AUTO_TEST_CASE(ShouldRespondToReconciliationRequestTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};
    std::vector<uint8_t> skdata{};

    // We cannot respond to partially registered peers
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata, /*send_trickle*/true));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata, /*send_trickle*/true));

    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, 1).has_value());

    // We only respond to reconciliation requests after receiving one
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata, /*send_trickle*/true));
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/10, Q).has_value());

    // For the initial request, we only respond on trickle
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata, /*send_trickle*/false));
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata, /*send_trickle*/true));

    // The sketch data depends on the transactions we had on the peer's local set
    BOOST_REQUIRE(skdata.empty());

    // After responding, the reconciliation phase have moved from INIT_REQUESTED to INIT_RESPONDED, so we cannot respond again
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata, /*send_trickle*/true));

    // Reset the peer
    tracker.ForgetPeer(peer_id0);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/0, Q).has_value());

    // If the received request had size zero (peer_recon_set_size == 0), we will respond but shortcut, since reconciling is pointless
    tracker.AddToSet(peer_id0, Wtxid::FromUint256(frc.rand256()));
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata, /*send_trickle*/true));
    BOOST_REQUIRE(skdata.empty());

    // Reset the peer
    tracker.ForgetPeer(peer_id0);
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(!tracker.HandleReconciliationRequest(peer_id0, /*peer_recon_set_size*/10, Q).has_value());

    // If the peer has data to reconcile, adding some transactions to the peers local set will affect the serialized
    // sketch data we receive (which we're supposed to send to our peer)
    tracker.AddToSet(peer_id0, Wtxid::FromUint256(frc.rand256()));
    BOOST_REQUIRE(tracker.ShouldRespondToReconciliationRequest(peer_id0, skdata, /*send_trickle*/true));
    BOOST_REQUIRE(!skdata.empty());

    // We only respond to reconciliation requests if they are the initiator (peer is inbound)
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(!tracker.ShouldRespondToReconciliationRequest(peer_id1, skdata, /*send_trickle*/true));
}

BOOST_AUTO_TEST_CASE(IsInboundFanoutTargetTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);

    // Checking with an unregistered peer should always return false
    BOOST_REQUIRE(!tracker.IsInboundFanoutTarget(0));

    // Same applies for partially registered
    tracker.PreRegisterPeer(0);
    BOOST_REQUIRE(!tracker.IsInboundFanoutTarget(0));
    tracker.ForgetPeer(0);

    // Outbound peers are not included as inbound fanout targets
    int fanout_count = 0;
    for (auto i=0; i<50; i++) {
        tracker.PreRegisterPeer(i);
        BOOST_REQUIRE(!tracker.RegisterPeer(i, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, 1).has_value());
        fanout_count += tracker.IsInboundFanoutTarget(i);
    }
    BOOST_REQUIRE_EQUAL(fanout_count, 0);

    // Inbound peers are added as inbound targets probabilistically (based on INBOUND_FANOUT_DESTINATIONS_FRACTION)
    // Adding a few peers should ensure that we get some targets
    std::vector<int> fanout_targets;
    for (auto i=50; i<150; i++) {
        tracker.PreRegisterPeer(i);
        BOOST_REQUIRE(!tracker.RegisterPeer(i, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, 1).has_value());
        if (tracker.IsInboundFanoutTarget(i)) {
            fanout_targets.push_back(i);
        }
    }
    // TODO: This should be about INBOUND_FANOUT_DESTINATIONS_FRACTION of the total inbound registered peers.
    // Is there a better way of checking this?
    BOOST_REQUIRE(!fanout_targets.empty());

    // Forgeting peers removes them from targerts
    for (auto peer_id : fanout_targets) {
        tracker.ForgetPeer(peer_id);
        BOOST_REQUIRE(!tracker.IsInboundFanoutTarget(peer_id));
    }
}

BOOST_AUTO_TEST_CASE(GetSetNextInboundPeerRotationTimeTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    FastRandomContext frc{/*fDeterministic=*/true};

     // The next time start unset (it's set via SetNextInboundPeerRotationTime after initializing the tracker)
    auto next_time = tracker.GetNextInboundPeerRotationTime();
    BOOST_REQUIRE(next_time == std::chrono::microseconds(0));

    // Setting it to a specific value return the value
    for (auto i=0; i<10; i++) {
        next_time += std::chrono::microseconds{frc.rand32()};
        tracker.SetNextInboundPeerRotationTime(next_time);
        BOOST_REQUIRE(tracker.GetNextInboundPeerRotationTime() == next_time);
    }
}

BOOST_AUTO_TEST_CASE(RotateInboundFanoutTargetsTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    const int in_fanout_peers = 100;
    const int in_fanout_target_count = std::floor(in_fanout_peers * INBOUND_FANOUT_DESTINATIONS_FRACTION);

    // Add some inbound peers and check which of them are inbound targets
    std::vector<int> fanout_targets;
    for (auto i=0; i<in_fanout_peers; i++) {
        tracker.PreRegisterPeer(i);
        BOOST_REQUIRE(!tracker.RegisterPeer(i, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, 1).has_value());
        fanout_targets.push_back(i);
    }

    // Rotate the targets and check they have indded changed.
    std::vector<int> next_fanout_targets;
    for (auto i=0; i<10; i++) {
        tracker.RotateInboundFanoutTargets();

        for (auto i=0; i<in_fanout_peers; i++) {
            if (tracker.IsInboundFanoutTarget(i)) {
                next_fanout_targets.push_back(i);
            }
        }
        // Targets should be picked at random, and their size should be INBOUND_FANOUT_DESTINATIONS_FRACTION of our erlay inbounds
        BOOST_REQUIRE(fanout_targets != next_fanout_targets);
        BOOST_REQUIRE_EQUAL(next_fanout_targets.size(), in_fanout_target_count);
        fanout_targets = next_fanout_targets;
        next_fanout_targets.clear();
    }

    // If the number of inbounds goes down, so does the sizes of the inbound targets
    const int reduced_fanout_peers = in_fanout_peers / 2;
    const int reduced_in_fanout_target_count = std::floor(reduced_fanout_peers * INBOUND_FANOUT_DESTINATIONS_FRACTION);
    BOOST_REQUIRE(in_fanout_target_count != reduced_in_fanout_target_count);
    for (auto i=reduced_fanout_peers; i<in_fanout_peers; i++) {
        tracker.ForgetPeer(i);
    }

    tracker.RotateInboundFanoutTargets();
    for (auto i=0; i<reduced_fanout_peers; i++) {
        if (tracker.IsInboundFanoutTarget(i)) {
            next_fanout_targets.push_back(i);
        }
    }
    BOOST_REQUIRE(fanout_targets != next_fanout_targets);
    BOOST_REQUIRE_EQUAL(next_fanout_targets.size(), reduced_in_fanout_target_count);
}

BOOST_AUTO_TEST_SUITE_END()
