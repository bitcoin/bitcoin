// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees.h>
#include <policy/fees_args.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;
} // namespace

void initialize_policy_estimator_io()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET_INIT(policy_estimator_io, initialize_policy_estimator_io)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FuzzedAutoFileProvider fuzzed_auto_file_provider = ConsumeAutoFile(fuzzed_data_provider);
    AutoFile fuzzed_auto_file{fuzzed_auto_file_provider.open()};
    // Re-using block_policy_estimator across runs to avoid costly creation of CBlockPolicyEstimator object.
    static CBlockPolicyEstimator block_policy_estimator{FeeestPath(*g_setup->m_node.args)};
    if (block_policy_estimator.Read(fuzzed_auto_file)) {
        block_policy_estimator.Write(fuzzed_auto_file);
    }
}
