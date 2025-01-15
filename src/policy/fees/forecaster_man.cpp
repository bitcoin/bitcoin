// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/forecaster.h>
#include <policy/fees/forecaster_man.h>
#include <policy/fees/forecaster_util.h>

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
