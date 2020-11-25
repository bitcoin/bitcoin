// Copyright (c) 2014-2019 The Bitcoin Core developers
// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <net.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_tests, TestingSetup)

class CMainWithHalvingsParams : public CMainParams
{
public:
    CMainWithHalvingsParams(int nSubsidyHalvingInterval)
    {
        consensus.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
    }
};

static void TestBlockSubsidyHalvings(const CChainParams& params)
{
    int maxHalvings = 64;
    CAmount nInitialSubsidy = VeriBlock::getCoinbaseSubsidy(50 * COIN, 0, params);

    CAmount nPreviousSubsidy = nInitialSubsidy * 2; // for height == 0
    BOOST_CHECK_EQUAL(nPreviousSubsidy, nInitialSubsidy * 2);
    for (int nHalvings = 0; nHalvings < maxHalvings; nHalvings++) {
        int nHeight = nHalvings * params.GetConsensus().nSubsidyHalvingInterval;
        CAmount nSubsidy = GetBlockSubsidy(nHeight, params);
        BOOST_CHECK(nSubsidy <= nInitialSubsidy);
        BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
        nPreviousSubsidy = nSubsidy;
    }
    BOOST_CHECK_EQUAL(GetBlockSubsidy(maxHalvings * params.GetConsensus().nSubsidyHalvingInterval, params), 0);
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
{
    auto chainParams = CMainWithHalvingsParams(nSubsidyHalvingInterval);
    TestBlockSubsidyHalvings(chainParams);
}

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    TestBlockSubsidyHalvings(*chainParams); // As in main
    TestBlockSubsidyHalvings(150); // As in regtest
    TestBlockSubsidyHalvings(1000); // Just another interval
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::REGTEST);
    CAmount nSum = 0;
    // skip first 1000 blocks to make sure POP security is ON
    for (int nHeight = 1000; nHeight < 14000000; nHeight += 1000) {
        CAmount nSubsidy = GetBlockSubsidy(nHeight, *chainParams);
        BOOST_CHECK(nSubsidy <= 50 * COIN);
        nSum += nSubsidy * 1000;
        BOOST_CHECK(MoneyRange(nSum));
    }
    // with 50 vBTC payout:
//    BOOST_CHECK_EQUAL(nSum, CAmount{2099999997690000});
    // with 50*60% vBTC payout and RegTest
    BOOST_CHECK_EQUAL(nSum, CAmount{47244115000});
}

static bool ReturnFalse() { return false; }
static bool ReturnTrue() { return true; }

BOOST_AUTO_TEST_CASE(test_combiner_all)
{
    boost::signals2::signal<bool (), CombinerAll> Test;
    BOOST_CHECK(Test());
    Test.connect(&ReturnFalse);
    BOOST_CHECK(!Test());
    Test.connect(&ReturnTrue);
    BOOST_CHECK(!Test());
    Test.disconnect(&ReturnFalse);
    BOOST_CHECK(Test());
    Test.disconnect(&ReturnTrue);
    BOOST_CHECK(Test());
}
BOOST_AUTO_TEST_SUITE_END()
