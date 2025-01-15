// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_MAN_H
#define BITCOIN_POLICY_FEES_FORECASTER_MAN_H

#include <memory>
#include <optional>
#include <unordered_map>

class CBlockPolicyEstimator;
class Forecaster;
class ForecastResult;

struct ConfirmationTarget;

enum class ForecastType;

/** \class FeeRateForecasterManager
 * Module for managing and utilising multiple fee rate forecasters to provide a forecast for a target.
 *
 * The FeeRateForecasterManager class allows for the registration of multiple fee rate
 * forecasters.
 */
class FeeRateForecasterManager
{
private:
    //! Map of all registered forecasters to their shared pointers.
    std::unordered_map<ForecastType, std::shared_ptr<Forecaster>> forecasters;

public:
    /**
     * Register a forecaster to provide fee rate estimates.
     *
     * @param[in] forecaster shared pointer to a Forecaster instance.
     */
    void RegisterForecaster(std::shared_ptr<Forecaster> forecaster);

    /*
     * Return the pointer to block policy estimator.
     */
    CBlockPolicyEstimator* GetBlockPolicyEstimator();

    /**
     * Get a fee rate estimate from all registered forecasters for a given confirmation target.
     *
     * Polls all registered forecasters and selects the lowest fee rate
     * estimate with acceptable confidence.
     *
     * @param[in] target The target within which the transaction should be confirmed.
     * @return A pair consisting of the forecast result and a vector of forecaster names.
     */
    std::pair<std::optional<ForecastResult>, std::vector<std::string>> GetFeeEstimateFromForecasters(ConfirmationTarget& target);
};

#endif // BITCOIN_POLICY_FEES_FORECASTER_MAN_H
