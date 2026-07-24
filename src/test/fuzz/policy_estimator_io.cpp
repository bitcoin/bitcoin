// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/estimator_args.h>
#include <policy/fees/mempool_estimator.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <memory>

namespace {
const TestingSetup* g_setup;
} // namespace

void initialize_policy_estimator_io()
{
    static const auto testing_setup{
        MakeNoLogFileContext<const TestingSetup>(ChainType::REGTEST, TestOpts{.setup_net = false})};
    g_setup = testing_setup.get();
}

FUZZ_TARGET(policy_estimator_io, .init = initialize_policy_estimator_io)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    // Reuse estimators across runs to avoid costly object creation.
    static CBlockPolicyEstimator block_policy_estimator{
        BlockPolicyFeeEstPath(*g_setup->m_node.args), DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    {
        AutoFile fuzzed_auto_file_block_policy{fuzzed_file_provider.open()};
        if (block_policy_estimator.Read(fuzzed_auto_file_block_policy)) {
            block_policy_estimator.Write(fuzzed_auto_file_block_policy);
        }
        (void)fuzzed_auto_file_block_policy.fclose();
    }
    static MemPoolFeeRateEstimator mempool_feerate_estimator{
        MempoolPolicyEstimatorPath(*g_setup->m_node.args),
        g_setup->m_node.mempool.get(),
        g_setup->m_node.chainman.get()};
    {
        AutoFile fuzzed_auto_file_mempool_policy{fuzzed_file_provider.open()};
        if (mempool_feerate_estimator.Read(fuzzed_auto_file_mempool_policy)) {
            mempool_feerate_estimator.Write(fuzzed_auto_file_mempool_policy);
        }
        // Write() does not take ownership; close explicitly because it may
        // have written to the fuzzed file.
        (void)fuzzed_auto_file_mempool_policy.fclose();
    }
}
