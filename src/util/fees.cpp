// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/fees.h>

#include <util/strencodings.h>

#include <cassert>

std::string FeeRateEstimatorTypeToString(FeeRateEstimatorType feerate_estimator_type)
{
    switch (feerate_estimator_type) {
    case FeeRateEstimatorType::NONE:
        return "None";
    case FeeRateEstimatorType::BLOCK_POLICY:
        return "Block Policy Fee Rate Estimator";
    case FeeRateEstimatorType::MEMPOOL_POLICY:
        return "Mempool Fee Rate Estimator";
    }
    // no default case, so the compiler can warn about missing cases
    assert(false);
}

FeeRateEstimatorType FeeRateEstimatorTypeFromString(std::string_view feerate_estimator_type)
{
    const auto normalized{ToLower(feerate_estimator_type)};
    if (normalized == "block_policy") return FeeRateEstimatorType::BLOCK_POLICY;
    return FeeRateEstimatorType::NONE;
}
