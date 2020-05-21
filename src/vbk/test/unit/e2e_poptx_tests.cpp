// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <bootstraps.h>
#include <chain.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <vbk/test/util/e2e_fixture.hpp>
#include <vbk/util.hpp>
#include <veriblock/alt-util.hpp>
#include <veriblock/mock_miner.hpp>

using altintegration::AltPayloads;
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
    BOOST_REQUIRE(block.vtx.size() == 2);

    {
        BOOST_REQUIRE(ChainActive().Tip()->GetBlockHash() == block.GetHash());
        auto btc = pop->getLastKnownBTCBlocks(1)[0];
        BOOST_REQUIRE(btc == popminer.btc().getBestChain().tip()->getHash());
        auto vbk = pop->getLastKnownVBKBlocks(1)[0];
        BOOST_REQUIRE(vbk == popminer.vbk().getBestChain().tip()->getHash());
    }

    // endorse another tip
    block = endorseAltBlockAndMine(tip->GetBlockHash(), 1);
    auto lastHash = ChainActive().Tip()->GetBlockHash();
    {
        BOOST_REQUIRE(lastHash == block.GetHash());
        auto btc = pop->getLastKnownBTCBlocks(1)[0];
        BOOST_REQUIRE(btc == popminer.btc().getBestChain().tip()->getHash());
        auto vbk = pop->getLastKnownVBKBlocks(1)[0];
        BOOST_REQUIRE(vbk == popminer.vbk().getBestChain().tip()->getHash());
    }

    // create block that is not on main chain
    auto fork1tip = CreateAndProcessBlock({}, ChainActive().Tip()->pprev->pprev->GetBlockHash(), cbKey);

    // endorse block that is not on main chain
    block = endorseAltBlockAndMine(fork1tip.GetHash(), 1);
    BOOST_CHECK(ChainActive().Tip()->GetBlockHash() == lastHash);
}

BOOST_AUTO_TEST_SUITE_END()
