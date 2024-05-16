// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <compat/compat.h>
#include <net.h>
#include <net_processing.h>
#include <netaddress.h>
#include <netbase.h>
#include <netgroup.h>
#include <node/connection_types.h>
#include <node/protocol_version.h>
#include <protocol.h>
#include <random.h>
#include <test/util/logging.h>
#include <test/util/net.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <util/chaintype.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

struct LogIPsTestingSetup : public TestingSetup {
    LogIPsTestingSetup()
        : TestingSetup{ChainType::MAIN, /*extra_args=*/{"-logips"}} {}
};

BOOST_FIXTURE_TEST_SUITE(net_peer_connection_tests, LogIPsTestingSetup)

static CService ip(uint32_t i)
{
    struct in_addr s;
    s.s_addr = i;
    return CService{CNetAddr{s}, Params().GetDefaultPort()};
}

/** Create a peer and connect to it. If the optional `address` (IP/CJDNS only) isn't passed, a random address is created. */
static void AddPeer(NodeId& id, std::vector<CNode*>& nodes, PeerManager& peerman, ConnmanTestMsg& connman, ConnectionType conn_type, bool onion_peer = false, std::optional<std::string> address = std::nullopt)
{
    CAddress addr{};

    if (address.has_value()) {
        addr = CAddress{MaybeFlipIPv6toCJDNS(LookupNumeric(address.value(), Params().GetDefaultPort())), NODE_NONE};
    } else if (onion_peer) {
        auto tor_addr{g_insecure_rand_ctx.randbytes(ADDR_TORV3_SIZE)};
        BOOST_REQUIRE(addr.SetSpecial(OnionToString(tor_addr)));
    }

    while (!addr.IsLocal() && !addr.IsRoutable()) {
        addr = CAddress{ip(g_insecure_rand_ctx.randbits(32)), NODE_NONE};
    }

    BOOST_REQUIRE(addr.IsValid());

    const bool inbound_onion{onion_peer && conn_type == ConnectionType::INBOUND};

    nodes.emplace_back(new CNode{++id,
                                 /*sock=*/nullptr,
                                 addr,
                                 /*nKeyedNetGroupIn=*/0,
                                 /*nLocalHostNonceIn=*/0,
                                 CAddress{},
                                 /*addrNameIn=*/"",
                                 conn_type,
                                 /*inbound_onion=*/inbound_onion});
    CNode& node = *nodes.back();
    node.SetCommonVersion(PROTOCOL_VERSION);

    peerman.InitializeNode(node, ServiceFlags(NODE_NETWORK | NODE_WITNESS));
    node.fSuccessfullyConnected = true;

    connman.AddTestNode(node);
}

BOOST_AUTO_TEST_CASE(test_addnode_getaddednodeinfo_and_connection_detection)
{
    auto connman = std::make_unique<ConnmanTestMsg>(0x1337, 0x1337, *m_node.addrman, *m_node.netgroupman, Params());
    auto peerman = PeerManager::make(*connman, *m_node.addrman, nullptr, *m_node.chainman, *m_node.mempool, {});
    NodeId id{0};
    std::vector<CNode*> nodes;

    // Connect a localhost peer.
    {
        ASSERT_DEBUG_LOG("Added connection to 127.0.0.1:8333 peer=1");
        AddPeer(id, nodes, *peerman, *connman, ConnectionType::MANUAL, /*onion_peer=*/false, /*address=*/"127.0.0.1");
        BOOST_REQUIRE(nodes.back() != nullptr);
    }

    // Call ConnectNode(), which is also called by RPC addnode onetry, for a localhost
    // address that resolves to multiple IPs, including that of the connected peer.
    // The connection attempt should consistently fail due to the check in ConnectNode().
    for (int i = 0; i < 10; ++i) {
        ASSERT_DEBUG_LOG("Not opening a connection to localhost, already connected to 127.0.0.1:8333");
        BOOST_CHECK(!connman->ConnectNodePublic(*peerman, "localhost", ConnectionType::MANUAL));
    }

    // Add 3 more peer connections.
    AddPeer(id, nodes, *peerman, *connman, ConnectionType::OUTBOUND_FULL_RELAY);
    AddPeer(id, nodes, *peerman, *connman, ConnectionType::BLOCK_RELAY, /*onion_peer=*/true);
    AddPeer(id, nodes, *peerman, *connman, ConnectionType::INBOUND);

    // Add a CJDNS peer connection.
    AddPeer(id, nodes, *peerman, *connman, ConnectionType::INBOUND, /*onion_peer=*/false,
            /*address=*/"[fc00:3344:5566:7788:9900:aabb:ccdd:eeff]:1234");
    BOOST_CHECK(nodes.back()->IsInboundConn());
    BOOST_CHECK_EQUAL(nodes.back()->ConnectedThroughNetwork(), Network::NET_CJDNS);

    BOOST_TEST_MESSAGE("Call AddNode() for all the peers");
    for (auto node : connman->TestNodes()) {
        BOOST_CHECK(connman->AddNode({/*m_added_node=*/node->addr.ToStringAddrPort(), /*m_use_v2transport=*/true}));
        BOOST_TEST_MESSAGE(strprintf("peer id=%s addr=%s", node->GetId(), node->addr.ToStringAddrPort()));
    }

    BOOST_TEST_MESSAGE("\nCall AddNode() with 2 addrs resolving to existing localhost addnode entry; neither should be added");
    BOOST_CHECK(!connman->AddNode({/*m_added_node=*/"127.0.0.1", /*m_use_v2transport=*/true}));
    // OpenBSD doesn't support the IPv4 shorthand notation with omitted zero-bytes.
#if !defined(__OpenBSD__)
    BOOST_CHECK(!connman->AddNode({/*m_added_node=*/"127.1", /*m_use_v2transport=*/true}));
#endif

    BOOST_TEST_MESSAGE("\nExpect GetAddedNodeInfo to return expected number of peers with `include_connected` true/false");
    BOOST_CHECK_EQUAL(connman->GetAddedNodeInfo(/*include_connected=*/true).size(), nodes.size());
    BOOST_CHECK(connman->GetAddedNodeInfo(/*include_connected=*/false).empty());

    // Test AddedNodesContain()
    for (auto node : connman->TestNodes()) {
        BOOST_CHECK(connman->AddedNodesContain(node->addr));
    }
    AddPeer(id, nodes, *peerman, *connman, ConnectionType::OUTBOUND_FULL_RELAY);
    BOOST_CHECK(!connman->AddedNodesContain(nodes.back()->addr));

    BOOST_TEST_MESSAGE("\nPrint GetAddedNodeInfo contents:");
    for (const auto& info : connman->GetAddedNodeInfo(/*include_connected=*/true)) {
        BOOST_TEST_MESSAGE(strprintf("\nadded node: %s", info.m_params.m_added_node));
        BOOST_TEST_MESSAGE(strprintf("connected: %s", info.fConnected));
        if (info.fConnected) {
            BOOST_TEST_MESSAGE(strprintf("IP address: %s", info.resolvedAddress.ToStringAddrPort()));
            BOOST_TEST_MESSAGE(strprintf("direction: %s", info.fInbound ? "inbound" : "outbound"));
        }
    }

    BOOST_TEST_MESSAGE("\nCheck that all connected peers are correctly detected as connected");
    for (auto node : connman->TestNodes()) {
        BOOST_CHECK(connman->AlreadyConnectedPublic(node->addr));
    }

    // Clean up
    for (auto node : connman->TestNodes()) {
        peerman->FinalizeNode(*node);
    }
    connman->ClearTestNodes();
}

BOOST_AUTO_TEST_SUITE_END()
