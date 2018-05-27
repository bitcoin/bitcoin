// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "random.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(random_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(osrandom_tests)
{
    BOOST_CHECK(Random_SanityCheck());
}

BOOST_AUTO_TEST_CASE(fastrandom_tests)
{
    // Check that deterministic FastRandomContexts are deterministic
    FastRandomContext ctx1(true);
    FastRandomContext ctx2(true);

    BOOST_CHECK_EQUAL(ctx1.rand32(), ctx2.rand32());
    BOOST_CHECK_EQUAL(ctx1.rand32(), ctx2.rand32());
    BOOST_CHECK_EQUAL(ctx1.rand64(), ctx2.rand64());
    BOOST_CHECK_EQUAL(ctx1.randbits(3), ctx2.randbits(3));
    BOOST_CHECK(ctx1.randbytes(17) == ctx2.randbytes(17));
    BOOST_CHECK(ctx1.rand256() == ctx2.rand256());
    BOOST_CHECK_EQUAL(ctx1.randbits(7), ctx2.randbits(7));
    BOOST_CHECK(ctx1.randbytes(128) == ctx2.randbytes(128));
    BOOST_CHECK_EQUAL(ctx1.rand32(), ctx2.rand32());
    BOOST_CHECK_EQUAL(ctx1.randbits(3), ctx2.randbits(3));
    BOOST_CHECK(ctx1.rand256() == ctx2.rand256());
    BOOST_CHECK(ctx1.randbytes(50) == ctx2.randbytes(50));

    // Check that a nondeterministic ones are not
    FastRandomContext ctx3;
    FastRandomContext ctx4;
    BOOST_CHECK(ctx3.rand64() != ctx4.rand64()); // extremely unlikely to be equal
    BOOST_CHECK(ctx3.rand256() != ctx4.rand256());
    BOOST_CHECK(ctx3.randbytes(7) != ctx4.randbytes(7));
}

BOOST_AUTO_TEST_CASE(fastrandom_randbits)
{
    FastRandomContext ctx1;
    FastRandomContext ctx2;
    for (int bits = 0; bits < 63; ++bits) {
        for (int j = 0; j < 1000; ++j) {
            uint64_t rangebits = ctx1.randbits(bits);
            BOOST_CHECK_EQUAL(rangebits >> bits, 0);
            uint64_t range = ((uint64_t)1) << bits | rangebits;
            uint64_t rand = ctx2.randrange(range);
            BOOST_CHECK(rand < range);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
