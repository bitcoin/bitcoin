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
    Assume(forecasters.contains(ForecastType::BLOCK_POLICY));
    Forecaster* block_policy_estimator = forecasters.find(ForecastType::BLOCK_POLICY)->second.get();
    return dynamic_cast<CBlockPolicyEstimator*>(block_policy_estimator);
}

std::pair<std::optional<ForecastResult>, std::vector<std::string>> FeeRateForecasterManager::GetFeeEstimateFromForecasters(ConfirmationTarget& target)
{
    std::vector<std::string> err_messages;
    ForecastResult selected_forecast;

    for (const auto& forecaster : forecasters) {
        auto curr_forecast = forecaster.second->EstimateFee(target);

        if (curr_forecast.GetError().has_value()) {
            err_messages.emplace_back(
                strprintf("%s: %s", forecastTypeToString(forecaster.first), curr_forecast.GetError().value_or("")));
        }

        // Handle case where the block policy estimator does not have enough data for fee estimates.
        if (curr_forecast.Empty() && forecaster.first == ForecastType::BLOCK_POLICY) {
            return {std::nullopt, err_messages};
        }

        if (!curr_forecast.Empty()) {
            if (selected_forecast.Empty()) {
                // If there's no selected forecast, choose curr_forecast as selected_forecast.
                selected_forecast = curr_forecast;
            } else {
                // Otherwise, choose the smaller as selected_forecast.
                selected_forecast = std::min(selected_forecast, curr_forecast);
            }
        }
    }

    if (!selected_forecast.Empty()) {
        LogDebug(BCLog::ESTIMATEFEE,
                 "FeeEst %s: Block height %s, low priority fee rate %s %s/kvB, high priority fee rate %s %s/kvB.\n",
                 forecastTypeToString(selected_forecast.GetResponse().forecaster),
                 selected_forecast.GetResponse().current_block_height,
                 CFeeRate(selected_forecast.GetResponse().low_priority.fee, selected_forecast.GetResponse().low_priority.size).GetFeePerK(),
                 CURRENCY_ATOM,
                 CFeeRate(selected_forecast.GetResponse().high_priority.fee, selected_forecast.GetResponse().high_priority.size).GetFeePerK(),
                 CURRENCY_ATOM);
    }

    return {selected_forecast, err_messages};
}
