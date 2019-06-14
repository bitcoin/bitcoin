#include "omnicore/parsing.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(omnicore_obfuscation_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(prepare_obfuscated_hashes)
{
    std::string strSeed("1CdighsfdfRcj4ytQSskZgQXbUEamuMUNF");
    std::string vstrObfuscatedHashes[1+MAX_SHA256_OBFUSCATION_TIMES];
    PrepareObfuscatedHashes(strSeed, 5, vstrObfuscatedHashes);

    BOOST_CHECK_EQUAL(vstrObfuscatedHashes[1],
            "1D9A3DE5C2E22BF89A1E41E6FEDAB54582F8A0C3AE14394A59366293DD130C59");
    BOOST_CHECK_EQUAL(vstrObfuscatedHashes[2],
            "0800ED44F1300FB3A5980ECFA8924FEDB2D5FDBEF8B21BBA6526B4FD5F9C167C");
    BOOST_CHECK_EQUAL(vstrObfuscatedHashes[3],
            "7110A59D22D5AF6A34B7A196DAE7CCC0F27354B34E257832B9955611A9D79B06");
    BOOST_CHECK_EQUAL(vstrObfuscatedHashes[4],
            "AA3F890D32864BEA31EE9BD57D2247D8F8CE07B5ABAED9372F0B8999D28DB963");
}


BOOST_AUTO_TEST_SUITE_END()
