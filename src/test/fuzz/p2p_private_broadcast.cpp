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
// Used as the guided path in the ConsumeBool() below.
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
    auto& chainman{static_cast<TestChainstateManager&>(*node.chainman)};

    NodeClockContext clock_ctx{1610000000s};
    chainman.ResetIbd();

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

    // Seed with 1-3 transactions to test multiple pending broadcasts.
    const int num_txs{fuzzed_data_provider.ConsumeIntegralInRange(1, 3)};
    for (int i = 0; i < num_txs; ++i) {
        node.peerman->InitiateTxBroadcastPrivate(
            MakeTransactionRef(ConsumeTransaction(fuzzed_data_provider, /*prevout_txids=*/std::nullopt)));
    }

    LOCK(NetEventsInterface::g_msgproc_mutex);

    static NodeId node_id{0};
    // Create at least one PRIVATE_BROADCAST peer, optionally add others of random types.
    std::vector<CNode*> peers;

    CNode* pb_node = new CNode(
        /*id=*/node_id++,
        /*sock=*/std::make_shared<FuzzedSock>(fuzzed_data_provider),
        /*addrIn=*/ConsumeAddress(fuzzed_data_provider),
        /*nKeyedNetGroupIn=*/fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
        /*nLocalHostNonceIn=*/fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
        /*addrBindIn=*/ConsumeAddress(fuzzed_data_provider),
        /*addrNameIn=*/fuzzed_data_provider.ConsumeRandomLengthString(64),
        /*conn_type_in=*/ConnectionType::PRIVATE_BROADCAST,
        /*inbound_onion=*/false,
        /*network_key=*/fuzzed_data_provider.ConsumeIntegral<uint64_t>());

    peers.push_back(pb_node);
    connman.AddTestNode(*pb_node);
    node.peerman->InitializeNode(
        *pb_node,
        NODE_NONE); // We advertise our services as NODE_NONE to private broadcast peers (see PushNodeVersion()).

    // Optionally add extra peers of random connection types.
    const int extra_peers{fuzzed_data_provider.ConsumeIntegralInRange(0, 2)};
    for (int i = 0; i < extra_peers; ++i) {
        peers.push_back(ConsumeNodeAsUniquePtr(fuzzed_data_provider, node_id++).release());
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

        CSerializedNetMsg net_msg;

        // ConsumeBool() branch: guided path uses known message types,
        // unconstrained path explores arbitrary types for robustness.
        if (fuzzed_data_provider.ConsumeBool()) {
            net_msg.m_type = std::string{PickValue(fuzzed_data_provider, INBOUND_MSG_TYPES)};
        } else {
            net_msg.m_type = fuzzed_data_provider.ConsumeRandomLengthString(
                CMessageHeader::MESSAGE_TYPE_SIZE);
        }
        net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider, MAX_PROTOCOL_MESSAGE_LENGTH);

        connman.FlushSendBuffer(p2p_node);
        (void)connman.ReceiveMsgFrom(p2p_node, std::move(net_msg));

        bool more_work{true};
        while (more_work) {
            p2p_node.fPauseSend = false;
            try {
                more_work = connman.ProcessMessagesOnce(p2p_node);
            } catch (const std::ios_base::failure&) {
            }
            node.peerman->SendMessages(p2p_node);
        }
        // Verify outbound INV is well formed after handshake on private broadcast peers.
        if (p2p_node.IsPrivateBroadcastConn() && p2p_node.fSuccessfullyConnected) {
            LOCK(p2p_node.cs_vSend);
            for (const auto& msg : p2p_node.vSendMsg) {
                if (msg.m_type == NetMsgType::INV) {
                    DataStream ds{msg.data};
                    std::vector<CInv> invs;
                    ds >> invs;
                    Assert(invs.size() == 1);
                    Assert(invs[0].IsMsgTx());
                }
            }
        }
    }
    node.connman->StopNodes();
}
