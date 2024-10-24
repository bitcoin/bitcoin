// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/feerate.h>
#include <policy/forecaster.h>
#include <policy/forecaster_util.h>
#include <policy/forecasters/mempool.h>
#include <random.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <validation.h>


#include <test/util/setup_common.h>

#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(mempoolforecaster_tests, TestChain100Setup)

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

BOOST_AUTO_TEST_CASE(MempoolEstimator)
{
    auto mempool_fee_estimator = std::make_unique<MemPoolForecaster>(m_node.mempool.get(), &(m_node.chainman->ActiveChainstate()));

    LOCK2(cs_main, m_node.mempool->cs);
    {
        // Test when targetBlocks > MEMPOOL_FORECAST_MAX_TARGET
        const auto fee_estimate = mempool_fee_estimator->EstimateFee(MEMPOOL_FORECAST_MAX_TARGET + 1);
        BOOST_CHECK(fee_estimate.empty() == true);
        BOOST_CHECK(fee_estimate.m_error_ptr.value() == strprintf("Confirmation target %s is above maximum limit of %s, mempool conditions might change and forecasts above %s block may be unreliable",
                                                                  MEMPOOL_FORECAST_MAX_TARGET + 1, MEMPOOL_FORECAST_MAX_TARGET, MEMPOOL_FORECAST_MAX_TARGET));
    }

    BOOST_CHECK(m_node.mempool->GetTotalTxSize() == 0);
    TestMemPoolEntryHelper entry;

    const CAmount low_fee{CENT / 3000};
    const CAmount med_fee{CENT / 100};
    const CAmount high_fee{CENT / 10};

    // Test when there are not enough mempool transactions to get an accurate estimate
    {
        // Add transactions with high_fee fee until mempool transactions weight is more than 25th percent of DEFAULT_BLOCK_MAX_WEIGHT
        while (static_cast<int>(m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <= static_cast<int>(0.25 * DEFAULT_BLOCK_MAX_WEIGHT)) {
            m_node.mempool->addUnchecked(entry.Fee(high_fee).FromTx(make_random_tx()));
        }
        const auto fee_estimate = mempool_fee_estimator->EstimateFee(MEMPOOL_FORECAST_MAX_TARGET);
        BOOST_CHECK(fee_estimate.empty() == true);
        BOOST_CHECK(fee_estimate.m_error_ptr.value() == "No enough transactions in the mempool to provide a feerate forecast");
    }

    {
        // Add transactions with med_fee fee until mempool transactions weight is more than 50th percent of DEFAULT_BLOCK_MAX_WEIGHT
        while (static_cast<int>(m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <= static_cast<int>(0.5 * DEFAULT_BLOCK_MAX_WEIGHT)) {
            m_node.mempool->addUnchecked(entry.Fee(med_fee).FromTx(make_random_tx()));
        }
        const auto fee_estimate = mempool_fee_estimator->EstimateFee(MEMPOOL_FORECAST_MAX_TARGET);
        BOOST_CHECK(fee_estimate.empty() == true);
        BOOST_CHECK(fee_estimate.m_error_ptr.value() == "No enough transactions in the mempool to provide a feerate forecast");
    }

    // Mempool transactions are enough to provide feerate estimate
    {
        // Add low_fee transactions until mempool transactions weight is more than 75th percent of DEFAULT_BLOCK_MAX_WEIGHT
        while (static_cast<int>(m_node.mempool->GetTotalTxSize() * WITNESS_SCALE_FACTOR) <= static_cast<int>(0.75 * DEFAULT_BLOCK_MAX_WEIGHT)) {
            const auto txref = make_random_tx();
            m_node.mempool->addUnchecked(entry.Fee(low_fee).FromTx(txref));
        }

        const auto fee_estimate = mempool_fee_estimator->EstimateFee(MEMPOOL_FORECAST_MAX_TARGET);
        BOOST_CHECK(fee_estimate.empty() == false);
        const auto tx_vsize = entry.FromTx(make_random_tx()).GetTxSize();
        BOOST_CHECK(fee_estimate.m_opt.low_priority == CFeeRate(med_fee, tx_vsize));
        BOOST_CHECK(fee_estimate.m_opt.high_priority == CFeeRate(high_fee, tx_vsize));
    }
}

BOOST_AUTO_TEST_SUITE_END()
