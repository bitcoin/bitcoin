// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/mempool_estimator.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/feefrac.h>
#include <util/fees.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_FIXTURE_TEST_SUITE(mempool_fee_estimator_tests, TestingSetup)

static inline CTransactionRef MakeRandomTx()
{
    auto rng = FastRandomContext();
    auto tx = CMutableTransaction();
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vin[0].prevout.hash = Txid::FromUint256(rng.rand256());
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig << OP_TRUE;
    tx.vout[0].scriptPubKey = CScript() << OP_TRUE;
    tx.vout[0].nValue = COIN;
    return MakeTransactionRef(tx);
}

BOOST_AUTO_TEST_CASE(calculate_max_weight_percentiles)
{
    // With no chunks neither percentile can be populated.
    const auto empty = MemPoolFeeRateEstimator::CalculateMaxWeightPercentiles({});
    BOOST_CHECK(empty.p50.IsEmpty());
    BOOST_CHECK(empty.p75.IsEmpty());
    const int32_t chunk_size{10};
    const int32_t individual_tx_vsize = static_cast<int32_t>(DEFAULT_BLOCK_MAX_WEIGHT / WITNESS_SCALE_FACTOR) / chunk_size;
    const FeePerVSize super_high_fee_rate{500 * individual_tx_vsize, individual_tx_vsize};
    const FeePerVSize high_fee_rate{100 * individual_tx_vsize, individual_tx_vsize};
    const FeePerVSize medium_fee_rate{50 * individual_tx_vsize, individual_tx_vsize};
    const FeePerVSize low_fee_rate{10 * individual_tx_vsize, individual_tx_vsize};
    std::vector<FeePerVSize> chunk_feerates;
    chunk_feerates.reserve(chunk_size);
    for (int i = 0; i < chunk_size; ++i) {
        if (i < 3) {
            chunk_feerates.emplace_back(super_high_fee_rate);
        } else if (i < 5) {
            chunk_feerates.emplace_back(high_fee_rate);
        } else if (i < 8) {
            chunk_feerates.emplace_back(medium_fee_rate);
            // Once 50% coverage is reached but 75% is not, only the p50 (conservative)
            // percentile is populated; p75 (economical) is left empty for the caller to floor.
            if (i < 7) {
                const auto partial = MemPoolFeeRateEstimator::CalculateMaxWeightPercentiles(chunk_feerates);
                BOOST_CHECK_EQUAL(partial.p50.fee, high_fee_rate.fee);
                BOOST_CHECK_EQUAL(partial.p50.size, high_fee_rate.size);
                BOOST_CHECK(partial.p75.IsEmpty());
            }
        } else {
            chunk_feerates.emplace_back(low_fee_rate);
        }
    }
    const auto percentiles = MemPoolFeeRateEstimator::CalculateMaxWeightPercentiles(chunk_feerates);
    BOOST_CHECK_EQUAL(percentiles.p50.fee, high_fee_rate.fee);
    BOOST_CHECK_EQUAL(percentiles.p50.size, high_fee_rate.size);
    BOOST_CHECK_EQUAL(percentiles.p75.fee, medium_fee_rate.fee);
    BOOST_CHECK_EQUAL(percentiles.p75.size, medium_fee_rate.size);
}

BOOST_AUTO_TEST_CASE(MempoolFeeRateEstimator)
{
    auto mempool_estimator = MemPoolFeeRateEstimator(m_node.mempool.get(), m_node.chainman.get());
    BOOST_CHECK_EQUAL(mempool_estimator.MaximumTarget(), MEMPOOL_FEE_ESTIMATOR_MAX_TARGET);
    // Before the mempool has finished loading, no estimate is available.
    {
        const std::string unloaded_err = strprintf("%s: Mempool not loaded yet, no fee rate estimate available",
                                                   FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY));
        const auto result = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        BOOST_CHECK(!result);
        BOOST_CHECK_EQUAL(result.error().reason, unloaded_err);
    }
    m_node.mempool->SetLoadTried(true);
    {
        LOCK(m_node.mempool->cs);
        BOOST_CHECK_EQUAL(m_node.mempool->GetTotalTxSize(), 0);
    }
    // With an empty mempool there is nothing to build a feerate estimate from, so both
    // estimates fall back to the floor fee rate: the higher of the minimum relay fee rate
    // and the current mempool minimum fee rate.
    const FeePerVSize floor{std::max(m_node.mempool->m_opts.min_relay_feerate, m_node.mempool->GetMinFee()).GetFeePerVSize()};
    {
        const auto result = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        BOOST_REQUIRE(result.has_value());
        BOOST_CHECK(result->feerate == floor);
        BOOST_CHECK(result->feerate_estimator == FeeRateEstimatorType::MEMPOOL_POLICY);
        BOOST_CHECK_EQUAL(result->returned_target, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET);
    }
    TestMemPoolEntryHelper entry;
    const auto tx_vsize = entry.FromTx(MakeRandomTx()).GetTxSize();
    const CAmount low_fee{CENT / 3000};
    const CAmount med_fee{CENT / 100};
    const CAmount high_fee{CENT / 10};
    // A mempool that cannot fill 50% of a block leaves both percentiles empty,
    // so both estimate still fall back to the floor.
    {
        // Add high_fee transactions until mempool weight exceeds 25% of DEFAULT_BLOCK_MAX_WEIGHT.
        {
            LOCK2(cs_main, m_node.mempool->cs);
            while ((m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <= (DEFAULT_BLOCK_MAX_WEIGHT * 25 / 100)) {
                TryAddToMempool(*m_node.mempool, entry.Fee(high_fee).FromTx(MakeRandomTx()));
            }
        }
        const auto result = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        BOOST_REQUIRE(result.has_value());
        BOOST_CHECK(result->feerate == floor);
    }
    // A mempool that fills 50% of a block but not 75% has a conservative (p50)
    // estimate, while the economical (p75) estimate falls back to the floor.
    {
        // Add med_fee transactions until mempool weight exceeds 50% of DEFAULT_BLOCK_MAX_WEIGHT.
        {
            LOCK2(cs_main, m_node.mempool->cs);
            while ((m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <= (DEFAULT_BLOCK_MAX_WEIGHT * 50 / 100)) {
                TryAddToMempool(*m_node.mempool, entry.Fee(med_fee).FromTx(MakeRandomTx()));
            }
        }
        const auto conservative = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        const auto economical = mempool_estimator.EstimateFeeRate(/*conservative=*/false);
        BOOST_REQUIRE(conservative.has_value());
        BOOST_REQUIRE(economical.has_value());
        BOOST_CHECK(conservative->feerate == FeeFrac(med_fee, tx_vsize));
        BOOST_CHECK(economical->feerate == floor);
    }
    // Mempool transactions are enough to provide both feerate estimates.
    {
        // Add low_fee transactions until mempool transactions weight
        // is enough to reach the 75% coverage requirement
        {
            LOCK2(cs_main, m_node.mempool->cs);
            while ((m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <= (DEFAULT_BLOCK_MAX_WEIGHT * 75 / 100)) {
                TryAddToMempool(*m_node.mempool, entry.Fee(low_fee).FromTx(MakeRandomTx()));
            }
        }
        const auto result_conservative = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        const auto result_economical = mempool_estimator.EstimateFeeRate(/*conservative=*/false);
        BOOST_CHECK(result_conservative.has_value());
        BOOST_CHECK(result_economical.has_value());
        BOOST_CHECK(result_economical->feerate == FeeFrac(low_fee, tx_vsize));
        BOOST_CHECK(result_conservative->feerate == FeeFrac(med_fee, tx_vsize));
        BOOST_CHECK(result_conservative->feerate_estimator == FeeRateEstimatorType::MEMPOOL_POLICY);
        BOOST_CHECK(result_economical->feerate_estimator == FeeRateEstimatorType::MEMPOOL_POLICY);
        BOOST_CHECK_EQUAL(result_conservative->returned_target, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET);
        BOOST_CHECK_EQUAL(result_economical->returned_target, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET);
    }
}

BOOST_AUTO_TEST_SUITE_END()
