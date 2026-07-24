// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/mempool_entry.h>
#include <policy/fees/estimator_args.h>
#include <policy/fees/mempool_estimator.h>
#include <policy/policy.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <test/util/validation.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/feefrac.h>
#include <util/fees.h>
#include <util/time.h>
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

void AddRemovedBlock(MemPoolFeeRateEstimator& fee_est,
                     int32_t removed_txs_weight,
                     int32_t block_txs_weight,
                     unsigned int& height)
{
    std::vector<CTransactionRef> txs;
    std::vector<RemovedMempoolTransactionInfo> removed_txs;
    TestMemPoolEntryHelper entry;
    Assert(block_txs_weight >= removed_txs_weight);
    txs.emplace_back(MakeRandomTx()); // Add a coinbase tx
    while (block_txs_weight > 0) {
        auto tx = MakeRandomTx();
        auto tx_weight = GetTransactionWeight(*tx);
        if (block_txs_weight - tx_weight < 0) break;
        txs.emplace_back(tx);
        block_txs_weight -= tx_weight;
        if (removed_txs_weight - tx_weight >= 0) {
            removed_txs.emplace_back(entry.FromTx(tx));
            removed_txs_weight -= tx_weight;
        }
    }
    fee_est.MempoolTxsRemovedForBlock(txs, removed_txs, height, FastRandomContext().rand256());
    height += 1;
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
    // Mined-block stats are only tracked once initial block download is done.
    static_cast<TestChainstateManager&>(*m_node.chainman).JumpOutOfIbd();
    auto mempool_estimator = MemPoolFeeRateEstimator(MempoolPolicyEstimatorPath(*m_node.args), m_node.mempool.get(), m_node.chainman.get());
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

    BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
    {
        const auto result = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        const std::string unreliable_err{strprintf("%s: Mempool is unreliable for fee rate estimation",
                                                   FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY))};
        BOOST_CHECK(!result);
        BOOST_CHECK_EQUAL(result.error().reason, unreliable_err);
    }
    {
        MemPoolFeeRateEstimator custom_mempool_estimator{
            MempoolPolicyEstimatorPath(*m_node.args), m_node.mempool.get(), m_node.chainman.get()};
        unsigned int custom_height{100};
        for (size_t block_count{1}; block_count < MEMPOOL_HEALTH_WINDOW_BLOCKS; ++block_count) {
            AddRemovedBlock(custom_mempool_estimator,
                            /*removed_txs_weight=*/0,
                            /*block_txs_weight=*/0,
                            custom_height);
            BOOST_CHECK(!custom_mempool_estimator.IsMempoolHealthy());
        }
        {
            const int64_t low_activity_weight{1000};
            AddRemovedBlock(custom_mempool_estimator, low_activity_weight / 2, low_activity_weight, custom_height);
        }
        // Below one block worth of total activity across the full window, even
        // poor coverage in the only non-empty block is too noisy to reject the
        // mempool as unhealthy.
        BOOST_CHECK(custom_mempool_estimator.IsMempoolHealthy());
    }
    size_t block_count = 1;
    const int64_t weight{DEFAULT_BLOCK_MAX_WEIGHT / 2};
    unsigned int height = 100;
    // Equal weight
    while (block_count <= MEMPOOL_HEALTH_WINDOW_BLOCKS) {
        AddRemovedBlock(mempool_estimator, weight, weight, height);
        if (block_count < MEMPOOL_HEALTH_WINDOW_BLOCKS) {
            BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
        }
        block_count += 1;
    }
    // Total txs weight ~11999k WU (~3.0 blocks), removed txs ~11999k WU (~3.0 blocks); coverage = 100%.
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    // Adding a single underrepresented block will not make the mempool unhealthy
    // while the window coverage remains above the threshold.
    AddRemovedBlock(mempool_estimator, weight / 2, weight, height);
    // Total txs weight ~11999k WU (~3.0 blocks), removed txs ~10999k WU (~2.75 blocks); coverage = ~92%.
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    // Empty block
    // Total txs weight ~9999k WU (~2.5 blocks), removed txs ~8999k WU (~2.25 blocks); coverage = 90%.
    AddRemovedBlock(mempool_estimator, 0, 0, height);
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    // Total txs weight ~9999k WU (~2.5 blocks), removed txs ~7999k WU (~2.0 blocks); coverage = 80%.
    AddRemovedBlock(mempool_estimator, weight / 2, weight, height);
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    // Total txs weight ~9999k WU (~2.5 blocks), removed txs ~7000k WU (~1.75 blocks); coverage = 70%.
    AddRemovedBlock(mempool_estimator, weight / 2, weight, height);
    BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
    block_count = 1;
    while (block_count <= 3) {
        AddRemovedBlock(mempool_estimator, weight, weight, height);
        if (block_count < 3) {
            BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
        }
        block_count += 1;
    }
    // Total txs weight ~9999k WU (~2.5 blocks), removed txs ~7999k WU (~2.0 blocks); coverage = 80%.
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());

    // Reorg out and replace the last block. Replacing the tip block should keep a full
    // healthy window when the replacement block has good mempool representation.
    height -= 1;
    AddRemovedBlock(mempool_estimator, weight, weight, height);
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());

    // Reorg out the last two blocks. The estimator should discard the stale suffix,
    // become temporarily unhealthy due to having fewer than MEMPOOL_HEALTH_WINDOW_BLOCKS stats,
    // then recover after the replacement chain catches up.
    height -= 2;
    AddRemovedBlock(mempool_estimator, weight, weight, height);
    BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
    AddRemovedBlock(mempool_estimator, weight, weight, height);
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());

    // A forward height gap (e.g. stale persisted stats after an unclean shutdown
    // while the chain advanced) resets the tracked window entirely; the estimator
    // stays unhealthy until a full window of contiguous blocks is seen again.
    height += 3;
    AddRemovedBlock(mempool_estimator, weight, weight, height);
    BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
    for (size_t i = 1; i < MEMPOOL_HEALTH_WINDOW_BLOCKS; ++i) {
        AddRemovedBlock(mempool_estimator, weight, weight, height);
        if (i < MEMPOOL_HEALTH_WINDOW_BLOCKS - 1) {
            BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
        }
    }
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
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

        // The floor estimate is cached like any other; a second call returns the same value.
        const auto cached_result = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        BOOST_REQUIRE(cached_result.has_value());
        BOOST_CHECK(cached_result->feerate == floor);
    }
    TestMemPoolEntryHelper entry;
    const auto tx_vsize = entry.FromTx(MakeRandomTx()).GetTxSize();
    const CAmount low_fee{CENT / 3000};
    const CAmount med_fee{CENT / 100};
    const CAmount high_fee{CENT / 10};
    const CAmount very_high_fee{CENT};
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
        // Expire the cached floor estimate so the denser mempool is observed.
        SetMockTime(GetTime<std::chrono::seconds>() + CACHE_LIFE + std::chrono::seconds{1});
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
        SetMockTime(GetTime<std::chrono::seconds>() + CACHE_LIFE + std::chrono::seconds{1});
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
        // Expire the sparse-result cache before expecting the estimator to observe the denser mempool.
        SetMockTime(GetTime<std::chrono::seconds>() + CACHE_LIFE + std::chrono::seconds{1});
        const auto result_conservative =
            mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        const auto result_economical =
            mempool_estimator.EstimateFeeRate(/*conservative=*/false);
        BOOST_CHECK(result_conservative.has_value());
        BOOST_CHECK(result_economical.has_value());
        BOOST_CHECK(result_economical->feerate == FeeFrac(low_fee, tx_vsize));
        BOOST_CHECK(result_conservative->feerate == FeeFrac(med_fee, tx_vsize));
        BOOST_CHECK(result_conservative->feerate_estimator == FeeRateEstimatorType::MEMPOOL_POLICY);
        BOOST_CHECK(result_economical->feerate_estimator == FeeRateEstimatorType::MEMPOOL_POLICY);
        BOOST_CHECK_EQUAL(result_conservative->returned_target, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET);
        BOOST_CHECK_EQUAL(result_economical->returned_target, MEMPOOL_FEE_ESTIMATOR_MAX_TARGET);

        // Adding another 30% of very-high-fee transactions should change the
        // estimates after recomputation, but not while the cached estimate is fresh.
        {
            LOCK2(cs_main, m_node.mempool->cs);
            while ((m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <=
                   (DEFAULT_BLOCK_MAX_WEIGHT * 105 / 100)) {
                TryAddToMempool(*m_node.mempool, entry.Fee(very_high_fee).FromTx(MakeRandomTx()));
            }
        }
        BOOST_CHECK(mempool_estimator.EstimateFeeRate(/*conservative=*/false).value().feerate == FeeFrac(low_fee, tx_vsize));
        BOOST_CHECK(mempool_estimator.EstimateFeeRate(/*conservative=*/true).value().feerate == FeeFrac(med_fee, tx_vsize));
        // Expire the cache by advancing mock time past CACHE_LIFE so the next call recomputes.
        SetMockTime(GetTime<std::chrono::seconds>() + CACHE_LIFE + std::chrono::seconds{1});
        BOOST_CHECK(mempool_estimator.EstimateFeeRate(/*conservative=*/false).value().feerate == FeeFrac(med_fee, tx_vsize));
        BOOST_CHECK(mempool_estimator.EstimateFeeRate(/*conservative=*/true).value().feerate == FeeFrac(high_fee, tx_vsize));
    }
}

BOOST_AUTO_TEST_SUITE_END()
