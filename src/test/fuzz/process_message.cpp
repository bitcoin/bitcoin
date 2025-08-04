// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/consensus.h>
#include <net.h>
#include <net_processing.h>
#include <node/warnings.h>
#include <primitives/transaction.h>
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
#include <test/util/time.h>
#include <test/util/validation.h>
#include <util/check.h>
#include <util/time.h>
#include <validationinterface.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {
TestingSetup* g_setup;
std::string_view LIMIT_TO_MESSAGE_TYPE{};

void ResetChainman(TestingSetup& setup)
{
    SetMockTime(setup.m_node.chainman->GetParams().GenesisBlock().Time());
    setup.m_node.chainman.reset();
    setup.m_make_chainman();
    setup.LoadVerifyActivateChainstate();
    for (int i = 0; i < 2 * COINBASE_MATURITY; i++) {
        MineBlock(setup.m_node, {});
    }
    setup.m_node.validation_signals->SyncWithValidationInterfaceQueue();
}
} // namespace

void initialize_process_message()
{
    if (const auto val{std::getenv("LIMIT_TO_MESSAGE_TYPE")}) {
        LIMIT_TO_MESSAGE_TYPE = val;
        Assert(std::count(ALL_NET_MESSAGE_TYPES.begin(), ALL_NET_MESSAGE_TYPES.end(), LIMIT_TO_MESSAGE_TYPE)); // Unknown message type passed
    }

    static const auto testing_setup{
        MakeNoLogFileContext<TestingSetup>(
            /*chain_type=*/ChainType::REGTEST,
            {}),
    };
    g_setup = testing_setup.get();
    ResetChainman(*g_setup);
}

FUZZ_TARGET(process_message, .init = initialize_process_message)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    auto& connman = static_cast<ConnmanTestMsg&>(*g_setup->m_node.connman);
    connman.ResetAddrCache();
    connman.ResetMaxOutboundCycle();
    auto& chainman = static_cast<TestChainstateManager&>(*g_setup->m_node.chainman);
    const auto block_index_size{WITH_LOCK(chainman.GetMutex(), return chainman.BlockIndex().size())};
    ElapseTime elapse_time{1610000000s}; // any time to successfully reset ibd
    chainman.ResetIbd();
    chainman.DisableNextWrite();

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

    const std::string random_message_type{fuzzed_data_provider.ConsumeBytesAsString(CMessageHeader::MESSAGE_TYPE_SIZE).c_str()};
    if (!LIMIT_TO_MESSAGE_TYPE.empty() && random_message_type != LIMIT_TO_MESSAGE_TYPE) {
        return;
    }
    CNode& p2p_node = *ConsumeNodeAsUniquePtr(fuzzed_data_provider).release();

    connman.AddTestNode(p2p_node);
    FillNode(fuzzed_data_provider, connman, p2p_node);

    elapse_time.set(ConsumeTime(fuzzed_data_provider));

    CSerializedNetMsg net_msg;
    net_msg.m_type = random_message_type;
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
        g_setup->m_node.peerman->SendMessages(&p2p_node);
    }
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
    g_setup->m_node.connman->StopNodes();
    if (block_index_size != WITH_LOCK(chainman.GetMutex(), return chainman.BlockIndex().size())) {
        // Reuse the global chainman, but reset it when it is dirty
        ResetChainman(*g_setup);
    }
}
