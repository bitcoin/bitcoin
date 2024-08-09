// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <config/bitcoin-config.h>
#include <consensus/merkle.h>
#include <kernel/notifications_interface.h>
#include <kernel/warning.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <node/chainstate.h>
#include <node/miner.h>
#include <pow.h>
#include <random.h>
#include <scheduler.h>
#include <undo.h>
#include <validation.h>
#include <validationinterface.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <util/task_runner.h>
#include <util/thread.h>

#include <ranges>

namespace {

class KernelNotifications : public kernel::Notifications
{
public:
    kernel::InterruptResult blockTip(SynchronizationState, CBlockIndex&) override { return {}; }
    void headerTip(SynchronizationState, int64_t height, int64_t timestamp, bool presync) override {}
    void progress(const bilingual_str& title, int progress_percent, bool resume_possible) override {}
    void warningSet(kernel::Warning id, const bilingual_str& message) override {}
    void warningUnset(kernel::Warning id) override {}
    void flushError(const bilingual_str& message) override
    {
        assert(false);
    }
    void fatalError(const bilingual_str& message) override
    {
        assert(false);
    }
};

/**
 * Manage the state for a run of the fuzz target.
 *
 * This stores our utxos and keep them in sync with the state of the chain. This also allows to register some block
 * hashes to assert the validity of a given block.
 */
class StateManager : public CValidationInterface {
    //! Utxos created in a coinbase transaction which hasn't matured yet. Those are keyed by the height at
    //! which they can be spent in the next block.
    std::unordered_map<int, std::pair<COutPoint, CTxOut>> m_immature_utxos;

    //! Utxos which can be spent in the next block.
    std::unordered_map<COutPoint, CTxOut, SaltedOutpointHasher> m_spendable_utxos;

    //! Utxos which were spent. Kept around to add them back in case of reorg.
    std::unordered_map<COutPoint, CTxOut, SaltedOutpointHasher> m_spent_utxos;

    //! Block hashes of blocks we assume are valid. Unbounded but we assume we won't add millions such entries.
    std::unordered_set<uint256, SaltedTxidHasher> m_assert_valid_blocks;

public:
    /** Create a transaction spending a random amount of utxos from our wallet, if possible. */
    CTransactionRef CreateTransaction(FuzzedDataProvider& prov)
    {
        if (m_spendable_utxos.empty()) return {};
        CMutableTransaction tx;

        // Pick the utxos to use as input of our transaction. Drop them from our set of available
        // utxos to avoid reusing them in another transaction before a block gets connected.
        // FIXME: can we be smarter than erasing a hundred times? For instance keeping track of reserved utxos?
        const auto input_count{prov.ConsumeIntegralInRange(1, std::min((int)m_spendable_utxos.size(), 100))};
        tx.vin.resize(input_count);
        CAmount in_value{0};
        auto it{m_spendable_utxos.begin()};
        for (int i{0}; i < input_count; ++i) {
            auto [outpoint, coin] = *it++;
            in_value += coin.nValue;
            tx.vin[i].prevout = outpoint;
            tx.vin[i].scriptWitness.stack = std::vector<std::vector<uint8_t>>{WITNESS_STACK_ELEM_OP_TRUE};
            m_spendable_utxos.erase(outpoint);
            m_spent_utxos.emplace(outpoint, std::move(coin));
        }

        const auto out_count{prov.ConsumeIntegralInRange(1, 100)};
        tx.vout.resize(out_count);
        for (int i{0}; i < out_count; ++i) {
            tx.vout[i].scriptPubKey = P2WSH_OP_TRUE;
            tx.vout[i].nValue = in_value / out_count;
        }

        return MakeTransactionRef(std::move(tx));
    }

    /** Record a block hash to assert the validity of the block once it'll have been checked. */
    void AssertBlockValidity(uint256 block_hash)
    {
        m_assert_valid_blocks.insert(std::move(block_hash));
    }

protected:
    /** We subscribe to the block checked notification to let the fuzz target assert the validity of some blocks. */
    void BlockChecked(const CBlock& block, const BlockValidationState& state) override
    {
        if (!state.IsValid()) {
            // FIXME: since we never delete records is it possible that we re-create the same block but treat it as
            // invalid this time (min_pow_checked = false for instance) and it would fail this assert?
            assert(!m_assert_valid_blocks.contains(block.GetHash()));
        }
    }

    /** We subscribe to the block connected notifications to update our "wallet view", ie the coins we can spend at the
     * next block. */
    void BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock> &block, const CBlockIndex *pindex) override
    {
        // First of all record the new coinbase output.
        COutPoint coinbase_op{block->vtx[0]->GetHash(), 0};
        CTxOut coinbase_coin{block->vtx[0]->vout[0]};
        m_immature_utxos.emplace(pindex->nHeight + COINBASE_MATURITY - 1, std::make_pair(coinbase_op, coinbase_coin));

        // Then record outputs spent and created by all other transactions in the block. In the fuzz target
        // blocks only ever contain transactions which spend and create our own outputs.
        for (const auto& tx: block->vtx | std::views::drop(1)) {
            for (size_t i{0}; i < tx->vout.size(); ++i) {
                COutPoint op{tx->GetHash(), (uint32_t)i};
                m_spendable_utxos.emplace(std::move(op), tx->vout[i]);
            }

            for (const auto& vin: tx->vin) {
                m_spendable_utxos.erase(vin.prevout);
            }
        }

        // Finally, record any newly matured utxo as spendable in the next block. Note we don't move
        // and erase the entry on purpose here, as we might need it again in case of reorg.
        const auto& matured_utxo{m_immature_utxos.find(pindex->nHeight)};
        if (matured_utxo != m_immature_utxos.end()) {
            m_spendable_utxos.emplace(matured_utxo->second.first, matured_utxo->second.second);
        }
    }

    /** We subscribe to the block disconnected notifications to update our "wallet view", the coins we can spend at the
     * next block. */
    void BlockDisconnected(const std::shared_ptr<const CBlock> &block, const CBlockIndex* pindex) override
    {
        // First of all drop the coinbase output for this block.
        m_immature_utxos.erase(pindex->nHeight + COINBASE_MATURITY - 1);

        // Then if we previously marked a coinbase output as newly matured for a coin at this height,
        // mark it as immature again.
        const auto& re_immatured_utxo{m_immature_utxos.find(pindex->nHeight)};
        if (re_immatured_utxo != m_immature_utxos.end()) {
            m_spendable_utxos.erase(re_immatured_utxo->second.first);
        }

        // Finally, drop any coin created in this block and re-insert formerly spent coins.
        for (const auto& tx: block->vtx) {
            for (size_t i{0}; i < tx->vout.size(); ++i) {
                COutPoint op{tx->GetHash(), (uint32_t)i};
                m_spendable_utxos.erase(op);
            }

            if (tx->IsCoinBase()) continue;
            for (const auto& vin: tx->vin) {
                auto spent_utxo{m_spent_utxos.find(vin.prevout)};
                if (spent_utxo != m_spent_utxos.end()) {
                    m_spendable_utxos.emplace(spent_utxo->first, spent_utxo->second);
                }
            }
        }
    }
};

//! See net_processing.
static const int MAX_HEADERS_RESULTS{2000};

//! To generate a random tmp datadir per process (necessary to fuzz with multiple cores).
static FastRandomContext g_insecure_rand_ctx_temp_path;

struct TestData {
    fs::path m_tmp_dir;
    fs::path m_datadir;
    fs::path m_init_datadir;
    const CChainParams m_chain_params{*CChainParams::RegTest({})};
    KernelNotifications m_notifs;
    util::SignalInterrupt m_interrupt;

    void Init() {
        SeedRandomForTest(SeedRand::SEED);
        const auto rand_str{g_insecure_rand_ctx_temp_path.rand256().ToString()};
        m_tmp_dir = fs::temp_directory_path() / "fuzz_chainstate_" PACKAGE_NAME / rand_str;
        fs::remove_all(m_tmp_dir);
        m_datadir = m_tmp_dir / "datadir";
        m_init_datadir = m_tmp_dir / "init_datadir";

        LogInstance().DisableLogging();
    }

    ~TestData() {
        fs::remove_all(m_tmp_dir);
    }
} g_test_data;

/** Consume a random block hash and height to be used as previous block. */
std::pair<uint256, int> RandomPrevBlock(FuzzedDataProvider& prov)
{
    auto hash{ConsumeDeserializable<uint256>(prov).value_or(uint256{})};
    // FIXME: it takes an int but it needs to be positive because there is a conversion to uint inside blockstorage.cpp:
    // node/blockstorage.cpp:968:45: runtime error: implicit conversion from type 'int' of value -2147483648 (32-bit, signed) to type 'unsigned int'
    const auto height{prov.ConsumeIntegralInRange<int>(0, std::numeric_limits<int>::max() - 1)};
    return {std::move(hash), height};
}

/** Sometimes get any random block from the index. Otherwise generate a random one. */
std::pair<uint256, int> RandomPrevBlock(FuzzedDataProvider& prov, node::BlockManager& blockman) NO_THREAD_SAFETY_ANALYSIS
{
    if (prov.ConsumeBool()) {
        const auto prev_block{&PickValue(prov, blockman.m_block_index).second};
        return {prev_block->GetBlockHash(), prev_block->nHeight};
    }
    return RandomPrevBlock(prov);
}

/** Create a random block. */
std::pair<CBlockHeader, int> CreateBlockHeader(FuzzedDataProvider& prov, std::pair<uint256, int> prev_block, bool set_merkle = false)
{
    CBlockHeader header;
    header.nVersion = prov.ConsumeIntegral<int32_t>();
    header.nTime = prov.ConsumeIntegral<uint32_t>();
    header.nBits = prov.ConsumeIntegral<uint32_t>();
    header.nNonce = prov.ConsumeIntegral<uint32_t>();
    if (set_merkle) {
        if (auto h = ConsumeDeserializable<uint256>(prov)) {
            header.hashMerkleRoot = *h;
        }
    }
    header.hashPrevBlock = std::move(prev_block.first);
    return std::make_pair(std::move(header), prev_block.second);
}

/** Create a coinbase transaction paying to an anyonecanspend for the given height. */
CTransactionRef CreateCoinbase(int height)
{
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vout.resize(1);
    tx.vout[0].scriptPubKey = P2WSH_OP_TRUE;
    tx.vout[0].nValue = (50 * COIN) >> 10; // Tolerate up to 10 halvings.
    tx.vin[0].scriptSig = CScript() << height << OP_0;
    return MakeTransactionRef(std::move(tx));
}

/** Create a random block and include random (and most likely invalid) transactions. */
std::pair<CBlock, int> CreateBlock(FuzzedDataProvider& prov, std::pair<uint256, int> prev_block)
{
    CBlock block;
    auto [block_header, height]{CreateBlockHeader(prov, std::move(prev_block))};
    *(static_cast<CBlockHeader*>(&block)) = std::move(block_header);

    block.vtx.push_back(CreateCoinbase(height));
    while (prov.ConsumeBool()) {
        if (auto tx = ConsumeDeserializable<CMutableTransaction>(prov, TX_WITH_WITNESS)) {
            block.vtx.push_back(MakeTransactionRef(std::move(*tx)));
        }
    }
    block.hashMerkleRoot = BlockMerkleRoot(block);

    return std::make_pair(std::move(block), height);
}

/** Create a consensus-valid random block.
 * If a non-empty list of transactions is passed include them. Otherwise if a StateManager is passed, try to fill the
 * block with a random number of transactions. */
CBlock CreateValidBlock(FuzzedDataProvider& prov, StateManager* state_manager, const ChainstateManager& chainman,
                        CBlockIndex* prev_block, std::vector<CTransactionRef> txs = {})
{
    assert(prev_block);
    CBlock block;
    block.nVersion = prov.ConsumeIntegralInRange(4, std::numeric_limits<int>::max());
    block.nNonce = prov.ConsumeIntegral<uint32_t>();
    node::UpdateTime(&block, chainman.GetConsensus(), prev_block);
    block.nBits = GetNextWorkRequired(prev_block, &block, chainman.GetConsensus());
    block.hashPrevBlock = prev_block->GetBlockHash();

    // Always create the coinbase. Then if a list of transactions was passed, use that. Otherwise
    // try to create a bunch of new transactions.
    block.vtx.push_back(CreateCoinbase(prev_block->nHeight + 1));
    if (!txs.empty()) {
        block.vtx.reserve(txs.size());
        block.vtx.insert(block.vtx.end(), std::make_move_iterator(txs.begin()), std::make_move_iterator(txs.end()));
        txs.erase(txs.begin(), txs.end());
    } else if (state_manager) {
        while (prov.ConsumeBool()) {
            if (auto tx{state_manager->CreateTransaction(prov)}) {
                block.vtx.push_back(std::move(tx));
                if (GetBlockWeight(block) > MAX_BLOCK_WEIGHT) {
                    block.vtx.pop_back();
                    break;
                }
            }
        }
    }
    chainman.GenerateCoinbaseCommitment(block, prev_block);
    block.hashMerkleRoot = BlockMerkleRoot(block);

    return block;
}

/** Make it possible to sanity check roundtrips to disk. */
bool operator==(const CBlock& a, const CBlock& b)
{
    return a.nVersion == b.nVersion
        && a.nTime == b.nTime
        && a.nBits == b.nBits
        && a.nNonce == b.nNonce
        && a.hashPrevBlock == b.hashPrevBlock
        && a.hashMerkleRoot == b.hashMerkleRoot;
}

} // namespace

void init_blockstorage()
{
    g_test_data.Init();

    // Mock the pow check to always pass since it is checked when loading blocks and we don't
    // want to be mining within the target.
    g_check_pow_mock = [](uint256 hash, unsigned int, const Consensus::Params&) {
        return true;
    };
}

FUZZ_TARGET(blockstorage, .init = init_blockstorage)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    // Start from a fresh datadir on every iteration.
    fs::remove_all(g_test_data.m_datadir / "blocks");
    fs::create_directories(g_test_data.m_datadir / "blocks");

    // Create the BlockManager and its index. The target needs to be ran under a RAM disk to store
    // the blk* files in memory. The index uses an in-memory LevelDb.
    uint64_t prune_target{0};
    if (fuzzed_data_provider.ConsumeBool()) {
        prune_target = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
    }
    node::BlockManager::Options blockman_opts{
        .chainparams = g_test_data.m_chain_params,
        .prune_target = prune_target,
        .blocks_dir = g_test_data.m_datadir / "blocks",
        .notifications = g_test_data.m_notifs,
    };
    auto blockman{node::BlockManager{g_test_data.m_interrupt, std::move(blockman_opts)}};
    {
        LOCK(cs_main);
        blockman.m_block_tree_db = std::make_unique<kernel::BlockTreeDB>(DBParams{
            .path = "", // Memory-only.
            .cache_bytes = nMaxBlockDBCache << 20,
            .memory_only = true,
        });
    }

    // Needed by AddToBlockIndex, reuse it to test both nullptr and not.
    CBlockIndex* dummy_best{nullptr};
    BlockValidationState dummy_valstate;

    // Load the genesis block.
    {
        LOCK(cs_main);
        assert(blockman.m_block_index.count(g_test_data.m_chain_params.GetConsensus().hashGenesisBlock) == 0);
        const CBlock& block = g_test_data.m_chain_params.GenesisBlock();
        FlatFilePos blockPos{blockman.SaveBlockToDisk(block, 0)};
        assert(!blockPos.IsNull());
        assert(blockman.AddToBlockIndex(block, dummy_best));
        assert(!blockman.m_block_index.empty());
    }

    // This is used to store blocks which were created when accepting their header, to potentially
    // later be stored to disk entirely.
    std::vector<std::pair<CBlock, int>> blocks_in_flight;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10'000) {
        CallOneOf(fuzzed_data_provider,
            // Add a header to the block index. Sometimes save the header of a full block which could be saved to disk
            // later (see below). Otherwise save a random header for which we'll never store a block.
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                LOCK(cs_main);
                auto header{[&]() NO_THREAD_SAFETY_ANALYSIS {
                    LOCK(cs_main);
                    auto prev_block{RandomPrevBlock(fuzzed_data_provider, blockman)};
                    if (fuzzed_data_provider.ConsumeBool()) {
                        auto [block, height]{CreateBlock(fuzzed_data_provider, std::move(prev_block))};
                        auto header{*(static_cast<CBlockHeader*>(&block))};
                        blocks_in_flight.emplace_back(std::move(block), height);
                        return header;
                    } else {
                        return CreateBlockHeader(fuzzed_data_provider, std::move(prev_block), /*set_merkle=*/true).first;
                    }
                }()};
                assert(blockman.AddToBlockIndex(header, dummy_best));
                assert(blockman.LookupBlockIndex(header.GetHash()));
            },
            // Roundtrip the block index database. It should always succeed, since we mock the pow check.
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                LOCK(cs_main);
                assert(blockman.WriteBlockIndexDB());
                assert(blockman.LoadBlockIndexDB({}));
                // TODO: somehow compare m_block_tree_db before and after?
            },
            //// Write some random undo data for a random block from the index.
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                // Always at least one block is present but the genesis doesn't have a pprev.
                auto& block = PickValue(fuzzed_data_provider, blockman.m_block_index).second;
                if (block.pprev) {
                    if (auto undo_data = ConsumeDeserializable<CBlockUndo>(fuzzed_data_provider)) {
                        if (WITH_LOCK(::cs_main, return blockman.WriteUndoDataForBlock(*undo_data, dummy_valstate, block))) {
                            CBlockUndo undo_read;
                            assert(blockman.UndoReadFromDisk(undo_read, block));
                            // TODO: assert they're equal?
                        }
                    }
                }
            },
            // Create a new block and roundtrip it to disk. Sometimes pick a block for which we stored its header
            // already (if there is any), otherwise create a whole new block.
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                auto [block, height]{[&] {
                    LOCK(cs_main);
                    if (!blocks_in_flight.empty() && fuzzed_data_provider.ConsumeBool()) {
                        auto ret{std::move(blocks_in_flight.back())};
                        blocks_in_flight.pop_back();
                        return ret;
                    } else {
                        auto prev_block{RandomPrevBlock(fuzzed_data_provider, blockman)};
                        return CreateBlock(fuzzed_data_provider, std::move(prev_block));
                    }
                }()};
                const auto pos{blockman.SaveBlockToDisk(block, height)};
                blockman.GetBlockPosFilename(pos);
                CBlock read_block;
                blockman.ReadBlockFromDisk(read_block, pos);
                assert(block == read_block);
            },
            // Kitchen sink.
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                LOCK(cs_main);

                CCheckpointData dummy_data;
                blockman.GetLastCheckpoint(dummy_data);

                // Coverage for CheckBlockDataAvailability. It requires the lower and upper blocks to be correctly
                // ordered. There is always at least one block in the index, the genesis.
                const auto sz{blockman.m_block_index.size()};
                auto lower_it{blockman.m_block_index.begin()};
                std::advance(lower_it, fuzzed_data_provider.ConsumeIntegralInRange<decltype(sz)>(0, sz - 1));
                auto upper_it{lower_it};
                while (fuzzed_data_provider.ConsumeBool()) {
                    auto it = std::next(upper_it);
                    if (it == blockman.m_block_index.end()) break;
                    upper_it = it;
                }
                const auto& lower_block{lower_it->second};
                const auto& upper_block{upper_it->second};
                blockman.CheckBlockDataAvailability(upper_block, lower_block);

                // Get coverage for IsBlockPruned.
                blockman.IsBlockPruned(upper_block);
            }
        );
    };

    // At no point do we set an AssumeUtxo snapshot.
    assert(!blockman.m_snapshot_height);
}

void init_chainstate()
{
    // Make the pow check always pass to be able to mine a chain from inside the target.
    // TODO: we could have two mocks, once which passes, the other which fails. This way we can
    // also fuzz the codepath for invalid pow.
    g_check_pow_mock = [](uint256 hash, unsigned int, const Consensus::Params&) {
        return true;
    };

    // This sets up the init and working datadirs paths in a tmp folder.
    g_test_data.Init();

    // Create the chainstate for the initial datadir. On every round we'll restart from this chainstate instead of
    // re-creating one from scratch.
    fs::create_directories(g_test_data.m_init_datadir / "blocks");
    node::BlockManager::Options blockman_opts{
        .chainparams = g_test_data.m_chain_params,
        .blocks_dir = g_test_data.m_init_datadir / "blocks",
        .notifications = g_test_data.m_notifs,
    };
    ValidationSignals main_signals{std::make_unique<util::ImmediateTaskRunner>()};
    const ChainstateManager::Options chainman_opts{
        .chainparams = g_test_data.m_chain_params,
        .datadir = g_test_data.m_init_datadir,
        .check_block_index = false,
        .checkpoints_enabled = false,
        .minimum_chain_work = UintToArith256(uint256{}),
        .assumed_valid_block = uint256{},
        .notifications = g_test_data.m_notifs,
        .signals = &main_signals,
    };
    ChainstateManager chainman{g_test_data.m_interrupt, chainman_opts, blockman_opts};
    node::CacheSizes cache_sizes;
    cache_sizes.block_tree_db = 1;
    cache_sizes.coins_db = 2;
    cache_sizes.coins = 3;
    node::ChainstateLoadOptions load_opts {
        .require_full_verification = false,
        .coins_error_cb = nullptr,
    };
    auto [status, _] = node::LoadChainstate(chainman, cache_sizes, load_opts);
    assert(status == node::ChainstateLoadStatus::SUCCESS);

    // Connect the initial chain to get 10 spendable UTxOs at the start of every fuzzing round.
    const auto initial_blockchain{CreateBlockChain(110, g_test_data.m_chain_params)};
    BlockValidationState valstate;
    auto& chainstate{chainman.ActiveChainstate()};
    assert(chainstate.ActivateBestChain(valstate, nullptr));
    for (const auto& block : initial_blockchain) {
        bool new_block{false};
        assert(chainman.ProcessNewBlock(block, true, true, &new_block));
        assert(new_block);
    }

    LOCK(chainman.GetMutex());
    if (chainstate.CanFlushToDisk()) {
        chainstate.ForceFlushStateToDisk();
    }
}

FUZZ_TARGET(chainstate, .init = init_chainstate)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    // On every round start from a freshly copied initial datadir.
    fs::remove_all(g_test_data.m_datadir);
    fs::copy(g_test_data.m_init_datadir, g_test_data.m_datadir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);

    // We use the signals to assert blocks validity and keep up to date the utxos we can use to create valid
    // transactions.
    ValidationSignals main_signals{std::make_unique<util::ImmediateTaskRunner>()};

    // Create the chainstate..
    uint64_t prune_target{0};
    if (fuzzed_data_provider.ConsumeBool()) {
        prune_target = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
    }
    node::BlockManager::Options blockman_opts{
        .chainparams = g_test_data.m_chain_params,
        .prune_target = prune_target,
        .blocks_dir = g_test_data.m_datadir / "blocks",
        .notifications = g_test_data.m_notifs,
    };
    const ChainstateManager::Options chainman_opts{
        .chainparams = g_test_data.m_chain_params,
        .datadir = g_test_data.m_datadir,
        // TODO: make it possible to call CheckBlockIndex() without having set it here, and call it in CallOneOf().
        .check_block_index = true,
        .checkpoints_enabled = false,
        .minimum_chain_work = UintToArith256(uint256{}),
        .assumed_valid_block = uint256{},
        .notifications = g_test_data.m_notifs,
        .signals = &main_signals,
    };
    ChainstateManager chainman{g_test_data.m_interrupt, chainman_opts, blockman_opts};

    // ..And then load it.
    node::CacheSizes cache_sizes;
    cache_sizes.block_tree_db = 2 << 20;
    cache_sizes.coins_db = 2 << 22;
    cache_sizes.coins = (450 << 20) - (2 << 20) - (2 << 22);
    node::ChainstateLoadOptions load_opts {
        .prune = prune_target > 0,
        .require_full_verification = false,
        .coins_error_cb = nullptr,
    };
    auto [status, _] = node::LoadChainstate(chainman, cache_sizes, load_opts);
    assert(status == node::ChainstateLoadStatus::SUCCESS);

    // This stores our state for this run: our utxos and blocks to assert are valid.
    StateManager state_manager;
    main_signals.RegisterValidationInterface(&state_manager);

    std::vector<CBlock> blocks_in_flight;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10'000) {
        CallOneOf(fuzzed_data_provider,
            // Process a list of headers. Most of the time make it process the header of a valid block
            // cached for future processing.
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                std::vector<CBlockHeader> headers;

                // In some cases generate a random list of headers to be processed. Otherwise, create a single valid
                // block.
                // TODO: make it possible to generate a chain of more than one valid block.
                const bool is_random{fuzzed_data_provider.ConsumeBool()};
                const int headers_count{is_random ? fuzzed_data_provider.ConsumeIntegralInRange(1, MAX_HEADERS_RESULTS) : 1};
                headers.reserve(headers_count);

                // Need to determine whether we'll set the PoW as checked before asserting block validity.
                const bool min_pow_checked{fuzzed_data_provider.ConsumeBool()};

                if (is_random) {
                    for (int i = 0; i < headers_count; ++i) {
                        headers.push_back(CreateBlockHeader(fuzzed_data_provider, RandomPrevBlock(fuzzed_data_provider), /*set_merkle=*/true).first);
                    }
                } else {
                    // Sometimes branch off a random header, sometimes extend the tip of either the validated chain or
                    // the best header chain.
                    const bool extend_tip{fuzzed_data_provider.ConsumeBool()};
                    const bool extend_active_tip{extend_tip && fuzzed_data_provider.ConsumeBool()};
                    CBlockIndex* prev_block{[&]() NO_THREAD_SAFETY_ANALYSIS {
                        LOCK(chainman.GetMutex());
                        if (extend_tip) {
                            return extend_active_tip ? chainman.ActiveTip() : chainman.m_best_header;
                        }
                        return &PickValue(fuzzed_data_provider, chainman.m_blockman.m_block_index).second;
                    }()};
                    auto sm{extend_tip ? &state_manager : nullptr};
                    blocks_in_flight.push_back(CreateValidBlock(fuzzed_data_provider, sm, chainman, prev_block));
                    if (extend_active_tip && min_pow_checked) state_manager.AssertBlockValidity(blocks_in_flight.back().GetHash());
                    headers.emplace_back(blocks_in_flight.back());
                }

                BlockValidationState valstate;
                const bool res{chainman.ProcessNewBlockHeaders(headers, min_pow_checked, valstate)};
                assert(res || is_random || !min_pow_checked);
            },
            // Process a block. Most of the time make it proces one of the blocks in flight.
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                const bool process_in_flight{!blocks_in_flight.empty() && fuzzed_data_provider.ConsumeBool()};
                auto block{[&] {
                    if (process_in_flight) {
                        // Sometimes process a block of which we processed the header already. Note the block
                        // isn't necessarily valid.
                        auto block{std::move(blocks_in_flight.back())};
                        blocks_in_flight.pop_back();
                        return block;
                    } else {
                        // In the rest, sometimes create a new valid block building on top of either our validated chain
                        // tip or the header chain tip.
                        if (fuzzed_data_provider.ConsumeBool()) {
                            const auto prev_block{WITH_LOCK(chainman.GetMutex(), return fuzzed_data_provider.ConsumeBool() ? chainman.ActiveTip() : chainman.m_best_header)};
                            return CreateValidBlock(fuzzed_data_provider, &state_manager, chainman, prev_block);
                        } else {
                            // For invalid blocks create sometimes an otherwise valid block which branches from any header,
                            // and sometimes a completely random block.
                            if (fuzzed_data_provider.ConsumeBool()) {
                                std::unordered_map<COutPoint, CTxOut, SaltedOutpointHasher> empty_utxos;
                                const auto prev_block{WITH_LOCK(chainman.GetMutex(), return &PickValue(fuzzed_data_provider, chainman.m_blockman.m_block_index).second)};
                                return CreateValidBlock(fuzzed_data_provider, {}, chainman, prev_block);
                            } else {
                                LOCK(chainman.GetMutex());
                                return CreateBlock(fuzzed_data_provider, RandomPrevBlock(fuzzed_data_provider)).first;
                            }
                        }
                    }
                }()};
                const bool force_processing{fuzzed_data_provider.ConsumeBool()};
                const bool min_pow_checked{fuzzed_data_provider.ConsumeBool()};
                chainman.ProcessNewBlock(std::make_shared<CBlock>(std::move(block)), force_processing, min_pow_checked, /*new_block=*/nullptr);
            },
            // Create a reorg of any size.
            [&]() NO_THREAD_SAFETY_ANALYSIS {
                const auto cur_height{WITH_LOCK(chainman.GetMutex(), return chainman.ActiveHeight())};
                if (cur_height <= 0) return;
                std::unordered_map<COutPoint, CTxOut, SaltedOutpointHasher> empty_utxos;

                // Pick the depth of the reorg, and sometimes record the unconfirmed transactions to re-confirm them.
                auto reorg_height{fuzzed_data_provider.ConsumeIntegralInRange(1, cur_height)};
                auto reorg_depth{cur_height - reorg_height + 1};
                std::vector<CBlock> disconnected_blocks;
                if (fuzzed_data_provider.ConsumeBool()) {
                    // Bound the reorg depth if we are going to re-confirm transactions from disconnected blocks, to not
                    // have to deal with transactions spending disconnected coinbases.
                    reorg_height = std::max(reorg_height, cur_height - COINBASE_MATURITY + 1);
                    disconnected_blocks.resize(reorg_depth);
                }

                // Get a pointer to the first block in common between the current and the new chain, optionally
                // recording the disconnected transactions as we go.
                auto ancestor{WITH_LOCK(chainman.GetMutex(), return chainman.ActiveTip())};
                while (ancestor->nHeight >= reorg_height) {
                    if (!disconnected_blocks.empty()) {
                        const auto idx{ancestor->nHeight - reorg_height};
                        assert(chainman.m_blockman.ReadBlockFromDisk(disconnected_blocks[idx], *ancestor));
                    }
                    ancestor = ancestor->pprev;
                }

                // Create a chain as long, don't connect it yet.
                {
                LOCK(chainman.GetMutex());
                for (int i{0}; i < reorg_depth; ++i) {
                    std::vector<CTransactionRef> txs;
                    if (!disconnected_blocks.empty() && disconnected_blocks[i].vtx.size() > 1) {
                        txs = std::vector<CTransactionRef>{std::make_move_iterator(disconnected_blocks[i].vtx.begin() + 1), std::make_move_iterator(disconnected_blocks[i].vtx.end())};
                        disconnected_blocks[i] = CBlock{};
                    }
                    auto block{CreateValidBlock(fuzzed_data_provider, {}, chainman, ancestor, std::move(txs))};
                    state_manager.AssertBlockValidity(block.GetHash());
                    BlockValidationState valstate;
                    assert(chainman.AcceptBlock(std::make_shared<CBlock>(std::move(block)), valstate, &ancestor, true, nullptr, nullptr, true));
                }
                }

                // Make sure the new chain gets connected (a single additional block might not suffice).
                do {
                    auto block{CreateValidBlock(fuzzed_data_provider, {}, chainman, ancestor)};
                    state_manager.AssertBlockValidity(block.GetHash());
                    BlockValidationState valstate;
                    auto res{WITH_LOCK(chainman.GetMutex(), return chainman.AcceptBlock(std::make_shared<CBlock>(std::move(block)), valstate, &ancestor, true, nullptr, nullptr, true))};
                    assert(res);
                    BlockValidationState valstate2;
                    assert(chainman.ActiveChainstate().ActivateBestChain(valstate2));
                } while (chainman.ActiveTip() != ancestor);
            }
        );
    };

    // TODO: exercise the reindex logic?
    // TODO: sometimes run with an assumed chainstate too? We could be to generate a snapshot during init and
    // sometimes ActivateSnapshot() at the beginning.
}
