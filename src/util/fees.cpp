// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/fees.h>

#include <util/strencodings.h>

FeeRateEstimatorType FeeRateEstimatorTypeFromString(std::string_view feerate_estimator_type)
{
    const auto normalized{ToLower(feerate_estimator_type)};
    if (normalized == "block_policy") return FeeRateEstimatorType::BLOCK_POLICY;
    return FeeRateEstimatorType::NONE;
}
