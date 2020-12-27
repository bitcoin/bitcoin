// Copyright (c) 2020 The Bitcoin Core developers
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
#include <test/util/mining.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <util/memory.h>
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
#include <vector>

namespace {
const TestingSetup* g_setup;
} // namespace

void initialize_process_message()
{
    static TestingSetup setup{
        CBaseChainParams::REGTEST,
        {
            "-nodebuglogfile",
        },
    };
    g_setup = &setup;

    for (int i = 0; i < 2 * COINBASE_MATURITY; i++) {
        MineBlock(g_setup->m_node, CScript() << OP_TRUE);
    }
    SyncWithValidationInterfaceQueue();
}

void fuzz_target(const std::vector<uint8_t>& buffer, const std::string& LIMIT_TO_MESSAGE_TYPE)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    ConnmanTestMsg& connman = *(ConnmanTestMsg*)g_setup->m_node.connman.get();
    TestChainState& chainstate = *(TestChainState*)&g_setup->m_node.chainman->ActiveChainstate();
    chainstate.ResetIbd();
    const std::string random_message_type{fuzzed_data_provider.ConsumeBytesAsString(CMessageHeader::COMMAND_SIZE).c_str()};
    if (!LIMIT_TO_MESSAGE_TYPE.empty() && random_message_type != LIMIT_TO_MESSAGE_TYPE) {
        return;
    }
    const bool jump_out_of_ibd{fuzzed_data_provider.ConsumeBool()};
    if (jump_out_of_ibd) chainstate.JumpOutOfIbd();
    CDataStream random_bytes_data_stream{fuzzed_data_provider.ConsumeRemainingBytes<unsigned char>(), SER_NETWORK, PROTOCOL_VERSION};
    CNode& p2p_node = *MakeUnique<CNode>(0, ServiceFlags(NODE_NETWORK | NODE_WITNESS | NODE_BLOOM), 0, INVALID_SOCKET, CAddress{CService{in_addr{0x0100007f}, 7777}, NODE_NETWORK}, 0, 0, CAddress{}, std::string{}, ConnectionType::OUTBOUND_FULL_RELAY).release();
    p2p_node.fSuccessfullyConnected = true;
    p2p_node.nVersion = PROTOCOL_VERSION;
    p2p_node.SetCommonVersion(PROTOCOL_VERSION);
    connman.AddTestNode(p2p_node);
    g_setup->m_node.peerman->InitializeNode(&p2p_node);
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
FUZZ_TARGET_INIT(process_message_addr, initialize_process_message) { fuzz_target(buffer, "addr"); }
FUZZ_TARGET_INIT(process_message_block, initialize_process_message) { fuzz_target(buffer, "block"); }
FUZZ_TARGET_INIT(process_message_blocktxn, initialize_process_message) { fuzz_target(buffer, "blocktxn"); }
FUZZ_TARGET_INIT(process_message_cmpctblock, initialize_process_message) { fuzz_target(buffer, "cmpctblock"); }
FUZZ_TARGET_INIT(process_message_feefilter, initialize_process_message) { fuzz_target(buffer, "feefilter"); }
FUZZ_TARGET_INIT(process_message_filteradd, initialize_process_message) { fuzz_target(buffer, "filteradd"); }
FUZZ_TARGET_INIT(process_message_filterclear, initialize_process_message) { fuzz_target(buffer, "filterclear"); }
FUZZ_TARGET_INIT(process_message_filterload, initialize_process_message) { fuzz_target(buffer, "filterload"); }
FUZZ_TARGET_INIT(process_message_getaddr, initialize_process_message) { fuzz_target(buffer, "getaddr"); }
FUZZ_TARGET_INIT(process_message_getblocks, initialize_process_message) { fuzz_target(buffer, "getblocks"); }
FUZZ_TARGET_INIT(process_message_getblocktxn, initialize_process_message) { fuzz_target(buffer, "getblocktxn"); }
FUZZ_TARGET_INIT(process_message_getdata, initialize_process_message) { fuzz_target(buffer, "getdata"); }
FUZZ_TARGET_INIT(process_message_getheaders, initialize_process_message) { fuzz_target(buffer, "getheaders"); }
FUZZ_TARGET_INIT(process_message_headers, initialize_process_message) { fuzz_target(buffer, "headers"); }
FUZZ_TARGET_INIT(process_message_inv, initialize_process_message) { fuzz_target(buffer, "inv"); }
FUZZ_TARGET_INIT(process_message_mempool, initialize_process_message) { fuzz_target(buffer, "mempool"); }
FUZZ_TARGET_INIT(process_message_notfound, initialize_process_message) { fuzz_target(buffer, "notfound"); }
FUZZ_TARGET_INIT(process_message_ping, initialize_process_message) { fuzz_target(buffer, "ping"); }
FUZZ_TARGET_INIT(process_message_pong, initialize_process_message) { fuzz_target(buffer, "pong"); }
FUZZ_TARGET_INIT(process_message_sendcmpct, initialize_process_message) { fuzz_target(buffer, "sendcmpct"); }
FUZZ_TARGET_INIT(process_message_sendheaders, initialize_process_message) { fuzz_target(buffer, "sendheaders"); }
FUZZ_TARGET_INIT(process_message_tx, initialize_process_message) { fuzz_target(buffer, "tx"); }
FUZZ_TARGET_INIT(process_message_verack, initialize_process_message) { fuzz_target(buffer, "verack"); }
FUZZ_TARGET_INIT(process_message_version, initialize_process_message) { fuzz_target(buffer, "version"); }
