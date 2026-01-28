// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <policy/fees/estimator_args.h>

namespace {
const char* FEES_BASE_DIR = "fees";
const char* BLOCK_POLICY_ESTIMATES_FILENAME = "block_policy_estimates.dat";
const char* MEMPOOL_POLICY_ESTIMATES_FILENAME = "mempool_policy_estimates.dat";
} // namespace

fs::path BlockPolicyFeeestPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / FEES_BASE_DIR / BLOCK_POLICY_ESTIMATES_FILENAME;
}

fs::path MempoolPolicyFeeestPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / FEES_BASE_DIR / MEMPOOL_POLICY_ESTIMATES_FILENAME;
}
