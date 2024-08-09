// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <policy/fee_estimator.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/forecaster.h>
#include <policy/forecaster_util.h>
#include <txmempool.h>
#include <util/trace.h>


void FeeEstimator::RegisterForecaster(std::shared_ptr<Forecaster> forecaster)
{
    forecasters.emplace(forecaster->m_forecastType, forecaster);
}

std::pair<ForecastResult, std::vector<std::string>> FeeEstimator::GetFeeEstimateFromForecasters(unsigned int targetBlocks)
{
    ForecastResult::ForecastOptions opts;
    ForecastResult forecast = ForecastResult(opts, std::nullopt);

    std::vector<std::string> err_messages;
    if (targetBlocks <= 0) {
        err_messages.emplace_back("Confirmation target must be greater than zero.");
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
    Assume(mempool_forecaster != forecasters.end());
    const auto mempool_forecast = (*mempool_forecaster).second->EstimateFee(targetBlocks);
    if (mempool_forecast.empty() && mempool_forecast.error_message != std::nullopt) {
        err_messages.emplace_back(strprintf("%s: %s", forecastTypeToString(mempool_forecast.opt.forecaster), mempool_forecast.error_message.value()));
    }
    const auto policy_estimator_forecast = GetPolicyEstimatorEstimate(targetBlocks);
    if (mempool_forecast.empty() || policy_estimator_forecast < mempool_forecast) {
        forecast = policy_estimator_forecast;
    } else {
        forecast = mempool_forecast;
    }
    if (!forecast.empty()) {
        LogPrint(BCLog::ESTIMATEFEE, "FeeEst %s: Block height %s, low priority feerate %s %s/kvB, high priority feerate %s %s/kvB.\n",
                 forecastTypeToString(forecast.opt.forecaster), forecast.opt.block_height, forecast.opt.low_priority_estimate.GetFeePerK(),
                 CURRENCY_ATOM, forecast.opt.high_priority_estimate.GetFeePerK(), CURRENCY_ATOM);
        TRACE5(fee_estimator, estimate_calculated,
               targetBlocks,
               forecastTypeToString(forecast.opt.forecaster).c_str(),
               forecast.opt.block_height,
               forecast.opt.low_priority_estimate.GetFeePerK(),
               forecast.opt.high_priority_estimate.GetFeePerK());
    }
    return std::make_pair(forecast, err_messages);
};

ForecastResult FeeEstimator::GetPolicyEstimatorEstimate(unsigned int targetBlocks)
{
    ForecastResult::ForecastOptions opts;
    bool conservative = true;
    FeeCalculation feeCalcConservative;
    CFeeRate feeRate_conservative{block_policy_estimator.value()->estimateSmartFee(targetBlocks, &feeCalcConservative, conservative)};
    opts.high_priority_estimate = feeRate_conservative;
    FeeCalculation feeCalcEconomical;
    CFeeRate feeRate_economical{block_policy_estimator.value()->estimateSmartFee(targetBlocks, &feeCalcEconomical, !conservative)};
    opts.low_priority_estimate = feeRate_economical;
    opts.forecaster = ForecastType::BLOCK_POLICY_ESTIMATOR;
    return ForecastResult(opts, std::nullopt);
}

unsigned int FeeEstimator::MaxForecastingTarget()
{
    unsigned int max_target = 0;
    for (auto& forecaster : forecasters) {
        max_target = std::max(forecaster.second->MaxTarget(), max_target);
    }
    if (block_policy_estimator.has_value()) {
        max_target = std::max(max_target, block_policy_estimator.value()->HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE));
    }
    return max_target;
}
