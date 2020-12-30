// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <chain.h>
#include <validation.h>
#include <vbk/test/util/e2e_fixture.hpp>
#include <vbk/util.hpp>
#include <veriblock/alt-util.hpp>
#include <veriblock/mock_miner.hpp>

using altintegration::BtcBlock;
using altintegration::MockMiner;
using altintegration::PublicationData;
using altintegration::VbkBlock;
using altintegration::VTB;

BOOST_AUTO_TEST_SUITE(e2e_poptx_tests)

BOOST_FIXTURE_TEST_CASE(ValidBlockIsAccepted, E2eFixture)
{
    // altintegration and popminer configured to use BTC/VBK/ALT regtest.
    auto tip = ChainActive().Tip();
    BOOST_CHECK(tip != nullptr);

    // endorse tip
    CBlock block = endorseAltBlockAndMine(tip->GetBlockHash(), 10);
    BOOST_CHECK(block.popData.vtbs.size() == 10);
    BOOST_CHECK(block.popData.atvs.size() != 0);
    {
        BOOST_REQUIRE(ChainActive().Tip()->GetBlockHash() == block.GetHash());
        auto btc = VeriBlock::getLastKnownBTCBlocks(1)[0];
        BOOST_REQUIRE(btc == popminer.btc().getBestChain().tip()->getHash());
        auto vbk = VeriBlock::getLastKnownVBKBlocks(1)[0];
        BOOST_REQUIRE(vbk == popminer.vbk().getBestChain().tip()->getHash());
    }

    // endorse another tip
    block = endorseAltBlockAndMine(tip->GetBlockHash(), 1);
    BOOST_CHECK(block.popData.atvs.size() != 0);
    auto lastHash = ChainActive().Tip()->GetBlockHash();
    {
        BOOST_REQUIRE(lastHash == block.GetHash());
        auto btc = VeriBlock::getLastKnownBTCBlocks(1)[0];
        BOOST_REQUIRE(btc == popminer.btc().getBestChain().tip()->getHash());
        auto vbk = VeriBlock::getLastKnownVBKBlocks(1)[0];
        BOOST_REQUIRE(vbk == popminer.vbk().getBestChain().tip()->getHash());
    }
}

BOOST_AUTO_TEST_SUITE_END()
