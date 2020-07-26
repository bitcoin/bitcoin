// Copyright (c) 2020 The Bitcoin Core developers
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
#include <map>
#include <vector>

void initialize()
{
    InitializeFuzzingContext();
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider = ConsumeFile(fuzzed_data_provider);
    FILE* fuzzed_block_file = fuzzed_file_provider.open();
    if (fuzzed_block_file == nullptr) {
        return;
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        FlatFilePos flat_file_pos;
        std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
        LoadExternalBlockFile(Params(), fuzzed_block_file, &flat_file_pos, &blocks_with_unknown_parent);
    } else {
        LoadExternalBlockFile(Params(), fuzzed_block_file);
    }
}
