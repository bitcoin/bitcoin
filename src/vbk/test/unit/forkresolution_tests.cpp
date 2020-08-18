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
#include <veriblock/mock_miner.hpp>

using altintegration::BtcBlock;
using altintegration::MockMiner;
using altintegration::PublicationData;
using altintegration::VbkBlock;
using altintegration::VTB;

BOOST_AUTO_TEST_SUITE(forkresolution_tests)

BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_1_test, E2eFixture)
{
    for (int i = 0; i < 2; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock = ChainActive().Tip();

    InvalidateTestBlock(pblock);

    for (int i = 0; i < 3; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock2 = ChainActive().Tip();
    ReconsiderTestBlock(pblock);

    BOOST_CHECK(pblock2->GetBlockHash() == ChainActive().Tip()->GetBlockHash());
}

BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_2_test, E2eFixture)
{
    CreateAndProcessBlock({}, cbKey);
    CBlockIndex* pblock = ChainActive().Tip();
    InvalidateTestBlock(pblock);

    CreateAndProcessBlock({}, cbKey);

    ReconsiderTestBlock(pblock);

    BOOST_CHECK(pblock == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_3_test, E2eFixture)
{
    for (int i = 0; i < 2; i++) {
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

BOOST_FIXTURE_TEST_CASE(not_crossing_keystone_case_4_test, E2eFixture)
{
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

BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_1_test, E2eFixture)
{
    CBlockIndex* pblock = ChainActive().Tip();
    CreateAndProcessBlock({}, cbKey);
    InvalidateTestBlock(pblock);

    for (int i = 0; i < 3; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock2 = ChainActive().Tip();
    ReconsiderTestBlock(pblock);

    BOOST_CHECK(pblock2 == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_2_test, E2eFixture)
{
    CBlockIndex* pblock = ChainActive().Tip();
    InvalidateTestBlock(pblock);

    for (int i = 0; i < 2; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock2 = ChainActive()[100];
    CBlockIndex* pblock3 = ChainActive().Tip();
    InvalidateTestBlock(pblock2);

    for (int i = 0; i < 2; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    ReconsiderTestBlock(pblock);
    ReconsiderTestBlock(pblock2);

    BOOST_CHECK(pblock3 == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_3_test, E2eFixture)
{
    CBlockIndex* pblock = ChainActive().Tip();
    InvalidateTestBlock(pblock);
    CreateAndProcessBlock({}, cbKey);

    CBlockIndex* pblock2 = ChainActive().Tip();
    InvalidateTestBlock(pblock2);
    CreateAndProcessBlock({}, cbKey);

    ReconsiderTestBlock(pblock);
    ReconsiderTestBlock(pblock2);

    BOOST_CHECK(pblock == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_case_4_test, E2eFixture)
{
    CBlockIndex* pblock = ChainActive().Tip();
    CreateAndProcessBlock({}, cbKey);
    InvalidateTestBlock(pblock);

    for (int i = 0; i < 3; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock2 = ChainActive().Tip();
    InvalidateTestBlock(pblock2);

    for (int i = 0; i < 2; i++) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock3 = ChainActive().Tip();

    ReconsiderTestBlock(pblock);
    ReconsiderTestBlock(pblock2);

    BOOST_CHECK(pblock3 == ChainActive().Tip());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_invalid_1_test, E2eFixture)
{
    auto& config = *VeriBlock::GetPop().config;
    for (int i = 0; i < config.alt->getEndorsementSettlementInterval() + 2; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    CBlockIndex* pblock = ChainActive().Tip();

    auto* endorsedBlockIndex = ChainActive()[config.alt->getEndorsementSettlementInterval() - 2];

    endorseAltBlockAndMine(endorsedBlockIndex->GetBlockHash(), 10);
    CreateAndProcessBlock({}, cbKey);

    InvalidateTestBlock(pblock);

    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey);
    CreateAndProcessBlock({}, cbKey); // Add a few blocks which it is mean that for the old variant of the fork resolution it should be the main chain

    ReconsiderTestBlock(pblock);
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_with_pop_1_test, E2eFixture)
{
    // mine 20 blocks
    for (int i = 0; i < 20; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    auto* atip = ChainActive().Tip();
    auto* forkBlockNext = atip->GetAncestor(atip->nHeight - 13);
    auto* forkBlock = forkBlockNext->pprev;
    InvalidateTestBlock(forkBlockNext);

    for (int i = 0; i < 12; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    auto* btip = ChainActive().Tip();

    BOOST_CHECK_EQUAL(atip->nHeight, 120);
    BOOST_CHECK_EQUAL(btip->nHeight, 118);
    BOOST_CHECK_EQUAL(forkBlock->nHeight, 106);

    BOOST_CHECK(btip->GetBlockHash() == ChainActive().Tip()->GetBlockHash());
    auto* endorsedBlock = btip->GetAncestor(btip->nHeight - 6);
    CBlock expectedTip = endorseAltBlockAndMine(endorsedBlock->GetBlockHash(), 10);

    BOOST_CHECK(expectedTip.GetHash() == ChainActive().Tip()->GetBlockHash());

    ReconsiderTestBlock(forkBlockNext);

    BOOST_CHECK(expectedTip.GetHash() == ChainActive().Tip()->GetBlockHash());
}

BOOST_FIXTURE_TEST_CASE(crossing_keystone_without_pop_1_test, E2eFixture)
{
    // Similar scenario like in crossing_keystone_with_pop_1_test case
    // The main difference that we do not endorse any block so the best chain is the highest chain

    // mine 20 blocks
    for (int i = 0; i < 20; ++i) {
        CreateAndProcessBlock({}, cbKey);
    }

    auto* atip = ChainActive().Tip();
    auto* forkBlockNext = atip->GetAncestor(atip->nHeight - 13);
    InvalidateTestBlock(forkBlockNext);

    for (int i = 0; i < 12; ++i) {
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

const static std::string encoded_blocks = "000000205ce92ce6d527f09870890b96d8f6a4e4351929305f4567db547710fa8398c455546657c4a6665464cd9dc3364ed6a9147a67ebba5de2489fc137ab47edc133df5b050000ffff7f20040000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03510101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca0000000000000000000000000000000000000000000000000000000000000000000000000000002018cfd6fb11e1782ef472e3a67028b74c99520a29535501e3338e974eecc31a7946db5dc6dd5e3c3d0cdb1281b646fac293accd4a4d5d053199ff58288a84d86f6b050000ffff7f20000000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03520101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020828ccc41309faef0a48d52f6e323fecbfacb214c66a66c2d799ea77a8473de7f3494b8bd52516b132968624e3fb395a39c40fecd001a9558554c20ef578e03da93050000ffff7f20000000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03530101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca0000000000000000000000000000000000000000000000000000000000000000000000000000002065e53dadf2e199ade07a6e366a9763e5ce3b7e82c6c5267531ee0c0821bdf74d42175087d0b62af238fdbccacba6aeea36d76cd0936f68e57da4dcac6d5e89dfce050000ffff7f20010000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03540101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca000000000000000000000000000000000000000000000000000000000000000000000000000000208c36edd05502326e2604fab1b91d5fcf10338eb228459d3301d9bdaa0c73122c52d00fe30be8f375a2defaac5108ce45db776e22b4a7760a3233e805034e7b8ad3050000ffff7f20010000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03550101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca000000000000000000000000000000000000000000000000000000000000000000000000000000209c521898afc42531f5dca7657b9c1f980677ff707eb6e4dcdd3d1aa04d6e7c4a2df6a9ef0d9c18df2de839f9305033b7a1c5c463fea5115f7662db19e00abfacf2050000ffff7f20000000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03560101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020effc63ab6f91550134d19fdd4cfac108ae3cf6db7cabfa40165c46b25e7ade6aa6ccf83693dd79396a98d7d21a3043ea052a584d88810a7ae5518143f3c26f8940060000ffff7f20010000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03570101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020d5d070f04cb1b28db5fda66fb2d7050e276ac8c69feb4f74b06ac4d6ea0f392c371113157e42b6705a1ea92a3f960245011e802b3eb9c8c796830482d8e300b647060000ffff7f20010000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03580101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca000000000000000000000000000000000000000000000000000000000000000000000000000000201d3ff562cfc4ddc0084305456961e134403305e8bc62386528982168d534410579d5fde13e77149cddcad49b0aaec72db7a40e9289294a66707bdc727f8650b191060000ffff7f20000000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03590101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca0000000000000000000000000000000000000000000000000000000000000000000000000000002099b3e27f705bed7d21d8d53fd695d862b5e21acbc950219da96efc8d4e678c0b98d38867fdaaf942402481be55088dbcb82af2fdcf4531c9375002c4883eb39ee8060000ffff7f20000000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff035a0101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca0000000000000000000000000000000000000000000000000000000000000000000000000000002099fc54f6ec0cd16ec9cb911f2e775645b1c6bf319ad80ffbd3e3f22886329a727e9908846de27a51e4c88e64692d19e900fa3e3083ad5ee466c74d48f5029e51fe060000ffff7f20000000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff035b0101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020144178d75ced81f267c4afaab36e9cf2c8d668b097b102457a2ba46630211c09d8b7da9a7018c5abff37c4f4d0ec2c5cefa59c04874bee2df81ecdaa3c7326ef2c070000ffff7f20010000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff035c0101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020aad4ea1056f917c51f410312dad90719f99f857408c191200f0f1fb7e9b91f4a66a64f0b24cd552acf80cde1a1315908dad5e0274ce1a2ef2065a5d67bdf47d745070000ffff7f20020000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff035d0101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020b916dc4d422bc8009f7f857302f5041c2cf9fc89bcba98c7c2bb0e279700311c0531908a0d2ebba5baf8aecb64e09869d21aa9446da830fd3634798636dc61698e070000ffff7f20010000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff035e0101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca0000000000000000000000000000000000000000000000000000000000000000000000000000002016c482e3db631ffe5ecb0abeb32f5db7349aa893b01223c5e89753aef335ed642fd89c94c4457f45bf882caa7f25587124833d3bf0e3ea34d749e6651808c812d5070000ffff7f20000000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff035f0101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020ea658212f77e33bb2cb3654312c71a3a40c6a0fa4518959effb928f7b6988e6792296e9ebf414667ac21665009f8fa6a24c0fb5c62614b5388cbfa9938cb431ef3070000ffff7f20010000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03600101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca0000000000000000000000000000000000000000000000000000000000000000000000000000002013c740ebda9d5852f5a303c735991b9eecf2bd8bb3fd7e950d89b4f1c897dc281ffa1dba4d79ea24ddf4e6136ceccdb945a0822acd35e5cca8219883e9003e0541080000ffff7f20020000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0401110101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020281360fb7f6c3256a7c0441209a0e15c2da87bb4e0d1328d9d7092e655669f47dd987a8ced74a0f45a69700d287bbcf7a3d373ed4640f09d10215ce7ae74a52e8b080000ffff7f20030000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0401120101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020f73d2924d09c86fb47bb0202e970296b12a1391f6a70ea1fc40c38e29ff53f0eacfe383361f8fae0887bf2667e541014c197788309e2c6901a97164b1f511f63ed080000ffff7f20010000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0401130101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca00000000000000000000000000000000000000000000000000000000000000000000000000000020ee48e6007b55f1be6fa5eb037e9c51478b6301366a4c36f439221488ae579371038b4ba234c3ded817a4f0074e7afd85c8a9aad0eb474ef514274c2fbd51fb56fa080000ffff7f20000000000102000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0401140101ffffffff03005ed0b200000000232103574b06f10a0523dd05b1273f9dc9b72203d6a1ac5d8f2ca74e1e3f212368f79eac0000000000000000266a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf90000000000000000256a233ae6ca000000000000000000000000000000000000000000000000000000000000000000000000";

BOOST_FIXTURE_TEST_CASE(fill_candidates_set, E2eFixture)
{
    BOOST_CHECK_EQUAL(ChainstateActive().setBlockIndexCandidates.size(), 1);

    std::vector<CBlock> blocks;
    std::vector<unsigned char> bytes = ParseHex(encoded_blocks);
    CDataStream read_stream(bytes, SER_NETWORK, PROTOCOL_VERSION);
    for (int i = 0; i < 20; ++i) {
        CBlock block;
        read_stream >> block;
        blocks.push_back(block);
        ProcessNewBlock(Params(), std::make_shared<CBlock>(block), true, nullptr);
    }

    BOOST_CHECK_EQUAL(ChainstateActive().setBlockIndexCandidates.size(), 2);
    InvalidateTestBlock(ChainActive()[1]);

    BOOST_CHECK_EQUAL(ChainActive().Tip()->nHeight, 20);
    BOOST_CHECK(ChainActive().Tip()->GetBlockHash() == blocks.rbegin()->GetHash());
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
