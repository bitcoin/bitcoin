// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data.h>
#include <chainparams.h>
#include <clientversion.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>
#include <validation.h>

/**
 * The LoadExternalBlockFile() function is used during -reindex and -loadblock.
 *
 * Create a test file that's similar to a datadir/blocks/blk?????.dat file,
 * It contains around 134 copies of the same block (typical size of real block files).
 * For each block in the file, LoadExternalBlockFile() won't find its parent,
 * and so will skip the block. (In the real system, it will re-read the block
 * from disk later when it encounters its parent.)
 *
 * This benchmark measures the performance of deserializing the block (or just
 * its header, beginning with PR 16981).
 */
static void LoadExternalBlockFile(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};

    // Create a single block as in the blocks files (magic bytes, block size,
    // block data) as a stream object.
    const fs::path blkfile{testing_setup.get()->m_path_root / "blk.dat"};
    DataStream ss{};
    auto params{testing_setup->m_node.chainman->GetParams()};
    ss << params.MessageStart();
    ss << static_cast<uint32_t>(benchmark::data::block413567.size());
    // We can't use the streaming serialization (ss << benchmark::data::block413567)
    // because that first writes a compact size.
    ss << Span{benchmark::data::block413567};

    // Create the test file.
    {
        // "wb+" is "binary, O_RDWR | O_CREAT | O_TRUNC".
        FILE* file{fsbridge::fopen(blkfile, "wb+")};
        // Make the test block file about 128 MB in length.
        for (size_t i = 0; i < node::MAX_BLOCKFILE_SIZE / ss.size(); ++i) {
            if (fwrite(ss.data(), 1, ss.size(), file) != ss.size()) {
                throw std::runtime_error("write to test file failed\n");
            }
        }
        fclose(file);
    }

    std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
    FlatFilePos pos;
    bench.run([&] {
        // "rb" is "binary, O_RDONLY", positioned to the start of the file.
        // The file will be closed by LoadExternalBlockFile().
        AutoFile file{fsbridge::fopen(blkfile, "rb")};
        Assert(testing_setup->m_node.chainman->LoadExternalBlockFile(file, &pos, &blocks_with_unknown_parent));
    });
    fs::remove(blkfile);
}

BENCHMARK(LoadExternalBlockFile, benchmark::PriorityLevel::HIGH);
