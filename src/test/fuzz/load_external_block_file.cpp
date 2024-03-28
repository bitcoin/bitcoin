// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <clientversion.h>
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

FUZZ_TARGET(load_external_block_file, .init = initialize_load_external_block_file)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    AutoFile fuzzed_block_file{fuzzed_file_provider.open()};
    if (fuzzed_block_file.IsNull()) {
        return;
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        // Corresponds to the -reindex case (track orphan blocks across files).
        FlatFilePos flat_file_pos;
        std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
        Assert(g_setup->m_node.chainman->LoadExternalBlockFile(fuzzed_block_file, &flat_file_pos, &blocks_with_unknown_parent));
    } else {
        // Corresponds to the -loadblock= case (orphan blocks aren't tracked across files).
        Assert(g_setup->m_node.chainman->LoadExternalBlockFile(fuzzed_block_file));
    }
}
