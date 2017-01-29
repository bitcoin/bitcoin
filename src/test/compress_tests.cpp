// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "compressor.h"
#include "util.h"
#include "test/test_bitcoin.h"

#include <stdint.h>

#include <boost/test/unit_test.hpp>

// amounts 0.00000001 .. 0.00100000
#define NUM_MULTIPLES_UNIT 100000

// amounts 0.01 .. 100.00
#define NUM_MULTIPLES_CENT 10000

// amounts 1 .. 10000
#define NUM_MULTIPLES_1BTC 10000

// amounts 50 .. 21000000
#define NUM_MULTIPLES_50BTC 1680000

BOOST_FIXTURE_TEST_SUITE(compress_tests, BasicTestingSetup)

bool static TestEncode(uint64_t in) {
    return in == CTxOutCompressor::DecompressAmount(CTxOutCompressor::CompressAmount(in));
}

bool static TestDecode(uint64_t in) {
    return in == CTxOutCompressor::CompressAmount(CTxOutCompressor::DecompressAmount(in));
}

bool static TestPair(uint64_t dec, uint64_t enc) {
    return CTxOutCompressor::CompressAmount(dec) == enc &&
           CTxOutCompressor::DecompressAmount(enc) == dec;
}

BOOST_AUTO_TEST_CASE(compress_amounts)
{
    BOOST_CHECK(TestPair(            0,       0x0));
    BOOST_CHECK(TestPair(            1,       0x1));
    BOOST_CHECK(TestPair(         CENT,       0x7));
    BOOST_CHECK(TestPair(         COIN,       0x9));
    BOOST_CHECK(TestPair(      50*COIN,      0x32));
    BOOST_CHECK(TestPair(84000000*COIN, 0x501BD00));

    for (uint64_t i = 1; i <= NUM_MULTIPLES_UNIT; i++)
        BOOST_CHECK(TestEncode(i));

    for (uint64_t i = 1; i <= NUM_MULTIPLES_CENT; i++)
        BOOST_CHECK(TestEncode(i * CENT));

    for (uint64_t i = 1; i <= NUM_MULTIPLES_1BTC; i++)
        BOOST_CHECK(TestEncode(i * COIN));

    for (uint64_t i = 1; i <= NUM_MULTIPLES_50BTC; i++)
        BOOST_CHECK(TestEncode(i * 50 * COIN));

    for (uint64_t i = 0; i < 100000; i++)
        BOOST_CHECK(TestDecode(i));
}

BOOST_AUTO_TEST_SUITE_END()
