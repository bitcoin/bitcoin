// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <clientversion.h>
#include <consensus/amount.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <node/kernel_notifications.h>
#include <node/mining_types.h>
#include <script/script.h>
#include <script/solver.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <util/chaintype.h>
#include <util/strencodings.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>
#include <test/util/common.h>
#include <test/util/logging.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>

#include <cstddef>
#include <cstdint>
#include <ios>
#include <span>
#include <utility>
#include <vector>

using kernel::CBlockFileInfo;
using node::STORAGE_HEADER_BYTES;
using node::BlockManager;
using node::KernelNotifications;
using node::MAX_BLOCKFILE_SIZE;
using namespace util::hex_literals;

// use BasicTestingSetup here for the data directory configuration, setup, and cleanup
BOOST_FIXTURE_TEST_SUITE(blockmanager_tests, BasicTestingSetup)

static std::vector<std::byte> SerializeBlock(const CBlock& block, const TransactionSerParams& params)
{
    DataStream stream;
    stream << params(block);
    return {stream.begin(), stream.end()};
}

BOOST_AUTO_TEST_CASE(blockmanager_find_block_pos)
{
    const auto params {CreateChainParams(ArgsManager{}, ChainType::MAIN)};
    KernelNotifications notifications{Assert(m_node.shutdown_request), m_node.exit_status, *Assert(m_node.warnings)};
    const BlockManager::Options blockman_opts{
        .chainparams = *params,
        .blocks_dir = m_args.GetBlocksDirPath(),
        .notifications = notifications,
        .block_tree_db_params = DBParams{
            .path = m_args.GetDataDirNet() / "blocks" / "index",
            .cache_bytes = 0,
        },
    };
    BlockManager blockman{*Assert(m_node.shutdown_signal), blockman_opts};
    // simulate adding a genesis block normally
    LOCK(::cs_main);
    BOOST_CHECK_EQUAL(blockman.WriteBlock(params->GenesisBlock(), 0).nPos, STORAGE_HEADER_BYTES);
    // simulate what happens during reindex
    // simulate a well-formed genesis block being found at offset 8 in the blk00000.dat file
    // the block is found at offset 8 because there is an 8 byte serialization header
    // consisting of 4 magic bytes + 4 length bytes before each block in a well-formed blk file.
    const FlatFilePos pos{0, STORAGE_HEADER_BYTES};
    blockman.UpdateBlockInfo(params->GenesisBlock(), 0, pos);
    // now simulate what happens after reindex for the first new block processed
    // the actual block contents don't matter, just that it's a block.
    // verify that the write position is at offset 0x12d.
    // this is a check to make sure that https://github.com/bitcoin/bitcoin/issues/21379 does not recur
    // 8 bytes (for serialization header) + 285 (for serialized genesis block) = 293
    // add another 8 bytes for the second block's serialization header and we get 293 + 8 = 301
    FlatFilePos actual{blockman.WriteBlock(params->GenesisBlock(), 1)};
    BOOST_CHECK_EQUAL(actual.nPos, STORAGE_HEADER_BYTES + ::GetSerializeSize(TX_WITH_WITNESS(params->GenesisBlock())) + STORAGE_HEADER_BYTES);
}

BOOST_FIXTURE_TEST_CASE(blockmanager_scan_unlink_already_pruned_files, TestChain100Setup)
{
    // Cap last block file size, and mine new block in a new block file.
    auto& chainman{*Assert(m_node.chainman)};
    auto& blockman{chainman.m_blockman};
    const CBlockIndex* old_tip{WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip())};
    WITH_LOCK(chainman.GetMutex(), blockman.GetBlockFileInfo(old_tip->GetBlockPos().nFile)->nSize = MAX_BLOCKFILE_SIZE);
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));

    // Prune the older block file, but don't unlink it
    int file_number;
    {
        LOCK(chainman.GetMutex());
        file_number = old_tip->GetBlockPos().nFile;
        blockman.PruneOneBlockFile(file_number);
    }

    const FlatFilePos pos(file_number, 0);

    // Check that the file is not unlinked after ScanAndUnlinkAlreadyPrunedFiles
    // if m_have_pruned is not yet set
    WITH_LOCK(chainman.GetMutex(), blockman.ScanAndUnlinkAlreadyPrunedFiles());
    BOOST_CHECK(!blockman.OpenBlockFile(pos, true).IsNull());

    // Check that the file is unlinked after ScanAndUnlinkAlreadyPrunedFiles
    // once m_have_pruned is set
    blockman.m_have_pruned = true;
    WITH_LOCK(chainman.GetMutex(), blockman.ScanAndUnlinkAlreadyPrunedFiles());
    BOOST_CHECK(blockman.OpenBlockFile(pos, true).IsNull());

    // Check that calling with already pruned files doesn't cause an error
    WITH_LOCK(chainman.GetMutex(), blockman.ScanAndUnlinkAlreadyPrunedFiles());

    // Check that the new tip file has not been removed
    const CBlockIndex* new_tip{WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip())};
    BOOST_CHECK_NE(old_tip, new_tip);
    const int new_file_number{WITH_LOCK(chainman.GetMutex(), return new_tip->GetBlockPos().nFile)};
    const FlatFilePos new_pos(new_file_number, 0);
    BOOST_CHECK(!blockman.OpenBlockFile(new_pos, true).IsNull());
}

BOOST_FIXTURE_TEST_CASE(blockmanager_block_data_availability, TestChain100Setup)
{
    // The goal of the function is to return the first not pruned block in the range [upper_block, lower_block].
    LOCK(::cs_main);
    auto& chainman = m_node.chainman;
    auto& blockman = chainman->m_blockman;
    const CBlockIndex& tip = *chainman->ActiveTip();

    // Function to prune all blocks from 'last_pruned_block' down to the genesis block
    const auto& func_prune_blocks = [&](CBlockIndex* last_pruned_block)
    {
        LOCK(::cs_main);
        CBlockIndex* it = last_pruned_block;
        while (it != nullptr && it->nStatus & BLOCK_HAVE_DATA) {
            it->nStatus &= ~BLOCK_HAVE_DATA;
            it = it->pprev;
        }
    };

    // 1) Return genesis block when all blocks are available
    BOOST_CHECK_EQUAL(&blockman.GetFirstBlock(tip, BLOCK_HAVE_DATA), chainman->ActiveChain()[0]);
    BOOST_CHECK(blockman.CheckBlockDataAvailability(tip, *chainman->ActiveChain()[0]));

    // 2) Check lower_block when all blocks are available
    CBlockIndex* lower_block = chainman->ActiveChain()[tip.nHeight / 2];
    BOOST_CHECK(blockman.CheckBlockDataAvailability(tip, *lower_block));

    // Ensure we don't fail due to the expected absence of undo data in the genesis block
    CBlockIndex* upper_block = chainman->ActiveChain()[2];
    CBlockIndex* genesis = chainman->ActiveChain()[0];
    BOOST_CHECK(blockman.CheckBlockDataAvailability(*upper_block, *genesis, BlockStatus{BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO}));
    // Ensure we detect absence of undo data in the first block
    chainman->ActiveChain()[1]->nStatus &= ~BLOCK_HAVE_UNDO;
    BOOST_CHECK(!blockman.CheckBlockDataAvailability(tip, *genesis, BlockStatus{BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO}));

    // Prune half of the blocks
    int height_to_prune = tip.nHeight / 2;
    CBlockIndex* first_available_block = chainman->ActiveChain()[height_to_prune + 1];
    CBlockIndex* last_pruned_block = first_available_block->pprev;
    func_prune_blocks(last_pruned_block);

    // 3) The last block not pruned is in-between upper-block and the genesis block
    BOOST_CHECK_EQUAL(&blockman.GetFirstBlock(tip, BLOCK_HAVE_DATA), first_available_block);
    BOOST_CHECK(blockman.CheckBlockDataAvailability(tip, *first_available_block));
    BOOST_CHECK(!blockman.CheckBlockDataAvailability(tip, *last_pruned_block));

    // Simulate that the first available block is missing undo data and
    // detect this by using a status mask.
    first_available_block->nStatus &= ~BLOCK_HAVE_UNDO;
    BOOST_CHECK(!blockman.CheckBlockDataAvailability(tip, *first_available_block, BlockStatus{BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO}));
    BOOST_CHECK(blockman.CheckBlockDataAvailability(tip, *first_available_block, BlockStatus{BLOCK_HAVE_DATA}));
}

BOOST_FIXTURE_TEST_CASE(blockmanager_block_data_part, TestChain100Setup)
{
    LOCK(::cs_main);
    auto& chainman{m_node.chainman};
    auto& blockman{chainman->m_blockman};
    const CBlockIndex& tip{*chainman->ActiveTip()};
    const FlatFilePos tip_block_pos{tip.GetBlockPos()};

    auto block{blockman.ReadRawBlock(tip_block_pos)};
    BOOST_REQUIRE(block);
    BOOST_REQUIRE_GE(block->size(), 200);

    const auto expect_part{[&](size_t offset, size_t size) {
        auto res{blockman.ReadRawBlock(tip_block_pos, std::pair{offset, size})};
        BOOST_CHECK(res);
        const auto& part{res.value()};
        BOOST_CHECK_EQUAL_COLLECTIONS(part.begin(), part.end(), block->begin() + offset, block->begin() + offset + size);
    }};

    expect_part(0, 20);
    expect_part(0, block->size() - 1);
    expect_part(0, block->size() - 10);
    expect_part(0, block->size());
    expect_part(1, block->size() - 1);
    expect_part(10, 20);
    expect_part(block->size() - 1, 1);
}

BOOST_FIXTURE_TEST_CASE(blockmanager_block_data_part_error, TestChain100Setup)
{
    LOCK(::cs_main);
    auto& chainman{m_node.chainman};
    auto& blockman{chainman->m_blockman};
    const CBlockIndex& tip{*chainman->ActiveTip()};
    const FlatFilePos tip_block_pos{tip.GetBlockPos()};

    auto block{blockman.ReadRawBlock(tip_block_pos)};
    BOOST_REQUIRE(block);
    BOOST_REQUIRE_GE(block->size(), 200);

    const auto expect_part_error{[&](size_t offset, size_t size) {
        auto res{blockman.ReadRawBlock(tip_block_pos, std::pair{offset, size})};
        BOOST_CHECK(!res);
        BOOST_CHECK_EQUAL(res.error(), node::ReadRawError::BadPartRange);
    }};

    expect_part_error(0, 0);
    expect_part_error(0, block->size() + 1);
    expect_part_error(0, std::numeric_limits<size_t>::max());
    expect_part_error(1, block->size());
    expect_part_error(2, block->size() - 1);
    expect_part_error(block->size() - 1, 2);
    expect_part_error(block->size() - 2, 3);
    expect_part_error(block->size() + 1, 0);
    expect_part_error(block->size() + 1, 1);
    expect_part_error(block->size() + 2, 2);
    expect_part_error(block->size(), 0);
    expect_part_error(block->size(), 1);
    expect_part_error(std::numeric_limits<size_t>::max(), 1);
    expect_part_error(std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
}

BOOST_FIXTURE_TEST_CASE(blockmanager_readblock_hash_mismatch, TestingSetup)
{
    CBlockIndex index;
    {
        LOCK(cs_main);
        const auto tip{m_node.chainman->ActiveTip()};
        index.nStatus = tip->nStatus;
        index.nDataPos = tip->nDataPos;
        index.phashBlock = &uint256::ONE; // mismatched block hash
    }

    { // Finish this log check before the other reader can emit the same error.
        ASSERT_DEBUG_LOG("GetHash() doesn't match index");
        CBlock block;
        BOOST_CHECK(!m_node.chainman->m_blockman.ReadBlock(block, index));
    }
    const auto pos{WITH_LOCK(cs_main, return index.GetBlockPos())};
    ASSERT_DEBUG_LOG("GetHash() doesn't match index");
    BOOST_CHECK(!m_node.chainman->m_blockman.ReadBlockWithoutWitness(pos, index.GetBlockHash()));
}

BOOST_FIXTURE_TEST_CASE(blockmanager_readblock_validation, RegTestingSetup)
{
    MineBlock(m_node, {}); // Adds a block with a coinbase witness on top of genesis
    LOCK(cs_main);
    auto& blockman{m_node.chainman->m_blockman};
    const auto check_read_fails{[&](const FlatFilePos& pos, const uint256& hash, const char* error) {
        { // Finish this log check before the other reader can emit the same error.
            ASSERT_DEBUG_LOG(error);
            CBlock block;
            BOOST_CHECK(!blockman.ReadBlock(block, pos, hash));
        }
        ASSERT_DEBUG_LOG(error);
        BOOST_CHECK(!blockman.ReadBlockWithoutWitness(pos, hash));
    }};
    for (const auto* index : {m_node.chainman->ActiveChain()[0], m_node.chainman->ActiveTip()}) {
        CBlock block;
        BOOST_REQUIRE(blockman.ReadBlock(block, *index));
        BOOST_CHECK_EQUAL(block.vtx[0]->HasWitness(), index->nHeight > 0);
        BOOST_CHECK(*Assert(blockman.ReadBlockWithoutWitness(index->GetBlockPos(), index->GetBlockHash())) == SerializeBlock(block, TX_NO_WITNESS));
    }
    check_read_fails(FlatFilePos{}, uint256::ZERO, "while reading raw block storage header");

    const auto& genesis{Params().GenesisBlock()};
    CBlock corrupt{genesis};
    corrupt.nBits = 0; // Invalid proof-of-work target, with an otherwise intact block
    check_read_fails(blockman.WriteBlock(corrupt, 0), corrupt.GetHash(), "Errors in block header");

    const auto genesis_pos{blockman.WriteBlock(genesis, 0)};
    // WriteBlock returns the payload position, after the storage magic and size.
    auto file{blockman.OpenBlockFile({genesis_pos.nFile, genesis_pos.nPos - STORAGE_HEADER_BYTES}, false)};
    BOOST_REQUIRE(!file.IsNull());
    // Shorten the recorded size to end before the first transaction, without truncating the file.
    file << Params().MessageStart() << unsigned(GetSerializeSize(CBlockHeader{}) + GetSizeOfCompactSize(genesis.vtx.size()));
    BOOST_REQUIRE_EQUAL(file.fclose(), 0);
    check_read_fails(genesis_pos, genesis.GetHash(), "Deserialize or I/O error");
}

BOOST_AUTO_TEST_CASE(blockmanager_flush_block_file)
{
    KernelNotifications notifications{Assert(m_node.shutdown_request), m_node.exit_status, *Assert(m_node.warnings)};
    node::BlockManager::Options blockman_opts{
        .chainparams = Params(),
        .blocks_dir = m_args.GetBlocksDirPath(),
        .notifications = notifications,
        .block_tree_db_params = DBParams{
            .path = m_args.GetDataDirNet() / "blocks" / "index",
            .cache_bytes = 0,
        },
    };
    BlockManager blockman{*Assert(m_node.shutdown_signal), blockman_opts};

    // Test blocks with no transactions, not even a coinbase
    CBlock block1;
    block1.nVersion = 1;
    CBlock block2;
    block2.nVersion = 2;
    CBlock block3;
    block3.nVersion = 3;

    // They are 80 bytes header + 1 byte 0x00 for vtx length
    constexpr int TEST_BLOCK_SIZE{81};

    // Blockstore is empty
    LOCK(::cs_main);
    BOOST_CHECK_EQUAL(blockman.CalculateCurrentUsage(), 0);

    // Write the first block to a new location.
    FlatFilePos pos1{blockman.WriteBlock(block1, /*nHeight=*/1)};

    // Write second block
    FlatFilePos pos2{blockman.WriteBlock(block2, /*nHeight=*/2)};

    // Two blocks in the file
    BOOST_CHECK_EQUAL(blockman.CalculateCurrentUsage(), (TEST_BLOCK_SIZE + STORAGE_HEADER_BYTES) * 2);

    // First two blocks are written as expected
    // Errors are expected because block data is junk, thrown AFTER successful read
    CBlock read_block;
    BOOST_CHECK_EQUAL(read_block.nVersion, 0);
    {
        ASSERT_DEBUG_LOG("Errors in block header");
        BOOST_CHECK(!blockman.ReadBlock(read_block, pos1, {}));
        BOOST_CHECK_EQUAL(read_block.nVersion, 1);
    }
    {
        ASSERT_DEBUG_LOG("Errors in block header");
        BOOST_CHECK(!blockman.ReadBlock(read_block, pos2, {}));
        BOOST_CHECK_EQUAL(read_block.nVersion, 2);
    }

    // During reindex, the flat file block storage will not be written to.
    // UpdateBlockInfo will, however, update the blockfile metadata.
    // Verify this behavior by attempting (and failing) to write block 3 data
    // to block 2 location.
    CBlockFileInfo* block_data = blockman.GetBlockFileInfo(0);
    BOOST_CHECK_EQUAL(block_data->nBlocks, 2);
    blockman.UpdateBlockInfo(block3, /*nHeight=*/3, /*pos=*/pos2);
    // Metadata is updated...
    BOOST_CHECK_EQUAL(block_data->nBlocks, 3);
    // ...but there are still only two blocks in the file
    BOOST_CHECK_EQUAL(blockman.CalculateCurrentUsage(), (TEST_BLOCK_SIZE + STORAGE_HEADER_BYTES) * 2);

    // Block 2 was not overwritten:
    BOOST_CHECK(!blockman.ReadBlock(read_block, pos2, {}));
    BOOST_CHECK_EQUAL(read_block.nVersion, 2);
}

BOOST_FIXTURE_TEST_CASE(prune_lock_update_and_delete, TestingSetup)
{
    LOCK(::cs_main);
    auto& chainman{*Assert(m_node.chainman)};
    auto& blockman{chainman.m_blockman};

    // Create a prune lock
    blockman.UpdatePruneLock("test_lock", node::PruneLockInfo{.height_first = 100});

    // Update it to a new height
    blockman.UpdatePruneLock("test_lock", node::PruneLockInfo{.height_first = 200});

    // Delete existing prune lock
    BOOST_CHECK(blockman.DeletePruneLock("test_lock"));

    // Verify deletion worked by trying to delete the same lock again
    BOOST_CHECK(!blockman.DeletePruneLock("test_lock"));

    // Deleting a non-existent lock returns false
    BOOST_CHECK(!blockman.DeletePruneLock("nonexistent"));
}

static void CheckStripped(const CBlock& block)
{
    const auto expected{SerializeBlock(block, TX_NO_WITNESS)};
    auto data{SerializeBlock(block, TX_WITH_WITNESS)};
    data.insert(data.end(), {std::byte{0xff}, std::byte{0x00}}); // Trailing bytes are ignored by the decoder and excluded by the strip
    CBlock decoded;
    SpanReader{data} >> TX_WITH_WITNESS(decoded);
    BOOST_CHECK(SerializeBlock(decoded, TX_NO_WITNESS) == expected);
    BOOST_CHECK(node::StripBlockWitness(std::move(data)) == expected);
    BOOST_CHECK(node::StripBlockWitness(expected) == expected); // Legacy encodings pass through unchanged
}

BOOST_AUTO_TEST_CASE(strip_mixed_transactions)
{
    CBlock block;
    block.nVersion = -1;
    block.nTime = 12345;
    block.nNonce = 54321;
    CheckStripped(block);

    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vout.emplace_back(50 * COIN, CScript{} << OP_TRUE);
    tx.vin[0].scriptWitness.stack = {std::vector<uint8_t>(32)};
    tx.vout.emplace_back(0, CScript{} << OP_RETURN << "aa21a9ed0000000000000000000000000000000000000000000000000000000000000000"_hex_v_u8);
    block.vtx.push_back(MakeTransactionRef(tx));
    tx.vin[0].scriptWitness.SetNull();
    tx.nLockTime = 42;
    block.vtx.push_back(MakeTransactionRef(tx));
    tx.vin.resize(3);
    tx.vin[0].scriptWitness.stack = {{}, {1, 2, 3}};
    tx.vin[2].scriptWitness.stack = {{}}; // A nonempty stack containing an empty item is still witness data
    block.vtx.push_back(MakeTransactionRef(tx));
    block.vtx.push_back(block.vtx[1]);
    CheckStripped(block);

    // Preserve the decoder's empty-transaction encoding even though consensus rejects these transactions.
    block.vtx = {MakeTransactionRef(CMutableTransaction{})};
    CheckStripped(block);
}

BOOST_AUTO_TEST_CASE(strip_compactsize_lengths)
{
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    CBlock block;
    for (const auto size : {0, 1, 252, 253, 65535, 65536}) {
        tx.vin[0].scriptSig.assign(size, 0x51);
        tx.vout[0].scriptPubKey = tx.vin[0].scriptSig;
        tx.vin[0].scriptWitness.stack = {std::vector<uint8_t>(size, 0x42)};
        block.vtx = {MakeTransactionRef(tx)};
        CheckStripped(block);
    }
    tx.vin[0].scriptSig.clear();
    tx.vout[0].scriptPubKey.clear();
    tx.vin[0].scriptWitness.stack = {{0x42}};
    const auto small_tx{MakeTransactionRef(tx)};
    for (const auto count : {1, 252, 253}) {
        tx.vin.resize(count);
        tx.vout.resize(count);
        tx.vin[0].scriptWitness.stack.assign(count, {});
        block.vtx = {MakeTransactionRef(tx)};
        block.vtx.resize(count, small_tx);
        CheckStripped(block);
    }
    // Five-byte counts for outputs and witness items
    tx.vin.resize(1);
    tx.vout.resize(65'536);
    tx.vin[0].scriptWitness.stack.resize(65'536);
    block.vtx = {MakeTransactionRef(tx)};
    CheckStripped(block);
}

BOOST_AUTO_TEST_CASE(strip_invalid_encodings)
{
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vin[0].scriptWitness.stack = {{1, 2, 3}};
    CBlock block;
    block.vtx = {MakeTransactionRef(tx), MakeTransactionRef(tx)};
    const auto check_rejected{[](std::span<const std::byte> data, const char* reason) {
        CBlock decoded;
        BOOST_CHECK_EXCEPTION(SpanReader{data} >> TX_WITH_WITNESS(decoded), std::ios_base::failure, HasReason(reason));
        BOOST_CHECK_EXCEPTION((void)node::StripBlockWitness({data.begin(), data.end()}), std::ios_base::failure, HasReason(reason));
    }};
    const auto full{SerializeBlock(block, TX_WITH_WITNESS)};
    for (size_t size{0}; size < full.size(); ++size) check_rejected({full.begin(), full.begin() + size}, "end of data");

    block.vtx.resize(1);
    const auto valid{SerializeBlock(block, TX_WITH_WITNESS)};
    for (uint8_t flag : {2, 3, 0xff}) {
        auto data{valid};
        data[86] = std::byte{flag};
        check_rejected(data, "Unknown transaction optional data");
    }
    auto empty_witness{valid};
    empty_witness[139] = std::byte{0};
    check_rejected(empty_witness, "Superfluous witness record");

    // CompactSizes for transactions, inputs, scriptSig, outputs, scriptPubKey, witness items and item length
    for (const auto offset : {80, 87, 124, 129, 138, 139, 140}) {
        for (const auto& [encoding, reason] : {
                 std::pair{"fd0000"_hex_v, "non-canonical"},
                 std::pair{"fe00000000"_hex_v, "non-canonical"},
                 std::pair{"ff0000000000000000"_hex_v, "non-canonical"},
                 std::pair{"fe01000002"_hex_v, "size too large"},
             }) {
            auto data{valid};
            data.erase(data.begin() + offset);
            data.insert(data.begin() + offset, encoding.begin(), encoding.end());
            check_rejected(data, reason);
            for (size_t size{1}; size < encoding.size(); ++size) check_rejected({data.begin(), data.begin() + offset + size}, "end of data");
        }
    }
}

BOOST_FIXTURE_TEST_CASE(blockmanager_add_to_block_index_best_header, TestingSetup)
{
    LOCK(::cs_main);
    auto& blockman{m_node.chainman->m_blockman};
    const CBlockIndex* genesis{m_node.chainman->ActiveTip()};

    CBlockHeader header;
    header.hashPrevBlock = genesis->GetBlockHash();
    header.nTime = genesis->nTime + 1;
    header.nBits = genesis->nBits;

    CBlockIndex* best_header{nullptr};
    header.nNonce = 1;
    CBlockIndex* first{blockman.AddToBlockIndex(header, best_header)};
    BOOST_CHECK_EQUAL(best_header, first);

    // A sibling with equal chain work must not replace the first-seen header
    header.nNonce = 2;
    CBlockIndex* second{blockman.AddToBlockIndex(header, best_header)};
    BOOST_CHECK(second != first);
    BOOST_CHECK(second->nChainWork == first->nChainWork);
    BOOST_CHECK_EQUAL(best_header, first);
}

BOOST_AUTO_TEST_SUITE_END()
