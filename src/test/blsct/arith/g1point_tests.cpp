// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <test/util/setup_common.h>

#include <algorithm>
#include <blsct/arith/elements.h>
#include <blsct/arith/g1point.h>
#include <blsct/arith/mcl_initializer.h>
#include <blsct/arith/scalar.h>
#include <boost/test/unit_test.hpp>
#include <set>
#include <streams.h>

BOOST_FIXTURE_TEST_SUITE(g1point_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_g1point_constructors)
{
    // Default
    {
        G1Point p;
        BOOST_CHECK(p.IsUnity() == true);
    }

    // std::vector<uint8_t>
    {
        auto g = G1Point::GetBasePoint();
        auto vch = g.GetVch();
        G1Point p(vch);
        BOOST_CHECK(g == p);
    }

    // G1Point
    {
        auto g = G1Point::GetBasePoint();
        G1Point p(g);
        BOOST_CHECK(g == p);
    }

    // mclBnG1
    {
        auto g = G1Point::GetBasePoint();
        G1Point p(g.m_p);
        BOOST_CHECK(g == p);
    }

    // uint256
    {
        G1Point p(uint256::ONE);
        auto s = p.GetString();
        BOOST_CHECK_EQUAL(s, "1 f6192bef86951fea27b115b4645cf5cf83bf067cf322647a1d1276c3d05208cb97cf72c5a0749fcfe631cf3fa246b9c 1764f91223eb414b6df18cc317537c17e242f678995b9894ef0d419725748a92ba0f5f58ecf5d403fae39cb41cc4e151");
    }
}

BOOST_AUTO_TEST_CASE(test_g1point_assign_op)
{
    auto g = G1Point::GetBasePoint();
    auto b = g + g;
    auto c = b;
    BOOST_CHECK(b == c);
}

BOOST_AUTO_TEST_CASE(test_g1point_point_add_sub)
{
    auto g = G1Point::GetBasePoint();
    auto p = g + g;
    auto q = p - g;
    BOOST_CHECK(q == g);
}

BOOST_AUTO_TEST_CASE(test_g1point_point_mul)
{
    auto g = G1Point::GetBasePoint();
    auto p = g + g + g;
    auto q = g * 3;
    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_CASE(test_g1point_point_equal_or_not_equal)
{
    auto g = G1Point::GetBasePoint();
    BOOST_CHECK(g == g);

    auto p = g + g;
    BOOST_CHECK(g != p);
}

BOOST_AUTO_TEST_CASE(test_g1point_double)
{
    auto g = G1Point::GetBasePoint();
    auto g2 = g + g;
    auto gd = g.Double();
    BOOST_CHECK(g2 == gd);

    auto g4 = g2 + g2;
    auto gdd = gd.Double();
    BOOST_CHECK(g4 == gdd);
}

BOOST_AUTO_TEST_CASE(test_g1point_get_base_point)
{
    auto g = G1Point::GetBasePoint();
    char g_act[1024];
    if (mclBnG1_getStr(g_act, sizeof(g_act), &g.m_p, 10) == 0) {
        BOOST_FAIL("Failed to get string representation of G");
    }
    const char* g_exp = "1 3685416753713387016781088315183077757961620795782546409894578378688607592378376318836054947676345821548104185464507 1339506544944476473020471379941921221584933875938349620426543736416511423956333506472724655353366534992391756441569";
    BOOST_CHECK_EQUAL(g_exp, g_act);
}

BOOST_AUTO_TEST_CASE(test_g1point_map_to_g1)
{
    // Different numbers should be mapped to different points
    std::set<std::string> xs;
    const size_t num_xs = 1000;
    for (size_t i = 0; i < num_xs; ++i) {
        std::stringstream ss;
        ss << i;
        std::string s = ss.str();
        std::vector<unsigned char> v(s.begin(), s.end());
        auto p = G1Point::MapToG1(v);
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
    G1Point::MapToG1(num_48_byte);

    // Empty vector should not be mapped to a point
    std::vector<uint8_t> empty_vec;
    BOOST_CHECK_THROW(G1Point::MapToG1(empty_vec), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_g1point_hash_and_map)
{
    std::vector<uint8_t> vec{
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

    // Use separate hash function to hash vec
    mclBnFp h;
    if (mclBnFp_setHashOf(&h, &vec[0], vec.size())) {
        BOOST_FAIL("mclBnFp_setHashOf failed");
    }
    char buf[1024];
    size_t ser_size = mclBnFp_serialize(buf, sizeof(buf), &h);
    if (ser_size == 0) {
        BOOST_FAIL("mclBnFp_serialize failed");
    }
    std::vector<uint8_t> hashedVec(buf, buf + ser_size);
    // mclBpFp_serialize serializes its value in big-endian

    // Then get g1 point from the hash
    auto p = G1Point::MapToG1(hashedVec, Endianness::Big);

    // Next, directly get g1 point from the vec, using integrated hash function
    auto q = G1Point::HashAndMap(vec);
    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_CASE(test_g1point_mulvec_mcl)
{
    auto base_point = G1Point::GetBasePoint();
    mclBnG1 p1, p2;
    p1 = base_point.m_p;
    mclBnG1_dbl(&p2, &p1);
    std::vector<mclBnG1> ps { p1, p2 };

    mclBnFr s1, s2;
    mclBnFr_setInt(&s1, 2);
    mclBnFr_setInt(&s2, 3);
    std::vector<mclBnFr> ss { s1, s2 };

    // p should be G^2 + (G+G)^3 = G^8
    auto p = G1Point::MulVec(ps, ss);
    auto q = base_point * 8;

    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_CASE(test_g1point_rand)
{
    unsigned int num_tries = 1000;
    unsigned int num_dups = 0;
    auto x = G1Point::Rand();
    for (size_t i = 0; i < num_tries; ++i) {
        auto y = G1Point::Rand();
        if (x == y) ++num_dups;
    }
    auto dupRatio = num_dups / (float) num_tries;
    BOOST_CHECK(dupRatio < 0.001);
}

BOOST_AUTO_TEST_CASE(test_g1point_is_unity)
{
    auto g = G1Point::GetBasePoint();
    BOOST_CHECK_EQUAL(g.IsUnity(), false);

    auto p = g - g;
    BOOST_CHECK_EQUAL(p.IsUnity(), true);
}

BOOST_AUTO_TEST_CASE(test_g1point_get_set_vch)
{
    G1Point p(uint256::ONE);
    auto vec = p.GetVch();

    G1Point q;
    BOOST_CHECK(p != q);

    q.SetVch(vec);
    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_CASE(test_g1point_get_string)
{
    auto g = G1Point::GetBasePoint();
    const char* g_exp = "1 3685416753713387016781088315183077757961620795782546409894578378688607592378376318836054947676345821548104185464507 1339506544944476473020471379941921221584933875938349620426543736416511423956333506472724655353366534992391756441569";
    auto g_act = g.GetString(10);
    BOOST_CHECK(g_exp == g_act);
}

BOOST_AUTO_TEST_CASE(test_g1point_get_serialize_size)
{
    G1Point p(uint256::ONE);
    auto ser_size = p.GetSerializeSize();
    BOOST_CHECK_EQUAL(ser_size, 49ul);
}

BOOST_AUTO_TEST_CASE(test_g1point_serialize_unserialize)
{
    G1Point p(uint256::ONE);
    CDataStream st(0, 0);
    p.Serialize(st);
    BOOST_CHECK_EQUAL(st.size(), 49ul);

    G1Point q;
    BOOST_CHECK(p != q);

    q.Unserialize(st);
    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_SUITE_END()
