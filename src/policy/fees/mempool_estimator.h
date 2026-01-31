// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H

#include <policy/fees/estimator.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <util/time.h>

#include <chrono>
#include <memory>
#include <vector>

class AutoFile;
class ChainstateManager;
class CTxMemPool;

struct BlockData;
struct RemovedMempoolTransactionInfo;

// Fee rate estimate for confirmation target above this is not reliable,
// as mempool conditions is likely going to change.
constexpr int MEMPOOL_FEE_ESTIMATOR_MAX_TARGET{2};

constexpr std::chrono::seconds CACHE_LIFE{7};

// Constants for mempool sanity checks.
constexpr size_t NUMBER_OF_BLOCKS = 6;
constexpr double HEALTHY_BLOCK_PERCENTILE = 0.75;

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
    MemPoolFeeRateEstimator(fs::path mempool_estimates_file_path, const CTxMemPool* mempool, ChainstateManager* chainman);

    ~MemPoolFeeRateEstimator() = default;

    /** Overridden from FeeRateEstimator. */
    FeeRateEstimatorResult EstimateFeeRate(int target, bool conservative) const override EXCLUSIVE_LOCKS_REQUIRED(!cs);

    unsigned int MaximumTarget() const override
    {
        return MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    };

    std::vector<BlockData> GetPrevBlockData() const EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        return prev_mined_blocks;
    }

    void MempoolTxsRemovedForBlock(const std::vector<CTransactionRef>& block_txs, const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    //! Checks if recent mined blocks indicate a healthy mempool state.
    bool IsMempoolHealthy() const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void Flush() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool Read(AutoFile& file) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool Write(AutoFile& file) EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    // !Checks if recent mined blocks indicate a healthy mempool state (internal-only).
    bool isMempoolHealthy(size_t current_height) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    //! Tracks the statistics of previously mined blocks.
    std::vector<BlockData> prev_mined_blocks GUARDED_BY(cs);

    const CTxMemPool* m_mempool;
    ChainstateManager* m_chainman;
    mutable Mutex cs;
    mutable MemPoolFeeRateEstimatorCache cache GUARDED_BY(cs);
    const fs::path m_mempool_estimates_file_path;
};
#endif // BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
