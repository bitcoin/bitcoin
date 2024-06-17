// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Unit tests for denial-of-service detection/prevention code

#include <banman.h>
#include <chainparams.h>
#include <common/args.h>
#include <net.h>
#include <net_processing.h>
#include <pubkey.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <serialize.h>
#include <test/util/net.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/string.h>
#include <util/time.h>
#include <validation.h>

#include <array>
#include <stdint.h>

#include <boost/test/unit_test.hpp>

static CService ip(uint32_t i)
{
    struct in_addr s;
    s.s_addr = i;
    return CService(CNetAddr(s), Params().GetDefaultPort());
}

BOOST_FIXTURE_TEST_SUITE(denialofservice_tests, TestingSetup)

// Test eviction of an outbound peer whose chain never advances
// Mock a node connection, and use mocktime to simulate a peer
// which never sends any headers messages.  PeerLogic should
// decide to evict that outbound peer, after the appropriate timeouts.
// Note that we protect 4 outbound nodes from being subject to
// this logic; this test takes advantage of that protection only
// being applied to nodes which send headers with sufficient
// work.
BOOST_AUTO_TEST_CASE(outbound_slow_chain_eviction)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    ConnmanTestMsg& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
    // Disable inactivity checks for this test to avoid interference
    connman.SetPeerConnectTimeout(99999s);
    PeerManager& peerman = *m_node.peerman;

    // Mock an outbound peer
    CAddress addr1(ip(0xa0b0c001), NODE_NONE);
    NodeId id{0};
    CNode dummyNode1{id++,
                     /*sock=*/nullptr,
                     addr1,
                     /*nKeyedNetGroupIn=*/0,
                     /*nLocalHostNonceIn=*/0,
                     CAddress(),
                     /*addrNameIn=*/"",
                     ConnectionType::OUTBOUND_FULL_RELAY,
                     /*inbound_onion=*/false};

    connman.Handshake(
        /*node=*/dummyNode1,
        /*successfully_connected=*/true,
        /*remote_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
        /*local_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
        /*version=*/PROTOCOL_VERSION,
        /*relay_txs=*/true);

    // This test requires that we have a chain with non-zero work.
    {
        LOCK(cs_main);
        BOOST_CHECK(m_node.chainman->ActiveChain().Tip() != nullptr);
        BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->nChainWork > 0);
    }

    // Test starts here
    BOOST_CHECK(peerman.SendMessages(&dummyNode1)); // should result in getheaders

    {
        LOCK(dummyNode1.cs_vSend);
        const auto& [to_send, _more, _msg_type] = dummyNode1.m_transport->GetBytesToSend(false);
        BOOST_CHECK(!to_send.empty());
    }
    connman.FlushSendBuffer(dummyNode1);

    int64_t nStartTime = GetTime();
    // Wait 21 minutes
    SetMockTime(nStartTime+21*60);
    BOOST_CHECK(peerman.SendMessages(&dummyNode1)); // should result in getheaders
    {
        LOCK(dummyNode1.cs_vSend);
        const auto& [to_send, _more, _msg_type] = dummyNode1.m_transport->GetBytesToSend(false);
        BOOST_CHECK(!to_send.empty());
    }
    // Wait 3 more minutes
    SetMockTime(nStartTime+24*60);
    BOOST_CHECK(peerman.SendMessages(&dummyNode1)); // should result in disconnect
    BOOST_CHECK(dummyNode1.fDisconnect == true);

    peerman.FinalizeNode(dummyNode1);
}

static void AddRandomOutboundPeer(NodeId& id, std::vector<CNode*>& vNodes, PeerManager& peerLogic, ConnmanTestMsg& connman, ConnectionType connType, bool onion_peer = false)
{
    CAddress addr;

    if (onion_peer) {
        auto tor_addr{g_insecure_rand_ctx.randbytes(ADDR_TORV3_SIZE)};
        BOOST_REQUIRE(addr.SetSpecial(OnionToString(tor_addr)));
    }

    while (!addr.IsRoutable()) {
        addr = CAddress(ip(g_insecure_rand_ctx.randbits(32)), NODE_NONE);
    }

    vNodes.emplace_back(new CNode{id++,
                                  /*sock=*/nullptr,
                                  addr,
                                  /*nKeyedNetGroupIn=*/0,
                                  /*nLocalHostNonceIn=*/0,
                                  CAddress(),
                                  /*addrNameIn=*/"",
                                  connType,
                                  /*inbound_onion=*/false});
    CNode &node = *vNodes.back();
    node.SetCommonVersion(PROTOCOL_VERSION);

    peerLogic.InitializeNode(node, ServiceFlags(NODE_NETWORK | NODE_WITNESS));
    node.fSuccessfullyConnected = true;

    connman.AddTestNode(node);
}

BOOST_AUTO_TEST_CASE(stale_tip_peer_management)
{
    NodeId id{0};
    auto connman = std::make_unique<ConnmanTestMsg>(0x1337, 0x1337, *m_node.addrman, *m_node.netgroupman, Params());
    auto peerLogic = PeerManager::make(*connman, *m_node.addrman, nullptr, *m_node.chainman, *m_node.mempool, *m_node.warnings, {});

    constexpr int max_outbound_full_relay = MAX_OUTBOUND_FULL_RELAY_CONNECTIONS;
    CConnman::Options options;
    options.m_max_automatic_connections = DEFAULT_MAX_PEER_CONNECTIONS;

    const auto time_init{GetTime<std::chrono::seconds>()};
    SetMockTime(time_init);
    const auto time_later{time_init + 3 * std::chrono::seconds{m_node.chainman->GetConsensus().nPowTargetSpacing} + 1s};
    connman->Init(options);
    std::vector<CNode *> vNodes;

    // Mock some outbound peers
    for (int i = 0; i < max_outbound_full_relay; ++i) {
        AddRandomOutboundPeer(id, vNodes, *peerLogic, *connman, ConnectionType::OUTBOUND_FULL_RELAY);
    }

    peerLogic->CheckForStaleTipAndEvictPeers();

    // No nodes should be marked for disconnection while we have no extra peers
    for (const CNode *node : vNodes) {
        BOOST_CHECK(node->fDisconnect == false);
    }

    SetMockTime(time_later);

    // Now tip should definitely be stale, and we should look for an extra
    // outbound peer
    peerLogic->CheckForStaleTipAndEvictPeers();
    BOOST_CHECK(connman->GetTryNewOutboundPeer());

    // Still no peers should be marked for disconnection
    for (const CNode *node : vNodes) {
        BOOST_CHECK(node->fDisconnect == false);
    }

    // If we add one more peer, something should get marked for eviction
    // on the next check (since we're mocking the time to be in the future, the
    // required time connected check should be satisfied).
    SetMockTime(time_init);
    AddRandomOutboundPeer(id, vNodes, *peerLogic, *connman, ConnectionType::OUTBOUND_FULL_RELAY);
    SetMockTime(time_later);

    peerLogic->CheckForStaleTipAndEvictPeers();
    for (int i = 0; i < max_outbound_full_relay; ++i) {
        BOOST_CHECK(vNodes[i]->fDisconnect == false);
    }
    // Last added node should get marked for eviction
    BOOST_CHECK(vNodes.back()->fDisconnect == true);

    vNodes.back()->fDisconnect = false;

    // Update the last announced block time for the last
    // peer, and check that the next newest node gets evicted.
    peerLogic->UpdateLastBlockAnnounceTime(vNodes.back()->GetId(), GetTime());

    peerLogic->CheckForStaleTipAndEvictPeers();
    for (int i = 0; i < max_outbound_full_relay - 1; ++i) {
        BOOST_CHECK(vNodes[i]->fDisconnect == false);
    }
    BOOST_CHECK(vNodes[max_outbound_full_relay-1]->fDisconnect == true);
    BOOST_CHECK(vNodes.back()->fDisconnect == false);

    vNodes[max_outbound_full_relay - 1]->fDisconnect = false;

    // Add an onion peer, that will be protected because it is the only one for
    // its network, so another peer gets disconnected instead.
    SetMockTime(time_init);
    AddRandomOutboundPeer(id, vNodes, *peerLogic, *connman, ConnectionType::OUTBOUND_FULL_RELAY, /*onion_peer=*/true);
    SetMockTime(time_later);
    peerLogic->CheckForStaleTipAndEvictPeers();

    for (int i = 0; i < max_outbound_full_relay - 2; ++i) {
        BOOST_CHECK(vNodes[i]->fDisconnect == false);
    }
    BOOST_CHECK(vNodes[max_outbound_full_relay - 2]->fDisconnect == false);
    BOOST_CHECK(vNodes[max_outbound_full_relay - 1]->fDisconnect == true);
    BOOST_CHECK(vNodes[max_outbound_full_relay]->fDisconnect == false);

    // Add a second onion peer which won't be protected
    SetMockTime(time_init);
    AddRandomOutboundPeer(id, vNodes, *peerLogic, *connman, ConnectionType::OUTBOUND_FULL_RELAY, /*onion_peer=*/true);
    SetMockTime(time_later);
    peerLogic->CheckForStaleTipAndEvictPeers();

    BOOST_CHECK(vNodes.back()->fDisconnect == true);

    for (const CNode *node : vNodes) {
        peerLogic->FinalizeNode(*node);
    }

    connman->ClearTestNodes();
}

BOOST_AUTO_TEST_CASE(block_relay_only_eviction)
{
    NodeId id{0};
    auto connman = std::make_unique<ConnmanTestMsg>(0x1337, 0x1337, *m_node.addrman, *m_node.netgroupman, Params());
    auto peerLogic = PeerManager::make(*connman, *m_node.addrman, nullptr, *m_node.chainman, *m_node.mempool, *m_node.warnings, {});

    constexpr int max_outbound_block_relay{MAX_BLOCK_RELAY_ONLY_CONNECTIONS};
    constexpr int64_t MINIMUM_CONNECT_TIME{30};
    CConnman::Options options;
    options.m_max_automatic_connections = DEFAULT_MAX_PEER_CONNECTIONS;

    connman->Init(options);
    std::vector<CNode*> vNodes;

    // Add block-relay-only peers up to the limit
    for (int i = 0; i < max_outbound_block_relay; ++i) {
        AddRandomOutboundPeer(id, vNodes, *peerLogic, *connman, ConnectionType::BLOCK_RELAY);
    }
    peerLogic->CheckForStaleTipAndEvictPeers();

    for (int i = 0; i < max_outbound_block_relay; ++i) {
        BOOST_CHECK(vNodes[i]->fDisconnect == false);
    }

    // Add an extra block-relay-only peer breaking the limit (mocks logic in ThreadOpenConnections)
    AddRandomOutboundPeer(id, vNodes, *peerLogic, *connman, ConnectionType::BLOCK_RELAY);
    peerLogic->CheckForStaleTipAndEvictPeers();

    // The extra peer should only get marked for eviction after MINIMUM_CONNECT_TIME
    for (int i = 0; i < max_outbound_block_relay; ++i) {
        BOOST_CHECK(vNodes[i]->fDisconnect == false);
    }
    BOOST_CHECK(vNodes.back()->fDisconnect == false);

    SetMockTime(GetTime() + MINIMUM_CONNECT_TIME + 1);
    peerLogic->CheckForStaleTipAndEvictPeers();
    for (int i = 0; i < max_outbound_block_relay; ++i) {
        BOOST_CHECK(vNodes[i]->fDisconnect == false);
    }
    BOOST_CHECK(vNodes.back()->fDisconnect == true);

    // Update the last block time for the extra peer,
    // and check that the next youngest peer gets evicted.
    vNodes.back()->fDisconnect = false;
    vNodes.back()->m_last_block_time = GetTime<std::chrono::seconds>();

    peerLogic->CheckForStaleTipAndEvictPeers();
    for (int i = 0; i < max_outbound_block_relay - 1; ++i) {
        BOOST_CHECK(vNodes[i]->fDisconnect == false);
    }
    BOOST_CHECK(vNodes[max_outbound_block_relay - 1]->fDisconnect == true);
    BOOST_CHECK(vNodes.back()->fDisconnect == false);

    for (const CNode* node : vNodes) {
        peerLogic->FinalizeNode(*node);
    }
    connman->ClearTestNodes();
}

BOOST_AUTO_TEST_CASE(peer_discouragement)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    auto banman = std::make_unique<BanMan>(m_args.GetDataDirBase() / "banlist", nullptr, DEFAULT_MISBEHAVING_BANTIME);
    auto connman = std::make_unique<ConnmanTestMsg>(0x1337, 0x1337, *m_node.addrman, *m_node.netgroupman, Params());
    auto peerLogic = PeerManager::make(*connman, *m_node.addrman, banman.get(), *m_node.chainman, *m_node.mempool, *m_node.warnings, {});

    CNetAddr tor_netaddr;
    BOOST_REQUIRE(
        tor_netaddr.SetSpecial("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"));
    const CService tor_service{tor_netaddr, Params().GetDefaultPort()};

    const std::array<CAddress, 3> addr{CAddress{ip(0xa0b0c001), NODE_NONE},
                                       CAddress{ip(0xa0b0c002), NODE_NONE},
                                       CAddress{tor_service, NODE_NONE}};

    const CNetAddr other_addr{ip(0xa0b0ff01)}; // Not any of addr[].

    std::array<CNode*, 3> nodes;

    banman->ClearBanned();
    NodeId id{0};
    nodes[0] = new CNode{id++,
                         /*sock=*/nullptr,
                         addr[0],
                         /*nKeyedNetGroupIn=*/0,
                         /*nLocalHostNonceIn=*/0,
                         CAddress(),
                         /*addrNameIn=*/"",
                         ConnectionType::INBOUND,
                         /*inbound_onion=*/false};
    nodes[0]->SetCommonVersion(PROTOCOL_VERSION);
    peerLogic->InitializeNode(*nodes[0], NODE_NETWORK);
    nodes[0]->fSuccessfullyConnected = true;
    connman->AddTestNode(*nodes[0]);
    peerLogic->UnitTestMisbehaving(nodes[0]->GetId(), DISCOURAGEMENT_THRESHOLD); // Should be discouraged
    BOOST_CHECK(peerLogic->SendMessages(nodes[0]));

    BOOST_CHECK(banman->IsDiscouraged(addr[0]));
    BOOST_CHECK(nodes[0]->fDisconnect);
    BOOST_CHECK(!banman->IsDiscouraged(other_addr)); // Different address, not discouraged

    nodes[1] = new CNode{id++,
                         /*sock=*/nullptr,
                         addr[1],
                         /*nKeyedNetGroupIn=*/1,
                         /*nLocalHostNonceIn=*/1,
                         CAddress(),
                         /*addrNameIn=*/"",
                         ConnectionType::INBOUND,
                         /*inbound_onion=*/false};
    nodes[1]->SetCommonVersion(PROTOCOL_VERSION);
    peerLogic->InitializeNode(*nodes[1], NODE_NETWORK);
    nodes[1]->fSuccessfullyConnected = true;
    connman->AddTestNode(*nodes[1]);
    peerLogic->UnitTestMisbehaving(nodes[1]->GetId(), DISCOURAGEMENT_THRESHOLD - 1);
    BOOST_CHECK(peerLogic->SendMessages(nodes[1]));
    // [0] is still discouraged/disconnected.
    BOOST_CHECK(banman->IsDiscouraged(addr[0]));
    BOOST_CHECK(nodes[0]->fDisconnect);
    // [1] is not discouraged/disconnected yet.
    BOOST_CHECK(!banman->IsDiscouraged(addr[1]));
    BOOST_CHECK(!nodes[1]->fDisconnect);
    peerLogic->UnitTestMisbehaving(nodes[1]->GetId(), 1); // [1] reaches discouragement threshold
    BOOST_CHECK(peerLogic->SendMessages(nodes[1]));
    // Expect both [0] and [1] to be discouraged/disconnected now.
    BOOST_CHECK(banman->IsDiscouraged(addr[0]));
    BOOST_CHECK(nodes[0]->fDisconnect);
    BOOST_CHECK(banman->IsDiscouraged(addr[1]));
    BOOST_CHECK(nodes[1]->fDisconnect);

    // Make sure non-IP peers are discouraged and disconnected properly.

    nodes[2] = new CNode{id++,
                         /*sock=*/nullptr,
                         addr[2],
                         /*nKeyedNetGroupIn=*/1,
                         /*nLocalHostNonceIn=*/1,
                         CAddress(),
                         /*addrNameIn=*/"",
                         ConnectionType::OUTBOUND_FULL_RELAY,
                         /*inbound_onion=*/false};
    nodes[2]->SetCommonVersion(PROTOCOL_VERSION);
    peerLogic->InitializeNode(*nodes[2], NODE_NETWORK);
    nodes[2]->fSuccessfullyConnected = true;
    connman->AddTestNode(*nodes[2]);
    peerLogic->UnitTestMisbehaving(nodes[2]->GetId(), DISCOURAGEMENT_THRESHOLD);
    BOOST_CHECK(peerLogic->SendMessages(nodes[2]));
    BOOST_CHECK(banman->IsDiscouraged(addr[0]));
    BOOST_CHECK(banman->IsDiscouraged(addr[1]));
    BOOST_CHECK(banman->IsDiscouraged(addr[2]));
    BOOST_CHECK(nodes[0]->fDisconnect);
    BOOST_CHECK(nodes[1]->fDisconnect);
    BOOST_CHECK(nodes[2]->fDisconnect);

    for (CNode* node : nodes) {
        peerLogic->FinalizeNode(*node);
    }
    connman->ClearTestNodes();
}

BOOST_AUTO_TEST_CASE(DoS_bantime)
{
    LOCK(NetEventsInterface::g_msgproc_mutex);

    auto banman = std::make_unique<BanMan>(m_args.GetDataDirBase() / "banlist", nullptr, DEFAULT_MISBEHAVING_BANTIME);
    auto connman = std::make_unique<CConnman>(0x1337, 0x1337, *m_node.addrman, *m_node.netgroupman, Params());
    auto peerLogic = PeerManager::make(*connman, *m_node.addrman, banman.get(), *m_node.chainman, *m_node.mempool, *m_node.warnings, {});

    banman->ClearBanned();
    int64_t nStartTime = GetTime();
    SetMockTime(nStartTime); // Overrides future calls to GetTime()

    CAddress addr(ip(0xa0b0c001), NODE_NONE);
    NodeId id{0};
    CNode dummyNode{id++,
                    /*sock=*/nullptr,
                    addr,
                    /*nKeyedNetGroupIn=*/4,
                    /*nLocalHostNonceIn=*/4,
                    CAddress(),
                    /*addrNameIn=*/"",
                    ConnectionType::INBOUND,
                    /*inbound_onion=*/false};
    dummyNode.SetCommonVersion(PROTOCOL_VERSION);
    peerLogic->InitializeNode(dummyNode, NODE_NETWORK);
    dummyNode.fSuccessfullyConnected = true;

    peerLogic->UnitTestMisbehaving(dummyNode.GetId(), DISCOURAGEMENT_THRESHOLD);
    BOOST_CHECK(peerLogic->SendMessages(&dummyNode));
    BOOST_CHECK(banman->IsDiscouraged(addr));

    peerLogic->FinalizeNode(dummyNode);
}

BOOST_AUTO_TEST_SUITE_END()
