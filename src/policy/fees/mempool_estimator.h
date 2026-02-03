// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H

#include <util/expected.h>
#include <util/feefrac.h>
#include <util/fees.h>

#include <vector>

class ChainstateManager;
class CTxMemPool;

// Fee rate estimate for confirmation target above this is not reliable,
// as mempool conditions are likely to change.
constexpr int MEMPOOL_FEE_ESTIMATOR_MAX_TARGET{2};

/**
 * Estimate the fee rate required for a transaction to be included in the next block.
 *
 * Uses Bitcoin Core's block-building algorithm to generate a block template from the mempool,
 * then calculates percentile fee rates from the selected chunks: the 75th percentile is returned
 * as the economical estimate and the 50th percentile as the conservative estimate.
 */
class MemPoolFeeRateEstimator
{
public:
    // Block percentiles fee rate (in sat/vB).
    struct Percentiles {
        FeePerVSize p50;
        FeePerVSize p75;
    };

    MemPoolFeeRateEstimator(const CTxMemPool* mempool, ChainstateManager* chainman)
        : m_mempool(mempool), m_chainman(chainman) {}
    /**
     * Calculate the 50th and 75th percentile fee rates from block template chunks,
     * sorted in descending mining-score order. A percentile is left empty when the
     * chunks cannot cover the corresponding fraction of a block.
     *
     * @param[in] chunk_feerates A vector of block template chunk fee rates sorted by descending mining score.
     */
    static Percentiles CalculateMaxWeightPercentiles(const std::vector<FeePerVSize>& chunk_feerates);
    util::Expected<FeeRateEstimation, FeeRateEstimationError> EstimateFeeRate(bool conservative) const;
    unsigned int MaximumTarget() const
    {
        return MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    }

private:
    const CTxMemPool* m_mempool;
    ChainstateManager* m_chainman;
};

#endif // BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
