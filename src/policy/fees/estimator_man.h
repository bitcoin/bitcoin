// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_ESTIMATOR_MAN_H
#define BITCOIN_POLICY_FEES_ESTIMATOR_MAN_H

#include <policy/fees/estimator.h>
#include <util/fees.h>

#include <memory>
#include <unordered_map>


enum class FeeRateEstimatorType;

/** \class FeeRateEstimatorManager
 * Manages multiple fee rate estimators.
 */
class FeeRateEstimatorManager
{
private:
    //! Map of all registered estimators to their unique pointers.
    std::unordered_map<FeeRateEstimatorType, std::unique_ptr<FeeRateEstimator>> estimators;

public:
    /**
     * Register a fee rate estimator.
     * @param[in] estimator unique pointer to a FeeRateEstimator instance.
     */
    void RegisterFeeRateEstimator(std::unique_ptr<FeeRateEstimator> estimator);
};

#endif // BITCOIN_POLICY_FEES_ESTIMATOR_MAN_H
