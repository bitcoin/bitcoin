// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
#define BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H

#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <util/fees.h>
#include <util/time.h>

#include <chrono>
#include <deque>
#include <memory>
#include <vector>

class ChainstateManager;
class CTxMemPool;

struct RemovedMempoolTransactionInfo;

// Fee rate estimate for confirmation target above this is not reliable,
// as mempool conditions are likely to change.
constexpr int MEMPOOL_FEE_ESTIMATOR_MAX_TARGET{2};
constexpr std::chrono::seconds CACHE_LIFE{7};

// Constants for mempool sanity checks.
constexpr size_t NUMBER_OF_BLOCKS = 6;
constexpr double MEMPOOL_REPRESENTATION_THRESHOLD = 0.75;

//! Weight statistics for a recently mined block, used to assess mempool coverage.
struct MinedBlockStats {
    uint64_t m_height{0};                   //!< Block height.
    uint64_t m_removed_block_txs_weight{0}; //!< Weight of mempool transactions removed for this block (excluding coinbase).
    uint64_t m_block_weight{0};             //!< Total non-coinbase transaction weight in the block.
};

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

    std::vector<MinedBlockStats> GetPrevBlockData() const EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        return {prev_mined_blocks.begin(), prev_mined_blocks.end()};
    }

    void MempoolTxsRemovedForBlock(const std::vector<CTransactionRef>& block_txs, const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    //! Checks if recent mined blocks indicate a healthy mempool state.
    bool IsMempoolHealthy() const EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    //! Checks if recent mined blocks indicate a healthy mempool state (internal-only).
    bool isMempoolHealthy() const EXCLUSIVE_LOCKS_REQUIRED(cs);
    //! Tracks weight statistics for the last NUMBER_OF_BLOCKS mined blocks.
    std::deque<MinedBlockStats> prev_mined_blocks GUARDED_BY(cs);

    const CTxMemPool* m_mempool;
    ChainstateManager* m_chainman;
    mutable Mutex cs;
    mutable MemPoolFeeRateEstimatorCache cache GUARDED_BY(cs);
};

#endif // BITCOIN_POLICY_FEES_MEMPOOL_ESTIMATOR_H
