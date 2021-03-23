// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net.h>
#include <net_processing.h>
#include <node/txreconciliation.h>
#include <node/txreconciliation_impl.h>
#include <protocol.h>

#include <test/util/common.h>
#include <test/util/logging.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <test/util/validation.h>

#include <boost/test/unit_test.hpp>

#include <cmath>

using namespace node;

/** Testing setup with transaction reconciliation enabled, for the tests that drive
 *  announcements through PeerManager rather than the tracker directly. */
struct TxReconciliationRelaySetup : public TestingSetup {
    TxReconciliationRelaySetup()
        : TestingSetup{ChainType::REGTEST,
                       TestOpts{.extra_args = {"-txreconciliation", "-txsendrate=1000"}}}
    {
        // We don't reconcile during IBD, so jump out of it to be able to test reconciliation.
        static_cast<TestChainstateManager&>(*m_node.chainman).JumpOutOfIbd();
    }

    ~TxReconciliationRelaySetup()
    {
        // The connman owns the test nodes and deletes them here, so this has to run before
        // the base fixture tears the connman down.
        static_cast<ConnmanTestMsg&>(*m_node.connman).ClearTestNodes();
    }

    /** Connect an inbound peer and complete a reconciliation handshake with it. */
    CNode& AddReconciliationPeer(NodeId id) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
    {
        auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
        auto* node = new CNode{id,
                               /*sock=*/nullptr,
                               CAddress{},
                               /*nKeyedNetGroupIn=*/0,
                               /*nLocalHostNonceIn=*/0,
                               CAddress{},
                               /*addrNameIn=*/"",
                               ConnectionType::INBOUND,
                               /*inbound_onion=*/false,
                               /*network_key=*/0};
        connman.AddTestNode(*node);

        // Version exchange, but stop short of VERACK. The peer has to answer our SENDTXRCNCL
        // before it, otherwise the node forgets the pre-registered state.
        connman.Handshake(*node, /*successfully_connected=*/false,
                          ServiceFlags(NODE_NETWORK | NODE_WITNESS),
                          ServiceFlags(NODE_NETWORK | NODE_WITNESS),
                          PROTOCOL_VERSION, /*relay_txs=*/true);

        connman.ReceiveMsgFrom(*node, NetMsg::Make(NetMsgType::WTXIDRELAY));
        connman.ReceiveMsgFrom(*node, NetMsg::Make(NetMsgType::SENDTXRCNCL,
                                                   TXRECONCILIATION_VERSION, REMOTE_SALT_RELAY));
        connman.ReceiveMsgFrom(*node, NetMsg::Make(NetMsgType::VERACK));
        node->fPauseSend = false;
        // Process all (three) messages, then send our own VERACK to complete the handshake.
        for (int i = 0; i < 3; ++i) connman.ProcessMessagesOnce(*node);
        BOOST_REQUIRE(node->fSuccessfullyConnected);
        m_node.peerman->SendMessages(*node);
        connman.FlushSendBuffer(*node);
        return *node;
    }

    /** The message type the node has queued for this peer, empty if nothing is pending. */
    std::string PendingMsgType(CNode& node)
    {
        LOCK(node.cs_vSend);
        const auto& [to_send, _more, msg_type] = node.m_transport->GetBytesToSend(false);
        return to_send.empty() ? "" : msg_type;
    }

    /** Add a random transaction straight to the mempool, no validation involved. */
    CTransactionRef AddMempoolTx()
    {
        CMutableTransaction mtx;
        mtx.vin.resize(1);
        // A distinct input per transaction, otherwise they all conflict and only one is added.
        mtx.vin[0].prevout = COutPoint{Txid::FromUint256(m_rng.rand256()), 0};
        mtx.vout.resize(1);
        mtx.vout[0].nValue = 1000;
        const auto tx{MakeTransactionRef(mtx)};
        TryAddToMempool(*m_node.mempool, TestMemPoolEntryHelper{}.FromTx(tx));
        return tx;
    }

    /** Announce everything queued, draining the rate limiter by advancing the clock. */
    void DrainAnnouncements(CNode& node, FakeNodeClock& clock) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
    {
        // Announcements wait on a randomized trickle deadline, so step the clock until both the
        // global backlog and the peer's queue drain. This should be drained well within 10 iterations.
        // Giving it a 6x marging to prevent flakiness, since the trickle deadline is randomized.
        CNodeStateStats stats;
        PeerManagerInfo info;
        for (int i = 0; i < 60; ++i) {
            clock += 1s;
            m_node.peerman->SendMessages(node);
            BOOST_REQUIRE(m_node.peerman->GetNodeStateStats(node.GetId(), stats));
            info = m_node.peerman->GetInfo();
            if (stats.m_inv_to_send == 0 && info.inbound_bucket.backlog_count == 0) return;
        }
        BOOST_FAIL("announcements did not drain: inv_to_send=" << stats.m_inv_to_send
                   << " backlog=" << info.inbound_bucket.backlog_count);
    }

    static constexpr uint64_t REMOTE_SALT_RELAY{1};
};

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

constexpr uint64_t REMOTE_SALT = 0;

// A pair of wtxids whose short ids collide on a link salted with (2, 1). Precomputed under
// BIP-330's derivation, 1 + (s mod 0xFFFFFFFF); regenerate if that ever changes.
const Wtxid COLLIDING_WTXID{Wtxid::FromUint256(uint256("fdfb332e93ecd3b28835f51f9d999278dba2c10fb45611229a17136a00a4d3c9"))};
const Wtxid COLLIDING_WTXID_2{Wtxid::FromUint256(uint256("8f16f117d30b68c6fcdd1ee033d2b4f8aba719088e5aff78c8f9536aeb1388b4"))};

BOOST_AUTO_TEST_CASE(RegisterPeerTest)
{
    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);

    // Prepare a peer for reconciliation.
    tracker.PreRegisterPeer(0);

    // Invalid version.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(/*peer_id=*/0, /*is_peer_inbound=*/true,
                                           /*peer_recon_version=*/0, REMOTE_SALT).value(),
                      ReconciliationError::PROTOCOL_VIOLATION);

    // Valid registration (inbound and outbound peers).
    BOOST_REQUIRE(!tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(!tracker.RegisterPeer(0, true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(0));
    BOOST_REQUIRE(!tracker.IsPeerRegistered(1));
    tracker.PreRegisterPeer(1);
    BOOST_REQUIRE(!tracker.RegisterPeer(1, false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(1));

    // Reconciliation version is higher than ours, should be able to register.
    BOOST_REQUIRE(!tracker.IsPeerRegistered(2));
    tracker.PreRegisterPeer(2);
    BOOST_REQUIRE(!tracker.RegisterPeer(2, true, TXRECONCILIATION_VERSION+1, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(2));

    // Try registering for the second time.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(1, false, TXRECONCILIATION_VERSION, REMOTE_SALT).value(), ReconciliationError::ALREADY_REGISTERED);

    // Do not register if there were no pre-registration for the peer.
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(100, true, TXRECONCILIATION_VERSION, REMOTE_SALT).value(), ReconciliationError::NOT_FOUND);
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

    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));

    // Forgotten peers do not count either.
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
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
    auto current_time = GetTime<std::chrono::microseconds>() + RECON_REQUEST_INTERVAL - 1s;

    // Not enough time passed.
    BOOST_REQUIRE(!tracker.IsPeerNextToReconcileWith(peer_id0, current_time));
    // Enough time passed.
    current_time += 1s;
    BOOST_REQUIRE(tracker.IsPeerNextToReconcileWith(peer_id0, current_time));
    // Enough time passed for another round, but the previous reconciliation is still pending.
    // With only one node in the queue, they will still be next.
    current_time += RECON_REQUEST_INTERVAL;
    BOOST_REQUIRE(tracker.IsPeerNextToReconcileWith(peer_id0, current_time));

    // Two-peer setup
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));

    // No outbound peers left, so the queue is empty and nobody can be next. Inbound peers are
    // never queued, since we don't initiate reconciliation with them.
    NodeId peer_id_inbound = 3;
    tracker.PreRegisterPeer(peer_id_inbound);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id_inbound, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(!tracker.IsPeerNextToReconcileWith(peer_id_inbound, current_time + RECON_REQUEST_INTERVAL));

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
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id2));
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
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    BOOST_REQUIRE_EQUAL(tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).value(), ReconciliationError::NOT_FOUND);

    // Removing peer after it is registered works.
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));

    // Removing a non-existing peer does nothing
    NodeId not_a_peer_id = 1;
    BOOST_REQUIRE(!tracker.IsPeerRegistered(not_a_peer_id));
    BOOST_REQUIRE(!tracker.ForgetPeer(not_a_peer_id));
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
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE(tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.AddToSet(peer_id0, wtxid).has_value());

    // If the peer is dropped, adding wtxids to its set should fail
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    Wtxid wtxid2{Wtxid::FromUint256(frc.rand256())};
    error = tracker.AddToSet(peer_id0, wtxid2).value();
    BOOST_REQUIRE_EQUAL(error.m_error, ReconciliationError::NOT_FOUND);
    BOOST_REQUIRE(!error.m_collision.has_value());

    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
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
    uint64_t precomputed_remote_salt = 1;

    // Precompute collision
    const Wtxid wtxid{COLLIDING_WTXID};
    const Wtxid collision{COLLIDING_WTXID_2};

    // Register the peer with a predefined salt so we can force the collision
    tracker.PreRegisterPeerWithSalt(peer_id0, 2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, precomputed_remote_salt).has_value());
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

    const Wtxid wtxid{COLLIDING_WTXID};
    const Wtxid collision{COLLIDING_WTXID_2};

    // If the peer is not registered, removing will fail
    BOOST_REQUIRE(!tracker.IsPeerRegistered(peer_id0));
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // This holds even if the peer is just pre-registered (register specific salt so we can also check collisions)
    tracker.PreRegisterPeerWithSalt(peer_id0, 2);
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // If the peer is registered but the transaction is not part of the set, this will fail too
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
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
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id0, wtxid));

    // Failing to remove must leave the short id mapping untouched. Dropping the entry of a
    // transaction that is still in the set would make a later colliding transaction go undetected
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeerWithSalt(peer_id1, 2);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound=*/true, TXRECONCILIATION_VERSION, /*remote_salt=*/1).has_value());
    BOOST_REQUIRE(!tracker.AddToSet(peer_id1, wtxid).has_value());
    // collision was never added, so there is nothing to remove.
    BOOST_REQUIRE(!tracker.TryRemovingFromSet(peer_id1, collision));
    // wtxid is still in the set, so its short id is still mapped and the collision is caught.
    auto collision_error = tracker.AddToSet(peer_id1, collision);
    BOOST_REQUIRE_EQUAL(collision_error.value().m_error, ReconciliationError::SHORTID_COLLISION);
    BOOST_REQUIRE_EQUAL(collision_error.value().GetCollision(), wtxid);
}

// Once a peer's reconciliation set is full, further transactions have to be fanned out instead.
BOOST_FIXTURE_TEST_CASE(AnnounceTxsFullReconciliationSet, TxReconciliationRelaySetup)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);
    FakeNodeClock clock{};
    CNode& recon{AddReconciliationPeer(0)};
    BOOST_REQUIRE(!recon.fDisconnect);
    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);

    {
        ASSERT_DEBUG_LOG("to the reconciliation set");
        for (uint32_t i = 0; i < MAX_RECONSET_SIZE; ++i) {
            m_node.peerman->InitiateTxBroadcastToAll(AddMempoolTx()->GetWitnessHash());
        }
        DrainAnnouncements(recon, clock);
        BOOST_REQUIRE(!recon.fDisconnect);
    }

    // All of them were reconciled, so nothing should have been announced.
    connman.FlushSendBuffer(recon);
    BOOST_CHECK_EQUAL(PendingMsgType(recon), "");

    // The next one no longer fits in the set, so it must be fanned out.
    {
        ASSERT_DEBUG_LOG("Reconciliation set maximum size reached");
        m_node.peerman->InitiateTxBroadcastToAll(AddMempoolTx()->GetWitnessHash());
        DrainAnnouncements(recon, clock);
    }
    BOOST_CHECK_EQUAL(PendingMsgType(recon), "inv");
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
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    auto [local_set_size, local_q_formatted] = std::get<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0));
    BOOST_REQUIRE_EQUAL(local_set_size, 0);
    BOOST_REQUIRE_EQUAL(local_q_formatted, static_cast<uint16_t>(std::ceil(Q * Q_PRECISION)));

    // We only initiate reconciliation with outbound peers. For inbounds, we expect them to initiate
    NodeId peer_id1 = 1;
    tracker.PreRegisterPeer(peer_id1);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id1, /*is_peer_inbound*/true, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id1)), ReconciliationError::WRONG_ROLE);

    // Start fresh
    BOOST_REQUIRE(tracker.ForgetPeer(peer_id0));
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());

    // After adding some transactions to a registered peer we expect its set to contain that amount of transactions
    int n_added_txs = 0;
    int n_target_txs = frc.randrange(42) + 1;
    while (n_added_txs < n_target_txs) {
        auto wtxid = Wtxid::FromUint256(frc.rand256());
        if (!tracker.AddToSet(peer_id0, wtxid).has_value()) ++n_added_txs;
    }
    std::tie(local_set_size, local_q_formatted) = std::get<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0));
    BOOST_REQUIRE_EQUAL(local_set_size, n_added_txs);
    BOOST_REQUIRE_EQUAL(local_q_formatted, static_cast<uint16_t>(std::ceil(Q * Q_PRECISION)));

    // Once we are reconciling with a peer, a new reconciliation cannot be initiated until the previous is completed
    BOOST_REQUIRE_EQUAL(std::get<ReconciliationError>(tracker.InitiateReconciliationRequest(peer_id0)), ReconciliationError::WRONG_PHASE);
}

BOOST_AUTO_TEST_CASE(InitiateReconciliationRequestQCeilTest)
{
    // BIP-330 requires the formatted q to be rounded up when narrowed from a
    // floating-point coefficient to the wire format. Using Q = 0.25 and
    // Q_PRECISION = 32767, the exact product is 8191.75 so we should expect 8192.
    static_assert(Q == 0.25);
    static_assert(Q_PRECISION == 32767);

    TxReconciliationTracker tracker(TXRECONCILIATION_VERSION);
    NodeId peer_id0 = 0;
    tracker.PreRegisterPeer(peer_id0);
    BOOST_REQUIRE(!tracker.RegisterPeer(peer_id0, /*is_peer_inbound*/false, TXRECONCILIATION_VERSION, REMOTE_SALT).has_value());
    auto [local_set_size, local_q_formatted] = std::get<ReconCoefficients>(tracker.InitiateReconciliationRequest(peer_id0));
    BOOST_REQUIRE_EQUAL(local_q_formatted, 8192);
}

BOOST_AUTO_TEST_SUITE_END()
