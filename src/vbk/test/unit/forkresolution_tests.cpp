// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <consensus/validation.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <vbk/config.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>

//static CBlock CreateTestBlock(TestChain100Setup& test, std::vector<CMutableTransaction> trxs = {})
//{
//    test.coinbaseKey.MakeNewKey(true);
//    CScript scriptPubKey = CScript() << ToByteVector(test.coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//
//    return test.CreateAndProcessBlock({}, scriptPubKey);
//}
//
//static void InvalidateTestBlock(CBlockIndex* pblock)
//{
//    BlockValidationState state;
//
//    InvalidateBlock(state, Params(), pblock);
//    ActivateBestChain(state, Params());
//    mempool.clear();
//}
//
//static void ReconsiderTestBlock(CBlockIndex* pblock)
//{
//    BlockValidationState state;
//
//    {
//        LOCK(cs_main);
//        ResetBlockFailureFlags(pblock);
//    }
//    ActivateBestChain(state, Params(), std::shared_ptr<const CBlock>());
//}
//
//BOOST_AUTO_TEST_SUITE(forkresolution_tests)
//
//
//BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_1_test, TestChain100Setup)
//{
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CBlockIndex* pblock = ChainActive().Tip();
//
//    InvalidateTestBlock(pblock);
//
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CBlockIndex* pblock2 = ChainActive().Tip();
//    ReconsiderTestBlock(pblock);
//
//    BOOST_CHECK(pblock2 == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_2_test, TestChain100Setup)
//{
//    CreateTestBlock(*this);
//    CBlockIndex* pblock = ChainActive().Tip();
//    InvalidateTestBlock(pblock);
//
//    CreateTestBlock(*this);
//
//    ReconsiderTestBlock(pblock);
//
//    BOOST_CHECK(pblock == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_3_test, TestChain100Setup)
//{
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CBlockIndex* pblock = ChainActive().Tip();
//    InvalidateTestBlock(pblock);
//
//    CreateTestBlock(*this);
//    CBlockIndex* pblock2 = ChainActive().Tip();
//    InvalidateTestBlock(pblock2);
//
//    CreateTestBlock(*this);
//
//    ReconsiderTestBlock(pblock);
//    ReconsiderTestBlock(pblock2);
//
//    BOOST_CHECK(pblock == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_4_test, TestChain100Setup)
//{
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock = ChainActive().Tip();
//
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock2 = ChainActive().Tip();
//
//    InvalidateTestBlock(pblock);
//
//    CreateTestBlock(*this);
//
//    CreateTestBlock(*this);
//
//    ReconsiderTestBlock(pblock);
//
//    BOOST_CHECK(pblock2 == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_1_test, TestChain100Setup)
//{
//    CBlockIndex* pblock = ChainActive().Tip();
//    CreateTestBlock(*this);
//    InvalidateTestBlock(pblock);
//
//    for (int i = 0; i < 3; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CBlockIndex* pblock2 = ChainActive().Tip();
//    ReconsiderTestBlock(pblock);
//
//    BOOST_CHECK(pblock2 == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_2_test, TestChain100Setup)
//{
//    CBlockIndex* pblock = ChainActive().Tip();
//    InvalidateTestBlock(pblock);
//
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CBlockIndex* pblock2 = ChainActive()[100];
//    CBlockIndex* pblock3 = ChainActive().Tip();
//    InvalidateTestBlock(pblock2);
//
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    ReconsiderTestBlock(pblock);
//    ReconsiderTestBlock(pblock2);
//
//    BOOST_CHECK(pblock3 == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_3_test, TestChain100Setup)
//{
//    CBlockIndex* pblock = ChainActive().Tip();
//    InvalidateTestBlock(pblock);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock2 = ChainActive().Tip();
//    InvalidateTestBlock(pblock2);
//    CreateTestBlock(*this);
//
//    ReconsiderTestBlock(pblock);
//    ReconsiderTestBlock(pblock2);
//
//    BOOST_CHECK(pblock == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_4_test, TestChain100Setup)
//{
//    CBlockIndex* pblock = ChainActive().Tip();
//    CreateTestBlock(*this);
//    InvalidateTestBlock(pblock);
//
//    for (int i = 0; i < 3; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CBlockIndex* pblock2 = ChainActive().Tip();
//    InvalidateTestBlock(pblock2);
//
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CBlockIndex* pblock3 = ChainActive().Tip();
//
//    ReconsiderTestBlock(pblock);
//    ReconsiderTestBlock(pblock2);
//
//    BOOST_CHECK(pblock3 == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_1_test, TestChain100Setup)
//{
//    VeriBlockTest::ServicesFixture service_fixture;
//
//    InvalidateTestBlock(ChainActive().Tip());
//
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock = ChainActive().Tip();
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblockwins = ChainActive().Tip();
//
//    ON_CALL(service_fixture.util_service_mock, compareForks).WillByDefault(
//      [&](const CBlockIndex& leftForkTip, const CBlockIndex& rightForkTip) -> int {
//        if (rightForkTip.GetBlockHash() == leftForkTip.GetBlockHash())
//            return 0;
//        if (rightForkTip.GetBlockHash() == pblockwins->GetBlockHash())
//            return -1;
//        if (leftForkTip.GetBlockHash() == pblockwins->GetBlockHash())
//            return 1;
//        return 0;
//    });
//
//    InvalidateTestBlock(pblock);
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this); // Add a few blocks which it is mean that for the old variant of the fork resolution it should be the main chain
//
//    ReconsiderTestBlock(pblock);
//
//    BOOST_CHECK(pblockwins == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_1_test_negative, TestChain100Setup)
//{
//    VeriBlockTest::ServicesFixture service_fixture;
//
//    InvalidateTestBlock(ChainActive().Tip());
//
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock = ChainActive().Tip();
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblockwins = ChainActive().Tip();
//    InvalidateTestBlock(pblock);
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this); // Add a few blocks which it is mean that for the old variant of the fork resolution it should be the main chain
//
//    ReconsiderTestBlock(pblock);
//
//    BOOST_CHECK(pblockwins != ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_2_test, TestChain100Setup)
//{
//    VeriBlockTest::ServicesFixture service_fixture;
//
//    InvalidateTestBlock(ChainActive().Tip());
//
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock = ChainActive().Tip();
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    InvalidateTestBlock(pblock);
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblockwins = ChainActive().Tip();
//
//    ON_CALL(service_fixture.util_service_mock, compareForks).WillByDefault(
//      [&](const CBlockIndex& leftForkTip, const CBlockIndex& rightForkTip) -> int {
//        if (rightForkTip.GetBlockHash() == leftForkTip.GetBlockHash())
//            return 0;
//        if (rightForkTip.GetBlockHash() == pblockwins->GetBlockHash())
//            return -1;
//        if (leftForkTip.GetBlockHash() == pblockwins->GetBlockHash())
//            return 1;
//        return 0;
//    });
//
//    ReconsiderTestBlock(pblock);
//
//    BOOST_CHECK(pblockwins == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_2_test_negative, TestChain100Setup)
//{
//    VeriBlockTest::ServicesFixture service_fixture;
//
//    InvalidateTestBlock(ChainActive().Tip());
//
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock = ChainActive().Tip();
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    InvalidateTestBlock(pblock);
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblockwins = ChainActive().Tip();
//    ReconsiderTestBlock(pblock);
//
//    BOOST_CHECK(pblockwins != ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_3_test, TestChain100Setup)
//{
//    VeriBlockTest::ServicesFixture service_fixture;
//
//    InvalidateTestBlock(ChainActive().Tip());
//
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock = ChainActive().Tip();
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    InvalidateTestBlock(pblock);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock2 = ChainActive().Tip();
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblockwins = ChainActive().Tip();
//
//    ON_CALL(service_fixture.util_service_mock, compareForks).WillByDefault(
//      [&](const CBlockIndex& leftForkTip, const CBlockIndex& rightForkTip) -> int {
//        if (rightForkTip.GetBlockHash() == leftForkTip.GetBlockHash())
//            return 0;
//        if (rightForkTip.GetBlockHash() == pblockwins->GetBlockHash())
//            return -1;
//        if (leftForkTip.GetBlockHash() == pblockwins->GetBlockHash())
//            return 1;
//        return 0;
//    });
//
//    InvalidateTestBlock(pblock2);
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    ReconsiderTestBlock(pblock);
//    ReconsiderTestBlock(pblock2);
//
//    BOOST_CHECK(pblockwins == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_3_test_negative, TestChain100Setup)
//{
//    VeriBlockTest::ServicesFixture service_fixture;
//
//    InvalidateTestBlock(ChainActive().Tip());
//
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock = ChainActive().Tip();
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    InvalidateTestBlock(pblock);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock2 = ChainActive().Tip();
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblockwins = ChainActive().Tip();
//    InvalidateTestBlock(pblock2);
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    ReconsiderTestBlock(pblock);
//    ReconsiderTestBlock(pblock2);
//
//    BOOST_CHECK(pblockwins != ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_4_test, TestChain100Setup)
//{
//    VeriBlockTest::ServicesFixture service_fixture;
//
//    InvalidateTestBlock(ChainActive().Tip());
//
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock = ChainActive().Tip();
//    for (int i = 0; i < 5; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 3; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    InvalidateTestBlock(pblock);
//
//    CreateTestBlock(*this);
//
//    for (int i = 0; i < 5; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 3; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock2 = ChainActive().Tip();
//    CreateTestBlock(*this);
//
//    InvalidateTestBlock(pblock2);
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock3 = ChainActive().Tip();
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    InvalidateTestBlock(pblock3);
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblockwins = ChainActive().Tip();
//    ON_CALL(service_fixture.util_service_mock, compareForks).WillByDefault(
//      [&](const CBlockIndex& leftForkTip, const CBlockIndex& rightForkTip) -> int {
//        if (rightForkTip.GetBlockHash() == leftForkTip.GetBlockHash())
//            return 0;
//        if (rightForkTip.GetBlockHash() == pblockwins->GetBlockHash())
//            return -1;
//        if (leftForkTip.GetBlockHash() == pblockwins->GetBlockHash())
//            return 1;
//        return 0;
//    });
//
//    ReconsiderTestBlock(pblock);
//    ReconsiderTestBlock(pblock2);
//    ReconsiderTestBlock(pblock3);
//
//    BOOST_CHECK(pblockwins == ChainActive().Tip());
//}
//
//BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_4_test_negative, TestChain100Setup)
//{
//    InvalidateTestBlock(ChainActive().Tip());
//
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock = ChainActive().Tip();
//    for (int i = 0; i < 5; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 3; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 2; i++) {
//        CreateTestBlock(*this);
//    }
//
//    InvalidateTestBlock(pblock);
//
//    CreateTestBlock(*this);
//
//    for (int i = 0; i < 5; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    for (int i = 0; i < 3; i++) {
//        CreateTestBlock(*this);
//    }
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock2 = ChainActive().Tip();
//    CreateTestBlock(*this);
//
//    InvalidateTestBlock(pblock2);
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblock3 = ChainActive().Tip();
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    InvalidateTestBlock(pblock3);
//
//    CreateTestBlock(*this);
//    CreateTestBlock(*this);
//
//    CBlockIndex* pblockwins = ChainActive().Tip();
//
//    ReconsiderTestBlock(pblock);
//    ReconsiderTestBlock(pblock2);
//    ReconsiderTestBlock(pblock3);
//
//    BOOST_CHECK(pblockwins != ChainActive().Tip());
//}
//
//BOOST_AUTO_TEST_SUITE_END()
