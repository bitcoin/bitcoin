// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_UTIL_H
#define BITCOIN_POLICY_FEES_FORECASTER_UTIL_H

#include <util/feefrac.h>

#include <optional>
#include <string>

/**
 * @enum ForecastType
 * Identifier for fee rate forecasters.
 */
enum class ForecastType {
    BLOCK_POLICY,
    MEMPOOL_FORECAST,
};

/**
 * @struct ForecastResult
 * Represents the response returned by a fee rate forecaster.
 */
struct ForecastResult {
    //! This identifies which forecaster is providing this feerate forecast
    std::optional<ForecastType> forecaster;

    //! Fee rate sufficient to confirm a package within target.
    FeeFrac feerate;

    //! The block height at which the forecast was made.
    unsigned int current_block_height{0};

    //! The confirmation target the forecast was made for.
    unsigned int returned_target{0};

    std::optional<std::string> error; ///< Optional error message.

    /**
     * Compare two ForecastResult objects based on fee rate.
     * @param other The other ForecastResult object to compare with.
     * @return true if the current object's fee rate is less than the other, false otherwise.
     */
    bool operator<(const ForecastResult& other) const
    {
        return feerate << other.feerate;
    }
};

// Block percentiles fee rate (in BTC/vB).
struct Percentiles {
    FeeFrac p25;
    FeeFrac p50;
    FeeFrac p75;
    FeeFrac p95;

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
 * @param[in] package_feerates A vector containing packages fee rates.
 * @param[in] total_weight The total weight units to use for percentile fee rate calculations.
 * @return Percentiles object containing the calculated percentile fee rates.
 */
Percentiles CalculatePercentiles(const std::vector<FeeFrac>& package_feerates, const int32_t total_weight);

std::string forecastTypeToString(ForecastType forecastType);

#endif // BITCOIN_POLICY_FEES_FORECASTER_UTIL_H
