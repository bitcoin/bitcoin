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

    // As long as the peer is registered, adding a new wtxid to the set should work
    tracker.PreRegisterPeer(peer_id0);
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

    // As long as the peer is registered and the transaction is not in the set, adding it should succeed
    for (size_t i = 0; i < MAX_RECONSET_SIZE; ++i)
        BOOST_REQUIRE(!tracker.AddToSet(peer_id1, Wtxid::FromUint256(frc.rand256())).has_value());

    // Adding one item over the limit should fail
    error = tracker.AddToSet(peer_id1, Wtxid::FromUint256(frc.rand256())).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::FULL_RECON_SET);
    BOOST_REQUIRE(!error.m_collision.has_value());
}

BOOST_AUTO_TEST_CASE(TryRemovingFromSetTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    FastRandomContext frc{/*fDeterministic=*/true};

    Wtxid wtxid{Wtxid::FromUint256(frc.rand256())};

    // If the peer is not registered, removing will fail
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // This holds even if the peer is just pre-registered
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // If the peer is registered but the transaction is not part of the set, this will fail too
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1).has_value());
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // Only if the transaction is found the method will return true
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
    BOOST_REQUIRE(tracker.TryRemovingFromSet(peer_id0, wtxid));
    // Removing twice won't work
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // And removing after forgetting the peer won't work either
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());
    tracker.ForgetPeer(peer_id0);
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));
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
