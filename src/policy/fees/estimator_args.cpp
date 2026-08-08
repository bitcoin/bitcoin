// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/estimator_args.h>

#include <common/args.h>
#include <util/log.h>

#include <system_error>

namespace {
constexpr const char* FEES_BASE_DIR{"fees"};
constexpr const char* BLOCK_POLICY_ESTIMATES_FILENAME{"block_policy_estimates.dat"};
constexpr const char* LEGACY_FEE_ESTIMATES_FILENAME{"fee_estimates.dat"};
constexpr const char* MEMPOOL_POLICY_ESTIMATOR_FILENAME{"mempool_policy_estimator.dat"};

fs::path LegacyFeeEstPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / LEGACY_FEE_ESTIMATES_FILENAME;
}
} // namespace

void MaybeMigrateLegacyFeeEstimates(const ArgsManager& argsman)
{
    const fs::path legacy_path{LegacyFeeEstPath(argsman)};
    const fs::path block_policy_path{BlockPolicyFeeEstPath(argsman)};
    if (!fs::exists(legacy_path)) return;
    std::error_code error;
    if (fs::exists(block_policy_path)) {
        fs::remove(legacy_path, error);
        if (error) {
            LogWarning("Failed to remove legacy fee estimates file %s: %s. Continuing anyway.", fs::PathToString(legacy_path), error.message());
            return;
        }
        LogInfo("Removed legacy fee estimates file %s.", fs::PathToString(legacy_path));
        return;
    }
    fs::create_directories(block_policy_path.parent_path(), error);
    if (error) {
        LogWarning("Failed to create block policy fee estimates directory %s: %s. Continuing without migration.", fs::PathToString(block_policy_path.parent_path()), error.message());
        return;
    }
    fs::rename(legacy_path, block_policy_path, error);
    if (error) {
        LogWarning("Failed to migrate fee estimates from %s to %s: %s. Continuing with fresh estimates.", fs::PathToString(legacy_path), fs::PathToString(block_policy_path), error.message());
        return;
    }
    LogInfo("Migrated fee estimates from %s to %s.", fs::PathToString(legacy_path), fs::PathToString(block_policy_path));
}

fs::path BlockPolicyFeeEstPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / FEES_BASE_DIR / BLOCK_POLICY_ESTIMATES_FILENAME;
}

fs::path MempoolPolicyEstimatorPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / FEES_BASE_DIR / MEMPOOL_POLICY_ESTIMATOR_FILENAME;
}
