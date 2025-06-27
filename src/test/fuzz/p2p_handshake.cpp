// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <consensus/consensus.h>
#include <net.h>
#include <net_processing.h>
#include <node/warnings.h>
#include <protocol.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/mining.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <util/time.h>
#include <validationinterface.h>

#include <ios>
#include <string>
#include <utility>
#include <vector>

namespace {
const TestingSetup* g_setup;

void initialize()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>(
        /*chain_type=*/ChainType::REGTEST);
    g_setup = testing_setup.get();
}
} // namespace

FUZZ_TARGET(p2p_handshake, .init = ::initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    auto& connman = static_cast<ConnmanTestMsg&>(*g_setup->m_node.connman);
    auto& chainman = static_cast<TestChainstateManager&>(*g_setup->m_node.chainman);
    SetMockTime(1610000000); // any time to successfully reset ibd
    chainman.ResetIbd();

    node::Warnings warnings{};
    NetGroupManager netgroupman{{}};
    AddrMan addrman{netgroupman, /*deterministic=*/true, /*consistency_check_ratio=*/0};
    auto peerman = PeerManager::make(connman, addrman,
                                     /*banman=*/nullptr, chainman,
                                     *g_setup->m_node.mempool, warnings,
                                     PeerManager::Options{
                                         .reconcile_txs = true,
                                         .deterministic_rng = true,
                                     });
    connman.SetMsgProc(peerman.get());

    LOCK(NetEventsInterface::g_msgproc_mutex);

    std::vector<CNode*> peers;
    const auto num_peers_to_add = fuzzed_data_provider.ConsumeIntegralInRange(1, 3);
    for (int i = 0; i < num_peers_to_add; ++i) {
        peers.push_back(ConsumeNodeAsUniquePtr(fuzzed_data_provider, i).release());
        connman.AddTestNode(*peers.back());
        peerman->InitializeNode(
            *peers.back(),
            static_cast<ServiceFlags>(fuzzed_data_provider.ConsumeIntegral<uint64_t>()));
    }

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
    {
        CNode& connection = *PickValue(fuzzed_data_provider, peers);
        if (connection.fDisconnect || connection.fSuccessfullyConnected) {
            // Skip if the connection was disconnected or if the version
            // handshake was already completed.
            continue;
        }

        SetMockTime(GetTime() +
                    fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(
                        -std::chrono::seconds{10min}.count(), // Allow mocktime to go backwards slightly
                        std::chrono::seconds{TIMEOUT_INTERVAL}.count()));

        CSerializedNetMsg net_msg;
        net_msg.m_type = PickValue(fuzzed_data_provider, ALL_NET_MESSAGE_TYPES);
        net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider, MAX_PROTOCOL_MESSAGE_LENGTH);

        connman.FlushSendBuffer(connection);
        (void)connman.ReceiveMsgFrom(connection, std::move(net_msg));

        bool more_work{true};
        while (more_work) {
            connection.fPauseSend = false;

            try {
                more_work = connman.ProcessMessagesOnce(connection);
            } catch (const std::ios_base::failure&) {
            }
            peerman->SendMessages(&connection);
        }
    }

    g_setup->m_node.connman->StopNodes();
}
