// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H

#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <util/fees.h>
#include <util/time.h>

#include <chrono>

class ChainstateManager;
class CTxMemPool;

// Fee rate estimate for confirmation target above this is not reliable,
// as mempool conditions are likely to change.
constexpr int MEMPOOL_FEE_ESTIMATOR_MAX_TARGET{2};
constexpr std::chrono::seconds CACHE_LIFE{7};

/**
 * MemPoolFeeRateEstimatorCache holds a cache of recent fee rate estimates.
 * A fresh fee rate is only provided if the cached value is older than CACHE_LIFE.
 */
class MemPoolFeeRateEstimatorCache
{
public:
    MemPoolFeeRateEstimatorCache() = default;
    MemPoolFeeRateEstimatorCache(const MemPoolFeeRateEstimatorCache&) = delete;
    MemPoolFeeRateEstimatorCache& operator=(const MemPoolFeeRateEstimatorCache&) = delete;
    /** Returns true if the cache is older than CACHE_LIFE. */
    bool IsStale() const;
    /** Returns {conservative, economical} feerates if available and not stale, nullopt otherwise. */
    std::optional<std::pair<FeePerVSize, FeePerVSize>> GetCachedEstimate() const;
    /** Returns the chain tip hash that the cache corresponds to. */
    uint256 GetChainTipHash() const;
    /** Update the cache with new estimates and chain tip hash. */
    void Update(FeePerVSize conservative, FeePerVSize economical, const uint256& current_tip_hash);

private:
    uint256 chain_tip_hash;
    std::pair<FeePerVSize, FeePerVSize> m_estimates;
    NodeClock::time_point last_updated{NodeClock::now() - CACHE_LIFE - std::chrono::seconds(1)};
};

/** \class MemPoolFeeRateEstimator
 * @brief Estimate the fee rate required for a transaction to be included in the next block.
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
    mutable MemPoolFeeRateEstimatorCache cache GUARDED_BY(cs);
};

#endif // BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
