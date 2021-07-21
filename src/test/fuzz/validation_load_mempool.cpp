// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <util/time.h>
#include <validation.h>

#include <cstdint>
#include <vector>

namespace {
const TestingSetup* g_setup;
} // namespace

void initialize_validation_load_mempool()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET_INIT(validation_load_mempool, initialize_validation_load_mempool)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    FuzzedFileProvider fuzzed_file_provider = ConsumeFile(fuzzed_data_provider);

    CTxMemPool pool{};
    auto fuzzed_fopen = [&](const fs::path&, const char*) {
        return fuzzed_file_provider.open();
    };
    (void)LoadMempool(pool, g_setup->m_node.chainman->ActiveChainstate(), fuzzed_fopen);
    (void)DumpMempool(pool, fuzzed_fopen, true);
}
