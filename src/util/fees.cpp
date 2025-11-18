// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/policy.h>
#include <util/feefrac.h>
#include <util/fees.h>

Percentiles CalculatePercentiles(const std::vector<FeePerVSize>& chunk_feerates, const int32_t total_weight)
{
    if (chunk_feerates.empty()) return Percentiles{};
    Assume(std::is_sorted(chunk_feerates.begin(), chunk_feerates.end(), std::greater<FeePerVSize>()));
    const int32_t p25_weight = 0.25 * total_weight;
    const int32_t p50_weight = 0.50 * total_weight;
    const int32_t p75_weight = 0.75 * total_weight;
    const int32_t p95_weight = 0.95 * total_weight;
    Percentiles percentiles{};
    int32_t accumulated_weight{0};
    for (const auto& curr_feerate : chunk_feerates) {
        accumulated_weight += curr_feerate.size * WITNESS_SCALE_FACTOR;
        if (accumulated_weight >= p25_weight && percentiles.p25.IsEmpty()) {
            percentiles.p25 = curr_feerate;
        }
        if (accumulated_weight >= p50_weight && percentiles.p50.IsEmpty()) {
            percentiles.p50 = curr_feerate;
        }
        if (accumulated_weight >= p75_weight && percentiles.p75.IsEmpty()) {
            percentiles.p75 = curr_feerate;
        }
        if (accumulated_weight >= p95_weight && percentiles.p95.IsEmpty()) {
            percentiles.p95 = curr_feerate;
            break; // Early exit once all percentiles are calculated
        }
    }
    // Return empty percentiles if we couldn't calculate the 95th percentile.
    return percentiles.p95.IsEmpty() ? Percentiles{} : percentiles;
}
