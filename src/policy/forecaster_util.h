// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FORECASTER_UTIL_H
#define BITCOIN_POLICY_FORECASTER_UTIL_H

#include <policy/feerate.h>

#include <optional>
#include <string>

enum class ForecastType {
    MEMPOOL_FORECAST,
    BLOCK_POLICY_ESTIMATOR,
};

struct ForecastResult {
    struct ForecastOptions {
        CFeeRate low_priority;
        CFeeRate high_priority;
        unsigned int block_height{0};
        ForecastType forecaster;
    };

    ForecastOptions m_opt;
    std::optional<std::string> m_error_ptr;

    ForecastResult(ForecastResult::ForecastOptions& options, const std::optional<std::string> error_ptr)
        : m_opt(options), m_error_ptr(error_ptr) {}

    bool empty() const
    {
        return m_opt.low_priority == CFeeRate() && m_opt.high_priority == CFeeRate();
    }

    bool operator<(const ForecastResult& forecast) const
    {
        return m_opt.high_priority < forecast.m_opt.high_priority;
    }

    ~ForecastResult() = default;
};

// Block percentiles fee rate (in sat/kvB).
struct BlockPercentiles {
    CFeeRate p5;  // 5th percentile
    CFeeRate p25; // 25th percentile
    CFeeRate p50; // 50th percentile
    CFeeRate p75; // 75th percentile

    // Default constructor that default initializes percentile fee rates.
    BlockPercentiles() = default;

    // Check if all percentiles are empty.
    bool empty() const
    {
        CFeeRate empty;
        return p5 == empty && p25 == empty && p50 == empty && p75 == empty;
    }
};

/**
 * Calculate the percentiles feerates.
 *
 * @param[in] fee_rate_stats The fee statistics (fee rate and vsize).
 * @return BlockPercentiles of a given fee statistics.
 * This method assumes fee_rate_stats is sorted by fee rate in descending order.
 */
BlockPercentiles CalculateBlockPercentiles(const std::vector<std::tuple<CFeeRate, uint32_t>>& fee_rate_stats);

std::string forecastTypeToString(ForecastType forecastType);

#endif // BITCOIN_POLICY_FORECASTER_UTIL_H
