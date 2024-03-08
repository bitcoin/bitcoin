// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_BLSCT_ARITH_SHARED_POINT_TESTS_H
#define BITCOIN_TEST_BLSCT_ARITH_SHARED_POINT_TESTS_H

#include <boost/test/unit_test.hpp>

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
    DataStream st{};
    p.Serialize(st);
    BOOST_CHECK_EQUAL(st.size(), Point::SERIALIZATION_SIZE);

    Point q;
    BOOST_CHECK(p != q);

    q.Unserialize(st);
    BOOST_CHECK(p == q);
}

#endif // BITCOIN_TEST_BLSCT_ARITH_SHARED_POINT_TESTS_H
