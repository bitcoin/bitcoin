// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <banman.h>
#include <chainparams.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <pubkey.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <serialize.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/string.h>
#include <util/system.h>
#include <util/time.h>
#include <timedata.h>
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

/** Mock the connections interface. */
class ConnectionsInterfaceMock : public ConnectionsInterface
{
public:
    bool ForNode(NodeId id, std::function<bool(CNode* pnode)> func) override { return true; };
    using NodeFn = std::function<void(CNode*)>;
    void ForEachNode(const NodeFn& func) override {};
    void ForEachNode(const NodeFn& func) const override {};
    void PushMessage(CNode* pnode, CSerializedNetMsg&& msg) override;
    CSipHasher GetDeterministicRandomizer(uint64_t id) const override { return {0, 0}; };
    void WakeMessageHandler() override {};
    bool OutboundTargetReached(bool historicalBlockServingLimit) const override { return true; };
    std::vector<CAddress> GetAddresses(size_t max_addresses,
                                       size_t max_pct,
                                       std::optional<Network> network) const override { return {}; };
    std::vector<CAddress> GetAddresses(CNode& requestor, size_t max_addresses, size_t max_pct) override { return {}; };
    bool DisconnectNode(const CNetAddr& addr) override { return true; };
    unsigned int GetReceiveFloodSize() const override { return 0; };
    int GetExtraFullOutboundCount() const override { return 0; };
    int GetExtraBlockRelayCount() const override { return 0; };
    void SetTryNewOutboundPeer(bool flag) override {};
    bool GetTryNewOutboundPeer() const override { return true; };
    bool GetNetworkActive() const override { return true; };
    bool GetUseAddrmanOutgoing() const override { return true; };
    void StartExtraBlockRelayPeers() override {};
    bool ShouldRunInactivityChecks(const CNode& node, std::chrono::seconds now) const override { return true; };
    bool CheckIncomingNonce(uint64_t nonce) override { return true; };

    void NodeReceiveMsgBytes(CNode& node, Span<const uint8_t> msg_bytes, bool& complete) const;
    bool ReceiveMsgFrom(CNode& node, CSerializedNetMsg& ser_msg) const;

    virtual ~ConnectionsInterfaceMock() {}

    /** Count of number of each message type sent */
    std::map<std::string, uint64_t> m_message_types_sent;
};

void ConnectionsInterfaceMock::PushMessage(CNode* pnode, CSerializedNetMsg&& msg)
{
    BOOST_TEST_MESSAGE(strprintf("sending message %s to peer %d", msg.m_type, pnode->GetId()));
    m_message_types_sent[msg.m_type]++;
}

void ConnectionsInterfaceMock::NodeReceiveMsgBytes(CNode& node, Span<const uint8_t> msg_bytes, bool& complete) const
{
    assert(node.ReceiveMsgBytes(msg_bytes, complete));
    if (complete) {
        size_t nSizeAdded = 0;
        auto it(node.vRecvMsg.begin());
        for (; it != node.vRecvMsg.end(); ++it) {
            // vRecvMsg contains only completed CNetMessage
            // the single possible partially deserialized message are held by TransportDeserializer
            nSizeAdded += it->m_raw_message_size;
        }
        {
            LOCK(node.cs_vProcessMsg);
            node.vProcessMsg.splice(node.vProcessMsg.end(), node.vRecvMsg, node.vRecvMsg.begin(), it);
            node.nProcessQueueSize += nSizeAdded;
        }
    }
}

bool ConnectionsInterfaceMock::ReceiveMsgFrom(CNode& node, CSerializedNetMsg& ser_msg) const
{
    std::vector<uint8_t> ser_msg_header;
    node.m_serializer->prepareForTransport(ser_msg, ser_msg_header);

    bool complete;
    NodeReceiveMsgBytes(node, ser_msg_header, complete);
    NodeReceiveMsgBytes(node, ser_msg.data, complete);
    return complete;
}

BOOST_FIXTURE_TEST_SUITE(peerman_tests, TestingSetup)

static void SendMessage(ConnectionsInterfaceMock& connman, PeerManager& peerman, CNode& node, CSerializedNetMsg& msg)
{
    std::atomic<bool> interrupt{false};
    (void)connman.ReceiveMsgFrom(node, msg);
    node.fPauseSend = false;
    LOCK(NetEventsInterface::g_msgproc_mutex);
    peerman.ProcessMessages(&node, interrupt);
    peerman.SendMessages(&node);
}

using HandshakeHookFn = std::function<void(ConnectionsInterfaceMock& connman, PeerManager& peerman, CNode& node)>;
static void Handshake(
    ConnectionsInterfaceMock& connman, PeerManager& peerman,
    CNode& node, ServiceFlags their_services,
    HandshakeHookFn pre_verack_hook = [](ConnectionsInterfaceMock& connman, PeerManager& peerman, CNode& node) {}) noexcept
{
    // Send version message
    const CNetMsgMaker mm{0};
    CSerializedNetMsg msg_version{
        mm.Make(NetMsgType::VERSION,
                PROTOCOL_VERSION,                              // current p2p version
                Using<CustomUintFormatter<8>>(their_services), // their service flags
                GetTime(),                                     // dummy time
                int64_t{},                                     // ignored service bits
                CService{},                                    // dummy
                int64_t{},                                     // ignored service bits
                CService{},                                    // ignored
                uint64_t{1},                                   // dummy nonce
                std::string{},                                 // dummy subver
                int32_t{},                                     // dummy starting_height
                true),                                         // fRelay
    };
    SendMessage(connman, peerman, node, msg_version);
    BOOST_CHECK(!node.fDisconnect);
    BOOST_CHECK_EQUAL(node.nVersion, PROTOCOL_VERSION);
    BOOST_CHECK_EQUAL(node.GetCommonVersion(), PROTOCOL_VERSION);
    CNodeStateStats stats;
    bool ret = peerman.GetNodeStateStats(node.GetId(), stats);
    BOOST_CHECK_EQUAL(ret, true);
    BOOST_CHECK_EQUAL(stats.their_services, their_services);

    pre_verack_hook(connman, peerman, node);

    // Send verack message
    CSerializedNetMsg msg_verack{mm.Make(NetMsgType::VERACK)};
    SendMessage(connman, peerman, node, msg_verack);
    BOOST_CHECK_EQUAL(node.fSuccessfullyConnected, true);

    TestOnlyResetTimeData();
}

BOOST_AUTO_TEST_CASE(version_handshake)
{
    auto connman = std::make_unique<ConnectionsInterfaceMock>();
    auto peerman = PeerManager::make(*connman, *m_node.addrman, nullptr,
                                     *m_node.chainman, *m_node.mempool, false);

    // Mock an outbound peer
    CAddress addr1(ip(0xa0b0c001), NODE_NONE);
    CNode node{/*id=*/0, /*sock=*/nullptr, addr1, /*nKeyedNetGroupIn=*/0, /*nLocalHostNonceIn=*/0, CAddress(),
               /*addrNameIn=*/"", ConnectionType::OUTBOUND_FULL_RELAY, /*inbound_onion=*/false};

    peerman->InitializeNode(node, /*our_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS));
    Handshake(*connman, *peerman, node, /*their_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS));

    BOOST_CHECK_EQUAL(connman->m_message_types_sent[NetMsgType::VERSION], 1);
    BOOST_CHECK_EQUAL(connman->m_message_types_sent[NetMsgType::WTXIDRELAY], 1);
    BOOST_CHECK_EQUAL(connman->m_message_types_sent[NetMsgType::SENDADDRV2], 1);
    BOOST_CHECK_EQUAL(connman->m_message_types_sent[NetMsgType::VERACK], 1);
    BOOST_CHECK_EQUAL(connman->m_message_types_sent[NetMsgType::GETADDR], 1);
    BOOST_CHECK_EQUAL(connman->m_message_types_sent[NetMsgType::SENDCMPCT], 1);
    BOOST_CHECK_EQUAL(connman->m_message_types_sent[NetMsgType::GETHEADERS], 1);
    BOOST_CHECK_EQUAL(connman->m_message_types_sent[NetMsgType::FEEFILTER], 1);

    peerman->FinalizeNode(node);
}

BOOST_AUTO_TEST_SUITE_END()
