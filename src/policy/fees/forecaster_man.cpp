// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/forecaster.h>
#include <policy/fees/forecaster_man.h>
#include <policy/fees/forecaster_util.h>

#include <algorithm>
#include <utility>

void FeeRateForecasterManager::RegisterForecaster(std::shared_ptr<Forecaster> forecaster)
{
    forecasters.emplace(forecaster->GetForecastType(), forecaster);
}

CBlockPolicyEstimator* FeeRateForecasterManager::GetBlockPolicyEstimator()
{
    Assert(forecasters.contains(ForecastType::BLOCK_POLICY));
    Forecaster* block_policy_estimator = forecasters.find(ForecastType::BLOCK_POLICY)->second.get();
    return dynamic_cast<CBlockPolicyEstimator*>(block_policy_estimator);
}

std::pair<ForecastResult, std::vector<std::string>> FeeRateForecasterManager::ForecastFeeRateFromForecasters(
    int target, bool conservative) const
{
    std::vector<std::string> err_messages;
    ForecastResult feerate_forecast;

    for (const auto& forecaster : forecasters) {
        auto curr_forecast = forecaster.second->ForecastFeeRate(target, conservative);

        if (curr_forecast.error.has_value()) {
            err_messages.emplace_back(
                strprintf("%s: %s", forecastTypeToString(forecaster.first), curr_forecast.error.value()));
        }

        // Handle case where the block policy forecaster does not have enough data.
        if (forecaster.first == ForecastType::BLOCK_POLICY && curr_forecast.feerate.IsEmpty()) {
            return {ForecastResult(), err_messages};
        }

        if (!curr_forecast.feerate.IsEmpty()) {
            if (feerate_forecast.feerate.IsEmpty()) {
                // If there's no selected forecast, choose curr_forecast as feerate_forecast.
                feerate_forecast = curr_forecast;
            } else {
                // Otherwise, choose the smaller as feerate_forecast.
                feerate_forecast = std::min(feerate_forecast, curr_forecast);
            }
        }
    }

    if (!feerate_forecast.feerate.IsEmpty()) {
        Assume(feerate_forecast.forecaster);
        LogInfo("Fee rate Forecaster %s: Block height %s, fee rate %s %s/kvB.\n",
                forecastTypeToString(feerate_forecast.forecaster.value()),
                feerate_forecast.current_block_height,
                CFeeRate(feerate_forecast.feerate.fee, feerate_forecast.feerate.size).GetFeePerK(),
                CURRENCY_ATOM);
    }

    return {feerate_forecast, err_messages};
}

unsigned int FeeRateForecasterManager::MaximumTarget() const
{
    unsigned int maximum_target{0};
    for (const auto& forecaster : forecasters) {
        maximum_target = std::max(maximum_target, forecaster.second->MaximumTarget());
    }
    return maximum_target;
}
