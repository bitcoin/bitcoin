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
#include <test/util/setup_common.h>
#include <util/memory.h>
#include <validationinterface.h>
#include <version.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

bool ProcessMessage(CNode* pfrom, const std::string& msg_type, CDataStream& vRecv, int64_t nTimeReceived, const CChainParams& chainparams, CTxMemPool& mempool, CConnman* connman, BanMan* banman, const std::atomic<bool>& interruptMsgProc);

namespace {

#ifdef MESSAGE_TYPE
#define TO_STRING_(s) #s
#define TO_STRING(s) TO_STRING_(s)
const std::string LIMIT_TO_MESSAGE_TYPE{TO_STRING(MESSAGE_TYPE)};
#else
const std::string LIMIT_TO_MESSAGE_TYPE;
#endif

const std::map<std::string, std::set<std::string>> EXPECTED_DESERIALIZATION_EXCEPTIONS = {
    {"CDataStream::read(): end of data: iostream error", {"addr", "block", "blocktxn", "cmpctblock", "feefilter", "filteradd", "filterload", "getblocks", "getblocktxn", "getdata", "getheaders", "headers", "inv", "notfound", "ping", "sendcmpct", "tx"}},
    {"CompactSize exceeds limit of type: iostream error", {"cmpctblock"}},
    {"differential value overflow: iostream error", {"getblocktxn"}},
    {"index overflowed 16 bits: iostream error", {"getblocktxn"}},
    {"index overflowed 16-bits: iostream error", {"cmpctblock"}},
    {"indexes overflowed 16 bits: iostream error", {"getblocktxn"}},
    {"non-canonical ReadCompactSize(): iostream error", {"addr", "block", "blocktxn", "cmpctblock", "filteradd", "filterload", "getblocks", "getblocktxn", "getdata", "getheaders", "headers", "inv", "notfound", "tx"}},
    {"ReadCompactSize(): size too large: iostream error", {"addr", "block", "blocktxn", "cmpctblock", "filteradd", "filterload", "getblocks", "getblocktxn", "getdata", "getheaders", "headers", "inv", "notfound", "tx"}},
    {"Superfluous witness record: iostream error", {"block", "blocktxn", "cmpctblock", "tx"}},
    {"Unknown transaction optional data: iostream error", {"block", "blocktxn", "cmpctblock", "tx"}},
};

const TestingSetup* g_setup;
} // namespace

void initialize()
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

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string random_message_type{fuzzed_data_provider.ConsumeBytesAsString(CMessageHeader::COMMAND_SIZE).c_str()};
    if (!LIMIT_TO_MESSAGE_TYPE.empty() && random_message_type != LIMIT_TO_MESSAGE_TYPE) {
        return;
    }
    CDataStream random_bytes_data_stream{fuzzed_data_provider.ConsumeRemainingBytes<unsigned char>(), SER_NETWORK, PROTOCOL_VERSION};
    CNode p2p_node{0, ServiceFlags(NODE_NETWORK | NODE_WITNESS | NODE_BLOOM), 0, INVALID_SOCKET, CAddress{CService{in_addr{0x0100007f}, 7777}, NODE_NETWORK}, 0, 0, CAddress{}, std::string{}, false};
    p2p_node.fSuccessfullyConnected = true;
    p2p_node.nVersion = PROTOCOL_VERSION;
    p2p_node.SetSendVersion(PROTOCOL_VERSION);
    g_setup->m_node.peer_logic->InitializeNode(&p2p_node);
    try {
        (void)ProcessMessage(&p2p_node, random_message_type, random_bytes_data_stream, GetTimeMillis(), Params(), *g_setup->m_node.mempool, g_setup->m_node.connman.get(), g_setup->m_node.banman.get(), std::atomic<bool>{false});
    } catch (const std::ios_base::failure& e) {
        const std::string exception_message{e.what()};
        const auto p = EXPECTED_DESERIALIZATION_EXCEPTIONS.find(exception_message);
        if (p == EXPECTED_DESERIALIZATION_EXCEPTIONS.cend() || p->second.count(random_message_type) == 0) {
            std::cout << "Unexpected exception when processing message type \"" << random_message_type << "\": " << exception_message << std::endl;
            assert(false);
        }
    }
    SyncWithValidationInterfaceQueue();
}
