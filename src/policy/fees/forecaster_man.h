// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_MAN_H
#define BITCOIN_POLICY_FEES_FORECASTER_MAN_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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

    /**
     * Get a fee rate forecast from all registered forecasters for a given confirmation target.
     *
     * Polls all registered forecasters and selects the lowest fee rate.
     *
     * @param[in] target The target within which the transaction should be confirmed.
     * @param[in] conservative If true, returns a higher fee rate for greater confirmation probability.
     * @return A pair consisting of the forecast result and a vector of error messages.
     */
    std::pair<ForecastResult, std::vector<std::string>> ForecastFeeRateFromForecasters(int target, bool conservative) const;

    /**
     * @brief Returns the maximum supported confirmation target from all forecasters.
     */
    unsigned int MaximumTarget() const;
};

#endif // BITCOIN_POLICY_FEES_FORECASTER_MAN_H
