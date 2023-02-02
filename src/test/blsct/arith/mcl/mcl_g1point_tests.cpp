// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <algorithm>
#include <blsct/arith/elements.h>
#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_initializer.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <boost/test/unit_test.hpp>
#include <set>
#include <streams.h>

BOOST_FIXTURE_TEST_SUITE(mcl_g1point_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_constructors)
{
    // Default
    {
        MclG1Point p;
        BOOST_CHECK(p.IsUnity() == true);
    }

    // std::vector<uint8_t>
    {
        auto g = MclG1Point::GetBasePoint();
        auto vch = g.GetVch();
        MclG1Point p(vch);
        BOOST_CHECK(g == p);
    }

    // G1Point
    {
        auto g = MclG1Point::GetBasePoint();
        MclG1Point p(g);
        BOOST_CHECK(g == p);
    }

    // mclBnG1
    {
        auto g = MclG1Point::GetBasePoint();
        MclG1Point p(g.m_p);
        BOOST_CHECK(g == p);
    }

    // uint256
    {
        MclG1Point p(uint256::ONE);
        auto s = p.GetString();
        BOOST_CHECK_EQUAL(s, "1 33331d389fc9c4966441e7546f1a945e59e68ffdb940b60f5b107e4f9ff90947938df8efac81e00a23f0bebe2917745 16b7a7540112dc359913951450b2e21a7c05e59b5580a00588761fc673b21284a13bc7d73e99dd116f9f196b1985ac55");
    }
}

BOOST_AUTO_TEST_CASE(test_assign_op)
{
    auto g = MclG1Point::GetBasePoint();
    auto b = g + g;
    auto c = b;
    BOOST_CHECK(b == c);
}

BOOST_AUTO_TEST_CASE(test_point_add_sub)
{
    auto g = MclG1Point::GetBasePoint();
    auto p = g + g;
    auto q = p - g;
    BOOST_CHECK(q == g);
}

BOOST_AUTO_TEST_CASE(test_point_mul)
{
    auto g = MclG1Point::GetBasePoint();
    auto p = g + g + g;
    auto q = g * 3;
    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_CASE(test_points_mul)
{
    auto scalars = std::vector<MclScalar>({
        MclScalar(1), MclScalar(2)
    });
    auto g = MclG1Point::GetBasePoint();
    auto p1 = g;
    auto p2 = g + g;
    auto qs = g * scalars;
    BOOST_CHECK(qs[0] == p1);
    BOOST_CHECK(qs[1] == p2);
}

BOOST_AUTO_TEST_CASE(test_point_equal_or_not_equal)
{
    auto g = MclG1Point::GetBasePoint();
    BOOST_CHECK(g == g);

    auto p = g + g;
    BOOST_CHECK(g != p);
}

BOOST_AUTO_TEST_CASE(test_double)
{
    auto g = MclG1Point::GetBasePoint();
    auto g2 = g + g;
    auto gd = g.Double();
    BOOST_CHECK(g2 == gd);

    auto g4 = g2 + g2;
    auto gdd = gd.Double();
    BOOST_CHECK(g4 == gdd);
}

BOOST_AUTO_TEST_CASE(test_get_base_point)
{
    auto g = MclG1Point::GetBasePoint();
    char g_act[1024];
    if (mclBnG1_getStr(g_act, sizeof(g_act), &g.m_p, 10) == 0) {
        BOOST_FAIL("Failed to get string representation of G");
    }
    const char* g_exp = "1 3685416753713387016781088315183077757961620795782546409894578378688607592378376318836054947676345821548104185464507 1339506544944476473020471379941921221584933875938349620426543736416511423956333506472724655353366534992391756441569";
    BOOST_CHECK_EQUAL(g_exp, g_act);
}

BOOST_AUTO_TEST_CASE(test_map_to_g1)
{
    // Different numbers should be mapped to different points
    std::set<std::string> xs;
    const size_t num_xs = 1000;
    for (size_t i = 0; i < num_xs; ++i) {
        std::stringstream ss;
        ss << i;
        std::string s = ss.str();
        std::vector<unsigned char> v(s.begin(), s.end());
        auto p = MclG1Point::MapToG1(v);
        xs.insert(p.GetString());
    }
    BOOST_CHECK_EQUAL(xs.size(), num_xs);

    // Just check if MapToG1 accepts a large number
    std::vector<uint8_t> num_48_byte{
        0x73,
        0xed,
        0xa7,
        0x53,
        0x29,
        0x9d,
        0x7d,
        0x48,
        0x33,
        0x39,
        0xd8,
        0x08,
        0x09,
        0xa1,
        0xd8,
        0x05,
        0x53,
        0xbd,
        0xa4,
        0x02,
        0xff,
        0xfe,
        0x5b,
        0xfe,
        0xff,
        0xff,
        0xff,
        0xff,
        0x00,
        0x00,
        0xd8,
        0x08,
        0x09,
        0xa1,
        0xd8,
        0x05,
        0x53,
        0xbd,
        0xa4,
        0x02,
        0xff,
        0xfe,
        0x5b,
        0xfe,
        0xff,
        0xff,
        0xff,
        0xff,
    };
    MclG1Point::MapToG1(num_48_byte);

    // Empty vector should not be mapped to a point
    std::vector<uint8_t> empty_vec;
    BOOST_CHECK_THROW(MclG1Point::MapToG1(empty_vec), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_rand)
{
    unsigned int num_tries = 1000;
    unsigned int num_dups = 0;
    auto x = MclG1Point::Rand();
    for (size_t i = 0; i < num_tries; ++i) {
        auto y = MclG1Point::Rand();
        if (x == y) ++num_dups;
    }
    auto dupRatio = num_dups / (float) num_tries;
    BOOST_CHECK(dupRatio < 0.001);
}

BOOST_AUTO_TEST_CASE(test_is_unity)
{
    auto g = MclG1Point::GetBasePoint();
    BOOST_CHECK_EQUAL(g.IsUnity(), false);

    auto p = g - g;
    BOOST_CHECK_EQUAL(p.IsUnity(), true);
}

BOOST_AUTO_TEST_CASE(test_get_set_vch)
{
    MclG1Point p(uint256::ONE);
    auto vec = p.GetVch();

    MclG1Point q;
    BOOST_CHECK(p != q);

    q.SetVch(vec);
    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_CASE(test_get_string)
{
    auto g = MclG1Point::GetBasePoint();
    const char* g_exp = "1 3685416753713387016781088315183077757961620795782546409894578378688607592378376318836054947676345821548104185464507 1339506544944476473020471379941921221584933875938349620426543736416511423956333506472724655353366534992391756441569";
    auto g_act = g.GetString(10);
    BOOST_CHECK(g_exp == g_act);
}

BOOST_AUTO_TEST_CASE(test_get_serialize_size)
{
    MclG1Point p(uint256::ONE);
    auto ser_size = p.GetSerializeSize();
    BOOST_CHECK_EQUAL(ser_size, 48ul);
}

BOOST_AUTO_TEST_CASE(test_serialize_unserialize)
{
    MclG1Point p(uint256::ONE);
    CDataStream st(0, 0);
    p.Serialize(st);
    BOOST_CHECK_EQUAL(st.size(), 49ul); // TODO check why 49 instead of 48

    MclG1Point q;
    BOOST_CHECK(p != q);

    q.Unserialize(st);
    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_CASE(test_get_hash_with_salt)
{
    auto g = MclG1Point::GetBasePoint();
    auto a = g.GetHashWithSalt(1);
    auto b = g.GetHashWithSalt(2);
    BOOST_CHECK(a != b);
}

BOOST_AUTO_TEST_CASE(test_operator_mul_g1point_by_scalars)
{
    MclScalar one(1);
    MclScalar two(2);
    std::vector<MclScalar> one_two { one, two };
    auto g = MclG1Point::GetBasePoint();

    auto act = g * one_two;
    std::vector<MclG1Point> exp { g, g + g };

    BOOST_CHECK(act == exp);
}

BOOST_AUTO_TEST_SUITE_END()