// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

#include <random.cpp>

using namespace std;

BOOST_FIXTURE_TEST_SUITE(hwrand_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(rdx86seedhwslow_test)
{
    const size_t DIGEST_LEN = 512/8; // 512 bits = 64 bytes

    unsigned char emptyoutput[DIGEST_LEN];
    memset(emptyoutput, 0, DIGEST_LEN);
    CSHA512 emptyhasher;
    emptyhasher.Finalize(emptyoutput);

    for (int i = 0; i < 5; i++)
    {
        CSHA512 hasher;
        unsigned char output[DIGEST_LEN];
        memset(output, 0, DIGEST_LEN);

        SeedHardwareSlow(hasher);
        hasher.Finalize(output);

        #if defined(__x86_64__) || defined(__amd64__) || defined(__i386__) || (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64)))
            bool isNotEqual = (memcmp(output, emptyoutput, DIGEST_LEN) != 0);
            if (g_rdseed_supported)
            {
                BOOST_CHECK(isNotEqual);
            }
            else
            {
                BOOST_CHECK(!isNotEqual);
            }
        #else
            // RDRand not supported on this platform so the hasher should have no data written to it
            bool isEqual = (memcmp(output, emptyoutput, DIGEST_LEN) == 0);
            BOOST_CHECK(isEqual);
        #endif
    }
}

BOOST_AUTO_TEST_CASE(rdx86seedhwfast_test)
{
    const size_t DIGEST_LEN = 512/8; // 512 bits = 64 bytes

    unsigned char emptyoutput[DIGEST_LEN];
    memset(emptyoutput, 0, DIGEST_LEN);
    CSHA512 emptyhasher;
    emptyhasher.Finalize(emptyoutput);

    for (int i = 0; i < 5; i++)
    {
        CSHA512 hasher;
        unsigned char output[DIGEST_LEN];
        memset(output, 0, DIGEST_LEN);

        SeedHardwareFast(hasher);
        hasher.Finalize(output);

        #if defined(__x86_64__) || defined(__amd64__) || defined(__i386__) || (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64)))
            bool isNotEqual = (memcmp(output, emptyoutput, DIGEST_LEN) != 0);
            if (g_rdrand_supported)
            {
                BOOST_CHECK(isNotEqual);
            }
            else
            {
                BOOST_CHECK(!isNotEqual);
            }
        #else
            // RDRand not supported on this platform so the hasher should have no data written to it
            bool isEqual = (memcmp(output, emptyoutput, DIGEST_LEN) == 0);
            BOOST_CHECK(isEqual);
        #endif
    }
}

BOOST_AUTO_TEST_SUITE_END()
