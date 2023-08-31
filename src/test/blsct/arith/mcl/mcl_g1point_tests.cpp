// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <algorithm>
#include <blsct/arith/elements.h>
#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <boost/test/unit_test.hpp>
#include <set>
#include <streams.h>
#include <uint256.h>

#define BASE_POINT_HEX "1 17f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb 8b3f481e3aaa0f1a09e30ed741d8ae4fcf5e095d5d00af600db18cb2c04b3edd03cc744a2888ae40caa232946c5e7e1"

#define LARGE_NUM_VECTOR(var) \
    std::vector<uint8_t> var { \
        0x73, 0xed, 0xa7, 0x53, 0x29, 0x9d, 0x7d, 0x48, \
        0x33, 0x39, 0xd8, 0x08, 0x09, 0xa1, 0xd8, 0x05, \
        0x53, 0xbd, 0xa4, 0x02, 0xff, 0xfe, 0x5b, 0xfe, \
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xd8, 0x08, \
        0x09, 0xa1, 0xd8, 0x05, 0x53, 0xbd, 0xa4, 0x02, \
        0xff, 0xfe, 0x5b, 0xfe, 0xff, 0xff, 0xff, 0xff, \
    };

using Point = MclG1Point;
using Scalar = MclScalar;

BOOST_FIXTURE_TEST_SUITE(mcl_g1point_tests, BasicTestingSetup)

#include <test/blsct/arith/shared_point_tests.h>

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(test_uint256_ctor)
{
    Point p(ONE);
    auto s = p.GetString();
    BOOST_CHECK_EQUAL(s, "1 33331d389fc9c4966441e7546f1a945e59e68ffdb940b60f5b107e4f9ff90947938df8efac81e00a23f0bebe2917745 16b7a7540112dc359913951450b2e21a7c05e59b5580a00588761fc673b21284a13bc7d73e99dd116f9f196b1985ac55");
}

BOOST_AUTO_TEST_CASE(test_get_hash_with_salt)
{
    auto g = Point::GetBasePoint();
    auto a = g.GetHashWithSalt(1);
    auto b = g.GetHashWithSalt(2);
    BOOST_CHECK(a != b);
}

BOOST_AUTO_TEST_CASE(test_map_str_to_point)
{
    // Different strings should be mapped to different points
    std::set<std::string> xs;
    const size_t num_xs = 1000;
    for (size_t i = 0; i < num_xs; ++i) {
        std::stringstream ss;
        ss << i;
        std::string s = ss.str();
        auto p = Point::MapToPoint(s);
        xs.insert(p.GetString());
    }
    BOOST_CHECK_EQUAL(xs.size(), num_xs);

    // Empty string should not be mapped to a point
    BOOST_CHECK_THROW(Point::MapToPoint(""), std::runtime_error);
}