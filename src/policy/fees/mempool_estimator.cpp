// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/mempool_estimator.h>

#include <logging.h>
#include <node/miner.h>
#include <policy/policy.h>
#include <util/feefrac.h>
#include <util/fees.h>
#include <validation.h>

#include <algorithm>
#include <numeric>

bool MemPoolFeeRateEstimatorCache::IsStale() const
{
    return (last_updated + CACHE_LIFE) < NodeClock::now();
}

std::optional<std::pair<FeePerVSize, FeePerVSize>> MemPoolFeeRateEstimatorCache::GetCachedEstimate() const
{
    if (IsStale()) return std::nullopt;
    return m_estimates;
}

uint256 MemPoolFeeRateEstimatorCache::GetChainTipHash() const
{
    return chain_tip_hash;
}

void MemPoolFeeRateEstimatorCache::Update(FeePerVSize conservative, FeePerVSize economical, const uint256& current_tip_hash)
{
    m_estimates = {conservative, economical};
    last_updated = NodeClock::now();
    chain_tip_hash = current_tip_hash;
}

void MemPoolFeeRateEstimator::MempoolTxsRemovedForBlock(const std::vector<CTransactionRef>& block_txs, const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight)
{
    LOCK(cs);
    Assert(!block_txs.empty());
    // Accumulate total block weight and removed mempool tx weight, both excluding the coinbase.
    const uint64_t block_weight = std::accumulate(block_txs.begin() + 1, block_txs.end(), uint64_t{0},
                                                  [](uint64_t acc, const CTransactionRef& tx) { return acc + static_cast<uint64_t>(GetTransactionWeight(*tx)); });
    const uint64_t removed_weight = std::accumulate(txs_removed_for_block.begin(), txs_removed_for_block.end(), uint64_t{0},
                                                    [](uint64_t acc, const RemovedMempoolTransactionInfo& tx) { return acc + static_cast<uint64_t>(GetTransactionWeight(*tx.info.m_tx)); });
    if (prev_mined_blocks.size() == NUMBER_OF_BLOCKS) prev_mined_blocks.pop_front();
    prev_mined_blocks.push_back({nBlockHeight, removed_weight, block_weight});
}

bool MemPoolFeeRateEstimator::IsMempoolHealthy() const
{
    LOCK(cs);
    return isMempoolHealthy();
}

bool MemPoolFeeRateEstimator::isMempoolHealthy() const
{
    AssertLockHeld(cs);
    if (prev_mined_blocks.size() < NUMBER_OF_BLOCKS) return false;
    uint64_t total_block_weight{0};
    uint64_t total_removed_weight{0};
    for (size_t i = 0; i < prev_mined_blocks.size(); ++i) {
        // Handle reorg by returning false (Dont clear as blocks are added it will stabilize).
        if (i > 0 && prev_mined_blocks[i].m_height != prev_mined_blocks[i - 1].m_height + 1) return false;
        total_block_weight += prev_mined_blocks[i].m_block_weight;
        total_removed_weight += prev_mined_blocks[i].m_removed_block_txs_weight;
    }
    if (!total_block_weight) return false;
    const double representation_ratio = static_cast<double>(total_removed_weight) / total_block_weight;
    LogDebug(BCLog::ESTIMATEFEE, "%s: mempool health avg=%.2f threshold=%.2f (mempool %s)",
             FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY),
             representation_ratio,
             MEMPOOL_REPRESENTATION_THRESHOLD,
             representation_ratio >= MEMPOOL_REPRESENTATION_THRESHOLD ? "healthy" : "unhealthy");
    return representation_ratio >= MEMPOOL_REPRESENTATION_THRESHOLD;
}

FeeRateEstimatorResult MemPoolFeeRateEstimator::EstimateFeeRate(bool conservative) const
{
    LOCK(cs);
    FeeRateEstimatorResult result;
    result.feerate_estimator = FeeRateEstimatorType::MEMPOOL_POLICY;
    if (!isMempoolHealthy()) {
        result.errors.emplace_back(strprintf("%s: Mempool is unreliable for fee rate estimation", FeeRateEstimatorTypeToString(result.feerate_estimator)));
        return result;
    }
    Chainstate& chainstate = WITH_LOCK(::cs_main, return m_chainman->CurrentChainstate());
    const uint256 tip_hash{WITH_LOCK(::cs_main, return Assume(chainstate.m_chain.Tip())->GetBlockHash())};
    const auto cached_estimate = cache.GetCachedEstimate();
    if (cached_estimate && tip_hash == cache.GetChainTipHash()) {
        result.returned_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
        result.feerate = conservative ? cached_estimate->first : cached_estimate->second;
        return result;
    }
    node::BlockAssembler::Options options;
    options.test_block_validity = false;
    node::BlockAssembler assembler{chainstate, m_mempool, options};
    const auto blocktemplate = assembler.CreateNewBlock();
    auto package_feerates = blocktemplate->m_package_feerates;
    // Sort again because the rounding up when converting from weight to vsize may cause slight misorder.
    std::sort(package_feerates.begin(), package_feerates.end(), [](auto& a, auto& b) { return a >> b; });
    const auto percentiles = CalculatePercentiles(package_feerates, DEFAULT_BLOCK_MAX_WEIGHT);
    if (percentiles.IsEmpty()) {
        // Mempool is too sparse; return max(minrelayfee, mempoolminfee) as a floor.
        const CFeeRate floor_feerate = std::max(m_mempool->m_opts.min_relay_feerate, m_mempool->GetMinFee());
        constexpr int32_t vsize{1000};
        const FeePerVSize floor{floor_feerate.GetFee(vsize), vsize};
        cache.Update(floor, floor, blocktemplate->block.hashPrevBlock);
        result.feerate = floor;
        result.returned_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
        const std::string err_message{strprintf("%s: Mempool is too sparse, returning max of minrelayfee and mempoolminfee: %s %s/kvB",
                                                FeeRateEstimatorTypeToString(result.feerate_estimator), floor_feerate.GetFeePerK(), CURRENCY_ATOM)};
        result.errors.emplace_back(err_message);
        LogDebug(BCLog::ESTIMATEFEE, "%s\n", err_message);
        return result;
    }
    LogDebug(BCLog::ESTIMATEFEE,
             "%s: Block template 25th percentile fee rate: %s %s/kvB, "
             "50th percentile fee rate: %s %s/kvB, 75th percentile fee rate: %s %s/kvB, "
             "95th percentile fee rate: %s %s/kvB\n",
             FeeRateEstimatorTypeToString(result.feerate_estimator),
             CFeeRate(percentiles.p25.fee, percentiles.p25.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p50.fee, percentiles.p50.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p75.fee, percentiles.p75.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p95.fee, percentiles.p95.size).GetFeePerK(), CURRENCY_ATOM);

    cache.Update(percentiles.p50, percentiles.p75, blocktemplate->block.hashPrevBlock);
    result.feerate = conservative ? percentiles.p50 : percentiles.p75;
    result.returned_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    return result;
}
