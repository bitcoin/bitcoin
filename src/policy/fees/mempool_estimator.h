// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H

#include <policy/fees/estimator.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <util/time.h>

#include <chrono>

class ChainstateManager;
class CTxMemPool;

// Fee rate estimate for confirmation target above this is not reliable,
// as mempool conditions is likely going to change.
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
    /** Returns true if the cache is older than 7 seconds. */
    bool IsStale() const;
    /** Returns the cached fee rate if available and not stale. */
    std::optional<Percentiles> GetCachedEstimate() const;
    /** Returns the chain tip hash that the cache corresponds to. */
    uint256 GetChainTipHash() const;
    /** Update the cache with a new estimates  and chain tip hash. */
    void Update(const Percentiles& new_estimates, const uint256& current_tip_hash);

private:
    uint256 chain_tip_hash;
    Percentiles estimates;
    NodeClock::time_point last_updated{NodeClock::now() - CACHE_LIFE - std::chrono::seconds(1)};
};

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
    mutable MemPoolFeeRateEstimatorCache cache GUARDED_BY(cs);
};
#endif // BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
