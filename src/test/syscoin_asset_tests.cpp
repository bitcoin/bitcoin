// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_syscoin_services.h>
#include <test/data/ethspv_valid.json.h>
#include <util/time.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <services/asset.h>
#include <base58.h>
#include <chainparams.h>
#include <boost/test/unit_test.hpp>
#include <iterator>
#include <core_io.h>
#include <key.h>
#include <math.h>
#include <key_io.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <services/rpc/assetrpc.h>
#include <services/assetconsensus.h>
#include <util/string.h>
using namespace std;
extern UniValue read_json(const std::string& jsondata);
BOOST_FIXTURE_TEST_SUITE(syscoin_asset_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE(generate_auxfees)
{
	tfm::format(std::cout,"Running generate_auxfees...\n");
    CAuxFeeDetails auxFeeDetails;
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(0, "0.01"));
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(10*COIN, "0.004"));
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(250*COIN, "0.002"));
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(2500*COIN, "0.0007"));
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(25000*COIN, "0.00007"));
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(250000*COIN, "0"));
 
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 5000000*COIN), 19456000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 25000000*COIN), 19456000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 10*COIN), 10000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 3000*COIN), 591000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 6*COIN), 6000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 250*COIN), 106000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 500*COIN), 156000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 1250*COIN), 306000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 2500*COIN), 556000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 7500*COIN), 906000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 20000*COIN), 1781000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 25000*COIN), 2131000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 60000*COIN), 2376000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 180000*COIN), 3216000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 700000*COIN), 6856000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 2500000*COIN), 19456000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 200.1005005*COIN), 86040200);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 2500001*COIN), 19456000000);
}
BOOST_AUTO_TEST_CASE(generate_auxfees1)
{
	tfm::format(std::cout,"Running generate_auxfees1...\n");
    CAuxFeeDetails auxFeeDetails;
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(0, "0.01"));
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(10*COIN, "0.004"));
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(250*COIN, "0.002"));
    auxFeeDetails.vecAuxFees.push_back(CAuxFee(2500*COIN, "0.0007"));
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 10*COIN), 10000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 3000*COIN), 591000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 2500*COIN), 556000000);
    BOOST_CHECK_EQUAL(getAuxFee(auxFeeDetails, 7500*COIN), 906000000);
}


BOOST_AUTO_TEST_SUITE_END ()
