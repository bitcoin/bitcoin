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
    result.current_block_height = static_cast<unsigned int>(current_height);
    node::BlockAssembler::Options options;
    options.test_block_validity = false;
    node::BlockAssembler assembler{chainstate, m_mempool, options};
    const auto blocktemplate = assembler.CreateNewBlock();
    const auto& m_package_feerates = blocktemplate->m_package_feerates;
    const auto percentiles = CalculatePercentiles(m_package_feerates, DEFAULT_BLOCK_MAX_WEIGHT);
    if (percentiles.empty()) {
        result.errors.emplace_back("Unable to provide a fee rate due to insufficient data");
        return result;
    }

    LogDebug(BCLog::ESTIMATEFEE,
             "Block height %s, Block template 25th percentile fee rate: %s %s/kvB, "
             "50th percentile fee rate: %s %s/kvB, 75th percentile fee rate: %s %s/kvB, "
             "95th percentile fee rate: %s %s/kvB\n",
             result.current_block_height,
             CFeeRate(percentiles.p25.fee, percentiles.p25.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p50.fee, percentiles.p50.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p75.fee, percentiles.p75.size).GetFeePerK(), CURRENCY_ATOM,
             CFeeRate(percentiles.p95.fee, percentiles.p95.size).GetFeePerK(), CURRENCY_ATOM);

    result.feerate = conservative ? percentiles.p50 : percentiles.p75;
    result.returned_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    return result;
}
