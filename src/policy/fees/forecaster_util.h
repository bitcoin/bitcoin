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
};

/**
 * @struct ForecastResult
 * Represents the response returned by a fee rate forecaster.
 */
struct ForecastResult {
    //! This identifies which forecaster is providing this feerate forecast
    ForecastType forecaster;

    //! Fee rate sufficient to confirm a package within target.
    FeeFrac feerate;

    //! The chain tip at which the forecast was made.
    unsigned int current_block_height{0};

    std::optional<std::string> m_error; ///< Optional error message.

    /**
     * Overloaded less than operator which compares two ForecastResult objects based on
     * their fee rate.
     * @param other The other ForecastResult object to compare with.
     * @return true if the current object's fee rate is less than the other, false otherwise.
     */
    bool operator<(const ForecastResult& other) const
    {
        return feerate << other.feerate;
    }
};

std::string forecastTypeToString(ForecastType forecastType);

#endif // BITCOIN_POLICY_FEES_FORECASTER_UTIL_H
