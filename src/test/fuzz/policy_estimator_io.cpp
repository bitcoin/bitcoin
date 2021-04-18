// Copyright (c) 2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

void initialize()
{
    InitializeFuzzingContext();
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FuzzedAutoFileProvider fuzzed_auto_file_provider = ConsumeAutoFile(fuzzed_data_provider);
    CAutoFile fuzzed_auto_file = fuzzed_auto_file_provider.open();
    // Re-using block_policy_estimator across runs to avoid costly creation of CBlockPolicyEstimator object.
    static CBlockPolicyEstimator block_policy_estimator;
    if (block_policy_estimator.Read(fuzzed_auto_file)) {
        block_policy_estimator.Write(fuzzed_auto_file);
    }
}
