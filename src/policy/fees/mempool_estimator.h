// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H

#include <policy/fees/estimator.h>
#include <sync.h>
#include <threadsafety.h>

class ChainstateManager;
class CTxMemPool;

// Fee rate estimate for confirmation target above this is not reliable,
// as mempool conditions is likely going to change.
constexpr int MEMPOOL_FEE_ESTIMATOR_MAX_TARGET{2};

/** \class MemPoolFeeRateEstimator
 * @brief Estimate the fee rate required for a transaction to be included in the next block.
 *
 * This FeeRateEstimator uses Bitcoin Core's block-building algorithm to generate the block
 * template that will likely be mined from unconfirmed transactions in the mempool. It calculates percentile
 * fee rates from the selected packages of the template: the 75th percentile fee rate is used as the economical
 * fee rate estimate, and the 50th fee rate percentile as the conservative estimates.
 */
class MemPoolFeeRateEstimator : public FeeRateEstimator
{
public:
    MemPoolFeeRateEstimator(const CTxMemPool* mempool, ChainstateManager* chainman)
        : FeeRateEstimator(FeeRateEstimatorType::MEMPOOL_POLICY), m_mempool(mempool), m_chainman(chainman) {};
    ~MemPoolFeeRateEstimator() = default;

    /** Overridden from FeeRateEstimator. */
    FeeRateEstimatorResult EstimateFeeRate(int target, bool conservative) const override EXCLUSIVE_LOCKS_REQUIRED(!cs);

    unsigned int MaximumTarget() const override
    {
        return MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    };

private:
    const CTxMemPool* m_mempool;
    ChainstateManager* m_chainman;
    mutable Mutex cs;
};
#endif // BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
