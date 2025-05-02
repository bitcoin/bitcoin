// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <kernel/blocktreestorage.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <pow.h>
#include <primitives/block.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/fs_helpers.h>
#include <util/hasher.h>

#include <boost/test/unit_test.hpp>

using kernel::BLOCK_FILES_FILE_DATA_START_POSITION;
using kernel::BLOCK_FILES_FILE_MAGIC;
using kernel::BLOCK_FILES_FILE_NAME;
using kernel::BLOCK_FILES_FILE_VERSION;
using kernel::BlockTreeStore;
using kernel::BlockTreeStoreError;
using kernel::CBlockFileInfo;
using kernel::HEADER_FILE_DATA_START_POSITION;
using kernel::HEADER_FILE_MAGIC;
using kernel::HEADER_FILE_NAME;
using kernel::HEADER_FILE_VERSION;
using kernel::LOG_FILE_NAME;
using kernel::LOG_FLAG_FILE_NAME;

BOOST_FIXTURE_TEST_SUITE(blocktreestorage_tests, BasicTestingSetup)

CBlockIndex* InsertBlockIndex(node::BlockMap& block_map, const uint256& hash)
{
    if (hash.IsNull()) {
        return nullptr;
    }
    const auto [mi, inserted]{block_map.try_emplace(hash)};
    CBlockIndex* pindex = &(*mi).second;
    if (inserted) {
        pindex->phashBlock = &((*mi).first);
    }
    return pindex;
}

CBlockFileInfo CreateFileInfo(int32_t seed)
{
    CBlockFileInfo info;
    info.nBlocks = seed;
    info.nSize = seed + 1;
    info.nUndoSize = seed + 2;
    info.nHeightFirst = seed + 3;
    info.nHeightLast = seed + 4;
    info.nTimeFirst = seed + 5;
    info.nTimeLast = seed + 6;
    return info;
}

struct ExpectedStoreState {
    std::map<int, CBlockFileInfo> file_infos{};
    node::BlockMap block_map{};
    bool pruned{false};
    bool reindexing{false};
    util::SignalInterrupt& interrupt;
    const CChainParams& params;
};

void CheckBlockFileInfo(uint32_t file, CBlockFileInfo& file_info, BlockTreeStore& store)
{
    CBlockFileInfo retrieved_info;
    BOOST_CHECK(store.ReadBlockFileInfo(file, retrieved_info));
    BOOST_CHECK_EQUAL(file_info.nBlocks, retrieved_info.nBlocks);
    BOOST_CHECK_EQUAL(file_info.nSize, retrieved_info.nSize);
    BOOST_CHECK_EQUAL(file_info.nUndoSize, retrieved_info.nUndoSize);
    BOOST_CHECK_EQUAL(file_info.nHeightFirst, retrieved_info.nHeightFirst);
    BOOST_CHECK_EQUAL(file_info.nHeightLast, retrieved_info.nHeightLast);
    BOOST_CHECK_EQUAL(file_info.nTimeFirst, retrieved_info.nTimeFirst);
    BOOST_CHECK_EQUAL(file_info.nTimeLast, retrieved_info.nTimeLast);

    DataStream a, b;
    a << file_info;
    b << retrieved_info;
    BOOST_CHECK_EQUAL(a.str(), b.str());
}

void CheckBlockMap(const node::BlockMap& store_block_map, const node::BlockMap& expected_block_map)
{
    LOCK(::cs_main);
    BOOST_CHECK_EQUAL(store_block_map.size(), expected_block_map.size());
    for (const auto& [block_hash, store_index] : store_block_map) {
        auto it = expected_block_map.find(block_hash);
        BOOST_REQUIRE(it != expected_block_map.end());
        const auto& expected_index = it->second;

        BOOST_CHECK_EQUAL(expected_index.nHeight, store_index.nHeight);
        BOOST_CHECK_EQUAL(expected_index.nStatus, store_index.nStatus);
        BOOST_CHECK_EQUAL(expected_index.nTx, store_index.nTx);
        BOOST_CHECK_EQUAL(expected_index.nFile, store_index.nFile);
        BOOST_CHECK_EQUAL(expected_index.nDataPos, store_index.nDataPos);
        BOOST_CHECK_EQUAL(expected_index.nUndoPos, store_index.nUndoPos);

        BOOST_CHECK_EQUAL(expected_index.nVersion, store_index.nVersion);
        if (expected_index.pprev == nullptr || store_index.pprev == nullptr) {
            BOOST_CHECK_EQUAL(expected_index.pprev, store_index.pprev);
        } else {
            BOOST_CHECK_EQUAL(Assert(expected_index.pprev)->GetBlockHeader().GetHash().ToString(), Assert(store_index.pprev)->GetBlockHeader().GetHash().ToString());
        }
        BOOST_CHECK_EQUAL(expected_index.hashMerkleRoot.ToString(), store_index.hashMerkleRoot.ToString());
        BOOST_CHECK_EQUAL(expected_index.nTime, store_index.nTime);
        BOOST_CHECK_EQUAL(expected_index.nBits, store_index.nBits);
        BOOST_CHECK_EQUAL(expected_index.nNonce, store_index.nNonce);

        DataStream store, expected;
        store << CDiskBlockIndex{&store_index};
        expected << CDiskBlockIndex{&expected_index};
        BOOST_CHECK_EQUAL(store.str(), expected.str());
    }
}

void CheckStoreContents(BlockTreeStore& store,
                        const ExpectedStoreState& expected_state,
                        const std::string& context)
{
    BOOST_TEST_CONTEXT(context)
    {
        for (auto& [file, info] : expected_state.file_infos) {
            CBlockFileInfo copy{info};
            CheckBlockFileInfo(file, copy, store);
        }
        int32_t last_block;
        store.ReadLastBlockFile(last_block);
        int32_t expected_last = expected_state.file_infos.empty() ? 0 : expected_state.file_infos.size() - 1;
        BOOST_CHECK_EQUAL(last_block, expected_last);

        LOCK(::cs_main);
        node::BlockMap block_map;
        BOOST_CHECK(store.LoadBlockIndexGuts(
            expected_state.params.GetConsensus(),
            [&](const uint256& hash) { return InsertBlockIndex(block_map, hash); },
            expected_state.interrupt));
        CheckBlockMap(block_map, expected_state.block_map);

        bool pruned, reindexing;
        store.ReadPruned(pruned);
        store.ReadReindexing(reindexing);
        BOOST_CHECK_EQUAL(pruned, expected_state.pruned);
        BOOST_CHECK_EQUAL(reindexing, expected_state.reindexing);
    }
}

std::vector<CBlockIndex*> BlockMapToVector(node::BlockMap& test_block_map)
{
    std::vector<CBlockIndex*> blocks;
    blocks.reserve(test_block_map.size());
    for (auto& [hash, index] : test_block_map) {
        blocks.push_back(&index);
    }
    return blocks;
}

std::vector<std::pair<int, const CBlockFileInfo*>> FileInfosToPairs(std::span<CBlockFileInfo> file_infos)
{
    std::vector<std::pair<int, const CBlockFileInfo*>> pairs;
    pairs.reserve(file_infos.size());
    for (size_t i{0}; i < file_infos.size(); ++i) {
        pairs.emplace_back(static_cast<int>(i), &file_infos[i]);
    }
    return pairs;
}

std::vector<std::pair<int, const CBlockFileInfo*>> FileInfosToPairs(const std::map<int, CBlockFileInfo>& file_infos)
{
    std::vector<std::pair<int, const CBlockFileInfo*>> pairs;
    pairs.reserve(file_infos.size());
    for (const auto& [i, info] : file_infos) {
        pairs.emplace_back(i, &info);
    }
    return pairs;
}

BOOST_AUTO_TEST_CASE(HeaderFilesFormat)
{
    fs::path block_tree_store_dir{m_args.GetDataDirBase()};
    auto header_file_path{block_tree_store_dir / HEADER_FILE_NAME};
    auto block_files_file_path{block_tree_store_dir / BLOCK_FILES_FILE_NAME};
    BlockTreeStore store{block_tree_store_dir};

    AutoFile header_file{fsbridge::fopen(header_file_path, "rb")};
    uint32_t magic;
    header_file >> magic;
    BOOST_CHECK_EQUAL(magic, HEADER_FILE_MAGIC);
    uint32_t version;
    header_file >> version;
    BOOST_CHECK_EQUAL(version, HEADER_FILE_VERSION);
    header_file.seek(0, SEEK_END);
    long filesize = header_file.tell();
    BOOST_CHECK_EQUAL(filesize, HEADER_FILE_DATA_START_POSITION);
    (void)header_file.fclose();

    AutoFile block_files_file{fsbridge::fopen(block_files_file_path, "rb")};
    block_files_file >> magic;
    BOOST_CHECK_EQUAL(magic, BLOCK_FILES_FILE_MAGIC);
    block_files_file >> version;
    BOOST_CHECK_EQUAL(version, BLOCK_FILES_FILE_VERSION);
    block_files_file.seek(0, SEEK_END);
    filesize = block_files_file.tell();
    BOOST_CHECK_EQUAL(filesize, BLOCK_FILES_FILE_DATA_START_POSITION);
    (void)block_files_file.fclose();
}

BOOST_AUTO_TEST_CASE(BlockTreeStoreInvalidFiles)
{
    fs::path block_tree_store_dir{m_args.GetDataDirBase()};

    auto header_file_path{block_tree_store_dir / HEADER_FILE_NAME};
    auto block_files_file_path{block_tree_store_dir / BLOCK_FILES_FILE_NAME};

    BlockTreeStore{block_tree_store_dir};
    fs::remove(header_file_path);
    BOOST_CHECK_THROW(BlockTreeStore{block_tree_store_dir}, BlockTreeStoreError);

    // If both files are gone, a new store may be created
    fs::remove(block_files_file_path);
    BlockTreeStore{block_tree_store_dir};
    BOOST_CHECK(fs::exists(header_file_path));
    BOOST_CHECK(fs::exists(block_files_file_path));

    fs::remove(block_files_file_path);
    BOOST_CHECK_THROW(BlockTreeStore{block_tree_store_dir}, BlockTreeStoreError);
    fs::remove(header_file_path);

    auto write_magic_and_version{[](const fs::path& path, uint32_t magic, uint32_t version) {
        AutoFile file{fsbridge::fopen(path, "rb+")};
        file.seek(0, SEEK_SET);
        file << magic;
        file << version;
        (void)file.fclose();
    }};
    BlockTreeStore{block_tree_store_dir};
    write_magic_and_version(header_file_path, 0, 0);
    BOOST_CHECK_THROW(BlockTreeStore{block_tree_store_dir}, BlockTreeStoreError);
    write_magic_and_version(header_file_path, HEADER_FILE_MAGIC, 0);
    BOOST_CHECK_THROW(BlockTreeStore{block_tree_store_dir}, BlockTreeStoreError);
    write_magic_and_version(header_file_path, HEADER_FILE_MAGIC, HEADER_FILE_VERSION);
    BlockTreeStore{block_tree_store_dir};
}

BOOST_AUTO_TEST_CASE(BlockTreeStoreIncompleteWrites)
{
    LOCK(::cs_main);
    fs::path block_tree_store_dir{m_args.GetDataDirBase()};
    auto log_file{block_tree_store_dir / LOG_FILE_NAME};
    auto log_flag_file{block_tree_store_dir / LOG_FLAG_FILE_NAME};
    auto params{CreateChainParams(gArgs, ChainType::REGTEST)};
    auto store{std::make_unique<BlockTreeStore>(block_tree_store_dir)};

    // Write and read a CBlockFileInfo and a CBlockIndex
    ExpectedStoreState expected_state{.interrupt = m_interrupt, .params = *params};
    int32_t seed{0};
    expected_state.file_infos.insert({0, CreateFileInfo(seed)});
    auto file_infos_to_write{FileInfosToPairs(expected_state.file_infos)};
    expected_state.block_map.try_emplace(params->GenesisBlock().GetHash(), params->GenesisBlock());
    CBlockIndex* block_index = &expected_state.block_map[params->GenesisBlock().GetHash()];
    BOOST_CHECK_EQUAL(block_index->header_pos, CBlockIndex::UNSET_HEADER_POS);
    auto block_indexes_to_write{BlockMapToVector(expected_state.block_map)};

    store->SetSimulateIncompleteLogWrite(true);

    // The log file should exist in an unclean state if we abort in the middle of writing to it
    node::BlockMap block_map;
    BOOST_CHECK_THROW(store->WriteBatchSync(file_infos_to_write, block_indexes_to_write), std::runtime_error);
    BOOST_CHECK(fs::exists(log_file));
    BOOST_CHECK(store->LoadBlockIndexGuts(
        params->GetConsensus(),
        [&](const uint256& hash) { return InsertBlockIndex(block_map, hash); },
        m_interrupt));
    BOOST_CHECK(block_map.empty());

    // The constructor should ignore the log file and not apply any pending state
    block_map.clear();
    store = std::make_unique<BlockTreeStore>(block_tree_store_dir);
    BOOST_CHECK(!fs::exists(log_flag_file));
    BOOST_CHECK(store->LoadBlockIndexGuts(
        params->GetConsensus(),
        [&](const uint256& hash) { return InsertBlockIndex(block_map, hash); },
        m_interrupt));
    BOOST_CHECK(block_map.empty());

    // Now simulate a crash in the middle of applying the log.
    block_map.clear();
    store->SetSimulateIncompleteLogApply(true);
    BOOST_CHECK_THROW(store->WriteBatchSync(file_infos_to_write, block_indexes_to_write), std::runtime_error);
    BOOST_CHECK(fs::exists(log_file));
    BOOST_CHECK(fs::exists(log_flag_file));
    BOOST_CHECK(store->LoadBlockIndexGuts(
        params->GetConsensus(),
        [&](const uint256& hash) { return InsertBlockIndex(block_map, hash); },
        m_interrupt));
    BOOST_CHECK(block_map.empty());

    // The constructor should now apply the log file and remove the flag
    block_map.clear();
    BOOST_CHECK(fs::exists(log_flag_file));
    store = std::make_unique<BlockTreeStore>(block_tree_store_dir);
    BOOST_CHECK(!fs::exists(log_flag_file));
    CheckStoreContents(*store, expected_state, "constructor applies log file");
    BOOST_CHECK_EQUAL(block_index->header_pos, HEADER_FILE_DATA_START_POSITION);

    // Simulate a write application failure and subsequent write application
    store->SetSimulateIncompleteLogApply(true);
    ++seed;
    expected_state.file_infos.insert({1, CreateFileInfo(seed)});
    file_infos_to_write = FileInfosToPairs(expected_state.file_infos);
    BOOST_CHECK_THROW(store->WriteBatchSync(file_infos_to_write, block_indexes_to_write), std::runtime_error);
    BOOST_CHECK(fs::exists(log_flag_file));
    store->SetSimulateIncompleteLogApply(false);
    store->WriteBatchSync({}, {});
    BOOST_CHECK(!fs::exists(log_flag_file));
    CheckStoreContents(*store, expected_state, "subsequent write applies log file");
}

BOOST_AUTO_TEST_CASE(BlockTreeStoreFlags)
{
    auto store{std::make_unique<BlockTreeStore>(m_args.GetDataDirBase())};
    bool reindexing = true;
    store->ReadReindexing(reindexing);
    BOOST_CHECK(!reindexing);
    store->WriteReindexing(true);
    store->ReadReindexing(reindexing);
    BOOST_CHECK(reindexing);
    store->WriteReindexing(false);
    store->ReadReindexing(reindexing);
    BOOST_CHECK(!reindexing);

    int last_block;
    store->ReadLastBlockFile(last_block);
    BOOST_CHECK_EQUAL(last_block, 0);

    bool pruned = false;
    store->ReadPruned(pruned);
    BOOST_CHECK(!pruned);
    store->WritePruned(true);
    store->ReadPruned(pruned);
    BOOST_CHECK(pruned);
    store->WritePruned(false);
    store->ReadPruned(pruned);
    BOOST_CHECK(!pruned);

    // Re-create the store and check that the data was persisted
    store->WritePruned(true);
    store->WriteReindexing(true);
    store = std::make_unique<BlockTreeStore>(m_args.GetDataDirBase());
    store->ReadPruned(pruned);
    store->ReadReindexing(reindexing);
    BOOST_CHECK(pruned);
    BOOST_CHECK(reindexing);
}

CBlockIndex* AddTestBlockIndex(node::BlockMap& test_block_map, const CBlockHeader& header, CBlockIndex* prev)
{
    LOCK(::cs_main);
    const auto [mi, inserted]{test_block_map.try_emplace(header.GetHash(), header)};
    CBlockIndex* pindex{&mi->second};
    pindex->phashBlock = &mi->first;
    pindex->pprev = prev;
    pindex->nHeight = prev ? prev->nHeight + 1 : 0;
    pindex->nStatus = prev ? prev->nStatus ^ BLOCK_FAILED_VALID : 0;
    pindex->nTx = prev ? prev->nTx + 3 : 0;
    pindex->nFile = prev ? prev->nFile + 4 : 0;
    pindex->nDataPos = prev ? prev->nDataPos + 100 : 0;
    pindex->nUndoPos = prev ? prev->nUndoPos + 101 : 0;
    return pindex;
}

void WriteAndCheckBlockIndex(BlockTreeStore& store,
                             ExpectedStoreState& expected_state,
                             node::BlockMap& block_map_to_write,
                             std::span<CBlockFileInfo> file_infos_to_write,
                             const std::string& context)
{
    LOCK(::cs_main);
    const auto block_indexes_to_write{BlockMapToVector(block_map_to_write)};
    const auto file_info_pairs_to_write{FileInfosToPairs(file_infos_to_write)};
    store.WriteBatchSync(file_info_pairs_to_write, block_indexes_to_write);
    CheckStoreContents(store, expected_state, context);
}

BOOST_AUTO_TEST_CASE(BlockTreeStoreRW)
{
    LOCK(::cs_main);
    fs::path block_tree_store_dir{m_args.GetDataDirBase()};
    auto header_file{block_tree_store_dir / HEADER_FILE_NAME};
    auto block_files_file{block_tree_store_dir / BLOCK_FILES_FILE_NAME};
    auto params{CreateChainParams(gArgs, ChainType::REGTEST)};
    BlockTreeStore store{block_tree_store_dir};

    node::BlockMap block_map_to_write;
    std::vector<CBlockFileInfo> file_info_to_write;
    ExpectedStoreState expected_state{.interrupt = m_interrupt, .params = *params};

    // Check that the store is empty
    CheckStoreContents(store, expected_state, "check empty store");

    // Write and read a CBlockFileInfo and a CBlockIndex
    int32_t counter = 0;
    file_info_to_write.emplace_back(CreateFileInfo(counter));
    expected_state.file_infos.emplace(counter, CreateFileInfo(counter));
    CBlockIndex* block_index = AddTestBlockIndex(block_map_to_write, params->GenesisBlock(), /*prev=*/nullptr);
    AddTestBlockIndex(expected_state.block_map, params->GenesisBlock(), /*prev*/ nullptr);
    BOOST_CHECK_EQUAL(block_index->header_pos, CBlockIndex::UNSET_HEADER_POS);
    WriteAndCheckBlockIndex(store, expected_state, block_map_to_write, file_info_to_write, "write and read single entries");
    BOOST_CHECK_EQUAL(block_index->header_pos, HEADER_FILE_DATA_START_POSITION);

    // Write another CBlockFileInfo and update the CBlockIndex
    ++counter;
    file_info_to_write.emplace_back(CreateFileInfo(counter));
    expected_state.file_infos.emplace(counter, CreateFileInfo(counter));
    block_index->nStatus = 120;
    expected_state.block_map[block_index->GetBlockHash()].nStatus = 120;
    block_index->nFile = 1;
    expected_state.block_map[block_index->GetBlockHash()].nFile = 1;
    WriteAndCheckBlockIndex(store, expected_state, block_map_to_write, file_info_to_write, "write another file info and update CBlockIndex");

    // Write an empty block map
    node::BlockMap empty_block_map{};
    WriteAndCheckBlockIndex(store, expected_state, empty_block_map, file_info_to_write, "write empty block map");

    // Write an empty file info vector
    std::vector<CBlockFileInfo> empty_file_info;
    WriteAndCheckBlockIndex(store, expected_state, block_map_to_write, empty_file_info, "write empty file info");

    // Update the new CBlockFileInfo and the CBlockIndex and check that the file sizes are unchanged
    block_index->nStatus = 99;
    expected_state.block_map[block_index->GetBlockHash()].nStatus = 99;
    block_index->nFile = 50;
    expected_state.block_map[block_index->GetBlockHash()].nFile = 50;
    file_info_to_write[1] = CreateFileInfo(3);
    expected_state.file_infos[1] = CreateFileInfo(3);
    auto header_file_size{fs::file_size(header_file)};
    auto block_files_file_size{fs::file_size(block_files_file)};
    WriteAndCheckBlockIndex(store, expected_state, block_map_to_write, file_info_to_write, "update existing entries");
    BOOST_CHECK_EQUAL(header_file_size, fs::file_size(header_file));
    BOOST_CHECK_EQUAL(block_files_file_size, fs::file_size(block_files_file));

    // Add more CBlockIndex entries to the store
    BOOST_CHECK_EQUAL(expected_state.block_map.size(), 1);
    for (uint8_t i = 0; i < 10; ++i) {
        CBlockHeader header;
        header.hashPrevBlock = block_index->GetBlockHash();
        header.nBits = params->GenesisBlock().nBits;
        header.nTime = block_index->nTime + 1;
        header.hashMerkleRoot = uint256{i};
        while (!CheckProofOfWork(header.GetHash(), header.nBits, params->GetConsensus())) {
            ++header.nNonce;
        }
        block_index = AddTestBlockIndex(expected_state.block_map, header, /*prev=*/block_index);
        // Add a couple of forks too
        if (i % 3 == 0) {
            block_index = block_index->pprev;
        }
    }
    BOOST_CHECK_EQUAL(expected_state.block_map.size(), 11);
    WriteAndCheckBlockIndex(store, expected_state, expected_state.block_map, file_info_to_write, "add a tree of block indexes");

    // Read and write back the same data and check that the file sizes are unchanged
    header_file_size = fs::file_size(header_file);
    node::BlockMap loaded_block_map;
    BOOST_CHECK(store.LoadBlockIndexGuts(
        params->GetConsensus(),
        [&](const uint256& hash) { return InsertBlockIndex(loaded_block_map, hash); },
        m_interrupt));
    WriteAndCheckBlockIndex(store, expected_state, loaded_block_map, file_info_to_write, "read and write the same data leaves no changes");
    BOOST_CHECK_EQUAL(header_file_size, fs::file_size(header_file));

    // Writing an invalid CBlockIndex (with invalid PoW) should fail to load
    block_map_to_write.begin()->second.nBits = 0;
    loaded_block_map.clear();
    store.WriteBatchSync(FileInfosToPairs(file_info_to_write), BlockMapToVector(block_map_to_write));
    BOOST_CHECK(!store.LoadBlockIndexGuts(
        params->GetConsensus(),
        [&](const uint256& hash) { return InsertBlockIndex(loaded_block_map, hash); },
        m_interrupt));
}

BOOST_AUTO_TEST_SUITE_END()
