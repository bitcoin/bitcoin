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

FeeRateEstimatorResult MemPoolFeeRateEstimator::EstimateFeeRate(bool conservative) const
{
    LOCK(cs);
    FeeRateEstimatorResult result;
    result.feerate_estimator = FeeRateEstimatorType::MEMPOOL_POLICY;
    Chainstate& chainstate = WITH_LOCK(::cs_main, return m_chainman->CurrentChainstate());
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
    result.feerate = conservative ? percentiles.p50 : percentiles.p75;
    result.returned_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
    return result;
}
