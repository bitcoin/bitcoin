// Copyright (c) 2014-2015 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// Copyright (c) 2014-2017 The Syscoin Core developers
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "validation.h"
#include "net.h"

#include "test/test_syscoin.h"

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

static void TestBlockSubsidyHalvings(const Consensus::Params& consensusParams)
{
    // tested in syscoin_tests.cpp
    //int maxHalvings = 64;
    //CAmount nInitialSubsidy = 50 * COIN;

    //CAmount nPreviousSubsidy = nInitialSubsidy * 2; // for height == 0
    //BOOST_CHECK_EQUAL(nPreviousSubsidy, nInitialSubsidy * 2);
    //for (int nHalvings = 0; nHalvings < maxHalvings; nHalvings++) {
    //    int nHeight = nHalvings * consensusParams.nSubsidyHalvingInterval;
    //    CAmount nSubsidy = GetBlockSubsidy(0, nHeight, consensusParams);
    //    BOOST_CHECK(nSubsidy <= nInitialSubsidy);
    //    BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
    //    nPreviousSubsidy = nSubsidy;
    //}
    //BOOST_CHECK_EQUAL(GetBlockSubsidy(0, maxHalvings * consensusParams.nSubsidyHalvingInterval, consensusParams), 0);
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
{
    // tested in syscoin_tests.cpp
    //Consensus::Params consensusParams;
    //consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
    //TestBlockSubsidyHalvings(consensusParams);
}

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    // tested in syscoin_tests.cpp
    //TestBlockSubsidyHalvings(Params(CBaseChainParams::MAIN).GetConsensus()); // As in main
    //TestBlockSubsidyHalvings(150); // As in regtest
    //TestBlockSubsidyHalvings(1000); // Just another interval
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const Consensus::Params& consensusParams = Params(CBaseChainParams::MAIN).GetConsensus();
    CAmount nSum = 528465209*COIN;
    for (int nHeight = 428770; nHeight < 30000000; nHeight++) {
        CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
        BOOST_CHECK(nSubsidy <= 38.5 * COIN);
        nSum += nSubsidy;
        BOOST_CHECK(MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, 88555117428588000ULL);
}

bool ReturnFalse() { return false; }
bool ReturnTrue() { return true; }

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
