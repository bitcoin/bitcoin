// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <policy/fee_estimator.h>
#include <policy/forecaster.h>
#include <policy/forecaster_util.h>
#include <txmempool.h>
#include <util/trace.h>

void FeeEstimator::RegisterForecaster(std::unique_ptr<Forecaster> forecaster)
{
    forecasters.emplace(forecaster->GetForecastType(), std::move(forecaster));
}

int FeeEstimator::MaxForecastingTarget()
{
    int max_target{0};
    for (const auto& forecaster : forecasters) {
        max_target = std::max(forecaster.second->MaxTarget(), max_target);
    }
    if (block_policy_estimator.has_value()) {
        const auto policy_estimator_max_target{static_cast<int>(block_policy_estimator.value()->HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE))};
        max_target = std::max(max_target, policy_estimator_max_target);
    }
    return max_target;
}

ForecastResult FeeEstimator::GetPolicyEstimatorEstimate(int targetBlocks)
{
    ForecastResult::ForecastOptions forecast_options;
    bool conservative = true;
    FeeCalculation feeCalcConservative;
    CFeeRate feeRate_conservative{block_policy_estimator.value()->estimateSmartFee(targetBlocks, &feeCalcConservative, conservative)};
    forecast_options.high_priority = feeRate_conservative;
    FeeCalculation feeCalcEconomical;
    CFeeRate feeRate_economical{block_policy_estimator.value()->estimateSmartFee(targetBlocks, &feeCalcEconomical, !conservative)};
    forecast_options.low_priority = feeRate_economical;
    forecast_options.forecaster = ForecastType::BLOCK_POLICY_ESTIMATOR;
    forecast_options.block_height = feeCalcEconomical.bestheight;
    return ForecastResult(forecast_options, std::nullopt);
}

std::pair<ForecastResult, std::vector<std::string>> FeeEstimator::GetFeeEstimateFromForecasters(int targetBlocks)
{
    ForecastResult::ForecastOptions forecast_options;
    ForecastResult forecast = ForecastResult(forecast_options, std::nullopt);
    std::vector<std::string> err_messages;

    const auto maximum_conf_target{MaxForecastingTarget()};
    if (targetBlocks > maximum_conf_target) {
        err_messages.emplace_back(strprintf("Confirmation target %s exceeds forecasters maximum target of %s.", targetBlocks, maximum_conf_target));
        return std::make_pair(forecast, err_messages);
    }

    if (m_mempool == nullptr) {
        err_messages.emplace_back("Mempool not available.");
        return std::make_pair(forecast, err_messages);
    }

    {
        LOCK(m_mempool->cs);
        if (!m_mempool->GetLoadTried()) {
            err_messages.emplace_back("Mempool not finished loading; can't get accurate feerate forecast");
            return std::make_pair(forecast, err_messages);
        }

        if (m_mempool->size() == 0) {
            err_messages.emplace_back("No transactions available in the mempool");
            return std::make_pair(forecast, err_messages);
        }
    }

    auto mempool_forecaster = forecasters.find(ForecastType::MEMPOOL_FORECAST);
    if (mempool_forecaster != forecasters.end()) {
        forecast = (*mempool_forecaster).second->EstimateFee(targetBlocks);
        if (forecast.empty() && forecast.m_error_ptr.has_value()) {
            err_messages.emplace_back(strprintf("%s: %s", forecastTypeToString(forecast.m_opt.forecaster), forecast.m_error_ptr.value()));
        }
    }

    const auto policy_estimator_forecast = GetPolicyEstimatorEstimate(targetBlocks);
    if (forecast.empty() || policy_estimator_forecast < forecast) {
        forecast = policy_estimator_forecast;
    }
    if (!forecast.empty()) {
        LogDebug(BCLog::ESTIMATEFEE, "FeeEst %s: Block height %s, low priority feerate %s %s/kvB, high priority feerate %s %s/kvB.\n",
                 forecastTypeToString(forecast.m_opt.forecaster), forecast.m_opt.block_height, forecast.m_opt.low_priority.GetFeePerK(),
                 CURRENCY_ATOM, forecast.m_opt.high_priority.GetFeePerK(), CURRENCY_ATOM);
        TRACE5(fee_estimator, estimate_calculated,
               targetBlocks,
               forecastTypeToString(forecast.m_opt.forecaster).c_str(),
               forecast.m_opt.block_height,
               forecast.m_opt.low_priority.GetFeePerK(),
               forecast.m_opt.high_priority.GetFeePerK());
    }
    return std::make_pair(forecast, err_messages);
};
