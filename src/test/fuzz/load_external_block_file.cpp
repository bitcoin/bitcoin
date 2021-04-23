// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <flatfile.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <cstdint>
#include <vector>

namespace {
const TestingSetup* g_setup;
} // namespace

void initialize_load_external_block_file()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET_INIT(load_external_block_file, initialize_load_external_block_file)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider = ConsumeFile(fuzzed_data_provider);
    FILE* fuzzed_block_file = fuzzed_file_provider.open();
    if (fuzzed_block_file == nullptr) {
        return;
    }
    std::vector<fs::path> blk_paths = {"no_such_file"};
    std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
    bool write_to_disk = fuzzed_data_provider.ConsumeBool();
    g_setup->m_node.chainman->ActiveChainstate().LoadExternalBlockFile(blk_paths, 0, fuzzed_block_file, blocks_with_unknown_parent, write_to_disk);
}
