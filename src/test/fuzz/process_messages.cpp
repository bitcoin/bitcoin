// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/consensus.h>
#include <net.h>
#include <net_processing.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/memory.h>
#include <validation.h>
#include <validationinterface.h>

const TestingSetup* g_setup;

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

    ConnmanTestMsg& connman = *(ConnmanTestMsg*)g_setup->m_node.connman.get();
    std::vector<CNode*> peers;

    const auto num_peers_to_add = fuzzed_data_provider.ConsumeIntegralInRange(1, 3);
    for (int i = 0; i < num_peers_to_add; ++i) {
        const ServiceFlags service_flags = ServiceFlags(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
        const ConnectionType conn_type = fuzzed_data_provider.PickValueInArray({ConnectionType::INBOUND, ConnectionType::OUTBOUND, ConnectionType::MANUAL, ConnectionType::FEELER, ConnectionType::BLOCK_RELAY, ConnectionType::ADDR_FETCH});
        peers.push_back(MakeUnique<CNode>(i, service_flags, 0, INVALID_SOCKET, CAddress{CService{in_addr{0x0100007f}, 7777}, NODE_NETWORK}, 0, 0, CAddress{}, std::string{}, conn_type).release());
        CNode& p2p_node = *peers.back();

        p2p_node.fSuccessfullyConnected = true;
        p2p_node.fPauseSend = false;
        p2p_node.nVersion = PROTOCOL_VERSION;
        p2p_node.SetSendVersion(PROTOCOL_VERSION);
        g_setup->m_node.peer_logic->InitializeNode(&p2p_node);

        connman.AddTestNode(p2p_node);
    }

    while (fuzzed_data_provider.ConsumeBool()) {
        const std::string random_message_type{fuzzed_data_provider.ConsumeBytesAsString(CMessageHeader::COMMAND_SIZE).c_str()};

        CSerializedNetMsg net_msg;
        net_msg.m_type = random_message_type;
        net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider);

        CNode& random_node = *peers.at(fuzzed_data_provider.ConsumeIntegralInRange<int>(0, peers.size() - 1));

        (void)connman.ReceiveMsgFrom(random_node, net_msg);
        random_node.fPauseSend = false;

        try {
            connman.ProcessMessagesOnce(random_node);
        } catch (const std::ios_base::failure&) {
        }
    }
    SyncWithValidationInterfaceQueue();
    LOCK2(::cs_main, g_cs_orphans); // See init.cpp for rationale for implicit locking order requirement
    g_setup->m_node.connman->StopNodes();
}
