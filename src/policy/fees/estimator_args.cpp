// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/estimator_args.h>

#include <common/args.h>

namespace {
const char* FEES_BASE_DIR = "fees";
const char* BLOCK_POLICY_ESTIMATES_FILENAME = "block_policy_estimates.dat";
const char* MEMPOOL_POLICY_ESTIMATES_FILENAME = "mempool_policy_estimates.dat";
} // namespace

fs::path BlockPolicyFeeEstPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / FEES_BASE_DIR / BLOCK_POLICY_ESTIMATES_FILENAME;
}

fs::path MempoolPolicyFeeEstPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / FEES_BASE_DIR / MEMPOOL_POLICY_ESTIMATES_FILENAME;
}
