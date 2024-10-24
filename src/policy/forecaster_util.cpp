// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <policy/forecaster_util.h>
#include <policy/policy.h>


BlockPercentiles CalculateBlockPercentiles(
    const std::vector<std::tuple<CFeeRate, uint32_t>>& fee_rate_stats)
{
    if (fee_rate_stats.empty()) return BlockPercentiles();

    auto start{fee_rate_stats.crbegin()};
    auto end{fee_rate_stats.crend()};

    int total_weight{0};
    const auto p5_weight{static_cast<int>(0.05 * DEFAULT_BLOCK_MAX_WEIGHT)};
    const auto p25_weight{static_cast<int>(0.25 * DEFAULT_BLOCK_MAX_WEIGHT)};
    const auto p50_weight{static_cast<int>(0.5 * DEFAULT_BLOCK_MAX_WEIGHT)};
    const auto p75_weight{static_cast<int>(0.75 * DEFAULT_BLOCK_MAX_WEIGHT)};

    auto percentiles{BlockPercentiles()};
    for (auto rit = start; rit != end; ++rit) {
        total_weight += static_cast<int>(std::get<1>(*rit) * WITNESS_SCALE_FACTOR);
        if (total_weight >= p5_weight && percentiles.p5 == CFeeRate(0)) {
            percentiles.p5 = std::get<0>(*rit);
        }
        if (total_weight >= p25_weight && percentiles.p25 == CFeeRate(0)) {
            percentiles.p25 = std::get<0>(*rit);
        }
        if (total_weight >= p50_weight && percentiles.p50 == CFeeRate(0)) {
            percentiles.p50 = std::get<0>(*rit);
        }
        if (total_weight >= p75_weight && percentiles.p75 == CFeeRate(0)) {
            percentiles.p75 = std::get<0>(*rit);
        }
    }
    return percentiles;
}

std::string forecastTypeToString(ForecastType forecastType)
{
    switch (forecastType) {
    case ForecastType::MEMPOOL_FORECAST:
        return std::string("Mempool Forecast");
    case ForecastType::BLOCK_POLICY_ESTIMATOR:
        return std::string("Block Policy Estimator");
    }
    // no default case, so the compiler can warn about missing cases
    assert(false);
}
