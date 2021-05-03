// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(txvalidation_tests)

/**
 * Ensure that the mempool won't accept coinbase transactions.
 */
BOOST_FIXTURE_TEST_CASE(tx_mempool_reject_coinbase, TestChain100Setup)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CMutableTransaction coinbaseTx;

    coinbaseTx.nVersion = 1;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vout.resize(1);
    coinbaseTx.vin[0].scriptSig = CScript() << OP_11 << OP_EQUAL;
    coinbaseTx.vout[0].nValue = 1 * CENT;
    coinbaseTx.vout[0].scriptPubKey = scriptPubKey;

    BOOST_CHECK(CTransaction(coinbaseTx).IsCoinBase());

    LOCK(cs_main);

    unsigned int initialPoolSize = m_node.mempool->size();
    const MempoolAcceptResult result = AcceptToMemoryPool(::ChainstateActive(), *m_node.mempool, MakeTransactionRef(coinbaseTx),
                true /* bypass_limits */);

    BOOST_CHECK(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);

    // Check that the transaction hasn't been added to mempool.
    BOOST_CHECK_EQUAL(m_node.mempool->size(), initialPoolSize);

    // Check that the validation state reflects the unsuccessful attempt.
    BOOST_CHECK(result.m_state.IsInvalid());
    BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "coinbase");
    BOOST_CHECK(result.m_state.GetResult() == TxValidationResult::TX_CONSENSUS);
}

BOOST_FIXTURE_TEST_CASE(package_tests, TestChain100Setup)
{
    LOCK(cs_main);
    unsigned int initialPoolSize = m_node.mempool->size();

    // Parent and Child Package
    CKey parent_key;
    parent_key.MakeNewKey(true);
    CScript parent_locking_script = GetScriptForDestination(PKHash(parent_key.GetPubKey()));
    auto mtx_parent = CreateValidMempoolTransaction(/* input_transaction */ m_coinbase_txns[0], /* vout */ 0,
                                                    /* input_height */ 0, /* input_signing_key */ coinbaseKey,
                                                    /* output_destination */ parent_locking_script,
                                                    /* output_amount */ CAmount(49 * COIN), /* submit */ false);
    CTransactionRef tx_parent = MakeTransactionRef(mtx_parent);

    CKey child_key;
    child_key.MakeNewKey(true);
    CScript child_locking_script = GetScriptForDestination(PKHash(child_key.GetPubKey()));
    auto mtx_child = CreateValidMempoolTransaction(/* input_transaction */ tx_parent, /* vout */ 0,
                                                   /* input_height */ 101, /* input_signing_key */ parent_key,
                                                   /* output_destination */ child_locking_script,
                                                   /* output_amount */ CAmount(48 * COIN), /* submit */ false);
    CTransactionRef tx_child = MakeTransactionRef(mtx_child);
    const auto result_parent_child = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool, {tx_parent, tx_child}, /* test_accept */ true);
    BOOST_CHECK_MESSAGE(result_parent_child.m_state.IsValid(),
                        "Package validation unexpectedly failed: " << result_parent_child.m_state.GetRejectReason());
    auto it_parent = result_parent_child.m_tx_results.find(tx_parent->GetWitnessHash());
    auto it_child = result_parent_child.m_tx_results.find(tx_child->GetWitnessHash());
    BOOST_CHECK(it_parent != result_parent_child.m_tx_results.end());
    BOOST_CHECK_MESSAGE(it_parent->second.m_state.IsValid(),
                        "Package validation unexpectedly failed: " << it_parent->second.m_state.GetRejectReason());
    BOOST_CHECK(it_child != result_parent_child.m_tx_results.end());
    BOOST_CHECK_MESSAGE(it_child->second.m_state.IsValid(),
                        "Package validation unexpectedly failed: " << it_child->second.m_state.GetRejectReason());

    // Check that mempool size hasn't changed.
    BOOST_CHECK_EQUAL(m_node.mempool->size(), initialPoolSize);
}
BOOST_AUTO_TEST_SUITE_END()
