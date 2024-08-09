// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FORECASTER_UTIL_H
#define BITCOIN_POLICY_FORECASTER_UTIL_H

#include <policy/feerate.h>

#include <optional>
#include <string>
#include <tuple>
#include <vector>

enum class ForecastType {
    MEMPOOL_FORECAST,
    BLOCK_POLICY_ESTIMATOR,
};

struct ForecastResult {
    struct ForecastOptions {
        CFeeRate low_priority_estimate{CFeeRate(0)};
        CFeeRate high_priority_estimate{CFeeRate(0)};
        unsigned int block_height{0};
        ForecastType forecaster;
    };

    ForecastOptions opt;
    std::optional<std::string> error_message;

    ForecastResult(ForecastResult::ForecastOptions& options, const std::optional<std::string> error_ptr)
        : opt(options), error_message(error_ptr) {}

    bool empty() const
    {
        return opt.low_priority_estimate == CFeeRate(0) && opt.high_priority_estimate == CFeeRate(0);
    }

    bool operator<(const ForecastResult& forecast) const
    {
        return opt.low_priority_estimate < forecast.opt.high_priority_estimate;
    }

    ~ForecastResult() = default;
};

// Block percentiles fee rate (in sat/kvB).
struct BlockPercentiles {
    CFeeRate p5;  // 5th percentile
    CFeeRate p25; // 25th percentile
    CFeeRate p50; // 50th percentile
    CFeeRate p75; // 75th percentile

    // Default constructor initializes all percentiles to CFeeRate(0).
    BlockPercentiles() : p5(CFeeRate(0)), p25(CFeeRate(0)), p50(CFeeRate(0)), p75(CFeeRate(0)) {}

    // Check if all percentiles are CFeeRate(0).
    bool empty() const
    {
        return p5 == CFeeRate(0) && p25 == CFeeRate(0) && p50 == CFeeRate(0) && p75 == CFeeRate(0);
    }
};

/**
 * Calculate the percentiles feerates.
 *
 * @param[in] fee_rate_stats The fee statistics (fee rate and vsize).
 * @return BlockPercentiles of a given fee statistics.
 */
BlockPercentiles CalculateBlockPercentiles(const std::vector<std::tuple<CFeeRate, uint32_t>>& fee_rate_stats);

std::string forecastTypeToString(ForecastType forecastType);

#endif // BITCOIN_POLICY_FORECASTER_UTIL_H
