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
	string newaddress = GetNewFundedAddress("node1");
    string newaddressfee = GetNewFundedAddress("node1");
    UniValue pubDataObj(UniValue::VOBJ);
    pubDataObj.pushKV("description", "AGX silver backed token, licensed and operated by Interfix corporation");
    UniValue auxfeesObj(UniValue::VOBJ);
    auxfeesObj.pushKV("address", newaddressfee);
    UniValue feestructArr(UniValue::VARR);
    UniValue boundsArr(UniValue::VARR);
    boundsArr.push_back("0");
    boundsArr.push_back("0.01");
    feestructArr.push_back(boundsArr);
    UniValue boundsArr1(UniValue::VARR);
    boundsArr1.push_back("10");
    boundsArr1.push_back("0.004");
    feestructArr.push_back(boundsArr1);
    UniValue boundsArr2(UniValue::VARR);
    boundsArr2.push_back("250");
    boundsArr2.push_back("0.002");
    feestructArr.push_back(boundsArr2);
    UniValue boundsArr3(UniValue::VARR);
    boundsArr3.push_back("2500");
    boundsArr3.push_back("0.0007");
    feestructArr.push_back(boundsArr3);
    UniValue boundsArr4(UniValue::VARR);
    boundsArr4.push_back("25000");
    boundsArr4.push_back("0.00007");
    feestructArr.push_back(boundsArr4);
    UniValue boundsArr5(UniValue::VARR);
    boundsArr5.push_back("2500000");
    boundsArr5.push_back("0");
    feestructArr.push_back(boundsArr5);
    auxfeesObj.pushKV("fs", feestructArr);
    pubDataObj.pushKV("af", auxfeesObj);
    CWitnessAddress address;
    const std::string& pubDataStr = pubDataObj.write();
 
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 5000000*COIN,8, address), 19456000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 25000000*COIN,8, address), 19456000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 10*COIN,8, address), 10000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 3000*COIN,8, address), 591000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 6*COIN,8, address), 6000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 250*COIN,8, address), 106000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 500*COIN,8, address), 156000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 1250*COIN,8, address), 306000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 2500*COIN,8, address), 556000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 7500*COIN,8, address), 906000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 20000*COIN,8, address), 1781000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 25000*COIN,8, address), 2131000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 60000*COIN,8, address), 2376000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 180000*COIN,8, address), 3216000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 700000*COIN,8, address), 6856000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 2500000*COIN,8, address), 19456000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 200.1005005*COIN,8, address), 86040200);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 2500001*COIN,8, address), 19456000000);
}
BOOST_AUTO_TEST_CASE(generate_auxfees1)
{
	tfm::format(std::cout,"Running generate_auxfees1...\n");
	string newaddress = GetNewFundedAddress("node1");
    string newaddressfee = GetNewFundedAddress("node1");
    UniValue pubDataObj(UniValue::VOBJ);
    pubDataObj.pushKV("description", "AGX silver backed token, licensed and operated by Interfix corporation");
    UniValue auxfeesObj(UniValue::VOBJ);
    auxfeesObj.pushKV("address", newaddressfee);
    UniValue feestructArr(UniValue::VARR);
    UniValue boundsArr(UniValue::VARR);
    boundsArr.push_back("0");
    boundsArr.push_back("0.01");
    feestructArr.push_back(boundsArr);
    UniValue boundsArr1(UniValue::VARR);
    boundsArr1.push_back("10");
    boundsArr1.push_back("0.004");
    feestructArr.push_back(boundsArr1);
    UniValue boundsArr2(UniValue::VARR);
    boundsArr2.push_back("250");
    boundsArr2.push_back("0.002");
    feestructArr.push_back(boundsArr2);
    UniValue boundsArr3(UniValue::VARR);
    boundsArr3.push_back("2500");
    boundsArr3.push_back("0.0007");
    feestructArr.push_back(boundsArr3);
    auxfeesObj.pushKV("fs", feestructArr);
    pubDataObj.pushKV("af", auxfeesObj);
    CWitnessAddress address;
    const std::string& pubDataStr = pubDataObj.write();
 
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 10*COIN,8, address), 10000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 3000*COIN,8, address), 591000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 2500*COIN,8, address), 556000000);
    BOOST_CHECK_EQUAL(getAuxFee(pubDataStr, 7500*COIN,8, address), 906000000);
}


BOOST_AUTO_TEST_SUITE_END ()
