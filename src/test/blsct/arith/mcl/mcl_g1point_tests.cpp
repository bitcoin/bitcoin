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

#define ONE uint256::ONE

BOOST_AUTO_TEST_CASE(test_constructors)
{
    // Default
    {
        Point p;
        BOOST_CHECK(p.IsZero() == true);

        Point p2;
        BOOST_CHECK(p.GetVch() == p2.GetVch());
        BOOST_CHECK(p == p2);
    }

    // std::vector<uint8_t>
    {
        auto g = Point::GetBasePoint();
        auto vch = g.GetVch();
        Point p(vch);
        BOOST_CHECK(g == p);
    }

    // Point
    {
        auto g = Point::GetBasePoint();
        Point p(g);
        BOOST_CHECK(g == p);
    }

    // Point underlying
    {
        auto g = Point::GetBasePoint();
        Point p(g.m_point);
        BOOST_CHECK(g == p);
    }
}

BOOST_AUTO_TEST_CASE(test_assign_op)
{
    auto g = Point::GetBasePoint();
    auto b = g + g;
    auto c = b;
    BOOST_CHECK(b == c);
}

BOOST_AUTO_TEST_CASE(test_point_add_sub)
{
    auto g = Point::GetBasePoint();
    auto p = g + g;
    auto q = p - g;
    BOOST_CHECK(q == g);
}

BOOST_AUTO_TEST_CASE(test_point_mul)
{
    // consistency check with addition
    {
        const auto g = Point::GetBasePoint();
        Point g_sum;

        for (size_t i=1; i<=50; ++i) {
            g_sum = g_sum + Point::GetBasePoint();
            Point q = g * i;
            BOOST_CHECK(g_sum == q);
        }
    }
    // non base point times scalar
    {
        auto g = Point::GetBasePoint();
        Scalar two(2);
        Scalar four(4);
        auto g_times_2 = g * two;
        auto lhs = g_times_2 * two;
        auto rhs = g * four;
        BOOST_CHECK(lhs == rhs);
    }
}

BOOST_AUTO_TEST_CASE(test_points_mul)
{
    std::vector<Scalar> scalars = {
        Scalar(1), Scalar(2), Scalar(3), Scalar(4)
    };
    Point g, p1, p2, p3, p4;
    g = Point::GetBasePoint();
    p1 = g;
    p2 = g + g;
    p3 = g + g + g;
    p4 = g + g + g + g;
    auto qs = g * scalars;
    BOOST_CHECK(qs.size() == 4);
    BOOST_CHECK(qs[0] == p1);
    BOOST_CHECK(qs[1] == p2);
    BOOST_CHECK(qs[2] == p3);
    BOOST_CHECK(qs[3] == p4);
}

BOOST_AUTO_TEST_CASE(test_point_equal_or_not_equal)
{
    auto g = Point::GetBasePoint();
    BOOST_CHECK(g == g);

    auto p = g + g;
    BOOST_CHECK(g != p);
}

BOOST_AUTO_TEST_CASE(test_double)
{
    auto p_by_double = Point::GetBasePoint();
    auto p_by_add = Point::GetBasePoint();

    for (size_t i=0; i<10; ++i) {
        p_by_add = p_by_add + p_by_add;
        p_by_double = p_by_double.Double();
        BOOST_CHECK(p_by_add == p_by_double);

    }
}

BOOST_AUTO_TEST_CASE(test_get_base_point)
{
    auto g = Point::GetBasePoint();
    auto g_act = g.GetString();
    const char* g_exp = BASE_POINT_HEX;
    BOOST_CHECK_EQUAL(g_exp, g_act);
}

BOOST_AUTO_TEST_CASE(test_map_vec_to_point)
{
    // Different numbers should be mapped to different points
    std::set<std::string> xs;
    const size_t num_xs = 1000;
    for (size_t i = 0; i < num_xs; ++i) {
        std::stringstream ss;
        ss << i;
        std::string s = ss.str();
        std::vector<unsigned char> v(s.begin(), s.end());
        auto p = Point::MapToPoint(v);
        xs.insert(p.GetString());
    }
    BOOST_CHECK_EQUAL(xs.size(), num_xs);

    // Just check if MapToPoint accepts a large number
    LARGE_NUM_VECTOR(large_num);
    Point::MapToPoint(large_num);

    // Empty vector should not be mapped to a point
    std::vector<uint8_t> empty_vec;
    BOOST_CHECK_THROW(Point::MapToPoint(empty_vec), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_rand)
{
    unsigned int num_tries = 1000;
    unsigned int num_dups = 0;
    auto x = Point::Rand();
    for (size_t i = 0; i < num_tries; ++i) {
        auto y = Point::Rand();
        if (x == y) ++num_dups;
    }
    auto dupRatio = num_dups / (float)num_tries;
    BOOST_CHECK(dupRatio < 0.001);
}

BOOST_AUTO_TEST_CASE(test_is_zero)
{
    auto g = Point::GetBasePoint();
    BOOST_CHECK_EQUAL(g.IsZero(), false);

    auto p = g - g;
    BOOST_CHECK_EQUAL(p.IsZero(), true);
}

BOOST_AUTO_TEST_CASE(test_get_set_vch)
{
    Point p = Point::GetBasePoint();
    auto vec = p.GetVch();

    Point q;
    BOOST_CHECK(p != q);

    q.SetVch(vec);
    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_CASE(test_get_string)
{
    auto g = Point::GetBasePoint();
    const char* g_exp = BASE_POINT_HEX;
    auto g_act = g.GetString();
    BOOST_CHECK(g_exp == g_act);
}

BOOST_AUTO_TEST_CASE(test_serialize_unserialize)
{
    Point p = Point::GetBasePoint();
    CDataStream st(0, 0);
    p.Serialize(st);
    BOOST_CHECK_EQUAL(st.size(), Point::SERIALIZATION_SIZE);

    Point q;
    BOOST_CHECK(p != q);

    q.Unserialize(st);
    BOOST_CHECK(p == q);
}

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

BOOST_AUTO_TEST_SUITE_END()
