// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <key_io.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>


struct TestChain100NoDIP0001Setup : public TestChain100Setup {
    TestChain100NoDIP0001Setup()
        : TestChain100Setup{CBaseChainParams::REGTEST, {"-testactivationheight=dip0001@2000"}} {}
};

struct TestChain100DIP3Setup : public TestChain100Setup {
    TestChain100DIP3Setup()
        : TestChain100Setup{CBaseChainParams::REGTEST, {"-dip3params=101:500"}} {}
};


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
    const MempoolAcceptResult result = m_node.chainman->ProcessTransaction(MakeTransactionRef(coinbaseTx));

    BOOST_CHECK(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);

    // Check that the transaction hasn't been added to mempool.
    BOOST_CHECK_EQUAL(m_node.mempool->size(), initialPoolSize);

    // Check that the validation state reflects the unsuccessful attempt.
    BOOST_CHECK(result.m_state.IsInvalid());
    BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "coinbase");
    BOOST_CHECK(result.m_state.GetResult() == TxValidationResult::TX_CONSENSUS);
}

/**
 * Ensure that a negative nVersion is rejected by the mempool and in a block.
 */
BOOST_FIXTURE_TEST_CASE(tx_reject_negative_version, TestChain100DIP3Setup)
{
    const CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CMutableTransaction tx;
    tx.nVersion = -1;
    tx.vin.emplace_back(COutPoint{m_coinbase_txns[0]->GetHash(), 0});
    tx.vout.emplace_back(1 * COIN, scriptPubKey);
    FillableSigningProvider keystore;
    keystore.AddKey(coinbaseKey);
    BOOST_REQUIRE(SignSignature(keystore, *m_coinbase_txns[0], tx, 0, SIGHASH_ALL));

    {
        LOCK(cs_main);
        const MempoolAcceptResult result = m_node.chainman->ProcessTransaction(MakeTransactionRef(tx));
        BOOST_CHECK(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);
        BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "bad-txns-version");
    }

    const int height{WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Height())};
    const auto block = std::make_shared<CBlock>(CreateBlock({tx}, scriptPubKey, m_node.chainman->ActiveChainstate()));
    BOOST_CHECK(!m_node.chainman->ProcessNewBlock(block, /*force_processing=*/true, /*new_block=*/nullptr));
    BOOST_CHECK_EQUAL(WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Height()), height);
}
BOOST_AUTO_TEST_SUITE_END()
