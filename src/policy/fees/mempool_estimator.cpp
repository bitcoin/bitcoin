// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/miner.h>
#include <policy/fees/estimator.h>
#include <policy/fees/mempool_estimator.h>
#include <policy/policy.h>
#include <util/fees.h>
#include <validation.h>

bool MemPoolFeeRateEstimatorCache::IsStale() const
{
    return (last_updated + CACHE_LIFE) < NodeClock::now();
}

std::optional<Percentiles> MemPoolFeeRateEstimatorCache::GetCachedEstimate() const
{
    if (IsStale()) return std::nullopt;
    return estimates;
}

uint256 MemPoolFeeRateEstimatorCache::GetChainTipHash() const
{
    return chain_tip_hash;
}

void MemPoolFeeRateEstimatorCache::Update(const Percentiles& new_estimates, const uint256& current_tip_hash)
{
    estimates = new_estimates;
    last_updated = NodeClock::now();
    chain_tip_hash = current_tip_hash;
}

void MemPoolFeeRateEstimator::MempoolTxsRemovedForBlock(const std::vector<CTransactionRef>& block_txs, const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight)
{
    LOCK(cs);
    BlockData new_block_data;
    new_block_data.m_height = nBlockHeight;
    // Calculates total block weight excluding the coinbase transaction
    Assume(block_txs.size() > 0);
    new_block_data.m_block_weight = std::accumulate(block_txs.begin() + 1, block_txs.end(), 0u, [](size_t acc, const CTransactionRef& tx) {
        return acc + static_cast<double>(GetTransactionWeight(*tx));
    });
    new_block_data.m_removed_block_txs_weight = std::accumulate(txs_removed_for_block.begin(), txs_removed_for_block.end(), 0u,
                                                                [](size_t acc, const RemovedMempoolTransactionInfo& tx) { return acc + static_cast<double>(GetTransactionWeight(*(tx.info.m_tx))); });
    if (prev_mined_blocks.size() == NUMBER_OF_BLOCKS) {
        prev_mined_blocks.erase(prev_mined_blocks.begin());
    }
    prev_mined_blocks.emplace_back(new_block_data);
}


bool MemPoolFeeRateEstimator::IsMempoolHealthy() const
{
    LOCK(cs);
    auto chain_tip = WITH_LOCK(cs_main, return m_chainman->ActiveTip());
    if (!chain_tip) return false;
    return isMempoolHealthy(static_cast<size_t>(chain_tip->nHeight));
}

bool MemPoolFeeRateEstimator::isMempoolHealthy(size_t current_height) const
{
    AssertLockHeld(cs);
    if (prev_mined_blocks.size() < NUMBER_OF_BLOCKS) return false;
    auto total_block_txs_weight = prev_mined_blocks[0].m_block_weight;
    auto total_removed_txs_weight = prev_mined_blocks[0].m_removed_block_txs_weight;
    for (size_t i = 1; i < prev_mined_blocks.size(); ++i) {
        const auto& current = prev_mined_blocks[i];
        const auto& previous = prev_mined_blocks[i - 1];
        // Handle reorg by returning false (Dont clear as blocks are added it will stabilize).
        if (current.m_height != previous.m_height + 1) return false;
        total_block_txs_weight += current.m_block_weight;
        total_removed_txs_weight += current.m_removed_block_txs_weight;
    }
    if (prev_mined_blocks.back().m_height != current_height) {
        LogDebug(BCLog::ESTIMATEFEE, "%s: Most Recent BlockData is not the chain tip", FeeRateEstimatorTypeToString(GetFeeRateEstimatorType()));
    }
    if (!total_block_txs_weight) return false;
    auto average_percentile = total_removed_txs_weight / total_block_txs_weight;
    LogDebug(BCLog::ESTIMATEFEE, "%s: mempool health avg=%.2f threshold=%.2f (mempool %s)",
             FeeRateEstimatorTypeToString(GetFeeRateEstimatorType()),
             average_percentile,
             HEALTHY_BLOCK_PERCENTILE,
             average_percentile >= HEALTHY_BLOCK_PERCENTILE ? "healthy" : "unhealthy");
    return average_percentile >= HEALTHY_BLOCK_PERCENTILE;
}

FeeRateEstimatorResult MemPoolFeeRateEstimator::EstimateFeeRate(int target, bool conservative) const
{
    LOCK(cs);
    Assume(m_mempool);
    Assume(m_chainman);
    FeeRateEstimatorResult result;
    result.feerate_estimator = GetFeeRateEstimatorType();
    Chainstate& chainstate = WITH_LOCK(::cs_main, return m_chainman->CurrentChainstate());
    int current_height = WITH_LOCK(::cs_main, return chainstate.m_chain.Height());
    Assume(current_height > -1);
    uint256 tip_hash = WITH_LOCK(::cs_main, return *Assume(chainstate.m_chain.Tip())->phashBlock);
    result.current_block_height = static_cast<unsigned int>(current_height);
    if (!isMempoolHealthy(static_cast<size_t>(result.current_block_height))) {
        result.errors.emplace_back(strprintf("%s: Mempool is unreliable for fee rate estimation", FeeRateEstimatorTypeToString(result.feerate_estimator)));
        return result;
    }

    const auto cached_estimate = cache.GetCachedEstimate();
    const auto known_chain_tip_hash = cache.GetChainTipHash();
    if (cached_estimate && tip_hash == known_chain_tip_hash) {
        result.returned_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
        result.feerate = conservative ? cached_estimate->p50 : cached_estimate->p75;
        return result;
    }

    node::BlockAssembler::Options options;
    options.test_block_validity = false;
    node::BlockAssembler assembler{chainstate, m_mempool, options};
    const auto blocktemplate = assembler.CreateNewBlock();
    const auto& m_package_feerates = blocktemplate->m_package_feerates;
    const auto percentiles = CalculatePercentiles(m_package_feerates, DEFAULT_BLOCK_MAX_WEIGHT);
    if (percentiles.empty()) {
        result.errors.emplace_back(strprintf("%s: Unable to provide a fee rate due to insufficient data", FeeRateEstimatorTypeToString(result.feerate_estimator)));
        return result;
    }

    LogDebug(BCLog::ESTIMATEFEE,
             "%s: Block height %s, Block template 25th percentile fee rate: %s %s/kvB, "
             "50th percentile fee rate: %s %s/kvB, 75th percentile fee rate: %s %s/kvB, "
             "95th percentile fee rate: %s %s/kvB\n",
             FeeRateEstimatorTypeToString(result.feerate_estimator), result.current_block_height,
             CFeeRate(percentiles.p25.fee, percentiles.p25.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p50.fee, percentiles.p50.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p75.fee, percentiles.p75.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p95.fee, percentiles.p95.size).GetFeePerK(), CURRENCY_ATOM);

    cache.Update(percentiles, tip_hash);
    result.feerate = conservative ? percentiles.p50 : percentiles.p75;
    result.returned_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    return result;
}
