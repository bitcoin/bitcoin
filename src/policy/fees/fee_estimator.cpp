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

FeeEstimator::~FeeEstimator() = default;
