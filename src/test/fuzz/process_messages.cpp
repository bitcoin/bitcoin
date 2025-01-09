// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/consensus.h>
#include <net.h>
#include <net_processing.h>
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
} // namespace

void initialize_process_messages()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>(
            /*chain_type=*/ChainType::REGTEST,
            {.extra_args = {"-txreconciliation"}});
    g_setup = testing_setup.get();
    for (int i = 0; i < 2 * COINBASE_MATURITY; i++) {
        MineBlock(g_setup->m_node, {});
    }
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
}

FUZZ_TARGET(process_messages, .init = initialize_process_messages)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    ConnmanTestMsg& connman = *static_cast<ConnmanTestMsg*>(g_setup->m_node.connman.get());
    auto& chainman = static_cast<TestChainstateManager&>(*g_setup->m_node.chainman);
    SetMockTime(1610000000); // any time to successfully reset ibd
    chainman.ResetIbd();

    LOCK(NetEventsInterface::g_msgproc_mutex);

    std::vector<CNode*> peers;
    const auto num_peers_to_add = fuzzed_data_provider.ConsumeIntegralInRange(1, 3);
    for (int i = 0; i < num_peers_to_add; ++i) {
        peers.push_back(ConsumeNodeAsUniquePtr(fuzzed_data_provider, i).release());
        CNode& p2p_node = *peers.back();

        FillNode(fuzzed_data_provider, connman, p2p_node);

        connman.AddTestNode(p2p_node);
    }

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 30)
    {
        const std::string random_message_type{fuzzed_data_provider.ConsumeBytesAsString(CMessageHeader::MESSAGE_TYPE_SIZE).c_str()};

        const auto mock_time = ConsumeTime(fuzzed_data_provider);
        SetMockTime(mock_time);

        CSerializedNetMsg net_msg;
        net_msg.m_type = random_message_type;
        net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider, MAX_PROTOCOL_MESSAGE_LENGTH);

        CNode& random_node = *PickValue(fuzzed_data_provider, peers);

        connman.FlushSendBuffer(random_node);
        (void)connman.ReceiveMsgFrom(random_node, std::move(net_msg));

        bool more_work{true};
        while (more_work) { // Ensure that every message is eventually processed in some way or another
            random_node.fPauseSend = false;

            try {
                more_work = connman.ProcessMessagesOnce(random_node);
            } catch (const std::ios_base::failure&) {
            }
            g_setup->m_node.peerman->SendMessages(&random_node);
        }
    }
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
    g_setup->m_node.connman->StopNodes();
}
