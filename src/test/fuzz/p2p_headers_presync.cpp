// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <blockencodings.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <node/peerman_args.h>
#include <pow.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <uint256.h>
#include <validation.h>

namespace {
constexpr uint32_t FUZZ_MAX_HEADERS_RESULTS{16};

class HeadersSyncSetup : public TestingSetup
{
    std::vector<CNode*> m_connections;

public:
    HeadersSyncSetup(const ChainType chain_type, TestOpts opts) : TestingSetup(chain_type, opts)
    {
        PeerManager::Options peerman_opts;
        node::ApplyArgsManOptions(*m_node.args, peerman_opts);
        peerman_opts.max_headers_result = FUZZ_MAX_HEADERS_RESULTS;
        // The peerman's rng is a global that is reused, so it will be reused
        // and may cause non-determinism between runs. This may even influence
        // the global RNG, because seeding may be done from the global one. For
        // now, avoid it influencing the global RNG, and initialize it with a
        // constant instead.
        peerman_opts.deterministic_rng = true;
        // No txs are relayed. Disable irrelevant and possibly
        // non-deterministic code paths.
        peerman_opts.ignore_incoming_txs = true;
        m_node.peerman = PeerManager::make(*m_node.connman, *m_node.addrman,
                                           m_node.banman.get(), *m_node.chainman,
                                           *m_node.mempool, *m_node.warnings, peerman_opts);

        CConnman::Options options;
        options.m_msgproc = m_node.peerman.get();
        m_node.connman->Init(options);
    }

    void ResetAndInitialize() EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex);
    void SendMessage(FuzzedDataProvider& fuzzed_data_provider, CSerializedNetMsg&& msg)
        EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex);
};

void HeadersSyncSetup::ResetAndInitialize()
{
    m_connections.clear();
    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
    connman.StopNodes();

    static NodeId id{0};
    std::vector<ConnectionType> conn_types = {
        ConnectionType::OUTBOUND_FULL_RELAY,
        ConnectionType::BLOCK_RELAY,
        ConnectionType::INBOUND
    };

    for (auto conn_type : conn_types) {
        CAddress addr{};
        m_connections.push_back(new CNode(id++, nullptr, addr, 0, 0, addr, "", conn_type, false));
        CNode& p2p_node = *m_connections.back();

        connman.Handshake(
            /*node=*/p2p_node,
            /*successfully_connected=*/true,
            /*remote_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
            /*local_services=*/ServiceFlags(NODE_NETWORK | NODE_WITNESS),
            /*version=*/PROTOCOL_VERSION,
            /*relay_txs=*/true);

        connman.AddTestNode(p2p_node);
    }
}

void HeadersSyncSetup::SendMessage(FuzzedDataProvider& fuzzed_data_provider, CSerializedNetMsg&& msg)
{
    auto& connman = static_cast<ConnmanTestMsg&>(*m_node.connman);
    CNode& connection = *PickValue(fuzzed_data_provider, m_connections);

    connman.FlushSendBuffer(connection);
    (void)connman.ReceiveMsgFrom(connection, std::move(msg));
    connection.fPauseSend = false;
    try {
        connman.ProcessMessagesOnce(connection);
    } catch (const std::ios_base::failure&) {
    }
    m_node.peerman->SendMessages(&connection);
}

CBlockHeader ConsumeHeader(FuzzedDataProvider& fuzzed_data_provider, const uint256& prev_hash, uint32_t prev_nbits)
{
    CBlockHeader header;
    header.nNonce = 0;
    // Either use the previous difficulty or let the fuzzer choose. The upper target in the
    // range comes from the bits value of the genesis block, which is 0x1d00ffff. The lower
    // target comes from the bits value of mainnet block 840000, which is 0x17034219.
    // Calling lower_target.SetCompact(0x17034219) and upper_target.SetCompact(0x1d00ffff)
    // should return the values below.
    //
    // RPC commands to verify:
    // getblockheader 000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f
    // getblockheader 0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5
    if (fuzzed_data_provider.ConsumeBool()) {
        header.nBits = prev_nbits;
    } else {
        arith_uint256 lower_target = UintToArith256(uint256{"0000000000000000000342190000000000000000000000000000000000000000"});
        arith_uint256 upper_target = UintToArith256(uint256{"00000000ffff0000000000000000000000000000000000000000000000000000"});
        arith_uint256 target = ConsumeArithUInt256InRange(fuzzed_data_provider, lower_target, upper_target);
        header.nBits = target.GetCompact();
    }
    header.nTime = ConsumeTime(fuzzed_data_provider);
    header.hashPrevBlock = prev_hash;
    header.nVersion = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    return header;
}

CBlock ConsumeBlock(FuzzedDataProvider& fuzzed_data_provider, const uint256& prev_hash, uint32_t prev_nbits)
{
    auto header = ConsumeHeader(fuzzed_data_provider, prev_hash, prev_nbits);
    // In order to reach the headers acceptance logic, the block is
    // constructed in a way that will pass the mutation checks.
    CBlock block{header};
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vout[0].nValue = 0;
    tx.vin[0].scriptSig.resize(2);
    block.vtx.push_back(MakeTransactionRef(tx));
    block.hashMerkleRoot = block.vtx[0]->GetHash().ToUint256();
    return block;
}

void FinalizeHeader(CBlockHeader& header, const ChainstateManager& chainman)
{
    while (!CheckProofOfWork(header.GetHash(), header.nBits, chainman.GetParams().GetConsensus())) {
        ++(header.nNonce);
    }
}

// Global setup works for this test as state modification (specifically in the
// block index) would indicate a bug.
HeadersSyncSetup* g_testing_setup;

void initialize()
{
    static auto setup{
        MakeNoLogFileContext<HeadersSyncSetup>(ChainType::MAIN,
                                               {
                                                   .setup_validation_interface = false,
                                               }),
    };
    g_testing_setup = setup.get();
}
} // namespace

FUZZ_TARGET(p2p_headers_presync, .init = initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    // The steady clock is currently only used for logging, so a constant
    // time-point seems acceptable for now.
    ElapseSteady elapse_steady{};

    ChainstateManager& chainman = *g_testing_setup->m_node.chainman;
    CBlockHeader base{chainman.GetParams().GenesisBlock()};
    SetMockTime(base.nTime);

    LOCK(NetEventsInterface::g_msgproc_mutex);

    g_testing_setup->ResetAndInitialize();

    // The chain is just a single block, so this is equal to 1
    size_t original_index_size{WITH_LOCK(cs_main, return chainman.m_blockman.m_block_index.size())};
    arith_uint256 total_work{WITH_LOCK(cs_main, return chainman.m_best_header->nChainWork)};

    std::vector<CBlockHeader> all_headers;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
    {
        auto finalized_block = [&]() {
            CBlock block = ConsumeBlock(fuzzed_data_provider, base.GetHash(), base.nBits);
            FinalizeHeader(block, chainman);
            return block;
        };

        // Send low-work headers, compact blocks, and blocks
        CallOneOf(
            fuzzed_data_provider,
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                // Send FUZZ_MAX_HEADERS_RESULTS headers
                std::vector<CBlock> headers;
                headers.resize(FUZZ_MAX_HEADERS_RESULTS);
                for (CBlock& header : headers) {
                    header = ConsumeHeader(fuzzed_data_provider, base.GetHash(), base.nBits);
                    FinalizeHeader(header, chainman);
                    base = header;
                }

                all_headers.insert(all_headers.end(), headers.begin(), headers.end());

                auto headers_msg = NetMsg::Make(NetMsgType::HEADERS, TX_WITH_WITNESS(headers));
                g_testing_setup->SendMessage(fuzzed_data_provider, std::move(headers_msg));
            },
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                // Send a compact block
                auto block = finalized_block();
                CBlockHeaderAndShortTxIDs cmpct_block{block, fuzzed_data_provider.ConsumeIntegral<uint64_t>()};

                all_headers.push_back(block);

                auto headers_msg = NetMsg::Make(NetMsgType::CMPCTBLOCK, TX_WITH_WITNESS(cmpct_block));
                g_testing_setup->SendMessage(fuzzed_data_provider, std::move(headers_msg));
            },
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                // Send a block
                auto block = finalized_block();

                all_headers.push_back(block);

                auto headers_msg = NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(block));
                g_testing_setup->SendMessage(fuzzed_data_provider, std::move(headers_msg));
            });
    }

    // This is a conservative overestimate, as base is only moved forward when sending headers. In theory,
    // the longest chain generated by this test is 1600 (FUZZ_MAX_HEADERS_RESULTS * 100) headers. In that case,
    // this variable will accurately reflect the chain's total work.
    total_work += CalculateClaimedHeadersWork(all_headers);

    // This test should never create a chain with more work than MinimumChainWork.
    assert(total_work < chainman.MinimumChainWork());

    // The headers/blocks sent in this test should never be stored, as the chains don't have the work required
    // to meet the anti-DoS work threshold. So, if at any point the block index grew in size, then there's a bug
    // in the headers pre-sync logic.
    assert(WITH_LOCK(cs_main, return chainman.m_blockman.m_block_index.size()) == original_index_size);
}
