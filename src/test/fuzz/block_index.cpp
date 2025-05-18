// Copyright (c) 2023 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <node/blockstorage.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <validation.h>

namespace {

const BasicTestingSetup* g_setup;

// Hardcoded block hash and nBits to make sure the blocks we store pass the pow check.
uint256 g_block_hash;

bool operator==(const CBlockFileInfo& a, const CBlockFileInfo& b)
{
    return a.nBlocks == b.nBlocks &&
        a.nSize == b.nSize &&
        a.nUndoSize == b.nUndoSize &&
        a.nHeightFirst == b.nHeightFirst &&
        a.nHeightLast == b.nHeightLast &&
        a.nTimeFirst == b.nTimeFirst &&
        a.nTimeLast == b.nTimeLast;
}

CBlockHeader ConsumeBlockHeader(FuzzedDataProvider& provider)
{
    CBlockHeader header;
    header.nVersion = provider.ConsumeIntegral<decltype(header.nVersion)>();
    header.hashPrevBlock = g_block_hash;
    header.hashMerkleRoot = g_block_hash;
    header.nTime = provider.ConsumeIntegral<decltype(header.nTime)>();
    header.nBits = Params().GenesisBlock().nBits;
    header.nNonce = provider.ConsumeIntegral<decltype(header.nNonce)>();
    return header;
}

} // namespace

void init_block_index()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::MAIN);
    g_setup = testing_setup.get();
    g_block_hash = Params().GenesisBlock().GetHash();
}

FUZZ_TARGET(block_index, .init = init_block_index)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    auto block_index = kernel::BlockTreeDB(DBParams{
        .path = "", // Memory only.
        .cache_bytes = 1 << 20, // 1MB.
        .memory_only = true,
    });

    // Generate a number of block files to be stored in the index.
    int files_count = fuzzed_data_provider.ConsumeIntegralInRange(1, 100);
    std::vector<std::unique_ptr<CBlockFileInfo>> files;
    files.reserve(files_count);
    std::vector<std::pair<int, const CBlockFileInfo*>> files_info;
    files_info.reserve(files_count);
    for (int i = 0; i < files_count; i++) {
        if (auto file_info = ConsumeDeserializable<CBlockFileInfo>(fuzzed_data_provider)) {
            files.push_back(std::make_unique<CBlockFileInfo>(std::move(*file_info)));
            files_info.emplace_back(i, files.back().get());
        } else {
            return;
        }
    }

    // Generate a number of block headers to be stored in the index.
    int blocks_count = fuzzed_data_provider.ConsumeIntegralInRange(files_count * 10, files_count * 100);
    std::vector<std::unique_ptr<CBlockIndex>> blocks;
    blocks.reserve(blocks_count);
    std::vector<const CBlockIndex*> blocks_info;
    blocks_info.reserve(blocks_count);
    for (int i = 0; i < blocks_count; i++) {
        CBlockHeader header{ConsumeBlockHeader(fuzzed_data_provider)};
        blocks.push_back(std::make_unique<CBlockIndex>(std::move(header)));
        blocks.back()->phashBlock = &g_block_hash;
        blocks_info.push_back(blocks.back().get());
    }

    // Store these files and blocks in the block index. It should not fail.
    assert(block_index.WriteBatchSync(files_info, files_count - 1, blocks_info));

    // We should be able to read every block file info we stored. Its value should correspond to
    // what we stored above.
    CBlockFileInfo info;
    for (const auto& [n, file_info]: files_info) {
        assert(block_index.ReadBlockFileInfo(n, info));
        assert(info == *file_info);
    }

    // We should be able to read the last block file number. Its value should be consistent.
    int last_block_file;
    assert(block_index.ReadLastBlockFile(last_block_file));
    assert(last_block_file == files_count - 1);

    // We should be able to flip and read the reindexing flag.
    bool reindexing;
    block_index.WriteReindexing(true);
    block_index.ReadReindexing(reindexing);
    assert(reindexing);
    block_index.WriteReindexing(false);
    block_index.ReadReindexing(reindexing);
    assert(!reindexing);

    // We should be able to set and read the value of any random flag.
    const std::string flag_name = fuzzed_data_provider.ConsumeRandomLengthString(100);
    bool flag_value;
    block_index.WriteFlag(flag_name, true);
    block_index.ReadFlag(flag_name, flag_value);
    assert(flag_value);
    block_index.WriteFlag(flag_name, false);
    block_index.ReadFlag(flag_name, flag_value);
    assert(!flag_value);

    // We should be able to load everything we've previously stored. Note to assert on the
    // return value we need to make sure all blocks pass the pow check.
    const auto params{Params().GetConsensus()};
    const auto inserter = [&](const uint256&) {
        return blocks.back().get();
    };
    WITH_LOCK(::cs_main, assert(block_index.LoadBlockIndexGuts(params, inserter, g_setup->m_interrupt)));
}
