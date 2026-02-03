// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/feerate.h>
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

BOOST_AUTO_TEST_CASE(MempoolFeeRateEstimator)
{
    auto mempool_estimator = MemPoolFeeRateEstimator(m_node.mempool.get(), m_node.chainman.get());
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
    // Test when there are not enough mempool transactions to get an accurate estimate
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
        BOOST_CHECK(!result.feerate.IsEmpty());
        BOOST_CHECK_EQUAL(result.errors.back(), sparse_err);
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
        BOOST_CHECK(!result.feerate.IsEmpty());
        BOOST_CHECK_EQUAL(result.errors.back(), sparse_err);
        BOOST_CHECK(result.feerate == FeePerVSize(floor_feerate.GetFee(vsize), vsize));
    }
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
