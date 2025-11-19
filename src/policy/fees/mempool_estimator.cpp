// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/mempool_estimator.h>

#include <logging.h>
#include <node/miner.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <sync.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/feefrac.h>
#include <util/fees.h>
#include <validation.h>

#include <algorithm>
#include <numeric>
#include <string>
#include <utility>

namespace {
void AddMinedBlockStats(std::deque<MinedBlockStats>& mined_blocks, MinedBlockStats stats)
{
    const auto stale_begin{std::find_if(mined_blocks.begin(), mined_blocks.end(), [&](const MinedBlockStats& block) {
        return block.m_height >= stats.m_height;
    })};
    const auto stale_count{std::distance(stale_begin, mined_blocks.end())};
    if (stale_count > 0) {
        LogDebug(BCLog::ESTIMATEFEE,
                 "%s: connected block height=%s discards tracked mined-block stats "
                 "from height=%s to height=%s; stale_stats=%s",
                 FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY),
                 stats.m_height,
                 stale_begin->m_height,
                 mined_blocks.back().m_height,
                 stale_count);
    }
    mined_blocks.erase(stale_begin, mined_blocks.end());
    if (!mined_blocks.empty() && mined_blocks.back().m_height + 1 != stats.m_height) {
        LogDebug(BCLog::ESTIMATEFEE,
                 "%s: clearing mined-block stats after height gap; tracked_stats=%s "
                 "expected_height=%s received_height=%s",
                 FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY),
                 mined_blocks.size(),
                 mined_blocks.back().m_height + 1,
                 stats.m_height);
        mined_blocks.clear();
    }

    if (mined_blocks.size() == MEMPOOL_HEALTH_WINDOW_BLOCKS) mined_blocks.pop_front();
    mined_blocks.push_back(stats);
}
} // namespace

MemPoolFeeRateEstimator::Percentiles MemPoolFeeRateEstimator::CalculateMaxWeightPercentiles(const std::vector<FeePerVSize>& chunk_feerates)
{
    Assume(std::is_sorted(chunk_feerates.begin(), chunk_feerates.end(), [](const auto& a, const auto& b) { return ByRatio{a} > ByRatio{b}; }));
    constexpr int32_t total_weight{DEFAULT_BLOCK_MAX_WEIGHT};
    const int32_t p50_weight{total_weight / 2};
    const int32_t p75_weight{total_weight * 3 / 4};
    Percentiles percentiles{};
    int32_t accumulated_weight{0};
    for (const auto& curr_feerate : chunk_feerates) {
        accumulated_weight += curr_feerate.size * WITNESS_SCALE_FACTOR;
        if (accumulated_weight >= p50_weight && percentiles.p50.IsEmpty()) {
            percentiles.p50 = curr_feerate;
        }
        if (accumulated_weight >= p75_weight && percentiles.p75.IsEmpty()) {
            percentiles.p75 = curr_feerate;
            break;
        }
    }
    return percentiles;
}

bool MemPoolFeeRateEstimatorCache::IsStale() const
{
    return !m_fee_rate_estimation || (m_last_updated + CACHE_LIFE) < NodeClock::now();
}

std::optional<MemPoolFeeRateEstimatorCache::FeeRateEstimate>
MemPoolFeeRateEstimatorCache::GetCachedEstimate(const uint256& tip_hash) const
{
    if (IsStale() || tip_hash != m_tip_hash) return std::nullopt;
    return m_fee_rate_estimation;
}

void MemPoolFeeRateEstimatorCache::Update(FeePerVSize conservative, FeePerVSize economical, const uint256& tip_hash)
{
    m_fee_rate_estimation = {conservative, economical};
    m_tip_hash = tip_hash;
    m_last_updated = NodeClock::now();
}

void MemPoolFeeRateEstimatorCache::Clear()
{
    m_fee_rate_estimation.reset();
    m_tip_hash.SetNull();
    m_last_updated = {};
}

//! Build the error result for a failed mempool fee rate estimation.
static util::Unexpected<FeeRateEstimationError> EstimationError(std::string error)
{
    return EstimationError(FeeRateEstimatorType::MEMPOOL_POLICY, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET, std::move(error));
}

void MemPoolFeeRateEstimator::MempoolTxsRemovedForBlock(
    const std::vector<CTransactionRef>& block_txs,
    const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block,
    unsigned int block_height)
{
    // Skip during initial block download or reindex: the mempool is empty, so
    // these blocks carry no information about mempool representation.
    if (m_chainman->IsInitialBlockDownload()) return;
    LOCK(cs);
    Assert(!block_txs.empty());
    // Accumulate total block weight and removed mempool tx weight, both excluding the coinbase.
    const auto get_tx_weight = [](const CTransactionRef& tx) {
        return static_cast<uint64_t>(GetTransactionWeight(*tx));
    };
    // Skip block_txs[0], which is the coinbase.
    const uint64_t block_weight = std::accumulate(block_txs.begin() + 1, block_txs.end(), uint64_t{0},
                                                  [&](uint64_t acc, const CTransactionRef& tx) {
                                                      return acc + get_tx_weight(tx);
                                                  });
    const uint64_t removed_weight = std::accumulate(
        txs_removed_for_block.begin(), txs_removed_for_block.end(), uint64_t{0},
        [&](uint64_t acc, const RemovedMempoolTransactionInfo& tx) {
            return acc + get_tx_weight(tx.info.m_tx);
        });
    AddMinedBlockStats(m_prev_mined_blocks, {block_height, removed_weight, block_weight});
    m_cache.Clear();
}

// Require at least one block worth of activity across the window before using
// the coverage ratio as a representative mempool health signal.
static constexpr uint64_t MIN_REPRESENTATIVE_WINDOW_WEIGHT{DEFAULT_BLOCK_MAX_WEIGHT};

bool MemPoolFeeRateEstimator::IsMempoolHealthy() const
{
    LOCK(cs);
    const auto estimator_name{FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY)};
    if (m_prev_mined_blocks.size() < MEMPOOL_HEALTH_WINDOW_BLOCKS) {
        LogDebug(BCLog::ESTIMATEFEE, "%s: mempool health check failed; tracked_blocks=%s required_blocks=%s",
                 estimator_name, m_prev_mined_blocks.size(), MEMPOOL_HEALTH_WINDOW_BLOCKS);
        return false;
    }
    uint64_t total_block_weight{0};
    uint64_t total_removed_weight{0};
    uint64_t expected_height{m_prev_mined_blocks.front().m_height};
    for (const auto& block : m_prev_mined_blocks) {
        Assume(block.m_height == expected_height);
        ++expected_height;
        total_block_weight += block.m_block_weight;
        total_removed_weight += block.m_removed_block_txs_weight;
    }
    // Too little block activity for the coverage ratio to be meaningful; skip it.
    if (total_block_weight < MIN_REPRESENTATIVE_WINDOW_WEIGHT) {
        LogDebug(BCLog::ESTIMATEFEE, "%s: mempool health check passed; low activity, total_block_weight=%s minimum=%s",
                 estimator_name, total_block_weight, MIN_REPRESENTATIVE_WINDOW_WEIGHT);
        return true;
    }
    const double representation_ratio = static_cast<double>(total_removed_weight) / total_block_weight;
    LogDebug(BCLog::ESTIMATEFEE,
             "%s: mempool health check %s; removed_weight=%s total_block_weight=%s "
             "coverage=%.2f required_coverage=%.2f",
             estimator_name,
             representation_ratio >= MEMPOOL_REPRESENTATION_THRESHOLD ? "passed" : "failed",
             total_removed_weight,
             total_block_weight,
             representation_ratio,
             MEMPOOL_REPRESENTATION_THRESHOLD);
    return representation_ratio >= MEMPOOL_REPRESENTATION_THRESHOLD;
}

util::Expected<FeeRateEstimation, FeeRateEstimationError> MemPoolFeeRateEstimator::EstimateFeeRate(bool conservative) const
{
    constexpr auto estimator_type{FeeRateEstimatorType::MEMPOOL_POLICY};
    if (!m_mempool->GetLoadTried()) {
        return EstimationError(strprintf("%s: Mempool not loaded yet, no fee rate estimate available", FeeRateEstimatorTypeToString(estimator_type)));
    }
    if (!IsMempoolHealthy()) {
        return EstimationError(strprintf("%s: Mempool is unreliable for fee rate estimation",
                                         FeeRateEstimatorTypeToString(estimator_type)));
    }
    // The estimator lock is not held while building a block template, so
    // in a rare edge case concurrent callers may duplicate work.
    //
    // Cached estimates are tagged with the chain tip they were computed on
    // and only served from the cache while that tip is current.
    //
    // The estimate returned directly below may still reflect a tip that went
    // stale during the call; that is an accepted tradeoff of not holding
    // locks across block assembly.
    Chainstate& chainstate{WITH_LOCK(::cs_main, return m_chainman->CurrentChainstate())};
    const uint256 tip_hash{WITH_LOCK(::cs_main, return Assume(chainstate.m_chain.Tip())->GetBlockHash())};
    {
        LOCK(cs);
        const auto cached_estimate = m_cache.GetCachedEstimate(tip_hash);
        if (cached_estimate) {
            const auto cached_feerate{
                conservative ? cached_estimate->m_conservative : cached_estimate->m_economical};
            return FeeRateEstimation{estimator_type, cached_feerate, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET};
        }
    }
    node::BlockCreateOptions options;
    options.test_block_validity = false;
    node::BlockAssembler assembler{chainstate, m_mempool, options};
    const auto blocktemplate = assembler.CreateNewBlock();
    auto package_feerates = blocktemplate->m_package_feerates;
    // Sort again because the rounding up when converting from weight to vsize may cause slight misorder.
    std::sort(package_feerates.begin(), package_feerates.end(), [](auto& a, auto& b) { return ByRatio{a} > ByRatio{b}; });
    const auto percentiles = CalculateMaxWeightPercentiles(package_feerates);
    // Fall back to a relayable floor (the higher of the min relay fee and the current
    // mempool min fee) for any percentile the mempool was too sparse to fill.
    const FeePerVSize floor{std::max(m_mempool->m_opts.min_relay_feerate, m_mempool->GetMinFee()).GetFeePerVSize()};
    const FeePerVSize p50{percentiles.p50.IsEmpty() ? floor : percentiles.p50};
    const FeePerVSize p75{percentiles.p75.IsEmpty() ? floor : percentiles.p75};
    WITH_LOCK(cs, m_cache.Update(p50, p75, blocktemplate->block.hashPrevBlock));
    LogDebug(BCLog::ESTIMATEFEE, "%s: conservative/economical fee rate: %s/%s %s/kvB",
             FeeRateEstimatorTypeToString(estimator_type), CFeeRate(p50).GetFeePerK(),
             CFeeRate(p75).GetFeePerK(), CURRENCY_ATOM);
    return FeeRateEstimation{
        estimator_type, conservative ? p50 : p75, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET};
}
