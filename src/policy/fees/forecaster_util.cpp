// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/forecaster_util.h>
#include <policy/policy.h>

#include <algorithm>

Percentiles CalculatePercentiles(const std::vector<FeeFrac>& package_feerates, const int32_t total_weight)
{
    if (package_feerates.empty()) return Percentiles{};
    int32_t accumulated_weight{0};
    const int32_t p25_weight = 0.25 * total_weight;
    const int32_t p50_weight = 0.50 * total_weight;
    const int32_t p75_weight = 0.75 * total_weight;
    const int32_t p95_weight = 0.95 * total_weight;
    auto last_tracked_feerate = package_feerates.front();
    auto percentiles = Percentiles{};

    // Process histogram entries while maintaining monotonicity
    for (const auto& curr_feerate : package_feerates) {
        accumulated_weight += curr_feerate.size * WITNESS_SCALE_FACTOR;
        // Maintain monotonicity by taking the minimum between the current and last tracked fee rate
        last_tracked_feerate = std::min(last_tracked_feerate, curr_feerate, [](const FeeFrac& a, const FeeFrac& b) {
            return std::is_lt(FeeRateCompare(a, b));
        });
        if (accumulated_weight >= p25_weight && percentiles.p25.IsEmpty()) {
            percentiles.p25 = last_tracked_feerate;
        }
        if (accumulated_weight >= p50_weight && percentiles.p50.IsEmpty()) {
            percentiles.p50 = last_tracked_feerate;
        }
        if (accumulated_weight >= p75_weight && percentiles.p75.IsEmpty()) {
            percentiles.p75 = last_tracked_feerate;
        }
        if (accumulated_weight >= p95_weight && percentiles.p95.IsEmpty()) {
            percentiles.p95 = last_tracked_feerate;
            break; // Early exit once all percentiles are calculated
        }
    }

    // Return empty percentiles if we couldn't calculate the 95th percentile.
    return percentiles.p95.IsEmpty() ? Percentiles{} : percentiles;
}

std::string forecastTypeToString(ForecastType forecastType)
{
    switch (forecastType) {
    case ForecastType::MEMPOOL_FORECAST:
        return std::string("Mempool Forecast");
    case ForecastType::BLOCK_POLICY:
        return std::string("Block Policy Estimator");
    }
    // no default case, so the compiler can warn about missing cases
    assert(false);
}
