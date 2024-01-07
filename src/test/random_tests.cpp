// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>

#include <test/util/setup_common.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>

BOOST_FIXTURE_TEST_SUITE(random_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(osrandom_tests)
{
    BOOST_CHECK(Random_SanityCheck());
}

BOOST_AUTO_TEST_CASE(fastrandom_tests)
{
    // Check that deterministic FastRandomContexts are deterministic
    g_mock_deterministic_tests = true;
    FastRandomContext ctx1(true);
    FastRandomContext ctx2(true);

    for (int i = 10; i > 0; --i) {
        BOOST_CHECK_EQUAL(GetRand(std::numeric_limits<uint64_t>::max()), uint64_t{10393729187455219830U});
        BOOST_CHECK_EQUAL(GetRandInt(std::numeric_limits<int>::max()), int{769702006});
        BOOST_CHECK_EQUAL(GetRandMicros(std::chrono::hours{1}).count(), 2917185654);
        BOOST_CHECK_EQUAL(GetRandMillis(std::chrono::hours{1}).count(), 2144374);
    }
    {
        constexpr SteadySeconds time_point{1s};
        FastRandomContext ctx{true};
        BOOST_CHECK_EQUAL(7, ctx.rand_uniform_delay(time_point, 9s).time_since_epoch().count());
        BOOST_CHECK_EQUAL(-6, ctx.rand_uniform_delay(time_point, -9s).time_since_epoch().count());
        BOOST_CHECK_EQUAL(1, ctx.rand_uniform_delay(time_point, 0s).time_since_epoch().count());
        BOOST_CHECK_EQUAL(1467825113502396065, ctx.rand_uniform_delay(time_point, 9223372036854775807s).time_since_epoch().count());
        BOOST_CHECK_EQUAL(-970181367944767837, ctx.rand_uniform_delay(time_point, -9223372036854775807s).time_since_epoch().count());
        BOOST_CHECK_EQUAL(24761, ctx.rand_uniform_delay(time_point, 9h).time_since_epoch().count());
    }
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
    {
        struct MicroClock {
            using duration = std::chrono::microseconds;
        };
        FastRandomContext ctx{true};
        // Check with clock type
        BOOST_CHECK_EQUAL(47222, ctx.rand_uniform_duration<MicroClock>(1s).count());
        // Check with time-point type
        BOOST_CHECK_EQUAL(2782, ctx.rand_uniform_duration<SteadySeconds>(9h).count());
    }

    // Check that a nondeterministic ones are not
    g_mock_deterministic_tests = false;
    for (int i = 10; i > 0; --i) {
        BOOST_CHECK(GetRand(std::numeric_limits<uint64_t>::max()) != uint64_t{10393729187455219830U});
        BOOST_CHECK(GetRandInt(std::numeric_limits<int>::max()) != int{769702006});
        BOOST_CHECK(GetRandMicros(std::chrono::hours{1}) != std::chrono::microseconds{2917185654});
        BOOST_CHECK(GetRandMillis(std::chrono::hours{1}) != std::chrono::milliseconds{2144374});
    }

    {
        FastRandomContext ctx3, ctx4;
        BOOST_CHECK(ctx3.rand64() != ctx4.rand64()); // extremely unlikely to be equal
    }
    {
        FastRandomContext ctx3, ctx4;
        BOOST_CHECK(ctx3.rand256() != ctx4.rand256());
    }
    {
        FastRandomContext ctx3, ctx4;
        BOOST_CHECK(ctx3.randbytes(7) != ctx4.randbytes(7));
    }
}

BOOST_AUTO_TEST_CASE(fastrandom_randbits)
{
    FastRandomContext ctx1;
    FastRandomContext ctx2;
    for (int bits = 0; bits < 63; ++bits) {
        for (int j = 0; j < 1000; ++j) {
            uint64_t rangebits = ctx1.randbits(bits);
            BOOST_CHECK_EQUAL(rangebits >> bits, 0U);
            uint64_t range = ((uint64_t)1) << bits | rangebits;
            uint64_t rand = ctx2.randrange(range);
            BOOST_CHECK(rand < range);
        }
    }
}

/** Does-it-compile test for compatibility with standard C++11 RNG interface. */
BOOST_AUTO_TEST_CASE(stdrandom_test)
{
    FastRandomContext ctx;
    std::uniform_int_distribution<int> distribution(3, 9);
    for (int i = 0; i < 100; ++i) {
        int x = distribution(ctx);
        BOOST_CHECK(x >= 3);
        BOOST_CHECK(x <= 9);

        std::vector<int> test{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::shuffle(test.begin(), test.end(), ctx);
        for (int j = 1; j <= 10; ++j) {
            BOOST_CHECK(std::find(test.begin(), test.end(), j) != test.end());
        }
        Shuffle(test.begin(), test.end(), ctx);
        for (int j = 1; j <= 10; ++j) {
            BOOST_CHECK(std::find(test.begin(), test.end(), j) != test.end());
        }
    }
}

/** Test that Shuffle reaches every permutation with equal probability. */
BOOST_AUTO_TEST_CASE(shuffle_stat_test)
{
    FastRandomContext ctx(true);
    uint32_t counts[5 * 5 * 5 * 5 * 5] = {0};
    for (int i = 0; i < 12000; ++i) {
        int data[5] = {0, 1, 2, 3, 4};
        Shuffle(std::begin(data), std::end(data), ctx);
        int pos = data[0] + data[1] * 5 + data[2] * 25 + data[3] * 125 + data[4] * 625;
        ++counts[pos];
    }
    unsigned int sum = 0;
    double chi_score = 0.0;
    for (int i = 0; i < 5 * 5 * 5 * 5 * 5; ++i) {
        int i1 = i % 5, i2 = (i / 5) % 5, i3 = (i / 25) % 5, i4 = (i / 125) % 5, i5 = i / 625;
        uint32_t count = counts[i];
        if (i1 == i2 || i1 == i3 || i1 == i4 || i1 == i5 || i2 == i3 || i2 == i4 || i2 == i5 || i3 == i4 || i3 == i5 || i4 == i5) {
            BOOST_CHECK(count == 0);
        } else {
            chi_score += ((count - 100.0) * (count - 100.0)) / 100.0;
            BOOST_CHECK(count > 50);
            BOOST_CHECK(count < 150);
            sum += count;
        }
    }
    BOOST_CHECK(chi_score > 58.1411); // 99.9999% confidence interval
    BOOST_CHECK(chi_score < 210.275);
    BOOST_CHECK_EQUAL(sum, 12000U);
}

BOOST_AUTO_TEST_SUITE_END()
