// Copyright (c) 2020 The Widecoin Core developers
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
    FlatFilePos flat_file_pos;
    LoadExternalBlockFile(Params(), fuzzed_block_file, fuzzed_data_provider.ConsumeBool() ? &flat_file_pos : nullptr);
}
