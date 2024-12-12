// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FORECASTER_UTIL_H
#define BITCOIN_POLICY_FORECASTER_UTIL_H

#include <util/feefrac.h>

#include <optional>
#include <string>

enum class ForecastType {
    MEMPOOL_FORECAST,
    BLOCK_POLICY_ESTIMATOR,
};

struct ForecastResponse {
    FeeFrac low_priority;
    FeeFrac high_priority;
    unsigned int current_block_height{0};
    ForecastType forecaster;
};

class ForecastResult
{
public:
    ForecastResult(ForecastResponse response, std::optional<std::string> error = std::nullopt)
        : m_response(std::move(response)), m_error(std::move(error)) {}

    bool empty() const
    {
        return m_response.low_priority.IsEmpty() && m_response.high_priority.IsEmpty();
    }

    bool operator<(const ForecastResult& other) const
    {
        return m_response.high_priority < other.m_response.high_priority;
    }

    const ForecastResponse& GetResponse() const
    {
        return m_response;
    }

    const std::optional<std::string>& GetError() const
    {
        return m_error;
    }

private:
    ForecastResponse m_response;
    std::optional<std::string> m_error;
};

enum class ConfirmationTargetType {
    BLOCKS,
};

struct ConfirmationTarget {
    unsigned int value;
    ConfirmationTargetType type;
};

// Block percentiles fee rate (in sat/kvB).
struct Percentiles {
    FeeFrac p25; // 25th percentile
    FeeFrac p50; // 50th percentile
    FeeFrac p75; // 75th percentile
    FeeFrac p95; // 5th percentile

    Percentiles() = default;

    bool empty() const
    {
        return p25.IsEmpty() && p50.IsEmpty() && p75.IsEmpty() && p95.IsEmpty();
    }
};

/**
 * Calculates the percentile fee rates from a given vector of fee rates.
 *
 * This function assumes that the fee rates in the input vector are sorted in descending order
 * based on mining score priority. It ensures that the calculated percentile fee rates
 * are monotonically decreasing by filtering out outliers. Outliers can occur when
 * the mining score of a transaction increases due to the inclusion of its ancestors
 * in different transaction packages.
 *
 * @param[in] feerateHistogram A vector containing fee rates and their corresponding virtual sizes.
 * @return Percentiles object containing the calculated percentile fee rates.
 */
Percentiles CalculatePercentiles(const std::vector<FeeFrac>& feerateHistogram, const int32_t total_weight);


std::string forecastTypeToString(ForecastType forecastType);

#endif // BITCOIN_POLICY_FORECASTER_UTIL_H