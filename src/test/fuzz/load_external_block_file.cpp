// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <clientversion.h>
#include <flatfile.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/time.h>
#include <validation.h>

#include <cstdint>
#include <vector>

extern void MakeRandDeterministicDANGEROUS(const uint256& seed) noexcept;

namespace {
TestingSetup* g_setup;

/** Recreate the chainman from a deterministic genesis baseline. */
void ResetChainman(TestingSetup& setup, FakeNodeClock& node_clock)
{
    node_clock.set(setup.m_node.chainman->GetParams().GenesisBlock().Time());
    MakeRandDeterministicDANGEROUS(uint256::ZERO);
    setup.m_node.chainman.reset();
    setup.m_make_chainman();
    setup.LoadVerifyActivateChainstate();
}
} // namespace

void initialize_load_external_block_file()
{
    FakeNodeClock init_clock{}; // Uses the existing mock time
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>(
        ChainType::REGTEST,
        {
            .setup_net = false,
        });
    g_setup = testing_setup.get();
    ResetChainman(*g_setup, init_clock);
}

FUZZ_TARGET(load_external_block_file, .init = initialize_load_external_block_file)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    AutoFile fuzzed_block_file{fuzzed_file_provider.open()};
    if (fuzzed_block_file.IsNull()) {
        return;
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        // Corresponds to the -reindex case (track orphan blocks across files).
        FlatFilePos flat_file_pos;
        std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
        g_setup->m_node.chainman->LoadExternalBlockFile(fuzzed_block_file, &flat_file_pos, &blocks_with_unknown_parent);
    } else {
        // Corresponds to the -loadblock= case (orphan blocks aren't tracked across files).
        g_setup->m_node.chainman->LoadExternalBlockFile(fuzzed_block_file);
    }

    ResetChainman(*g_setup, clock);
}
