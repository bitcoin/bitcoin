// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FORECASTER_H
#define BITCOIN_POLICY_FORECASTER_H

#include <policy/forecaster_util.h>

/** \class Forecaster
 * Abstract base class for fee rate forecasters.
 *
 * Derived classes must provide concrete implementations for virtual method.
 */
class Forecaster
{
protected:
    const ForecastType m_forecastType;

public:
    Forecaster(ForecastType forecastType) : m_forecastType(forecastType) {}

    ForecastType GetForecastType() const { return m_forecastType; }

    /**
     * Estimate the fee rate required for transaction confirmation
      for a given target.
     *
     * This pure virtual function must be overridden by derived classes
     * to provide a ForecastResult for the specified target.
     *
     * @return ForecastResult containing the estimated fee rate.
     */
    virtual ForecastResult EstimateFee(ConfirmationTarget& target) = 0;

    virtual ~Forecaster() = default;
};

#endif // BITCOIN_POLICY_FORECASTER_H
