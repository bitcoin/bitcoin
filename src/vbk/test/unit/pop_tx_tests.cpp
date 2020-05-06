// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <vbk/test/util/e2e_fixture.hpp>
#include <vbk/test/util/tx.hpp>
#include <vbk/pop_service_impl.hpp>

BOOST_AUTO_TEST_SUITE(pop_tx_tests)

BOOST_FIXTURE_TEST_CASE(No_mempool_for_bad_payloads_pop_tx_test, E2eFixture)
{
    unsigned int initialPoolSize = mempool.size();
    auto tip = ChainActive().Tip();
    BOOST_CHECK(tip != nullptr);
    auto atv = endorseAltBlock(tip->GetBlockHash(), {});
    // erase signature to make ATV check fail
    atv.transaction.signature = std::vector<unsigned char>(atv.transaction.signature.size(), 0);
    CScript sig;
    sig << atv.toVbkEncoding() << OP_CHECKATV;
    sig << OP_CHECKPOP;
    auto popTx = VeriBlock::MakePopTx(sig);

    BOOST_CHECK(VeriBlock::isPopTx(CTransaction(popTx)));

    TxValidationState state;
    auto tx_ref = MakeTransactionRef<const CMutableTransaction&>(popTx);
    {
        LOCK(cs_main);
        auto result = AcceptToMemoryPool(mempool, state, tx_ref,
            nullptr /* plTxnReplaced */, false /* bypass_limits */, 0 /* nAbsurdFee */, false /* test accept */);
        BOOST_CHECK(!result);
    }
    BOOST_CHECK_EQUAL(mempool.size(), initialPoolSize);
}

BOOST_FIXTURE_TEST_CASE(Payloads_expiration_pop_tx_test, E2eFixture)
{
    // Generate a 600-block chain:
    coinbaseKey.MakeNewKey(true);
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    for (int i = 0; i < 500; i++) {
        std::vector<CMutableTransaction> noTxns;
        CBlock b = CreateAndProcessBlock(noTxns, scriptPubKey);
        m_coinbase_txns.push_back(b.vtx[0]);
    }

    assert(ChainActive().Tip() != nullptr);
    assert(ChainActive().Tip()->nHeight == 600);

    unsigned int initialPoolSize = mempool.size();
    auto tip = ChainActive().Tip();
    BOOST_CHECK(tip != nullptr);
    auto atv = endorseAltBlock(tip->GetAncestor(40)->GetBlockHash(), {});
    CScript sig;
    sig << atv.toVbkEncoding() << OP_CHECKATV;
    sig << OP_CHECKPOP;
    auto popTx = VeriBlock::MakePopTx(sig);

    BOOST_CHECK(VeriBlock::isPopTx(CTransaction(popTx)));

    TxValidationState state;
    auto tx_ref = MakeTransactionRef<const CMutableTransaction&>(popTx);
    {
        LOCK(cs_main);
        auto result = AcceptToMemoryPool(mempool, state, tx_ref,
            nullptr /* plTxnReplaced */, true /* bypass_limits */, 0 /* nAbsurdFee */, false /* test accept */);
        BOOST_CHECK(result);
        BOOST_CHECK_EQUAL(mempool.size(), initialPoolSize + 1);
    }

    atv = endorseAltBlock(tip->GetBlockHash(), {});
    sig.clear();
    sig << atv.toVbkEncoding() << OP_CHECKATV;
    sig << OP_CHECKPOP;
    popTx = VeriBlock::MakePopTx(sig);

    BOOST_CHECK(VeriBlock::isPopTx(CTransaction(popTx)));

    tx_ref = MakeTransactionRef<const CMutableTransaction&>(popTx);
    {
        LOCK(cs_main);
        auto result = AcceptToMemoryPool(mempool, state, tx_ref,
            nullptr /* plTxnReplaced */, false /* bypass_limits */, 0 /* nAbsurdFee */, false /* test accept */);
        BOOST_CHECK(result);
        BOOST_CHECK_EQUAL(mempool.size(), initialPoolSize + 1);
    }
}

//
//BOOST_FIXTURE_TEST_CASE(DisconnectBlock_restore_iputs_ignore_pop_tx_test, TestChain100Setup)
//{
//    CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
//
//    BOOST_CHECK(VeriBlock::isPopTx(CTransaction(popTx)));
//
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//    CBlock block = CreateAndProcessBlock({popTx}, scriptPubKey);
//
//    BOOST_CHECK(ChainActive().Tip()->GetBlockHash() == block.GetHash()); // check that our block is the Tip of the current blockChain
//    {
//        LOCK(cs_main);
//        CCoinsViewCache view(&ChainstateActive().CoinsTip());
//        BOOST_CHECK(ChainstateActive().DisconnectBlock(block, ChainActive().Tip(), view) == DISCONNECT_OK);
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE(GetP2SHSigOpCount_pop_tx_test, TestChain100Setup)
//{
//    CTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
//
//    BOOST_CHECK(VeriBlock::isPopTx(popTx));
//
//    {
//        LOCK(cs_main);
//        CCoinsViewCache view(&ChainstateActive().CoinsTip());
//        BOOST_CHECK_EQUAL(GetP2SHSigOpCount(popTx, view), 0);
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE(GetTransactionSigOpCost_pop_tx_test, TestChain100Setup)
//{
//    CTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
//
//    BOOST_CHECK(VeriBlock::isPopTx(popTx));
//
//    {
//        LOCK(cs_main);
//        CCoinsViewCache view(&ChainstateActive().CoinsTip());
//        int64_t expected_value = GetLegacySigOpCount(popTx) * WITNESS_SCALE_FACTOR;
//        BOOST_CHECK_EQUAL(GetTransactionSigOpCost(popTx, view, 0), expected_value);
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE(AreInputsStandard_pop_tx_test, TestChain100Setup)
//{
//    CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
//
//    BOOST_CHECK(VeriBlock::isPopTx(CTransaction(popTx)));
//
//    {
//        LOCK(cs_main);
//        CCoinsViewCache view(&ChainstateActive().CoinsTip());
//
//        BOOST_CHECK_EQUAL(AreInputsStandard(CTransaction(popTx), view), true);
//
//        //set another hex value
//        popTx.vin[0].prevout.hash.SetHex("ffff");
//
//        BOOST_CHECK_EQUAL(AreInputsStandard(CTransaction(popTx), view), false);
//    }
//}
//
//BOOST_AUTO_TEST_CASE(IsStandardTx_pop_tx_test)
//{
//    VeriBlock::setService<VeriBlock::Config>(new VeriBlock::Config());
//    CTransaction popTx = VeriBlockTest::makePopTx({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {{1}});
//
//    std::string reason;
//
//    BOOST_CHECK(IsStandardTx(popTx, DEFAULT_PERMIT_BAREMULTISIG, CFeeRate(DUST_RELAY_TX_FEE), reason));
//
//    VeriBlock::getService<VeriBlock::Config>().max_pop_script_size = 2;
//
//    BOOST_CHECK(!IsStandardTx(popTx, DEFAULT_PERMIT_BAREMULTISIG, CFeeRate(DUST_RELAY_TX_FEE), reason));
//}
//
//BOOST_FIXTURE_TEST_CASE(EvalScript_script_size_pop_tx_test, BasicTestingSetup)
//{
//    CTransaction popTx = VeriBlockTest::makePopTx({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {{1}});
//
//    std::vector<std::vector<unsigned char>> stack;
//    unsigned int flags = 0 | SCRIPT_VERIFY_POP;
//    ScriptError error(SCRIPT_ERR_UNKNOWN_ERROR);
//    PrecomputedTransactionData data(popTx);
//
//    BOOST_CHECK(EvalScript(stack, popTx.vin[0].scriptSig, flags, CachingTransactionSignatureChecker(&popTx, 0, 0, false, data), SigVersion::BASE, &error));
//
//    VeriBlock::getService<VeriBlock::Config>().max_pop_script_size = 2;
//
//    BOOST_CHECK(!EvalScript(stack, popTx.vin[0].scriptSig, flags, CachingTransactionSignatureChecker(&popTx, 0, 0, false, data), SigVersion::BASE, &error));
//}
//
//BOOST_AUTO_TEST_CASE(GetValueIn_pop_tx_test)
//{
//    CTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
//
//    CCoinsView coinsDummy;
//    CCoinsViewCache coins(&coinsDummy);
//
//    BOOST_CHECK_EQUAL(coins.GetValueIn(popTx), 0);
//}
//
//BOOST_AUTO_TEST_CASE(HaveInputs_pop_tx_test)
//{
//    CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
//
//    CCoinsView coinsDummy;
//    CCoinsViewCache coins(&coinsDummy);
//
//    BOOST_CHECK(coins.HaveInputs(CTransaction(popTx)));
//
//    popTx.vin[0].prevout.hash.SetHex("ffff");
//
//    BOOST_CHECK(!coins.HaveInputs(CTransaction(popTx)));
//}
//
//BOOST_FIXTURE_TEST_CASE(CheckSequenceLocks_pop_tx_test, TestChain100Setup)
//{
//    CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
//
//    LockPoints lp;
//
//    LOCK(cs_main);
//    LOCK(::mempool.cs);
//
//    BOOST_CHECK(CheckSequenceLocks(::mempool, CTransaction(popTx), STANDARD_LOCKTIME_VERIFY_FLAGS, &lp));
//
//    popTx.vin[0].prevout.hash.SetHex("ffff");
//
//    BOOST_CHECK(!CheckSequenceLocks(::mempool, CTransaction(popTx), STANDARD_LOCKTIME_VERIFY_FLAGS, &lp));
//}
//
//
//BOOST_FIXTURE_TEST_CASE(GetBlockWeight_pop_tx_test, TestChain100Setup)
//{
//    CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
//
//    BOOST_CHECK(VeriBlock::isPopTx(CTransaction(popTx)));
//
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//    CBlock block = CreateAndProcessBlock({popTx}, scriptPubKey);
//
//    int64_t block_size1 = GetBlockWeight(block);
//
//    block.vtx.erase(block.vtx.begin() + 1); // remove pop tx from block
//
//    int64_t block_size2 = GetBlockWeight(block);
//
//    BOOST_CHECK(block_size1 == block_size2);
//}
//
//BOOST_FIXTURE_TEST_CASE(ExtractMatches_pop_tx_test, TestChain100Setup)
//{
//    class CPartialMerkleTreeTest : public CPartialMerkleTree
//    {
//    public:
//        CPartialMerkleTreeTest()
//        {
//            // more than MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT but less than MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT + max_pop_tx_amount
//            nTransactions = MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT + 1;
//
//            size_t size = 1;
//            vHash.resize(size);
//            vHash[0] = uint256S("0x123123123123123123123");
//            vBits.resize(size);
//
//            fBad = false;
//        }
//    };
//
//    CPartialMerkleTreeTest tree;
//    std::vector<uint256> vMatch;
//    std::vector<unsigned int> vnIndex;
//    uint256 res = tree.ExtractMatches(vMatch, vnIndex);
//    BOOST_CHECK(res == uint256S("0x0000000000000000000000000000000000000000000123123123123123123123"));
//}

BOOST_AUTO_TEST_SUITE_END()
