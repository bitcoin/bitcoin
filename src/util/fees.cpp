// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/check.h>
#include <util/fees.h>

#include <util/strencodings.h>

#include <optional>
#include <string_view>

std::string_view FeeRateEstimatorTypeToString(FeeRateEstimatorType feerate_estimator_type)
{
    switch (feerate_estimator_type) {
    case FeeRateEstimatorType::AUTO:
        return "auto";
    case FeeRateEstimatorType::BLOCK_POLICY:
        return "block_policy";
    case FeeRateEstimatorType::MEMPOOL_POLICY:
        return "mempool_policy";
    }
    // no default case, so the compiler can warn about missing cases
    AssertUnreachable();
}

std::optional<FeeRateEstimatorType> FeeRateEstimatorTypeFromString(std::string_view feerate_estimator_type)
{
    const auto normalized{ToLower(feerate_estimator_type)};
    if (normalized == "auto") return FeeRateEstimatorType::AUTO;
    if (normalized == "block_policy") return FeeRateEstimatorType::BLOCK_POLICY;
    if (normalized == "mempool_policy") return FeeRateEstimatorType::MEMPOOL_POLICY;
    return std::nullopt;
}
