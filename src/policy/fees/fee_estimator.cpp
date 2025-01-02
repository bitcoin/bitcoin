// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/fee_estimator.h>
#include <policy/fees.h>
#include <policy/fees/forecaster.h>
#include <policy/fees/forecaster_util.h>

#include <utility>

FeeEstimator::FeeEstimator(const fs::path& block_policy_estimator_file_path, const bool read_stale_block_policy_estimates)
    : block_policy_estimator(std::make_unique<CBlockPolicyEstimator>(block_policy_estimator_file_path, read_stale_block_policy_estimates))
{
}

FeeEstimator::FeeEstimator() : block_policy_estimator(std::nullopt) {}

void FeeEstimator::RegisterForecaster(std::unique_ptr<Forecaster>&& forecaster)
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
    if (feerate_conservative == CFeeRate(0) || feerate_economical == CFeeRate(0)) {
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
