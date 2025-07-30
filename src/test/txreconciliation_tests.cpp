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

    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*peer_recon_version=*/true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
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
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::NOT_FOUND);

    // Removing peer after it is registered works.
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, 1), ReconciliationRegisterResult::SUCCESS);
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));
    tracker.ForgetPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    // Removing a non-existing peer does nothing
    NodeId not_a_peer_id = 1;
    BOOST_REQUIRE(!tracker.IsPeerRegistered(not_a_peer_id));
    tracker.ForgetPeer(not_a_peer_id);
}

BOOST_AUTO_TEST_SUITE_END()
