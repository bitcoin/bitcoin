// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H

#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <util/expected.h>
#include <util/feefrac.h>
#include <util/fees.h>
#include <util/time.h>

#include <chrono>
#include <optional>
#include <vector>

class ChainstateManager;
class CTxMemPool;

// Fee rate estimate for confirmation target above this is not reliable,
// as mempool conditions are likely to change.
constexpr int MEMPOOL_FEE_ESTIMATOR_MAX_TARGET{2};
constexpr std::chrono::seconds CACHE_LIFE{7};

/**
 * MemPoolFeeRateEstimatorCache holds a cache of recent fee rate estimates.
 * A cached fee rate is only provided while it is not older than CACHE_LIFE
 * and the chain tip has not changed.
 */
class MemPoolFeeRateEstimatorCache
{
public:
    MemPoolFeeRateEstimatorCache() = default;
    MemPoolFeeRateEstimatorCache(const MemPoolFeeRateEstimatorCache&) = delete;
    MemPoolFeeRateEstimatorCache& operator=(const MemPoolFeeRateEstimatorCache&) = delete;
    /** Returns true if the cache is older than CACHE_LIFE. */
    bool IsStale() const;
    struct FeeRateEstimate {
        FeePerVSize m_conservative;
        FeePerVSize m_economical;
    };
    /** Returns cached estimates if not stale and computed on tip_hash, nullopt otherwise. */
    std::optional<FeeRateEstimate> GetCachedEstimate(const uint256& tip_hash) const;
    /** Update the cache with new estimates computed on tip_hash. */
    void Update(FeePerVSize conservative, FeePerVSize economical, const uint256& tip_hash);

private:
    std::optional<FeeRateEstimate> m_fee_rate_estimation;
    uint256 m_tip_hash;
    NodeClock::time_point m_last_updated{};
};

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
    util::Expected<FeeRateEstimation, FeeRateEstimationError> EstimateFeeRate(bool conservative) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    unsigned int MaximumTarget() const
    {
        return MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    }

private:
    const CTxMemPool* m_mempool;
    ChainstateManager* m_chainman;
    mutable Mutex cs;
    mutable MemPoolFeeRateEstimatorCache m_cache GUARDED_BY(cs);
};

#endif // BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
