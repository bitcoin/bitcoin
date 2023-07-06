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
#include <validation.h>

namespace {
constexpr uint32_t FUZZ_MAX_HEADERS_RESULTS{16};

class HeadersSyncSetup : public TestingSetup
{
    std::vector<CNode*> m_connections;

public:
    HeadersSyncSetup(const ChainType chain_type = ChainType::MAIN,
                     const std::vector<const char*>& extra_args = {})
        : TestingSetup(chain_type, extra_args)
    {
        PeerManager::Options peerman_opts;
        node::ApplyArgsManOptions(*m_node.args, peerman_opts);
        peerman_opts.max_headers_result = FUZZ_MAX_HEADERS_RESULTS;
        m_node.peerman = PeerManager::make(*m_node.connman, *m_node.addrman,
                                           m_node.banman.get(), *m_node.chainman,
                                           *m_node.mempool, peerman_opts);

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

    NodeId id{0};
    std::vector<ConnectionType> conn_types = {};
    conn_types.resize(1, ConnectionType::OUTBOUND_FULL_RELAY);
    conn_types.resize(conn_types.size() + 1, ConnectionType::BLOCK_RELAY);
    conn_types.resize(conn_types.size() + 1, ConnectionType::INBOUND);

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
    // Either use the previous difficulty or let the fuzzer choose
    header.nBits = fuzzed_data_provider.ConsumeBool() ?
                       prev_nbits :
                       fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0x17058EBE, 0x1D00FFFF);
    header.nTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    header.hashPrevBlock = prev_hash;
    return header;
}

CBlock ConsumeBlock(FuzzedDataProvider& fuzzed_data_provider, const uint256& prev_hash, uint32_t prev_nbits)
{
    auto header = ConsumeHeader(fuzzed_data_provider, prev_hash, prev_nbits);
    // Doesn't need to be a valid block but it needs to have one transaction
    // for compact block construction.
    CBlock block{header};
    block.vtx.push_back(MakeTransactionRef(CMutableTransaction{}));
    return block;
}

void FinalizeHeader(CBlockHeader& header)
{
    while (!CheckProofOfWork(header.GetHash(), header.nBits, Params().GetConsensus())) {
        ++(header.nNonce);
    }
}

// Global setup works for this test as state modification (specifically in the
// block index) would indicate a bug.
HeadersSyncSetup* g_testing_setup;

void initialize()
{
    // Reduce proof-of-work check to one bit.
    g_check_pow_mock = [](uint256 hash, unsigned int, const Consensus::Params&) {
        return (hash.data()[31] & 0x80) == 0;
    };

    static auto setup = MakeNoLogFileContext<HeadersSyncSetup>(ChainType::MAIN, {"-checkpoints=0"});
    g_testing_setup = setup.get();
}
} // namespace

FUZZ_TARGET(p2p_headers_sync, .init = initialize)
{
    ChainstateManager& chainman = *g_testing_setup->m_node.chainman;

    LOCK(NetEventsInterface::g_msgproc_mutex);

    g_testing_setup->ResetAndInitialize();

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    CBlockHeader base{Params().GenesisBlock()};
    SetMockTime(Params().GenesisBlock().nTime);

    size_t original_index_size{WITH_LOCK(cs_main, return chainman.m_blockman.m_block_index.size())};

    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes(), 100)
    {
        auto finalized_block = [&]() {
            auto prev = WITH_LOCK(cs_main,
                                  return PickValue(fuzzed_data_provider, chainman.m_blockman.m_block_index))
                            .second.GetBlockHeader();
            CBlock block = ConsumeBlock(fuzzed_data_provider, prev.GetHash(), prev.nBits);
            FinalizeHeader(block);
            return block;
        };

        CallOneOf(
            fuzzed_data_provider,
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                // Send FUZZ_MAX_HEADERS_RESULTS headers
                std::vector<CBlock> headers;
                headers.resize(FUZZ_MAX_HEADERS_RESULTS);
                for (CBlock& header : headers) {
                    header = ConsumeHeader(fuzzed_data_provider, base.GetHash(), base.nBits);
                    FinalizeHeader(header);
                    base = header;
                }

                auto headers_msg = NetMsg::Make(NetMsgType::HEADERS, TX_WITH_WITNESS(headers));
                g_testing_setup->SendMessage(fuzzed_data_provider, std::move(headers_msg));
            },
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                // Send a compact block
                auto block = finalized_block();
                CBlockHeaderAndShortTxIDs cmpct_block{block};

                auto headers_msg = NetMsg::Make(NetMsgType::CMPCTBLOCK, TX_WITH_WITNESS(cmpct_block));
                g_testing_setup->SendMessage(fuzzed_data_provider, std::move(headers_msg));
            },
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                // Send a block
                auto block = finalized_block();

                auto headers_msg = NetMsg::Make(NetMsgType::BLOCK, TX_WITH_WITNESS(block));
                g_testing_setup->SendMessage(fuzzed_data_provider, std::move(headers_msg));
            });
    }

    // If the block index grew in size, then we found a bug in the headers pre-sync logic.
    assert(WITH_LOCK(cs_main, return chainman.m_blockman.m_block_index.size()) == original_index_size);

    SyncWithValidationInterfaceQueue();
}
