// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H

#include <sync.h>
#include <threadsafety.h>
#include <util/fees.h>

class ChainstateManager;
class CTxMemPool;

// Fee rate estimate for confirmation target above this is not reliable,
// as mempool conditions are likely to change.
constexpr int MEMPOOL_FEE_ESTIMATOR_MAX_TARGET{2};

/**
 * Estimate the fee rate required for a transaction to be included in the next block.
 *
 * Uses Bitcoin Core's block-building algorithm to generate a block template from the mempool,
 * then calculates percentile fee rates from the selected packages: the 75th percentile is returned
 * as the economical estimate and the 50th percentile as the conservative estimate.
 */
class MemPoolFeeRateEstimator
{
public:
    MemPoolFeeRateEstimator(const CTxMemPool* mempool, ChainstateManager* chainman)
        : m_mempool(mempool), m_chainman(chainman)
    {
        assert(m_mempool);
        assert(m_chainman);
    }
    FeeRateEstimatorResult EstimateFeeRate(bool conservative) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    unsigned int MaximumTarget() const
    {
        return MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    }

private:
    const CTxMemPool* m_mempool;
    ChainstateManager* m_chainman;
    mutable Mutex cs;
};

#endif // BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
