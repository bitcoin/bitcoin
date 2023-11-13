// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <config/bitcoin-config.h>
#include <consensus/merkle.h>
#include <kernel/warning.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <node/chainstate.h>
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
#include <util/fs_helpers.h>
#include <util/thread.h>

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

//! To generate a random tmp datadir per process (necessary to fuzz with multiple cores).
static FastRandomContext g_insecure_rand_ctx_temp_path;

struct TestData {
    fs::path m_tmp_dir;
    fs::path m_datadir;
    const CChainParams m_chain_params{*CChainParams::Main()};
    KernelNotifications m_notifs;
    util::SignalInterrupt m_interrupt;
    std::unique_ptr<const BasicTestingSetup> m_test_setup;

    void Init() {
        SeedRandomForTest(SeedRand::SEED);
        const auto rand_str{g_insecure_rand_ctx_temp_path.rand256().ToString()};
        m_tmp_dir = fs::temp_directory_path() / "fuzz_chainstate_" PACKAGE_NAME / rand_str;
        fs::remove_all(m_tmp_dir);
        m_datadir = m_tmp_dir / "datadir";
        fs::create_directories(m_datadir / "blocks");

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

/** In 90% of the cases, get any random block from the index. Otherwise generate a random one. */
std::pair<uint256, int> RandomPrevBlock(FuzzedDataProvider& prov, node::BlockManager& blockman) NO_THREAD_SAFETY_ANALYSIS
{
    if (prov.ConsumeIntegralInRange<int>(0, 9) > 0) {
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
    tx.vout[0].nValue = 50 * COIN; // We assume we don't mine so many blocks at once..
    tx.vin[0].scriptSig = CScript() << (height + 1) << OP_0;
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
