// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <net.h>
#include <netmessagemaker.h>
#include <random.h>
#include <streams.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>

static CService ip(uint32_t i)
{
    struct in_addr s;
    s.s_addr = i;
    return CService(CNetAddr(s), Params().GetDefaultPort());
}

static void ProcessMessageBlock(benchmark::Bench& bench)
{

    FastRandomContext frc{/*fDeterministic=*/true};
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();

    ConnmanTestMsg& connman = static_cast<ConnmanTestMsg&>(*test_setup->m_node.connman);
    PeerManager& peerman = *test_setup->m_node.peerman;

    // Mock an outbound peer
    CAddress node_addr(ip(0xb33fc4f3), NODE_NONE);
    CNode peernode{ /*id=*/0,
                 /*sock=*/nullptr,
                 node_addr,
                 /*nKeyedNetGroupIn=*/0,
                 /*nLocalHostNonceIn=*/0,
                 CAddress(),
                 /*addrNameIn=*/"",
                 ConnectionType::OUTBOUND_FULL_RELAY,
                 /*inbound_onion=*/false};

    // Must have a successful version handshake before we can process
    // other messages.
    {
        LOCK(NetEventsInterface::g_msgproc_mutex);
        connman.Handshake(
            /*node=*/peernode,
            /*successfully_connected=*/true,
            /*remote_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
            /*local_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
            /*version=*/PROTOCOL_VERSION,
            /*relay_txs=*/true);
        peerman.SendMessages(&peernode); // should result in getheaders
        connman.FlushSendBuffer(peernode);
    }

    // We only need this to get the size of the serialized block
    DataStream block_stream{benchmark::data::block413567};
    CBlock block{};
    block_stream >> TX_WITH_WITNESS(block);

    bench.batch(benchmark::data::block413567.size()).unit("byte").run([&] {
        LOCK(NetEventsInterface::g_msgproc_mutex);
        CSerializedNetMsg msg_block{
            NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(block))
        };
        connman.ReceiveMsgFrom(peernode, std::move(msg_block));
        peernode.fPauseSend = false;
        connman.ProcessMessagesOnce(peernode);
    });
}

BENCHMARK(ProcessMessageBlock, benchmark::PriorityLevel::HIGH);
