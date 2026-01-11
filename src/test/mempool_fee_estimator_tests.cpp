// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/estimator.h>
#include <policy/fees/mempool_estimator.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/feefrac.h>
#include <util/fees.h>
#include <util/strencodings.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <memory>
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
    int conf_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET + 1;
    std::string data_err = strprintf("%s: Unable to provide a fee rate due to insufficient data", FeeRateEstimatorTypeToString(FeeRateEstimatorType::MEMPOOL_POLICY));
    {
        const auto result = mempool_estimator.EstimateFeeRate(conf_target, /*conservative=*/true);
        BOOST_CHECK(result.feerate.IsEmpty());
        BOOST_CHECK_EQUAL(result.errors.back(), data_err);
    }

    {
        LOCK(m_node.mempool->cs);
        BOOST_CHECK_EQUAL(m_node.mempool->GetTotalTxSize(), 0);
    }
    TestMemPoolEntryHelper entry;
    const CAmount low_fee{CENT / 3000};
    const CAmount med_fee{CENT / 100};
    const CAmount high_fee{CENT / 10};
    conf_target = MEMPOOL_FEE_ESTIMATOR_MAX_TARGET;
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
        const auto result = mempool_estimator.EstimateFeeRate(conf_target, /*conservative=*/true);
        BOOST_CHECK(result.feerate.IsEmpty());
        BOOST_CHECK_EQUAL(result.errors.back(), data_err);
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
        const auto result = mempool_estimator.EstimateFeeRate(conf_target, /*conservative=*/true);
        BOOST_CHECK(result.feerate.IsEmpty());
        BOOST_CHECK_EQUAL(result.errors.back(), data_err);
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
            mempool_estimator.EstimateFeeRate(conf_target, /*conservative=*/true);
        const auto result_economical =
            mempool_estimator.EstimateFeeRate(conf_target, /*conservative=*/false);
        BOOST_CHECK(!result_conservative.feerate.IsEmpty());
        BOOST_CHECK(!result_economical.feerate.IsEmpty());
        const auto tx_vsize = entry.FromTx(make_random_tx()).GetTxSize();
        BOOST_CHECK(result_economical.feerate == FeeFrac(low_fee, tx_vsize));
        BOOST_CHECK(result_conservative.feerate == FeeFrac(med_fee, tx_vsize));
    }
}

BOOST_AUTO_TEST_SUITE_END()
