// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <consensus/validation.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <vbk/pop_service.hpp>
#include <vbk/test/util/e2e_fixture.hpp>
#include <veriblock/pop.hpp>


using altintegration::BtcBlock;
using altintegration::MockMiner;
using altintegration::PublicationData;
using altintegration::VbkBlock;
using altintegration::VTB;

struct FrFixture : public E2eFixture {
    CBlockIndex* tip;

    FrFixture()
    {
        tip = MineToKeystone();
        BOOST_CHECK(tip != nullptr);
    }
};

BOOST_AUTO_TEST_SUITE(forkresolution_tests)

BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_1_test, FrFixture)
{
    //make sure keystone interval is big enough so we do not cross it
    BOOST_CHECK(pop->getConfig().getAltParams().getKeystoneInterval() >= 3);

    for (size_t i = 0; i < 2; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock = ChainActive().Tip();

    InvalidateTestBlock(pblock);

    for (size_t i = 0; i < 3; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock2 = ChainActive().Tip();
    ReconsiderTestBlock(pblock);

    BOOST_CHECK(pblock2->GetBlockHash() == ChainActive().Tip()->GetBlockHash());
}

BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_2_test, FrFixture)
{
    //make sure keystone interval is big enough so we do not cross it
    BOOST_CHECK(pop->getConfig().getAltParams().getKeystoneInterval() >= 2);

    CreateAndProcessBlock({}, cbKey);
    CBlockIndex* pblock = ChainActive().Tip();
    InvalidateTestBlock(pblock);

    CreateAndProcessBlock({}, cbKey);

    ReconsiderTestBlock(pblock);

    BOOST_CHECK(pblock == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_3_test, FrFixture)
{
    //make sure keystone interval is big enough so we do not cross it
    BOOST_CHECK(pop->getConfig().getAltParams().getKeystoneInterval() >= 3);

    for (size_t i = 0; i < 2; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock = ChainActive().Tip();
    InvalidateTestBlock(pblock);

    CBlockIndex* pblock2 = ChainActive().Tip();
    InvalidateTestBlock(pblock2);

    ReconsiderTestBlock(pblock);
    ReconsiderTestBlock(pblock2);

    BOOST_CHECK(pblock == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_4_test, FrFixture)
{
    //make sure keystone interval is big enough so we do not cross it
    BOOST_CHECK(pop->getConfig().getAltParams().getKeystoneInterval() >= 3);

    CreateAndProcessBlock({}, cbKey);

    CBlockIndex* pblock = ChainActive().Tip();

    CreateAndProcessBlock({}, cbKey);

    CBlockIndex* pblock2 = ChainActive().Tip();

    InvalidateTestBlock(pblock);

    CreateAndProcessBlock({}, cbKey);

    CreateAndProcessBlock({}, cbKey);

    ReconsiderTestBlock(pblock);

    BOOST_CHECK(pblock2 == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_1_test, FrFixture)
{
    CBlockIndex* keystone = ChainActive().Tip();
    CreateAndProcessBlock({}, cbKey);
    InvalidateTestBlock(keystone);

    for (size_t i = 0; i < 3; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock2 = ChainActive().Tip();
    ReconsiderTestBlock(keystone);

    BOOST_CHECK(pblock2 == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_2_test, FrFixture)
{
    CBlockIndex* keystone = ChainActive().Tip();
    InvalidateTestBlock(keystone);

    for (size_t i = 0; i < 2; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock2 = ChainActive()[100];
    CBlockIndex* pblock3 = ChainActive().Tip();
    InvalidateTestBlock(pblock2);

    for (size_t i = 0; i < 2; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    ReconsiderTestBlock(keystone);
    ReconsiderTestBlock(pblock2);

    BOOST_CHECK(pblock3 == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_3_test, FrFixture)
{
    CBlockIndex* keystone = ChainActive().Tip();
    InvalidateTestBlock(keystone);
    CreateAndProcessBlock({}, cbKey);

    CBlockIndex* pblock2 = ChainActive().Tip();
    InvalidateTestBlock(pblock2);
    CreateAndProcessBlock({}, cbKey);

    ReconsiderTestBlock(keystone);
    ReconsiderTestBlock(pblock2);

    BOOST_CHECK(keystone == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_4_test, FrFixture)
{
    CBlockIndex* keystone = ChainActive().Tip();
    CreateAndProcessBlock({}, cbKey);
    InvalidateTestBlock(keystone);

    for (size_t i = 0; i < 3; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock2 = ChainActive().Tip();
    InvalidateTestBlock(pblock2);

    for (int i = 0; i < 2; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock3 = ChainActive().Tip();

    ReconsiderTestBlock(keystone);
    ReconsiderTestBlock(pblock2);

    BOOST_CHECK(pblock3 == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_invalid_1_test, FrFixture)
{
    auto& config = VeriBlock::GetPop().getConfig();
    for (size_t i = 0; i < config.alt->getEndorsementSettlementInterval() + 2; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock = ChainActive().Tip();

    auto* endorsedBlockIndex = ChainActive()[config.alt->getEndorsementSettlementInterval() - 2];

    endorseAltBlockAndMine(endorsedBlockIndex->GetBlockHash(), 10);
    CreateAndProcessBlock({}, cbKey);

    InvalidateTestBlock(pblock);

    // Add a few blocks so that the native implementation of the fork resolution should activate main chain
    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey); 

    ReconsiderTestBlock(pblock);
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_1_test, FrFixture)
{
    auto& config = VeriBlock::GetPop().getConfig();
    int startHeight = ChainActive().Tip()->nHeight;

    // mine a keystone interval + 20 of blocks
    for (size_t i = 0; i < config.alt->getKeystoneInterval() + 20; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    auto* atip = ChainActive().Tip();
    auto* forkBlockNext = atip->GetAncestor(startHeight + config.alt->getKeystoneInterval());
    auto* forkBlock = forkBlockNext->pprev;
    InvalidateTestBlock(forkBlockNext);

    for (size_t i = 0; i < 12; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    auto* btip = ChainActive().Tip();

    BOOST_CHECK_EQUAL(atip->nHeight, startHeight + config.alt->getKeystoneInterval() + 20);
    BOOST_CHECK_EQUAL(btip->nHeight, startHeight + config.alt->getKeystoneInterval() + 11);
    BOOST_CHECK_EQUAL(forkBlock->nHeight + 1, startHeight + config.alt->getKeystoneInterval());

    BOOST_CHECK(btip->GetBlockHash() == ChainActive().Tip()->GetBlockHash());
    auto* endorsedBlock = btip->GetAncestor(btip->nHeight - 6);
    CBlock expectedTip = endorseAltBlockAndMine(endorsedBlock->GetBlockHash(), 10);

    BOOST_CHECK(expectedTip.GetHash() == ChainActive().Tip()->GetBlockHash());

    ReconsiderTestBlock(forkBlockNext);

    BOOST_CHECK(expectedTip.GetHash() == ChainActive().Tip()->GetBlockHash());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_2_test, FrFixture)
{
    // this scenario tests a case when longer fork without endorsements crosses
    // a keystone and shorter fork with POP endorsements does not cross
    // a keystone

    auto& config = VeriBlock::GetPop().getConfig();
    int startHeight = ChainActive().Tip()->nHeight;

    //make sure keystone interval is big enough so we do not cross it
    BOOST_CHECK(pop->getConfig().getAltParams().getKeystoneInterval() >= 3);

    // mine a keystone interval + 5 of blocks
    for (size_t i = 0; i < config.alt->getKeystoneInterval() + 5; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    auto* atip = ChainActive().Tip();
    auto* forkBlockNext = atip->GetAncestor(startHeight + 1);
    auto* forkBlock = forkBlockNext->pprev;
    InvalidateTestBlock(forkBlockNext);

    for (size_t i = 0; i < 2; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    auto* btip = ChainActive().Tip();

    BOOST_CHECK_EQUAL(atip->nHeight, startHeight + config.alt->getKeystoneInterval() + 5);
    BOOST_CHECK_EQUAL(btip->nHeight, startHeight + 2);
    BOOST_CHECK_EQUAL(forkBlock->nHeight, startHeight);

    BOOST_CHECK(btip->GetBlockHash() == ChainActive().Tip()->GetBlockHash());
    auto* endorsedBlock = btip->GetAncestor(btip->nHeight - 1);
    CBlock expectedTip = endorseAltBlockAndMine(endorsedBlock->GetBlockHash(), 10);

    BOOST_CHECK(expectedTip.GetHash() == ChainActive().Tip()->GetBlockHash());

    ReconsiderTestBlock(forkBlockNext);

    BOOST_CHECK(atip->GetBlockHash() == ChainActive().Tip()->GetBlockHash());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_without_pop_1_test, FrFixture)
{
    // Similar scenario like in crossing_keystone_with_pop_1_test case
    // The main difference that we do not endorse any block so the best chain is the highest chain
    auto& config = VeriBlock::GetPop().getConfig();
    int startHeight = ChainActive().Tip()->nHeight;

    // mine a keystone interval of blocks
    for (size_t i = 0; i < config.alt->getKeystoneInterval() + 20; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    auto* atip = ChainActive().Tip();
    auto* forkBlockNext = atip->GetAncestor(startHeight + config.alt->getKeystoneInterval());
    InvalidateTestBlock(forkBlockNext);

    for (size_t i = 0; i < 12; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    auto* btip = ChainActive().Tip();

    BOOST_CHECK(btip->GetBlockHash() == ChainActive().Tip()->GetBlockHash());
    CreateAndProcessBlock({}, cbKey);
    auto* expectedTip = ChainActive().Tip();

    BOOST_CHECK(expectedTip->GetBlockHash() == ChainActive().Tip()->GetBlockHash());

    ReconsiderTestBlock(forkBlockNext);

    BOOST_CHECK(atip->GetBlockHash() == ChainActive().Tip()->GetBlockHash());
}

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
BOOST_AUTO_TEST_SUITE_END()
