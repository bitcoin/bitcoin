// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(subsidy_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

    uint32_t nPrevBits;
    int32_t nPrevHeight;
    CAmount nSubsidy;

    // details for block 4249 (subsidy returned will be for block 4250)
    nPrevBits = 0x1c4a47c4;
    nPrevHeight = 4249;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 50000000000ULL);

    // details for block 4249 (subsidy returned will be for block 4250)
    // v20 should make difference for blocks with low diff, regardless of their height
    nPrevBits = 0x1c4a47c4;
    nPrevHeight = 4249;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ true);
    BOOST_CHECK_EQUAL(nSubsidy, 500000000ULL);

    // details for block 4501 (subsidy returned will be for block 4502)
    nPrevBits = 0x1c4a47c4;
    nPrevHeight = 4501;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 5600000000ULL);

    // details for block 5464 (subsidy returned will be for block 5465)
    nPrevBits = 0x1c29ec00;
    nPrevHeight = 5464;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 2100000000ULL);

    // details for block 5465 (subsidy returned will be for block 5466)
    nPrevBits = 0x1c29ec00;
    nPrevHeight = 5465;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 12200000000ULL);

    // details for block 17588 (subsidy returned will be for block 17589)
    nPrevBits = 0x1c08ba34;
    nPrevHeight = 17588;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 6100000000ULL);

    // details for block 99999 (subsidy returned will be for block 100000)
    nPrevBits = 0x1b10cf42;
    nPrevHeight = 99999;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 500000000ULL);

    // details for block 210239 (subsidy returned will be for block 210240)
    nPrevBits = 0x1b11548e;
    nPrevHeight = 210239;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 500000000ULL);

    // 1st subsidy reduction happens here

    // details for block 210240 (subsidy returned will be for block 210241)
    nPrevBits = 0x1b10d50b;
    nPrevHeight = 210240;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 464285715ULL);

    // details for block 210240 (subsidy returned will be for block 210241)
    // v20 makes no difference for blocks with high enough diff while budgets aren't active yet
    nPrevBits = 0x1b10d50b;
    nPrevHeight = 210240;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ true);
    BOOST_CHECK_EQUAL(nSubsidy, 464285715ULL);

    // details for block 420480 (subsidy returned will be for block 210241)
    nPrevBits = 0x1b10d50b;
    nPrevHeight = 420480;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ false);
    BOOST_CHECK_EQUAL(nSubsidy, 388010205ULL); // 431122450 * 0.9

    // details for block 420480 (subsidy returned will be for block 210241)
    // budgets are active, reallocation matters now
    nPrevBits = 0x1b10d50b;
    nPrevHeight = 420480;
    nSubsidy = GetBlockSubsidyInner(nPrevBits, nPrevHeight, chainParams->GetConsensus(), /*fV20Active=*/ true);
    BOOST_CHECK_EQUAL(nSubsidy, 344897960ULL); // 431122450 * 0.8
}

BOOST_AUTO_TEST_SUITE_END()
