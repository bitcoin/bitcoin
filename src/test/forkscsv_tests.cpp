// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <limits>
#include <string>

#include "chainparams.h"
#include "forks_csv.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

namespace utf = boost::unit_test;


BOOST_FIXTURE_TEST_SUITE(forkscsv_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(forkscsv_read_test)
{
    CChainParams& params = ModifiableParams();

    // read of emptiness should be ok for now.
    // In a better implementation, overall file validation should warn the user
    // if no data lines were found in the file.
    std::istringstream is_0("");
    BOOST_CHECK(ReadForksCsv("main", is_0, params.GetModifiableConsensus()));

    std::istringstream is_1(
            "# deployment info for network 'main':\n"
            "main,0,csv,1462060800,1493596800,2016,1916,0,0,true\n"
            "main,28,testdummy,1199145601,1230767999,2016,1916,0,0,false\n");
    BOOST_CHECK(ReadForksCsv("main", is_1, params.GetModifiableConsensus()));

    std::istringstream is_2(
            "# deployment info for network 'main':\n"
            "main,0,csv,1462060800,1493596800,2016,1916,0,0,true\n"
            "main,1,segwit,1479168000,1510704000,2016,1916,0,0,true\n"
            "main,28,testdummy,1199145601,1230767999,2016,1916,0,0,false\n");
    BOOST_CHECK(ReadForksCsv("main", is_2, params.GetModifiableConsensus()));
}

/**
 * Test CSV dump of built-in defaults.
 * NOTE: the dump of 'main' data in this test depends on forkscsv_read_test
 * as long as the 'segwit' line is not built-in.
 * That is why an explicit dependency has been declared here.
 */
BOOST_AUTO_TEST_CASE(forkscsv_dumpforks_test, * utf::depends_on("forkscsv_tests/forkscsv_read_test"))
{
    BOOST_CHECK(NetworkDeploymentInfoCSV(CBaseChainParams::MAIN) == std::string(
            "# deployment info for network 'main':\n"
            "main,0,csv,1462060800,1493596800,2016,1916,0,0,true\n"
            "main,1,segwit,1479168000,1510704000,2016,1916,0,0,true\n"
            "main,28,testdummy,1199145601,1230767999,2016,1916,0,0,false\n"));

    BOOST_CHECK(NetworkDeploymentInfoCSV(CBaseChainParams::TESTNET) == std::string(
            "# deployment info for network 'test':\n"
            "test,0,csv,1456790400,1493596800,2016,1512,0,0,true\n"
            "test,1,segwit,1462060800,1493596800,2016,1512,0,0,true\n"
            "test,28,testdummy,1199145601,1230767999,2016,1512,0,0,false\n"));

    BOOST_CHECK(NetworkDeploymentInfoCSV(CBaseChainParams::REGTEST) == std::string(
            "# deployment info for network 'regtest':\n"
            "regtest,0,csv,0,999999999999,144,108,0,0,true\n"
            "regtest,1,segwit,0,999999999999,144,108,0,0,true\n"
            "regtest,28,testdummy,0,999999999999,144,108,0,0,false\n"));

    BOOST_CHECK_THROW( NetworkDeploymentInfoCSV("_foo_"), std::runtime_error );
}

/**
 * Test validation of CSV input fields
 */
BOOST_AUTO_TEST_CASE(forkscsv_validation_test)
{
    // bit number
    for (int num = 0; num < VERSIONBITS_NUM_BITS; num++) {
        BOOST_CHECK(ValidateBit(num));
    }
    BOOST_CHECK(!ValidateBit(VERSIONBITS_NUM_BITS + 1));
    BOOST_CHECK(!ValidateBit(-1));

    // network name
    BOOST_CHECK(ValidateNetwork(CBaseChainParams::MAIN));
    BOOST_CHECK(ValidateNetwork(CBaseChainParams::TESTNET));
    BOOST_CHECK(ValidateNetwork(CBaseChainParams::REGTEST));
    BOOST_CHECK(!ValidateNetwork("nonexistent_net"));
    BOOST_CHECK(!ValidateNetwork(""));

    // gbt_force
    BOOST_CHECK(ValidateGBTForce("true"));
    BOOST_CHECK(ValidateGBTForce("false"));
    BOOST_CHECK(ValidateGBTForce("True"));
    BOOST_CHECK(ValidateGBTForce("False"));
    BOOST_CHECK(ValidateGBTForce("TRUE"));
    BOOST_CHECK(ValidateGBTForce("FALSE"));
    BOOST_CHECK(!ValidateGBTForce("oui"));
    BOOST_CHECK(!ValidateGBTForce("non"));
    BOOST_CHECK(!ValidateGBTForce(""));

    // fork name
    BOOST_CHECK(ValidateForkName("a_fork"));
    BOOST_CHECK(ValidateForkName("a fork"));
    BOOST_CHECK(ValidateForkName("segwit"));
    BOOST_CHECK(!ValidateForkName(""));

    // window size
    BOOST_CHECK(!ValidateWindowSize(0));
    BOOST_CHECK(!ValidateWindowSize(1));
    BOOST_CHECK(!ValidateWindowSize(-1));
    BOOST_CHECK(ValidateWindowSize(2));
    BOOST_CHECK(ValidateWindowSize(3));
    BOOST_CHECK(ValidateWindowSize(100));
    BOOST_CHECK(ValidateWindowSize(10000));
    BOOST_CHECK(ValidateWindowSize(std::numeric_limits<int>::max()));
#pragma GCC diagnostic ignored "-Woverflow"
    BOOST_CHECK(!ValidateWindowSize(1 + std::numeric_limits<int>::max()));
#pragma GCC diagnostic pop

    // threshold size (2nd param is window)
    BOOST_CHECK(!ValidateThreshold(1,1));   // 1 is not valid window size
    BOOST_CHECK(!ValidateThreshold(0,1));   // 1 is not valid window size, 0 is not valid threshold
    BOOST_CHECK(!ValidateThreshold(0,2));   // 0 is not valid threshold
    BOOST_CHECK(ValidateThreshold(1,2));
    BOOST_CHECK(!ValidateThreshold(-1,2));
    BOOST_CHECK(!ValidateThreshold(2,1));
    BOOST_CHECK(ValidateThreshold(2,2));
    BOOST_CHECK(ValidateThreshold(1,100));
    BOOST_CHECK(ValidateThreshold(50,100));
    BOOST_CHECK(ValidateThreshold(99,100));
    BOOST_CHECK(ValidateThreshold(100,100));
    BOOST_CHECK(!ValidateThreshold(101,100));
    BOOST_CHECK(ValidateThreshold(1916,2016));
    BOOST_CHECK(ValidateThreshold(2016,2016));
    BOOST_CHECK(ValidateThreshold(2016,2016));
    BOOST_CHECK(!ValidateThreshold(2017,2016));
    BOOST_CHECK(ValidateThreshold(1, std::numeric_limits<int>::max()));

    // starttime / timeout
    BOOST_CHECK(!ValidateTimes(0, 0));     // starttime must be strictly less than timeout
    BOOST_CHECK(ValidateTimes(0, 1));      // starttime must be strictly less than timeout
    BOOST_CHECK(ValidateTimes(100, 1001)); // starttime must be strictly less than timeout
    BOOST_CHECK(ValidateTimes(0, 1001));   // starttime must be strictly less than timeout
    BOOST_CHECK(ValidateTimes(0, std::numeric_limits<int64_t>::max()));
    BOOST_CHECK(ValidateTimes(std::numeric_limits<int64_t>::max() - 1,
                              std::numeric_limits<int64_t>::max()));

    // minlockedblocks
    BOOST_CHECK(ValidateMinLockedBlocks(0));   // zero is ok
    BOOST_CHECK(ValidateMinLockedBlocks(1));
    BOOST_CHECK(ValidateMinLockedBlocks(100));
    BOOST_CHECK(!ValidateMinLockedBlocks(-1));
    BOOST_CHECK(ValidateMinLockedBlocks(std::numeric_limits<int>::max()));
#pragma GCC diagnostic ignored "-Woverflow"
    BOOST_CHECK(!ValidateMinLockedBlocks(1 + std::numeric_limits<int>::max()));
#pragma GCC diagnostic pop

    // minlockedtime
    BOOST_CHECK(ValidateMinLockedTime(0));   // zero is ok
    BOOST_CHECK(ValidateMinLockedTime(1));
    BOOST_CHECK(ValidateMinLockedTime(100));
    BOOST_CHECK(!ValidateMinLockedTime(-1));
    BOOST_CHECK(ValidateMinLockedTime(std::numeric_limits<int64_t>::max()));
#pragma GCC diagnostic ignored "-Woverflow"
    BOOST_CHECK(!ValidateMinLockedTime(1 + std::numeric_limits<int64_t>::max()));
#pragma GCC diagnostic pop
}

BOOST_AUTO_TEST_SUITE_END()
