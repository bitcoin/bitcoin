// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <chainparams.h>
#include <consensus/consensus.h>
#include <net.h>
#include <net_processing.h>
#include <protocol.h>
#include <scheduler.h>
#include <script/script.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <txorphanage.h>
#include <validationinterface.h>
#include <version.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <string>

namespace {
const TestingSetup* g_setup;
} // namespace

size_t& GetNumMsgTypes()
{
    static size_t g_num_msg_types{0};
    return g_num_msg_types;
}
#define FUZZ_TARGET_MSG(msg_type)                                            \
    struct msg_type##_Count_Before_Main {                                    \
        msg_type##_Count_Before_Main()                                       \
        {                                                                    \
            ++GetNumMsgTypes();                                              \
        }                                                                    \
    } const static g_##msg_type##_count_before_main;                         \
    FUZZ_TARGET_INIT(process_message_##msg_type, initialize_process_message) \
    {                                                                        \
        fuzz_target(buffer, #msg_type);                                      \
    }

void initialize_process_message()
{
    Assert(GetNumMsgTypes() == getAllNetMessageTypes().size()); // If this fails, add or remove the message type below

    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    for (int i = 0; i < 2 * COINBASE_MATURITY; i++) {
        MineBlock(g_setup->m_node, CScript() << OP_TRUE);
    }
    SyncWithValidationInterfaceQueue();
}

void fuzz_target(FuzzBufferType buffer, const std::string& LIMIT_TO_MESSAGE_TYPE)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    ConnmanTestMsg& connman = *static_cast<ConnmanTestMsg*>(g_setup->m_node.connman.get());
    TestChainState& chainstate = *static_cast<TestChainState*>(&g_setup->m_node.chainman->ActiveChainstate());
    SetMockTime(1610000000); // any time to successfully reset ibd
    chainstate.ResetIbd();

    const std::string random_message_type{fuzzed_data_provider.ConsumeBytesAsString(CMessageHeader::COMMAND_SIZE).c_str()};
    if (!LIMIT_TO_MESSAGE_TYPE.empty() && random_message_type != LIMIT_TO_MESSAGE_TYPE) {
        return;
    }
    CNode& p2p_node = *ConsumeNodeAsUniquePtr(fuzzed_data_provider).release();

    const bool successfully_connected{fuzzed_data_provider.ConsumeBool()};
    p2p_node.fSuccessfullyConnected = successfully_connected;
    connman.AddTestNode(p2p_node);
    g_setup->m_node.peerman->InitializeNode(&p2p_node);
    FillNode(fuzzed_data_provider, p2p_node, /* init_version */ successfully_connected);

    const auto mock_time = ConsumeTime(fuzzed_data_provider);
    SetMockTime(mock_time);

    // fuzzed_data_provider is fully consumed after this call, don't use it
    CDataStream random_bytes_data_stream{fuzzed_data_provider.ConsumeRemainingBytes<unsigned char>(), SER_NETWORK, PROTOCOL_VERSION};
    try {
        g_setup->m_node.peerman->ProcessMessage(p2p_node, random_message_type, random_bytes_data_stream,
                                                GetTime<std::chrono::microseconds>(), std::atomic<bool>{false});
    } catch (const std::ios_base::failure&) {
    }
    {
        LOCK(p2p_node.cs_sendProcessing);
        g_setup->m_node.peerman->SendMessages(&p2p_node);
    }
    SyncWithValidationInterfaceQueue();
    LOCK2(::cs_main, g_cs_orphans); // See init.cpp for rationale for implicit locking order requirement
    g_setup->m_node.connman->StopNodes();
}

FUZZ_TARGET_INIT(process_message, initialize_process_message) { fuzz_target(buffer, ""); }
FUZZ_TARGET_MSG(addr);
FUZZ_TARGET_MSG(addrv2);
FUZZ_TARGET_MSG(block);
FUZZ_TARGET_MSG(blocktxn);
FUZZ_TARGET_MSG(cfcheckpt);
FUZZ_TARGET_MSG(cfheaders);
FUZZ_TARGET_MSG(cfilter);
FUZZ_TARGET_MSG(cmpctblock);
FUZZ_TARGET_MSG(feefilter);
FUZZ_TARGET_MSG(filteradd);
FUZZ_TARGET_MSG(filterclear);
FUZZ_TARGET_MSG(filterload);
FUZZ_TARGET_MSG(getaddr);
FUZZ_TARGET_MSG(getblocks);
FUZZ_TARGET_MSG(getblocktxn);
FUZZ_TARGET_MSG(getcfcheckpt);
FUZZ_TARGET_MSG(getcfheaders);
FUZZ_TARGET_MSG(getcfilters);
FUZZ_TARGET_MSG(getdata);
FUZZ_TARGET_MSG(getheaders);
FUZZ_TARGET_MSG(headers);
FUZZ_TARGET_MSG(inv);
FUZZ_TARGET_MSG(mempool);
FUZZ_TARGET_MSG(merkleblock);
FUZZ_TARGET_MSG(notfound);
FUZZ_TARGET_MSG(ping);
FUZZ_TARGET_MSG(pong);
FUZZ_TARGET_MSG(sendaddrv2);
FUZZ_TARGET_MSG(sendcmpct);
FUZZ_TARGET_MSG(sendheaders);
FUZZ_TARGET_MSG(tx);
FUZZ_TARGET_MSG(verack);
FUZZ_TARGET_MSG(version);
FUZZ_TARGET_MSG(wtxidrelay);
