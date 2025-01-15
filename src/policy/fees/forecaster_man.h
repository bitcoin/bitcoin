// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_MAN_H
#define BITCOIN_POLICY_FEES_FORECASTER_MAN_H

#include <memory>
#include <unordered_map>

class CBlockPolicyEstimator;
class Forecaster;
struct ForecastResult;

enum class ForecastType;

/** \class FeeRateForecasterManager
 * Manages multiple fee rate forecasters.
 */
class FeeRateForecasterManager
{
private:
    //! Map of all registered forecasters to their shared pointers.
    std::unordered_map<ForecastType, std::shared_ptr<Forecaster>> forecasters;

public:
    /**
     * Register a forecaster.
     * @param[in] forecaster shared pointer to a Forecaster instance.
     */
    void RegisterForecaster(std::shared_ptr<Forecaster> forecaster);

    /*
     * Return the pointer to block policy estimator.
     */
    CBlockPolicyEstimator* GetBlockPolicyEstimator();
};

#endif // BITCOIN_POLICY_FEES_FORECASTER_MAN_H
