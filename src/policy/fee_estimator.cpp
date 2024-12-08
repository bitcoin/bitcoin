// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <policy/fee_estimator.h>
#include <policy/fees.h>
#include <policy/forecaster.h>
#include <policy/forecaster_util.h>
#include <txmempool.h>

#include <algorithm>
#include <utility>

FeeEstimator::FeeEstimator(const fs::path& block_policy_estimator_file_path, const bool read_stale_block_policy_estimates, const CTxMemPool* mempool)
    : m_mempool(mempool), block_policy_estimator(std::make_unique<CBlockPolicyEstimator>(block_policy_estimator_file_path, read_stale_block_policy_estimates))
{
}

FeeEstimator::FeeEstimator() : block_policy_estimator(std::nullopt) {}

void FeeEstimator::RegisterForecaster(std::unique_ptr<Forecaster> forecaster)
{
    forecasters.emplace(forecaster->GetForecastType(), std::move(forecaster));
}

ForecastResult FeeEstimator::GetPolicyEstimatorEstimate(ConfirmationTarget& target) const
{
    ForecastResponse response;
    response.forecaster = ForecastType::BLOCK_POLICY_ESTIMATOR;
    if (target.type != ConfirmationTargetType::BLOCKS) {
        return ForecastResult(response, "Forecaster can only provide an estimate for block targets");
    }
    bool conservative = true;
    FeeCalculation feeCalcConservative;
    CFeeRate feerate_conservative{block_policy_estimator.value()->estimateSmartFee(target.value, &feeCalcConservative, conservative)};
    FeeCalculation feeCalcEconomical;
    CFeeRate feerate_economical{block_policy_estimator.value()->estimateSmartFee(target.value, &feeCalcEconomical, !conservative)};
    response.current_block_height = feeCalcEconomical.bestheight;
    if (feerate_conservative == CFeeRate(0) || feerate_economical == CFeeRate(0)){
        return ForecastResult(response, "Insufficient data or no feerate found");
    }
    // Note: size can be any positive non-zero integer; the evaluated fee/size will result in the same fee rate,
    // and we only care that the fee rate remains consistent.
    int32_t size = 1000;
    response.low_priority = FeeFrac(feerate_economical.GetFee(size), size);
    response.high_priority = FeeFrac(feerate_conservative.GetFee(size), size);
    return ForecastResult(response);
}

FeeEstimator::~FeeEstimator() = default;

std::pair<std::optional<ForecastResult>, std::vector<std::string>> FeeEstimator::GetFeeEstimateFromForecasters(ConfirmationTarget& target)
{
    std::vector<std::string> err_messages;

    // Check for mempool availability
    if (m_mempool == nullptr) {
        err_messages.emplace_back("Mempool not available.");
        return {std::nullopt, err_messages};
    }

    {
        LOCK(m_mempool->cs);
        if (!m_mempool->GetLoadTried()) {
            err_messages.emplace_back("Mempool not finished loading; can't get accurate feerate forecast.");
            return {std::nullopt, err_messages};
        }
    }

    // Retrieve forecasts from policy estimator and mempool forecasters
    const auto policy_estimator_forecast = GetPolicyEstimatorEstimate(target);
    if (policy_estimator_forecast.empty()) {
        err_messages.emplace_back(strprintf("%s: %s", forecastTypeToString(policy_estimator_forecast.GetResponse().forecaster),
                                            policy_estimator_forecast.GetError().value_or("")));
    }

    auto mempool_forecaster = forecasters.find(ForecastType::MEMPOOL_FORECAST);
    Assume(mempool_forecaster != forecasters.end());
    const auto mempool_forecast = mempool_forecaster->second->EstimateFee(target);
    if (mempool_forecast.empty()) {
        err_messages.emplace_back(strprintf("%s: %s", forecastTypeToString(mempool_forecast.GetResponse().forecaster),
                                            mempool_forecast.GetError().value_or("")));
    }

    std::optional<ForecastResult> selected_forecast{std::nullopt};
    if (!policy_estimator_forecast.empty() && !mempool_forecast.empty()) {
        // Use the forecast with the lower fee rate when both forecasts are available
        selected_forecast = std::min(mempool_forecast, policy_estimator_forecast);
    } else if (!policy_estimator_forecast.empty()) {
        // Use the policy estimator forecast if mempool forecast is not available
        selected_forecast = policy_estimator_forecast;
    } // Note: If both are empty, selected_forecast remains nullopt

    if (selected_forecast) {
        const auto& forecast = *selected_forecast;
        LogDebug(BCLog::ESTIMATEFEE, "FeeEst %s: Block height %s, low priority feerate %s %s/kvB, high priority feerate %s %s/kvB.\n",
                 forecastTypeToString(forecast.GetResponse().forecaster),
                 forecast.GetResponse().current_block_height,
                 CFeeRate(forecast.GetResponse().low_priority.fee, forecast.GetResponse().low_priority.size).GetFeePerK(),
                 CURRENCY_ATOM,
                 CFeeRate(forecast.GetResponse().high_priority.fee, forecast.GetResponse().high_priority.size).GetFeePerK(),
                 CURRENCY_ATOM);
    }
    return {selected_forecast, err_messages};
}
