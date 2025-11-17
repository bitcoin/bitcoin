// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_ESTIMATOR_H

#include <util/fees.h>

/** \class FeeRateEstimator
 *  @brief Abstract base class for fee rate estimators.
 */
class FeeRateEstimator
{
protected:
    const FeeRateEstimatorType m_feerate_estimator_type;

public:
    explicit FeeRateEstimator(FeeRateEstimatorType feerate_estimator_type) : m_feerate_estimator_type(feerate_estimator_type) {}

    FeeRateEstimatorType GetFeeRateEstimatorType() const { return m_feerate_estimator_type; }

    /**
     * @brief Returns the estimated fee rate for a package to confirm within the given target.
     * @param target Confirmation target in blocks.
     * @param conservative If true, returns a higher fee rate for greater confirmation probability.
     * @return Estimated fee rate.
     */
    virtual FeeRateEstimatorResult EstimateFeeRate(int target, bool conservative) const = 0;

    /**
     * @brief Returns the maximum supported confirmation target.
     */
    virtual unsigned int MaximumTarget() const = 0;

    virtual ~FeeRateEstimator() = default;
};

#endif // BITCOIN_POLICY_FEES_ESTIMATOR_H
