// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <kernel/headerstorage.h>
#include <logging.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/hasher.h>

#include <boost/test/unit_test.hpp>

using kernel::BLOCK_FILES_FILE_MAGIC;
using kernel::BLOCK_FILES_FILE_NAME;
using kernel::BLOCK_FILES_FILE_VERSION;
using kernel::BLOCK_FILES_PRUNE_FLAG_POS;
using kernel::BlockTreeStore;
using kernel::BlockTreeStoreError;
using kernel::HEADER_FILE_DATA_START_POS;
using kernel::HEADER_FILE_MAGIC;
using kernel::HEADER_FILE_NAME;
using kernel::HEADER_FILE_VERSION;

BOOST_FIXTURE_TEST_SUITE(headerstorage_tests, TestChain100Setup)

CBlockIndex* InsertBlockIndex(std::unordered_map<uint256, CBlockIndex, BlockHasher>& block_map, const uint256& hash)
{
    if (hash.IsNull()) {
        return nullptr;
    }
    LogInfo("Inserting a new element");
    const auto [mi, inserted]{block_map.try_emplace(hash)};
    CBlockIndex* pindex = &(*mi).second;
    if (inserted) {
        pindex->phashBlock = &((*mi).first);
    }
    return pindex;
}

void check_block_file_info(uint32_t file, CBlockFileInfo& file_info, BlockTreeStore& store)
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
}

void check_block_map(const std::unordered_map<uint256, CBlockIndex, BlockHasher>& block_map, const std::vector<CBlockIndex*>& blockinfo)
{
    LOCK(::cs_main);
    BOOST_CHECK_EQUAL(block_map.size(), blockinfo.size());
    for (const auto& block : blockinfo) {
        auto hash{block->GetBlockHeader().GetHash()};
        auto it = block_map.find(hash);
        BOOST_CHECK(it != block_map.end());
        const auto& index = it->second;
        BOOST_CHECK_EQUAL(index.nHeight, block->nHeight);
        BOOST_CHECK_EQUAL(index.nTime, block->nTime);
        BOOST_CHECK_EQUAL(index.nBits, block->nBits);
        BOOST_CHECK_EQUAL(index.nStatus, block->nStatus);
        BOOST_CHECK_EQUAL(index.nFile, block->nFile);
    }
}

BOOST_AUTO_TEST_CASE(HeaderFilesFormat)
{
    fs::path block_tree_store_dir{m_args.GetDataDirBase()};
    auto header_file_path{block_tree_store_dir / HEADER_FILE_NAME};
    auto block_files_file_path{block_tree_store_dir / BLOCK_FILES_FILE_NAME};
    auto params{CreateChainParams(gArgs, ChainType::REGTEST)};
    BlockTreeStore store{block_tree_store_dir, *params};

    auto header_file = AutoFile{fsbridge::fopen(header_file_path, "rb")};
    uint32_t magic;
    header_file >> magic;
    BOOST_CHECK_EQUAL(magic, HEADER_FILE_MAGIC);
    uint32_t version;
    header_file >> version;
    BOOST_CHECK_EQUAL(version, HEADER_FILE_VERSION);
    bool reindexing;
    header_file >> reindexing;
    BOOST_CHECK_EQUAL(reindexing, false);
    int64_t data_end;
    header_file >> data_end;
    BOOST_CHECK_EQUAL(data_end, HEADER_FILE_DATA_START_POS);
    header_file.seek(0, SEEK_END);
    long filesize = header_file.tell();
    BOOST_CHECK_GE(filesize, params->AssumedHeaderStoreSize());

    auto file = AutoFile{fsbridge::fopen(block_files_file_path, "rb")};
    file >> magic;
    BOOST_CHECK_EQUAL(magic, BLOCK_FILES_FILE_MAGIC);
    file >> version;
    BOOST_CHECK_EQUAL(version, BLOCK_FILES_FILE_VERSION);
    int32_t last_block;
    file >> last_block;
    BOOST_CHECK_EQUAL(last_block, 0);
    bool pruned;
    file >> pruned;
    BOOST_CHECK_EQUAL(pruned, false);
    file.seek(0, SEEK_END);
    filesize = file.tell();
    BOOST_CHECK_GE(filesize, BLOCK_FILES_PRUNE_FLAG_POS + 1);
}

BOOST_AUTO_TEST_CASE(HeaderStoreInvalidFiles)
{
    fs::path block_tree_store_dir{m_args.GetDataDirBase()};
    auto params{CreateChainParams(gArgs, ChainType::REGTEST)};
    ;
    auto header_file{block_tree_store_dir / HEADER_FILE_NAME};
    auto block_files_file{block_tree_store_dir / BLOCK_FILES_FILE_NAME};

    BlockTreeStore{block_tree_store_dir, *params};
    fs::remove(header_file);
    BOOST_CHECK_THROW(BlockTreeStore(block_tree_store_dir, *params), BlockTreeStoreError);
    // If both files are gone, a new store may be created
    fs::remove(block_files_file);
    BlockTreeStore(block_tree_store_dir, *params);
    BOOST_CHECK(fs::exists(header_file));
    BOOST_CHECK(fs::exists(block_files_file));
    fs::remove(block_files_file);
    BOOST_CHECK_THROW(BlockTreeStore(block_tree_store_dir, *params), BlockTreeStoreError);
}

BOOST_AUTO_TEST_CASE(HeaderStore)
{
    LOCK(::cs_main);
    std::unordered_map<uint256, CBlockIndex, BlockHasher> block_map;
    fs::path block_tree_store_dir{m_args.GetDataDirBase()};
    auto header_file{block_tree_store_dir / HEADER_FILE_NAME};
    auto block_files_file{block_tree_store_dir / BLOCK_FILES_FILE_NAME};
    auto params{CreateChainParams(gArgs, ChainType::REGTEST)};
    BlockTreeStore store{block_tree_store_dir, *params};

    bool reindexing = true;
    store.ReadReindexing(reindexing);
    BOOST_CHECK(!reindexing);
    store.WriteReindexing(true);
    store.ReadReindexing(reindexing);
    BOOST_CHECK(reindexing);
    store.WriteReindexing(false);
    store.ReadReindexing(reindexing);
    BOOST_CHECK(!reindexing);

    int last_block;
    store.ReadLastBlockFile(last_block);
    BOOST_CHECK_EQUAL(last_block, 0);

    bool pruned = false;
    store.ReadPruned(pruned);
    BOOST_CHECK(!pruned);
    store.WritePruned(true);
    store.ReadPruned(pruned);
    BOOST_CHECK(pruned);
    store.WritePruned(false);
    store.ReadPruned(pruned);
    BOOST_CHECK(!pruned);

    std::vector<std::pair<int, CBlockFileInfo*>> fileinfo;
    CBlockFileInfo info{};
    info.nBlocks = 1;
    info.nSize = 2;
    info.nUndoSize = 3;
    info.nHeightFirst = 4;
    info.nHeightLast = 5;
    info.nTimeFirst = 6;
    info.nTimeLast = 7;

    // Write and read a CBlockFileInfo and a CBlockIndex
    fileinfo.emplace_back(0, &info);
    int32_t last_file{1};
    std::vector<CBlockIndex*> blockinfo;
    auto block_index = std::make_unique<CBlockIndex>(params->GenesisBlock().GetBlockHeader());
    auto header_pos = block_index->header_pos;
    BOOST_CHECK_EQUAL(header_pos, 0);
    blockinfo.emplace_back(block_index.get());
    BOOST_CHECK(store.WriteBatchSync(fileinfo, last_file, blockinfo));
    BOOST_CHECK_EQUAL(block_index->header_pos, HEADER_FILE_DATA_START_POS);
    BOOST_CHECK(store.LoadBlockIndexGuts(
        params->GetConsensus(),
        [&](const uint256& hash) { return InsertBlockIndex(block_map, hash); },
        m_interrupt));
    check_block_map(block_map, blockinfo);

    // Write another CBlockFileInfo and update the CBlockIndex
    info.nBlocks = 2;
    info.nSize = 3;
    info.nUndoSize = 4;
    info.nHeightFirst = 5;
    info.nHeightLast = 6;
    info.nTimeFirst = 7;
    info.nTimeLast = 8;
    CBlockFileInfo info_two{};
    info_two.nBlocks = 1;
    info_two.nSize = 2;
    info_two.nUndoSize = 3;
    info_two.nHeightFirst = 4;
    info_two.nHeightLast = 5;
    info_two.nTimeFirst = 6;
    info_two.nTimeLast = 7;
    fileinfo.emplace_back(1, &info_two);
    block_index->nStatus = 120;
    block_index->nFile = 1;
    BOOST_CHECK(store.WriteBatchSync(fileinfo, last_file, blockinfo));
    block_map.clear();
    BOOST_CHECK(store.LoadBlockIndexGuts(
        params->GetConsensus(),
        [&](const uint256& hash) { return InsertBlockIndex(block_map, hash); },
        m_interrupt));
    check_block_map(block_map, blockinfo);

    // Update the new CBlockFileInfo and the CBlockIndex
    block_index->nStatus = 99;
    block_index->nFile = 50;
    info_two.nBlocks = 2;
    info_two.nSize = 3;
    info_two.nUndoSize = 4;
    info_two.nHeightFirst = 5;
    info_two.nHeightLast = 6;
    info_two.nTimeFirst = 7;
    info_two.nTimeLast = 8;
    block_map.clear();
    BOOST_CHECK(store.WriteBatchSync(fileinfo, last_file, blockinfo));
    BOOST_CHECK(store.LoadBlockIndexGuts(
        params->GetConsensus(),
        [&](const uint256& hash) { return InsertBlockIndex(block_map, hash); },
        m_interrupt));
    check_block_map(block_map, blockinfo);
    check_block_file_info(0, info, store);
    check_block_file_info(1, info_two, store);

    // Writing an invalid CBlockIndex should fail to load
    block_index->nBits = 0;
    block_map.clear();
    BOOST_CHECK(store.WriteBatchSync(fileinfo, last_file, blockinfo));
    BOOST_CHECK(!store.LoadBlockIndexGuts(
        params->GetConsensus(),
        [&](const uint256& hash) { return InsertBlockIndex(block_map, hash); },
        m_interrupt));
}

BOOST_AUTO_TEST_SUITE_END()
