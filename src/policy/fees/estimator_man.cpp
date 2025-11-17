// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/estimator.h>
#include <policy/fees/estimator_man.h>


void FeeRateEstimatorManager::RegisterFeeRateEstimator(std::unique_ptr<FeeRateEstimator> estimator)
{
    auto type = estimator->GetFeeRateEstimatorType();
    estimators.emplace(type, std::move(estimator));
}
