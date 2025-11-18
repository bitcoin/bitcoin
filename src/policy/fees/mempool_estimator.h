// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H

#include <policy/fees/estimator.h>
#include <sync.h>
#include <threadsafety.h>

class Chainstate;
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
    MemPoolFeeRateEstimator(const CTxMemPool* mempool, Chainstate* chainstate)
        : FeeRateEstimator(FeeRateEstimatorType::MEMPOOL_POLICY), m_mempool(mempool), m_chainstate(chainstate) {};
    ~MemPoolFeeRateEstimator() = default;

    /** Overridden from FeeRateEstimator. */
    FeeRateEstimatorResult EstimateFeeRate(int target, bool conservative) const override EXCLUSIVE_LOCKS_REQUIRED(!cs);

    unsigned int MaximumTarget() const override
    {
        return MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    };

private:
    const CTxMemPool* m_mempool;
    Chainstate* m_chainstate;
    mutable Mutex cs;
};
#endif // BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
