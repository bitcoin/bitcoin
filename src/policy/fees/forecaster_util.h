// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_UTIL_H
#define BITCOIN_POLICY_FEES_FORECASTER_UTIL_H

#include <primitives/transaction.h>
#include <util/feefrac.h>
#include <util/transaction_identifier.h>

#include <optional>
#include <set>
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
 * @class ForecastResult
 * Represents the response returned by a fee rate forecaster.
 */
class ForecastResult
{
public:
    /**
     * @struct ForecastResponse
     * Contains fee rate forecast metadata.
     */
    struct ForecastResponse {
        /**
         * An estimated fee rate as a result of a forecast for economical transactions.
         * This value is sufficient to confirm a package within the specified target.
         */
        FeeFrac low_priority;

        /**
         * An estimated fee rate as a result of forecast for high-priority transactions.
         * This value is for users willing to pay more to increase the likelihood of
         * confirmation within the specified target.
         */
        FeeFrac high_priority;

        /**
         * The chain tip at which the forecast was made.
         */
        unsigned int current_block_height{0};
        /* This identifies which forecaster is providing this feerate forecast */
        ForecastType forecaster;
    };

    /**
     * Default ForecastResult constructor.
     */
    ForecastResult() {}

    /**
     * Constructs a ForecastResult object.
     * @param response The forecast response data.
     * @param error An optional error message (default: nullopt).
     */
    ForecastResult(ForecastResponse response, std::optional<std::string> error = std::nullopt)
        : m_response(std::move(response)), m_error(std::move(error))
    {
    }

    /**
     * Checks if the forecast response is empty.
     * @return true if both low and high priority forecast estimates are empty, false otherwise.
     */
    bool Empty() const
    {
        return m_response.low_priority.IsEmpty() && m_response.high_priority.IsEmpty();
    }

    /**
     * Overloaded less than operator which compares two ForecastResult objects based on
     * their high priority estimates.
     * @param other The other ForecastResult object to compare with.
     * @return true if the current object's high priority estimate is less than the other, false otherwise.
     */
    bool operator<(const ForecastResult& other) const
    {
        return m_response.high_priority << other.m_response.high_priority;
    }

    /**
     * Retrieves the forecast response.
     * @return A reference to the forecast response metadata.
     */
    const ForecastResponse& GetResponse() const
    {
        return m_response;
    }

    /**
     * Retrieves the error message, if any.
     * @return An optional string containing the error message.
     */
    const std::optional<std::string>& GetError() const
    {
        return m_error;
    }

private:
    ForecastResponse m_response;        ///< The forecast response data.
    std::optional<std::string> m_error; ///< Optional error message.
};

/**
 * @enum ConfirmationTargetType
 * Defines the types of confirmation targets for fee rate forecasters.
 */
enum class ConfirmationTargetType {
    BLOCKS, /**< Forecasters providing estimates for a specific number of blocks use this type. */
};

/**
 * @struct ConfirmationTarget
 * Represents the input for a parameter of fee rate forecaster.
 */
struct ConfirmationTarget {
    unsigned int value;
    ConfirmationTargetType type;
    std::set<Txid> transactions_to_ignore{};
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
 * @param[in] package_feerates A vector containing fee rates and their corresponding virtual sizes.
 * @return Percentiles object containing the calculated percentile fee rates.
 */
Percentiles CalculatePercentiles(const std::vector<FeeFrac>& package_feerates, const int32_t total_weight);

std::string forecastTypeToString(ForecastType forecastType);

std::vector<FeeFrac> FilterPackages(std::vector<FeeFrac>& package_feerates,
                                    std::set<Txid>& transactions_to_ignore,
                                    const std::vector<CTransactionRef>& block_txs);

#endif // BITCOIN_POLICY_FEES_FORECASTER_UTIL_H
