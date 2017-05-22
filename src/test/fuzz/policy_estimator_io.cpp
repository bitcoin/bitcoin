// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees.h>
#include <policy/fees_args.h>
#include <policy/fees_input.h>
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
    static auto testing_setup = MakeNoLogFileContext<BasicTestingSetup>();
    // Re-using block_policy_estimator across runs to avoid costly creation of CBlockPolicyEstimator object.
    testing_setup->m_node.fee_estimator = std::make_unique<CBlockPolicyEstimator>();
    testing_setup->m_node.fee_estimator_input = std::make_unique<FeeEstInput>(*testing_setup->m_node.fee_estimator);
    testing_setup->m_node.fee_estimator_input->open(FeeestPath(*testing_setup->m_node.args), FeeestLogPath(*testing_setup->m_node.args));
    g_setup = testing_setup.get();
}

FUZZ_TARGET_INIT(policy_estimator_io, initialize_policy_estimator_io)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FuzzedAutoFileProvider fuzzed_auto_file_provider = ConsumeAutoFile(fuzzed_data_provider);
    AutoFile fuzzed_auto_file{fuzzed_auto_file_provider.open()};
    if (g_setup->m_node.fee_estimator->Read(fuzzed_auto_file)) {
        g_setup->m_node.fee_estimator->Write(fuzzed_auto_file);
    }
}
