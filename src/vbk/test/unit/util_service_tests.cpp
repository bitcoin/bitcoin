// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <gmock/gmock.h>

#include <consensus/validation.h>
#include <script/interpreter.h>
#include <string>
#include <test/util/setup_common.h>
#include <validation.h>
#include <vbk/entity/context_info_container.hpp>
#include <vbk/merkle.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/util.hpp>
#include <bootstraps.h>

using ::testing::Return;

BOOST_AUTO_TEST_SUITE(util_service_tests)

BOOST_AUTO_TEST_CASE(is_keystone)
{
    SelectParams("regtest");
    selectPopConfig("regtest", "regtest", true);

    CBlockIndex index;
    index.nHeight = 100; // multiple of 5
    BOOST_CHECK(VeriBlock::isKeystone(index));
    index.nHeight = 99; // not multiple of 5
    BOOST_CHECK(!VeriBlock::isKeystone(index));
}

BOOST_AUTO_TEST_CASE(get_previous_keystone)
{
    SelectParams("regtest");
    selectPopConfig("regtest", "regtest", true);

    std::vector<CBlockIndex> blocks;
    blocks.resize(10);
    blocks[0].pprev = nullptr;
    blocks[0].nHeight = 0;
    for (size_t i = 1; i < blocks.size(); i++) {
        blocks[i].pprev = &blocks[i - 1];
        blocks[i].nHeight = i;
    }

    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[9]) == &blocks[5]);
    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[8]) == &blocks[5]);
    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[7]) == &blocks[5]);
    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[6]) == &blocks[5]);
    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[5]) == &blocks[0]);
    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[4]) == &blocks[0]);
    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[3]) == &blocks[0]);
    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[2]) == &blocks[0]);
    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[1]) == &blocks[0]);
    BOOST_CHECK(VeriBlock::getPreviousKeystone(blocks[0]) == nullptr);
}

BOOST_AUTO_TEST_CASE(make_context_info)
{
    TestChain100Setup blockchain;

    CScript scriptPubKey = CScript() << ToByteVector(blockchain.coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = blockchain.CreateAndProcessBlock({}, scriptPubKey);

    LOCK(cs_main);

    CBlockIndex* index = LookupBlockIndex(block.GetHash());
    BOOST_REQUIRE(index != nullptr);

    uint256 txRoot{};
    auto keystones = VeriBlock::getKeystoneHashesForTheNextBlock(index->pprev);
    auto container = VeriBlock::ContextInfoContainer(index->nHeight, keystones, txRoot);

    // TestChain100Setup has blockchain with 100 blocks, new block is 101
    BOOST_CHECK(container.height == 101);
    BOOST_CHECK(container.keystones == keystones);
    BOOST_CHECK(container.getAuthenticated().size() == container.getUnauthenticated().size() + 32);
    BOOST_CHECK(container.getUnauthenticated().size() == 4 + 2 * 32);
}

BOOST_AUTO_TEST_SUITE_END()
