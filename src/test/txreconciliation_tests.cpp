// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txreconciliation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(RegisterPeerTest)
{
    TxReconciliationTracker tracker(1);
    const uint64_t salt = 0;

    // Prepare a peer for reconciliation.
    tracker.PreRegisterPeer(0);

    // Both roles are false, don't register.
    BOOST_CHECK(tracker.RegisterPeer(/*peer_id=*/0, /*is_peer_inbound=*/true,
                                      /*is_peer_recon_initiator=*/false,
                                      /*is_peer_recon_responder=*/false,
                                      /*peer_recon_version=*/1, salt) ==
                    ReconciliationRegisterResult::PROTOCOL_VIOLATION);

    // Invalid roles for the given connection direction.
    BOOST_CHECK(tracker.RegisterPeer(0, true, false, true, 1, salt) == ReconciliationRegisterResult::PROTOCOL_VIOLATION);
    BOOST_CHECK(tracker.RegisterPeer(0, false, true, false, 1, salt) == ReconciliationRegisterResult::PROTOCOL_VIOLATION);

    // Invalid version.
    BOOST_CHECK(tracker.RegisterPeer(0, true, true, false, 0, salt) == ReconciliationRegisterResult::PROTOCOL_VIOLATION);

    // Valid registration.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(tracker.RegisterPeer(0, true, true, false, 1, salt) == ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(0));

    // Reconciliation version is higher than ours, should be able to register.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(1));
    tracker.PreRegisterPeer(1);
    BOOST_REQUIRE(tracker.RegisterPeer(1, true, true, false, 2, salt) == ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(1));

    // Do not register if there were no pre-registration for the peer.
    BOOST_REQUIRE(tracker.RegisterPeer(100, true, true, false, 1, salt) == ReconciliationRegisterResult::NOT_FOUND);
    BOOST_CHECK(!tracker.IsPeerRegistered(100));
}

BOOST_AUTO_TEST_CASE(ForgetPeerTest)
{
    TxReconciliationTracker tracker(1);
    NodeId peer_id0 = 0;

    // Removing peer after pre-registring works and does not let to register the peer.
    tracker.PreRegisterPeer(peer_id0);
    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK(tracker.RegisterPeer(peer_id0, true, true, false, 1, 1) == ReconciliationRegisterResult::NOT_FOUND);

    // Removing peer after it is registered works.
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(tracker.RegisterPeer(peer_id0, true, true, false, 1, 1) == ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));
    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(IsPeerRegisteredTest)
{
    TxReconciliationTracker tracker(1);
    NodeId peer_id0 = 0;

    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    BOOST_REQUIRE(tracker.RegisterPeer(peer_id0, true, true, false, 1, 1) == ReconciliationRegisterResult::SUCCESS);
    BOOST_CHECK(tracker.IsPeerRegistered(peer_id0));

    tracker.ForgetPeer(peer_id0);
    BOOST_CHECK(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_SUITE_END()
