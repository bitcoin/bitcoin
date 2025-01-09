// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_H
#define BITCOIN_POLICY_FEES_FORECASTER_H

#include <policy/fees/forecaster_util.h>

/** \class Forecaster
 * Abstract base class for fee rate forecasters.
 */
class Forecaster
{
protected:
    const ForecastType m_forecastType;

public:
    Forecaster(ForecastType forecastType) : m_forecastType(forecastType) {}

    ForecastType GetForecastType() const { return m_forecastType; }

    /**
     * Forecast the fee rate required for package to confirm within
     * a specified target.
     *
     * This pure virtual function must be overridden by derived classes.
     * @param target The confirmation target for which to forecast the feerate for.
     * @return ForecastResult containing the forecasted fee rate.
     */
    virtual ForecastResult EstimateFee(ConfirmationTarget& target) = 0;

    virtual ~Forecaster() = default;
};

#endif // BITCOIN_POLICY_FEES_FORECASTER_H
