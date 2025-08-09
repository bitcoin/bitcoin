// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_H
#define BITCOIN_POLICY_FEES_FORECASTER_H

#include <policy/fees/forecaster_util.h>

/** \class Forecaster
 *  @brief Abstract base class for fee rate forecasters.
 */
class Forecaster
{
protected:
    const ForecastType m_forecast_type;

public:
    explicit Forecaster(ForecastType forecast_type) : m_forecast_type(forecast_type) {}

    ForecastType GetForecastType() const { return m_forecast_type; }

    /**
     * @brief Returns the estimated fee rate for a package to confirm within the given target.
     * @param target Confirmation target in blocks.
     * @param conservative If true, returns a higher fee rate for greater confirmation probability.
     * @return Predicted fee rate.
     */
    virtual ForecastResult ForecastFeeRate(int target, bool conservative) const = 0;

    /**
     * @brief Returns the maximum supported confirmation target.
     */
    virtual unsigned int MaximumTarget() const = 0;

    virtual ~Forecaster() = default;
};

#endif // BITCOIN_POLICY_FEES_FORECASTER_H
