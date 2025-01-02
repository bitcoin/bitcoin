// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_UTIL_H
#define BITCOIN_POLICY_FEES_FORECASTER_UTIL_H

#include <util/feefrac.h>

#include <optional>
#include <string>

enum class ForecastType {
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

std::string forecastTypeToString(ForecastType forecastType);

#endif // BITCOIN_POLICY_FEES_FORECASTER_UTIL_H
