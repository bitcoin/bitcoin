// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <kernel/mempool_entry.h>
#include <policy/fees/estimator_args.h>
#include <policy/fees/mempool_estimator.h>
#include <policy/policy.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/feefrac.h>
#include <util/fees.h>
#include <util/time.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_FIXTURE_TEST_SUITE(mempool_fee_estimator_tests, TestingSetup)

static inline CTransactionRef make_random_tx()
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

void AddRemovedBlock(MemPoolFeeRateEstimator& fee_est, int32_t removed_txs_weight, int32_t block_txs_weight, unsigned int& height)
{
    std::vector<CTransactionRef> txs;
    std::vector<RemovedMempoolTransactionInfo> removed_txs;
    TestMemPoolEntryHelper entry;
    Assert(block_txs_weight >= removed_txs_weight);
    txs.emplace_back(make_random_tx()); // Add a coinbase tx
    while (block_txs_weight > 0) {
        auto tx = make_random_tx();
        auto tx_weight = GetTransactionWeight(*tx);
        if (block_txs_weight - tx_weight < 0) break;
        txs.emplace_back(tx);
        block_txs_weight -= tx_weight;
        if (removed_txs_weight - tx_weight >= 0) {
            removed_txs.emplace_back(entry.FromTx(tx));
            removed_txs_weight -= tx_weight;
        }
    }
    fee_est.MempoolTxsRemovedForBlock(txs, removed_txs, height);
    height += 1;
}

BOOST_AUTO_TEST_CASE(MempoolFeeRateEstimator)
{
    auto mempool_estimator = MemPoolFeeRateEstimator(MempoolPolicyFeeEstPath(*m_node.args), m_node.mempool.get(), m_node.chainman.get());
    BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
    {
        const auto result = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        BOOST_CHECK(result.feerate.IsEmpty());
        BOOST_CHECK(result.errors.back() == strprintf("%s: Mempool is unreliable for fee rate estimation", FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY)));
    }
    size_t block_count = 1;
    const int64_t weight = 1000;
    unsigned int height = 100;
    // Equal weight
    while (block_count <= NUMBER_OF_BLOCKS) {
        AddRemovedBlock(mempool_estimator, weight, weight, height);
        if (block_count < NUMBER_OF_BLOCKS) {
            BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
        }
        block_count += 1;
    }
    // Total txs weight = ~6000, total removed mempool txs ~6000; similarities = 100%
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    // Adding a single block with weight less than the percentile will not make mempool unhealthy because we take the average.
    AddRemovedBlock(mempool_estimator, weight - 500, weight, height);
    // Total txs weight ~6000, total removed txs = ~5500; similarities = 91%
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    // Empty block
    // Total txs weight ~5000, total removed txs = ~4500; similarities = 90%
    AddRemovedBlock(mempool_estimator, 0, 0, height);
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    // Total txs weight ~5000, total removed txs = ~4000; similarities = 80%
    AddRemovedBlock(mempool_estimator, weight - 500, weight, height);
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    // Total txs weight ~5000, total removed txs = ~3500; similarities = 70% < 75% hence mempool is not healthy
    AddRemovedBlock(mempool_estimator, weight - 500, weight, height);
    BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
    block_count = 1;
    while (block_count <= 3) {
        AddRemovedBlock(mempool_estimator, weight, weight, height);
        if (block_count < 2) {
            BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
        }
        block_count += 1;
    }
    // Total txs weight = ~5000, total removed mempool txs ~4000; similarities = 80%
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    // Reorg out the last block
    height -= 1;
    AddRemovedBlock(mempool_estimator, weight, weight, height);
    BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
    // We are not sure now, mempool is not healthy until we have seen NUMBER_OF_BLOCKS more since the reorg
    // Test a scenario where all the next six blocks are empty
    for (size_t i = 0; i < NUMBER_OF_BLOCKS; i++) {
        AddRemovedBlock(mempool_estimator, 0, 0, height);
    }
    BOOST_CHECK(!mempool_estimator.IsMempoolHealthy());
    // Adding a block with block_txs_weight/removed_txs_weight above .75 should indicate a healthy mempool.
    AddRemovedBlock(mempool_estimator, weight, weight, height);
    BOOST_CHECK(mempool_estimator.IsMempoolHealthy());
    {
        LOCK(m_node.mempool->cs);
        BOOST_CHECK_EQUAL(m_node.mempool->GetTotalTxSize(), 0);
    }
    const CFeeRate floor_feerate = std::max(m_node.mempool->m_opts.min_relay_feerate, m_node.mempool->GetMinFee());
    const int32_t vsize{1000};
    const std::string sparse_err = strprintf("%s: Mempool is too sparse, returning max of minrelayfee and mempoolminfee: %s %s/kvB",
                                             FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY), floor_feerate.GetFeePerK(), CURRENCY_ATOM);
    {
        const auto result = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        BOOST_CHECK(!result.feerate.IsEmpty());
        BOOST_CHECK_EQUAL(result.errors.back(), sparse_err);
        BOOST_CHECK(result.feerate == FeePerVSize(floor_feerate.GetFee(vsize), vsize));
    }
    TestMemPoolEntryHelper entry;
    const auto tx_vsize = entry.FromTx(make_random_tx()).GetTxSize();
    const CAmount low_fee{CENT / 3000};
    const CAmount med_fee{CENT / 100};
    const CAmount high_fee{CENT / 10};
    // When the mempool has fewer transactions than needed to fill 95% of DEFAULT_BLOCK_MAX_WEIGHT,
    // CalculatePercentiles returns empty, but a healthy mempool still returns the floor feerate.
    {
        // Add transactions with high_fee fee until mempool transactions weight
        // is more than 25th percent of DEFAULT_BLOCK_MAX_WEIGHT
        {
            LOCK2(cs_main, m_node.mempool->cs);
            while (static_cast<int>(m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <= static_cast<int>(0.25 * DEFAULT_BLOCK_MAX_WEIGHT)) {
                TryAddToMempool(*m_node.mempool, entry.Fee(high_fee).FromTx(make_random_tx()));
            }
        }
        const auto result = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        // Cache was seeded by the empty-mempool call above, so this is a cache hit — no error re-emitted.
        BOOST_CHECK(!result.feerate.IsEmpty());
        BOOST_CHECK(result.errors.empty());
        BOOST_CHECK(result.feerate == FeePerVSize(floor_feerate.GetFee(vsize), vsize));
    }
    {
        // Add transactions with med_fee fee until mempool transactions weight
        // is more than 50th percent of DEFAULT_BLOCK_MAX_WEIGHT
        {
            LOCK2(cs_main, m_node.mempool->cs);
            while (static_cast<int>(m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <= static_cast<int>(0.5 * DEFAULT_BLOCK_MAX_WEIGHT)) {
                TryAddToMempool(*m_node.mempool, entry.Fee(med_fee).FromTx(make_random_tx()));
            }
        }
        const auto result = mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        // Still a cache hit (mempool < 95% of block weight) — floor returned, no error.
        BOOST_CHECK(!result.feerate.IsEmpty());
        BOOST_CHECK(result.errors.empty());
        BOOST_CHECK(result.feerate == FeePerVSize(floor_feerate.GetFee(vsize), vsize));
    }
    // Expire the cache by advancing mock time past CACHE_LIFE so the next call recomputes.
    SetMockTime(GetTime<std::chrono::seconds>() + CACHE_LIFE + std::chrono::seconds{1});
    // Mempool transactions are enough to provide feerate estimate
    {
        // Add low_fee transactions until mempool transactions weight
        // is more than 95th percent of DEFAULT_BLOCK_MAX_WEIGHT
        {
            LOCK2(cs_main, m_node.mempool->cs);
            while (static_cast<int>(m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <= static_cast<int>(0.95 * DEFAULT_BLOCK_MAX_WEIGHT)) {
                TryAddToMempool(*m_node.mempool, entry.Fee(low_fee).FromTx(make_random_tx()));
            }
        }
        const auto result_conservative =
            mempool_estimator.EstimateFeeRate(/*conservative=*/true);
        const auto result_economical =
            mempool_estimator.EstimateFeeRate(/*conservative=*/false);
        BOOST_CHECK(!result_conservative.feerate.IsEmpty());
        BOOST_CHECK(!result_economical.feerate.IsEmpty());
        BOOST_CHECK(result_economical.feerate == FeeFrac(low_fee, tx_vsize));
        BOOST_CHECK(result_conservative.feerate == FeeFrac(med_fee, tx_vsize));
    }
}

BOOST_AUTO_TEST_SUITE_END()
