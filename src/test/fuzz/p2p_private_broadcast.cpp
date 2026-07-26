// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <net.h>
#include <net_processing.h>
#include <protocol.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/validation.h>
#include <util/time.h>
#include <validationinterface.h>

#include <array>
#include <ios>
#include <memory>
#include <vector>

namespace {
TestingSetup* g_setup;

void initialize()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>(
        /*chain_type=*/ChainType::REGTEST);
    g_setup = testing_setup.get();
}

// Inbound message types with private broadcast specific handling.
// Used as the guided path in the CallOneOf() below.
constexpr std::array INBOUND_MSG_TYPES{
    NetMsgType::VERSION,
    NetMsgType::VERACK,
    NetMsgType::GETDATA,
    NetMsgType::PONG,
};
} // namespace

FUZZ_TARGET(p2p_private_broadcast, .init = ::initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    auto& node{g_setup->m_node};
    auto& connman{static_cast<ConnmanTestMsg&>(*node.connman)};
    connman.Reset();
    auto& chainman{static_cast<TestChainstateManager&>(*node.chainman)};

    FakeNodeClock clock_ctx{1610000000s};
    FakeSteadyClock steady_clock;
    chainman.ResetIbd();
    // Sometimes leave IBD: incoming TX processing (the broadcast-abort path)
    // returns early during IBD.
    if (fuzzed_data_provider.ConsumeBool()) chainman.JumpOutOfIbd();

    // Reset, so that dangling pointers can be detected by sanitizers.
    node.banman.reset();
    node.addrman.reset();
    node.peerman.reset();
    node.addrman = std::make_unique<AddrMan>(
        *node.netgroupman, /*deterministic=*/true, /*consistency_check_ratio=*/0);
    node.peerman = PeerManager::make(connman, *node.addrman,
                                     /*banman=*/nullptr, chainman,
                                     *node.mempool, *node.warnings,
                                     PeerManager::Options{
                                         .reconcile_txs = true,
                                         .deterministic_rng = true,
                                     });
    connman.SetMsgProc(node.peerman.get());
    connman.SetAddrman(*node.addrman);

    // Seed with 0-3 transactions to test multiple pending broadcasts; zero exercises
    // the connected-in-vain disconnect in PushPrivateBroadcastTx().
    const int num_txs{fuzzed_data_provider.ConsumeIntegralInRange(0, 3)};
    std::vector<CTransactionRef> seeded_txs;
    for (int i = 0; i < num_txs; ++i) {
        auto tx{MakeTransactionRef(ConsumeTransaction(fuzzed_data_provider, /*prevout_txids=*/std::nullopt))};
        (void)node.peerman->InitiateTxBroadcastPrivate(tx);
        seeded_txs.push_back(tx);
    }

    LOCK(NetEventsInterface::g_msgproc_mutex);

    static NodeId node_id{0};
    // Create at least one PRIVATE_BROADCAST peer, optionally add others of random types.
    std::vector<CNode*> peers;

    CNode* pb_node = new CNode(
        /*id=*/node_id++,
        /*sock=*/std::make_shared<FuzzedSock>(fuzzed_data_provider, steady_clock),
        /*addrIn=*/ConsumeAddress(fuzzed_data_provider),
        /*nKeyedNetGroupIn=*/0,
        /*nLocalHostNonceIn=*/0,
        /*addrBindIn=*/CService{},
        /*addrNameIn=*/"",
        /*conn_type_in=*/ConnectionType::PRIVATE_BROADCAST,
        /*inbound_onion=*/false,
        /*network_key=*/0);

    peers.push_back(pb_node);
    connman.AddTestNode(*pb_node);
    // Capture outbound messages to verify if well formed (and to learn the PING
    // nonce), before SocketSendData drains vSendMsg.
    connman.SetCaptureMessages(true);
    const auto CaptureMessageOrig = CaptureMessage;
    const CAddress pb_addr = pb_node->addr;
    std::optional<uint64_t> pb_ping_nonce;
    CaptureMessage = [&](const CAddress& addr, const std::string& msg_type,
                         std::span<const unsigned char> data, bool is_incoming) {
        if (is_incoming || addr != pb_addr) return;
        if (msg_type == NetMsgType::PING) {
            Assert(data.size() == sizeof(uint64_t));
            uint64_t nonce;
            SpanReader{data} >> nonce;
            pb_ping_nonce = nonce;
            return;
        }
        if (msg_type == NetMsgType::VERSION) {
            SpanReader ds{data};
            int32_t version;
            uint64_t my_services, your_services, my_services_dup, nonce;
            int64_t my_time;
            CService your_addr, my_addr;
            std::string user_agent;
            int32_t height;
            bool relay;
            ds >> version >> my_services >> my_time >>
                your_services >> CNetAddr::V1(your_addr) >>
                my_services_dup >> CNetAddr::V1(my_addr) >>
                nonce >> user_agent >> height >> relay;
            Assert(version == WTXID_RELAY_VERSION);
            Assert(my_services == NODE_NONE && my_services_dup == NODE_NONE);
            Assert(my_time == 0);
            Assert(your_services == NODE_NONE);
            Assert(your_addr == CService{});
            Assert(user_agent == "/pynode:0.0.1/");
            Assert(height == 0);
            Assert(!relay);
            return;
        }
        if (msg_type != NetMsgType::INV) return;
        SpanReader ds{data};
        std::vector<CInv> invs;
        ds >> invs;
        Assert(invs.size() == 1);
        Assert(invs[0].IsMsgTx());
    };

    // Complete handshake so PushPrivateBroadcastTx runs.
    connman.Handshake(
        /*node=*/*pb_node,
        /*successfully_connected=*/true,
        /*remote_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
        /*local_services=*/NODE_NONE,
        /*version=*/PROTOCOL_VERSION,
        /*relay_txs=*/true);

    // Optionally add extra peers of random connection types.
    const int extra_peers{fuzzed_data_provider.ConsumeIntegralInRange(0, 2)};
    for (int i = 0; i < extra_peers; ++i) {
        auto extra_peer{ConsumeNodeAsUniquePtr(fuzzed_data_provider, steady_clock, node_id++)};
        // An address collision would match the capture hook's filter and fail
        // its assertions on this peer's (legitimate) other-typed messages.
        if (extra_peer->addr == pb_addr) continue;
        peers.push_back(extra_peer.release());
        connman.AddTestNode(*peers.back());
        node.peerman->InitializeNode(
            *peers.back(),
            static_cast<ServiceFlags>(fuzzed_data_provider.ConsumeIntegral<uint64_t>()));
    }

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
    {
        // Pick any random peer to test interleaved message handling.
        CNode& p2p_node = *PickValue(fuzzed_data_provider, peers);
        if (p2p_node.fDisconnect) continue;

        clock_ctx += ConsumeDuration<std::chrono::seconds>(fuzzed_data_provider, 0s, 600s);

        std::optional<CSerializedNetMsg> net_msg;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                net_msg.emplace();
                net_msg->m_type = std::string{PickValue(fuzzed_data_provider, INBOUND_MSG_TYPES)};
            },
            [&] {
                net_msg.emplace();
                net_msg->m_type = fuzzed_data_provider.ConsumeRandomLengthString(CMessageHeader::MESSAGE_TYPE_SIZE);
            },
            [&] {
                (void)node.peerman->InitiateTxBroadcastPrivate(
                    MakeTransactionRef(ConsumeTransaction(fuzzed_data_provider, /*prevout_txids=*/std::nullopt)));
            },
            [&] {
                // Construct a valid GETDATA for a seeded tx to exercise the TX send path.
                if (p2p_node.IsPrivateBroadcastConn() &&
                    p2p_node.fSuccessfullyConnected &&
                    !seeded_txs.empty()) {
                    const auto& tx{PickValue(fuzzed_data_provider, seeded_txs)};
                    net_msg.emplace(NetMsg::Make(
                        NetMsgType::GETDATA,
                        std::vector<CInv>{{MSG_TX, tx->GetHash().ToUint256()}}));
                }
            },
            [&] {
                // Confirm reception of the pushed TX with a PONG matching the captured PING nonce.
                if (&p2p_node == pb_node && pb_ping_nonce) {
                    net_msg.emplace(NetMsg::Make(NetMsgType::PONG, *pb_ping_nonce));
                }
            },
            [&] {
                // Echo a seeded tx back from a non-private-broadcast peer to exercise
                // the received-from-network broadcast-abort path.
                if (!p2p_node.IsPrivateBroadcastConn() &&
                    p2p_node.fSuccessfullyConnected &&
                    !seeded_txs.empty()) {
                    const auto& tx{PickValue(fuzzed_data_provider, seeded_txs)};
                    net_msg.emplace(NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*tx)));
                }
            });

        if (net_msg) {
            if (net_msg->data.empty()) {
                net_msg->data = ConsumeRandomLengthByteVector(fuzzed_data_provider, MAX_PROTOCOL_MESSAGE_LENGTH);
            }
            connman.FlushSendBuffer(p2p_node);
            (void)connman.ReceiveMsgFrom(p2p_node, std::move(*net_msg));

            bool more_work{true};
            while (more_work) {
                p2p_node.fPauseSend = false;
                try {
                    more_work = connman.ProcessMessagesOnce(p2p_node);
                } catch (const std::ios_base::failure&) {
                }
                node.peerman->SendMessages(p2p_node);
            }
        }
    }

    CaptureMessage = CaptureMessageOrig;
    connman.SetCaptureMessages(false);

    node.connman->StopNodes();
}
