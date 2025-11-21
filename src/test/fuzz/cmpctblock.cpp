// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>
#include <net.h>
#include <net_processing.h>
#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/mining.h>
#include <test/util/net.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <util/fs.h>
#include <validationinterface.h>

#include <memory>
#include <vector>

using namespace util::hex_literals;

namespace {

//! Fee each created tx will pay.
const CAmount AMOUNT_FEE{1000};
//! Cached coinbases that each iteration can copy and use.
std::vector<COutPoint> g_mature_coinbase;
//! Constant value used to create valid headers.
uint32_t g_nBits;
//! Cached path to the datadir created during init.
fs::path g_cached_path;
//! One for each block the fuzzer generates.
struct BlockInfo {
    std::shared_ptr<CBlock> block;
    uint256 hash;
    uint32_t height;
};
//! Class to delete the statically-named datadir at the end of a fuzzing run.
class FuzzedDirectoryWrapper
{
private:
    fs::path staticdir;

public:
    FuzzedDirectoryWrapper(fs::path name) : staticdir(name) {}

    ~FuzzedDirectoryWrapper()
    {
        fs::remove_all(staticdir);
    }
};

} // namespace

void initialize_cmpctblock()
{
    std::vector<unsigned char> random_path_suffix(10);
    GetStrongRandBytes(random_path_suffix);
    std::string testdatadir = "-testdatadir=";
    std::string staticdir = "cmpctblock_cached" + HexStr(random_path_suffix);
    g_cached_path = fs::temp_directory_path() / staticdir;
    auto cached_datadir_arg = testdatadir + fs::PathToString(g_cached_path);

    const auto initial_setup = MakeNoLogFileContext<const TestingSetup>(
        /*chain_type=*/ChainType::REGTEST,
        {.extra_args = {cached_datadir_arg.c_str()},
         .coins_db_in_memory = false,
         .block_tree_db_in_memory = false});

    static const FuzzedDirectoryWrapper wrapper(g_cached_path);

    SetMockTime(Params().GenesisBlock().Time());

    node::BlockAssembler::Options options;
    options.coinbase_output_script = P2WSH_OP_TRUE;

    for (int i = 0; i < 2 * COINBASE_MATURITY; ++i) {
        COutPoint prevout{MineBlock(initial_setup->m_node, options)};
        if (i < COINBASE_MATURITY) {
            g_mature_coinbase.push_back(prevout);
        }
    }

    initial_setup->m_node.chainman->ActiveChainstate().ForceFlushStateToDisk();
    g_nBits = Params().GenesisBlock().nBits;
}

FUZZ_TARGET(cmpctblock, .init=initialize_cmpctblock)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    std::string fuzzcopydatadir = "-fuzzcopydatadir=";
    auto copy_datadir_arg = fuzzcopydatadir + fs::PathToString(g_cached_path);
    const auto mock_start_time{1610000000};

    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>(
        /*chain_type=*/ChainType::REGTEST,
        {.extra_args = {copy_datadir_arg.c_str(),
                        strprintf("-mocktime=%d", mock_start_time).c_str()},
         .coins_db_in_memory = false,
         .block_tree_db_in_memory = false});

    auto setup = testing_setup.get();

    setup->m_node.validation_signals->RegisterValidationInterface(setup->m_node.peerman.get());
    setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
    auto& chainman = static_cast<TestChainstateManager&>(*setup->m_node.chainman);
    chainman.ResetIbd();

    LOCK(NetEventsInterface::g_msgproc_mutex);

    std::vector<CNode*> peers;
    auto& connman = *static_cast<ConnmanTestMsg*>(setup->m_node.connman.get());
    for (int i = 0; i < 4; ++i) {
        peers.push_back(ConsumeNodeAsUniquePtr(fuzzed_data_provider, i).release());
        CNode& p2p_node = *peers.back();
        FillNode(fuzzed_data_provider, connman, p2p_node);
        connman.AddTestNode(p2p_node);
    }

    // Stores blocks generated this iteration.
    std::vector<BlockInfo> info;

    // Coinbase UTXOs for this iteration.
    std::vector<COutPoint> mature_coinbase = g_mature_coinbase;

    const CCoinsViewMemPool amount_view{WITH_LOCK(::cs_main, return &setup->m_node.chainman->ActiveChainstate().CoinsTip()), *setup->m_node.mempool};

    auto create_tx = [&]() -> CTransactionRef {
        CMutableTransaction tx_mut;
        tx_mut.version = CTransaction::CURRENT_VERSION;
        tx_mut.nLockTime = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<uint32_t>();

        // If the mempool is non-empty, choose a mempool outpoint. Otherwise, choose a coinbase.
        COutPoint outpoint;
        unsigned long mempool_size = setup->m_node.mempool->size();
        if (fuzzed_data_provider.ConsumeBool() && mempool_size != 0) {
            size_t random_idx = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, mempool_size - 1);
            LOCK(setup->m_node.mempool->cs);
            outpoint = COutPoint(setup->m_node.mempool->txns_randomized[random_idx].second->GetSharedTx()->GetHash(), 0);
        } else if (mature_coinbase.size() != 0) {
            auto pop = mature_coinbase.begin();
            std::advance(pop, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, mature_coinbase.size() - 1));
            outpoint = *pop;
            mature_coinbase.erase(pop);
        } else {
            // We have no utxos available to make a transaction.
            return nullptr;
        }

        const auto sequence = ConsumeSequence(fuzzed_data_provider);
        const auto script_sig = CScript{};
        const auto script_wit_stack = std::vector<std::vector<uint8_t>>{WITNESS_STACK_ELEM_OP_TRUE};

        CTxIn in;
        in.prevout = outpoint;
        in.nSequence = sequence;
        in.scriptSig = script_sig;
        in.scriptWitness.stack = script_wit_stack;
        tx_mut.vin.push_back(in);

        const CAmount amount_in = Assert(amount_view.GetCoin(outpoint))->out.nValue;
        const CAmount amount_out = amount_in - AMOUNT_FEE;
        tx_mut.vout.emplace_back(amount_out, P2WSH_OP_TRUE);

        auto tx = MakeTransactionRef(tx_mut);
        return tx;
    };

    auto create_block = [&]() {
        uint256 prev;
        uint32_t height;

        if (fuzzed_data_provider.ConsumeBool() || info.size() == 0) {
            LOCK(cs_main);
            prev = setup->m_node.chainman->ActiveChain().Tip()->GetBlockHash();
            height = setup->m_node.chainman->ActiveChain().Height() + 1;
        } else {
            size_t index = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, info.size() - 1);
            prev = info[index].hash;
            height = info[index].height + 1;
        }

        const auto new_time = WITH_LOCK(::cs_main, return setup->m_node.chainman->ActiveChain().Tip()->GetMedianTimePast() + 1);

        CBlockHeader header;
        header.nNonce = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        header.hashPrevBlock = prev;
        header.nBits = g_nBits;
        header.nTime = new_time;
        header.nVersion = fuzzed_data_provider.ConsumeIntegral<int32_t>();

        std::shared_ptr<CBlock> block = std::make_shared<CBlock>();
        *block = header;

        CMutableTransaction coinbase_tx;
        coinbase_tx.vin.resize(1);
        coinbase_tx.vin[0].prevout.SetNull();
        coinbase_tx.vin[0].scriptSig = CScript() << height << OP_0;
        coinbase_tx.vout.resize(1);
        coinbase_tx.vout[0].scriptPubKey = CScript() << OP_TRUE;
        coinbase_tx.vout[0].nValue = COIN;
        block->vtx.push_back(MakeTransactionRef(coinbase_tx));

        const auto mempool_size = setup->m_node.mempool->size();
        if (fuzzed_data_provider.ConsumeBool() && mempool_size != 0) {
            // Add txns from the mempool. Since we do not include parents, it may be an invalid block.
            size_t num_txns = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, mempool_size);
            size_t random_idx = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, mempool_size - 1);

            LOCK(setup->m_node.mempool->cs);
            for (size_t i = random_idx; i < random_idx + num_txns; ++i) {
                CTransactionRef mempool_tx = setup->m_node.mempool->txns_randomized[i % mempool_size].second->GetSharedTx();
                block->vtx.push_back(mempool_tx);
            }
        }

        // Create and add (possibly invalid) txns that are not in the mempool.
        if (fuzzed_data_provider.ConsumeBool()) {
            size_t new_txns = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 10);
            for (size_t i = 0; i < new_txns; ++i) {
                CTransactionRef non_mempool_tx = create_tx();
                if (non_mempool_tx != nullptr) {
                    block->vtx.push_back(non_mempool_tx);
                }
            }
        }

        CBlockIndex* pindexPrev{WITH_LOCK(::cs_main, return setup->m_node.chainman->m_blockman.LookupBlockIndex(prev))};
        setup->m_node.chainman->GenerateCoinbaseCommitment(*block, pindexPrev);

        bool mutated;
        block->hashMerkleRoot = BlockMerkleRoot(*block, &mutated);
        FinalizeHeader(*block, *setup->m_node.chainman);

        BlockInfo block_info;
        block_info.block = block;
        block_info.hash = block->GetHash();
        block_info.height = height;

        return block_info;
    };

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 1000)
    {
        CSerializedNetMsg net_msg;
        bool set_net_msg = true;

        CallOneOf(
            fuzzed_data_provider,
            [&]() {
                // Send a headers message for an existing block (if one exists).
                size_t num_blocks = info.size();
                if (num_blocks == 0) {
                    set_net_msg = false;
                    return;
                }

                // Choose an existing block and send a HEADERS message for it.
                size_t index = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, num_blocks - 1);
                std::vector<CBlock> headers;
                headers.emplace_back(info[index].block->GetBlockHeader());

                net_msg = NetMsg::Make(NetMsgType::HEADERS, TX_WITH_WITNESS(headers));
            },
            [&]() {
                // Send a sendcmpct message, optionally setting hb mode.
                bool hb = fuzzed_data_provider.ConsumeBool();
                net_msg = NetMsg::Make(NetMsgType::SENDCMPCT, /*high_bandwidth=*/hb, /*version=*/CMPCTBLOCKS_VERSION);
            },
            [&]() {
                // Mine a block, but don't send it.
                BlockInfo block_info = create_block();
                info.push_back(block_info);
                set_net_msg = false;
            },
            [&]() {
                // Send a transaction.
                CTransactionRef tx = create_tx();
                if (tx == nullptr) {
                    set_net_msg = false;
                    return;
                }

                net_msg = NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*tx));
            },
            [&]() {
                // Set mock time randomly or to tip's time.
                if (fuzzed_data_provider.ConsumeBool()) {
                    SetMockTime(ConsumeTime(fuzzed_data_provider));
                } else {
                    const uint64_t tip_time = WITH_LOCK(::cs_main, return setup->m_node.chainman->ActiveChain().Tip()->GetBlockTime());
                    SetMockTime(tip_time);
                }

                set_net_msg = false;
            });

        if (!set_net_msg) {
            continue;
        }

        CNode& random_node = *PickValue(fuzzed_data_provider, peers);
        connman.FlushSendBuffer(random_node);
        (void)connman.ReceiveMsgFrom(random_node, std::move(net_msg));

        bool more_work{true};
        while (more_work) {
            random_node.fPauseSend = false;

            more_work = connman.ProcessMessagesOnce(random_node);
            setup->m_node.peerman->SendMessages(&random_node);
        }
    }

    setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
    setup->m_node.connman->StopNodes();
}
