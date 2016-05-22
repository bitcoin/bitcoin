// Copyright (c) 2013-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "hash.h"
#include "utilstrencodings.h"
#include "test/test_bitcoin.h"

#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_FIXTURE_TEST_SUITE(hash_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(murmurhash3)
{

#define T(expected, seed, data) BOOST_CHECK_EQUAL(MurmurHash3(seed, ParseHex(data)), expected)

    // Test MurmurHash3 with various inputs. Of course this is retested in the
    // bloom filter tests - they would fail if MurmurHash3() had any problems -
    // but is useful for those trying to implement Bitcoin libraries as a
    // source of test data for their MurmurHash3() primitive during
    // development.
    //
    // The magic number 0xFBA4C795 comes from CBloomFilter::Hash()

    T(0x00000000, 0x00000000, "");
    T(0x6a396f08, 0xFBA4C795, "");
    T(0x81f16f39, 0xffffffff, "");

    T(0x514e28b7, 0x00000000, "00");
    T(0xea3f0b17, 0xFBA4C795, "00");
    T(0xfd6cf10d, 0x00000000, "ff");

    T(0x16c6b7ab, 0x00000000, "0011");
    T(0x8eb51c3d, 0x00000000, "001122");
    T(0xb4471bf8, 0x00000000, "00112233");
    T(0xe2301fa8, 0x00000000, "0011223344");
    T(0xfc2e4a15, 0x00000000, "001122334455");
    T(0xb074502c, 0x00000000, "00112233445566");
    T(0x8034d2a0, 0x00000000, "0011223344556677");
    T(0xb4698def, 0x00000000, "001122334455667788");

#undef T
}

BOOST_AUTO_TEST_CASE(siphash)
{
    CSipHasher hasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x726fdb47dd0e0e31ull);
    static const unsigned char t0[1] = {0};
    hasher.Write(t0, 1);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x74f839c593dc67fdull);
    static const unsigned char t1[7] = {1,2,3,4,5,6,7};
    hasher.Write(t1, 7);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x93f5f5799a932462ull);
    hasher.Write(0x0F0E0D0C0B0A0908ULL);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x3f2acc7f57c29bdbull);
    static const unsigned char t2[2] = {16,17};
    hasher.Write(t2, 2);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x4bc1b3f0968dd39cull);
    static const unsigned char t3[9] = {18,19,20,21,22,23,24,25,26};
    hasher.Write(t3, 9);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x2f2e6163076bcfadull);
    static const unsigned char t4[5] = {27,28,29,30,31};
    hasher.Write(t4, 5);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x7127512f72f27cceull);
    hasher.Write(0x2726252423222120ULL);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0x0e3ea96b5304a7d0ull);
    hasher.Write(0x2F2E2D2C2B2A2928ULL);
    BOOST_CHECK_EQUAL(hasher.Finalize(),  0xe612a3cb9ecba951ull);

    BOOST_CHECK_EQUAL(SipHashUint256(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL, uint256S("1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a09080706050403020100")), 0x7127512f72f27cceull);
}

BOOST_AUTO_TEST_SUITE_END()
