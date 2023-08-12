// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <uint256.h>

#include <cinttypes>
#include <cmath>
#include <limits>

#define CURVE_ORDER_VEC(x) \
std::vector<uint8_t> x = { \
    0x73, 0xed, 0xa7, 0x53, 0x29, 0x9d, 0x7d, 0x48, \
    0x33, 0x39, 0xd8, 0x08, 0x09, 0xa1, 0xd8, 0x05, \
    0x53, 0xbd, 0xa4, 0x02, 0xff, 0xfe, 0x5b, 0xfe, \
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, \
};

#define CURVE_ORDER_MINUS_1_VEC(x) \
std::vector<uint8_t> x = { \
    0x73, 0xed, 0xa7, 0x53, 0x29, 0x9d, 0x7d, 0x48, \
    0x33, 0x39, 0xd8, 0x08, 0x09, 0xa1, 0xd8, 0x05, \
    0x53, 0xbd, 0xa4, 0x02, 0xff, 0xfe, 0x5b, 0xfe, \
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, \
};

#define CURVE_ORDER_MINUS_1_STR "73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000"

#define CURVE_ORDER_MINUS_1_SCALAR(x) Scalar x("73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000", 16)
#define SCALAR_INT64_MIN(x) Scalar x("52435875175126190479447740508185965837690552500527637822594435327901726408705", 10);

using Scalar = MclScalar;

BOOST_FIXTURE_TEST_SUITE(mcl_scalar_tests, BasicTestingSetup)

#define ONE_BE_32(var) std::vector<uint8_t> var { \
    0x01, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, \
};

BOOST_AUTO_TEST_CASE(test_ctors) {
    //// default
    {
        Scalar n;
        BOOST_CHECK(n.IsZero());
    }
    //// vector<uint8_t>
    {
        {
            CURVE_ORDER_VEC(curve_order_be);
            CURVE_ORDER_MINUS_1_VEC(curve_order_minus_1_be);
            {
                Scalar a(curve_order_be);
                BOOST_CHECK_EQUAL(a.GetString(), "0");
            }
            {
                Scalar a(curve_order_minus_1_be);
                BOOST_CHECK_EQUAL(a.GetString(), CURVE_ORDER_MINUS_1_STR);
            }
        }
        {
            ONE_BE_32(one);
            Scalar a(one);
            BOOST_CHECK_EQUAL(a.GetString(), "100000000000000000000000000000000000000000000000000000000000000");
        }
    }
    //// uint256
    {
        ONE_BE_32(one)
        uint256 ui(one);
        Scalar a(ui);
        BOOST_CHECK_EQUAL(a.GetString(), "100000000000000000000000000000000000000000000000000000000000000");
    }
    {
        CURVE_ORDER_MINUS_1_VEC(vec);
        uint256 ui(vec);
        Scalar a(ui);
        BOOST_CHECK_EQUAL(a.GetString(), CURVE_ORDER_MINUS_1_STR);
    }
    {
        CURVE_ORDER_VEC(vec);
        uint256 ui(vec);
        Scalar a(ui);
        BOOST_CHECK_EQUAL(a.GetString(), "0");
    }
}

BOOST_AUTO_TEST_CASE(test_msg_int_ctor)
{
    {
        Scalar a(std::vector<uint8_t>{1,2,3}, 1);
        Scalar b(std::vector<uint8_t>{1,2,3}, 1);
        BOOST_CHECK(a == b);
    }
    {
        Scalar a(std::vector<uint8_t>{1,2,3}, 1);
        Scalar b(std::vector<uint8_t>{1,2,3}, 2);
        BOOST_CHECK(a != b);
    }
    {
        Scalar a(std::vector<uint8_t>{1,2,3}, 1);
        Scalar b(std::vector<uint8_t>{1,2,3,4}, 1);
        BOOST_CHECK(a != b);
    }
}

BOOST_AUTO_TEST_CASE(test_add)
{
    {
        Scalar a(1);
        Scalar b(2);
        Scalar c(3);
        BOOST_CHECK((a + b) == c);
    }
    {
        CURVE_ORDER_MINUS_1_SCALAR(a);
        Scalar b(1);
        Scalar c(0);
        BOOST_CHECK((a + b) == c);
    }
}

BOOST_AUTO_TEST_CASE(test_sub)
{
    {
        Scalar a(5);
        Scalar b(3);
        Scalar c(2);
        BOOST_CHECK((a - b) == c);
    }
    {
        Scalar a(0);
        Scalar b(1);
        CURVE_ORDER_MINUS_1_SCALAR(c);
        BOOST_CHECK((a - b) == c);
    }
}

BOOST_AUTO_TEST_CASE(test_mul)
{
    Scalar a(2);
    Scalar b(3);
    Scalar c(6);
    BOOST_CHECK((a * b) == c);
}

BOOST_AUTO_TEST_CASE(test_div)
{
    Scalar a(6);
    Scalar b(3);
    Scalar c(2);
    BOOST_CHECK((a / b) == c);
}

BOOST_AUTO_TEST_CASE(test_equal_or_not_equal_to_integer)
{
    Scalar a(6);
    int b = 6;
    int c = 5;
    BOOST_CHECK(a == b);
    BOOST_CHECK(a != c);
}

BOOST_AUTO_TEST_CASE(test_equal_or_not_equal_to_scalar)
{
    Scalar a(6);
    Scalar b(6);
    Scalar c(5);
    BOOST_CHECK(a == b);
    BOOST_CHECK(a != c);
}

BOOST_AUTO_TEST_CASE(test_invert)
{
    Scalar a(6);
    Scalar b = a.Invert();
    Scalar c = b.Invert();
    BOOST_CHECK(a == c);
}

BOOST_AUTO_TEST_CASE(test_invert_zero)
{
    Scalar a(0);
    BOOST_CHECK_THROW(a.Invert(), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_negate)
{
    Scalar a(6);
    Scalar b = a.Negate();
    BOOST_CHECK(a != b);
    BOOST_CHECK((a + b).IsZero());
    Scalar c = b.Negate();
    BOOST_CHECK(a == c);
}

BOOST_AUTO_TEST_CASE(test_square)
{
    Scalar a(9);
    Scalar b(81);
    Scalar c = a.Square();
    BOOST_CHECK(b == c);
}

BOOST_AUTO_TEST_CASE(test_cube)
{
    Scalar a(3);
    Scalar b(27);
    Scalar c = a.Cube();
    BOOST_CHECK(b == c);
}

BOOST_AUTO_TEST_CASE(test_rand)
{
    std::vector<bool> tf{true, false};
    for (auto exclude_zero : tf) {
        unsigned int num_tries = 1000000;
        unsigned int num_dups = 0;
        auto x = Scalar::Rand();

        for (size_t i = 0; i < num_tries; ++i) {
            auto y = Scalar::Rand(exclude_zero);
            if (exclude_zero && y == 0) BOOST_FAIL("expected non-zero");
            if (x == y) ++num_dups;
        }
        auto dup_ratio = num_dups / (float)num_tries;
        BOOST_CHECK(dup_ratio < 0.000001);
    }
}

BOOST_AUTO_TEST_CASE(test_getvch)
{
    std::vector<uint8_t> vec{
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
        14,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        26,
        27,
        28,
        29,
        30,
        31,
    };
    Scalar a(vec);
    {
        auto a_vec = a.GetVch();
        BOOST_CHECK(vec == a_vec);
    }
    {
        // with trim option on, the first 0 should have been removed in act
        auto act = a.GetVch(true);
        std::vector<uint8_t> exp(vec.size() - 1);
        std::copy(vec.begin() + 1, vec.end(), exp.begin());
        BOOST_CHECK(act == exp);
    }
}

BOOST_AUTO_TEST_CASE(test_get_and_setvch)
{
    std::vector<uint8_t> vec{
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
        14,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        26,
        27,
        28,
        29,
        30,
        31,
    };
    Scalar a(vec);
    auto a_vec = a.GetVch();

    Scalar b(0);
    b.SetVch(a_vec);
    BOOST_CHECK(a == b);
}

BOOST_AUTO_TEST_CASE(test_setpow2)
{
    for (size_t i = 0; i < 10; ++i) {
        Scalar a;
        a.SetPow2(i);
        BOOST_CHECK_EQUAL(a.GetUint64(), std::pow(2, i));
    }
}

BOOST_AUTO_TEST_CASE(test_pow)
{
    struct TestCase {
        int64_t a;
        int64_t b;
        int64_t c;
    };
    std::vector test_cases{
        TestCase{2, 0, 1},
        TestCase{2, 1, 2},
        TestCase{2, 2, 4},
        TestCase{2, 3, 8},
        TestCase{3, 5, 243},
        TestCase{19, 7, 893871739},
    };
    for (auto tc : test_cases) {
        Scalar a(tc.a);
        Scalar b(tc.b);
        Scalar c(tc.c);
        Scalar d = a.Pow(b);
        BOOST_CHECK(c == d);
    }

    // this is to check if calculation finishes within a reasonable amount of time
    Scalar y(1);
    y.Invert().Pow(y.Invert());
}

BOOST_AUTO_TEST_CASE(test_to_binary_vec)
{
    {
        Scalar a(0);
        auto act = a.ToBinaryVec();

        std::vector<bool> exp {false};
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(1);
        auto act = a.ToBinaryVec();
        std::vector<bool> exp {true};
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(2);
        auto act = a.ToBinaryVec();
        std::vector<bool> exp {true, false};
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(3);
        auto act = a.ToBinaryVec();
        std::vector<bool> exp {true, true};
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(4);
        auto act = a.ToBinaryVec();
        std::vector<bool> exp {true, false, false};
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(5);
        auto act = a.ToBinaryVec();
        std::vector<bool> exp {true, false, true};
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(6);
        auto act = a.ToBinaryVec();
        std::vector<bool> exp {true, true, false};
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(7);
        auto act = a.ToBinaryVec();
        std::vector<bool> exp {true, true, true};
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(UINT32_MAX);
        auto act = a.ToBinaryVec();
        std::vector<bool> exp;
        for (size_t i=0; i<32; ++i) {
            exp.push_back(true);
        }
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(UINT32_MAX);
        Scalar one(1);
        Scalar b = a + one;
        auto act = b.ToBinaryVec();
        std::vector<bool> exp;
        exp.push_back(true);
        for (size_t i=0; i<32; ++i) {
            exp.push_back(false);
        }
        BOOST_CHECK(act == exp);
    }
}

BOOST_AUTO_TEST_CASE(test_get_string_radix_16)
{
    {
        Scalar a(0);
        auto act = a.GetString();
        std::string exp("0");
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(15);
        auto act = a.GetString();
        std::string exp("f");
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(16);
        auto act = a.GetString();
        std::string exp("10");
        BOOST_CHECK(act == exp);
    }
    {
        Scalar a(0xffff);
        auto act = a.GetString();
        std::string exp("ffff");
        BOOST_CHECK(act == exp);
    }
    {
        uint32_t n = UINT32_MAX;
        Scalar a(n);
        auto act = a.GetString();
        std::string exp("ffffffff");
        BOOST_CHECK(act == exp);
    }
}


BOOST_AUTO_TEST_CASE(test_ctor_uint256_int64)
{
    //// int64_t
    {
        uint64_t one = 1;
        {
            // test up to shift = 62 excluding the sign bit
            for (size_t shift = 0; shift < 63; ++shift) {
                int64_t i = one << shift;
                Scalar a(i);
                BOOST_CHECK_EQUAL(a.GetUint64(), i);
            }
        }
        {
            int64_t i = -1;
            Scalar a(i);
            // fr order: 0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001
            //       -1: 0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000
            BOOST_CHECK_EQUAL(a.GetString().c_str(), "73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_bitwise_or)
{
    {
        // there is no bit that has 1 in both a and b
        Scalar a(0b0001000100010001);
        Scalar b(0b1100110011001100);
        Scalar exp(0b1101110111011101);
        auto act = a | b;
        BOOST_CHECK(act == exp);
    }
    {
        // there are bits that have 1 in both aband b
        Scalar a(0b0001000100010001);
        Scalar b(0b1101110111001101);
        Scalar exp(0b1101110111011101);
        auto act = a | b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is shorter than b. expects big-endian merge
        Scalar a(0b11111111);
        Scalar b(0b1000100010001000);
        Scalar exp(0b1000100011111111);
        auto act = a | b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is longer than b. expects big-endian merge
        Scalar a(0b1000100010001000);
        Scalar b(0b11111111);
        Scalar exp(0b1000100011111111);
        auto act = a | b;
        BOOST_CHECK(act == exp);
    }
}

BOOST_AUTO_TEST_CASE(test_bitwise_xor)
{
    {
        // there is no bit that has 1 in both a and b
        Scalar a(0b0001000100010001);
        Scalar b(0b1100110011001100);
        Scalar exp(0b1101110111011101);
        auto act = a ^ b;
        BOOST_CHECK(act == exp);
    }
    {
        // there are bits that have 1 in both aband b
        Scalar a(0b0001000100010001);
        Scalar b(0b1101110111001101);
        Scalar exp(0b1100110011011100);
        auto act = a ^ b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is shorter than b. expects big-endian merge
        Scalar a(0b11111111);
        Scalar b(0b1000100010001000);
        Scalar exp(0b1000100001110111);
        auto act = a ^ b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is longer than b. expects big-endian merge
        Scalar a(0b1000100010001000);
        Scalar b(0b11111111);
        Scalar exp(0b1000100001110111);
        auto act = a ^ b;
        BOOST_CHECK(act == exp);
    }
}

BOOST_AUTO_TEST_CASE(test_bitwise_and)
{
    {
        // there is no bit that has 1 in both a and b
        Scalar a(0b0001000100010001);
        Scalar b(0b1100110011001100);
        Scalar exp(0b0);
        auto act = a & b;
        BOOST_CHECK(act == exp);
    }
    {
        // there are bits that have 1 in both aband b
        Scalar a(0b0001000100010001);
        Scalar b(0b1101110111001101);
        Scalar exp(0b1000100000001);
        auto act = a & b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is shorter than b. expects big-endian merge
        Scalar a(0b11111111);
        Scalar b(0b1000100010001000);
        Scalar exp(0b10001000);
        auto act = a & b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is longer than b. expects big-endian merge
        Scalar a(0b1000100010001000);
        Scalar b(0b11111111);
        Scalar exp(0b10001000);
        auto act = a & b;
        BOOST_CHECK(act == exp);
    }
}

BOOST_AUTO_TEST_CASE(test_bitwise_compl)
{
    // ~ operator doesn't work w/ very large number such as ~1 i.e.
    // 0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe
    // which is 1 inverted in 32-byte buffer
    // due to limitation of mclBnFr_deserialize
    int64_t n = INT64_MAX;
    Scalar a(n);
    auto act = (~a).GetString(16);

    // ~INT64MAX is -9223372036854775808 which equals below in Fr
    std::string exp = "73eda753299d7d483339d80809a1d80553bda402fffe5bfe7fffffff00000001";
    BOOST_CHECK(act == exp);
}

BOOST_AUTO_TEST_CASE(test_shift_left)
{
    Scalar one(1);
    uint64_t exp = 1;
    // test up to the positive max of int64_t since assignment op takes int64_t as an input
    for (size_t i = 0; i < 63; ++i) {
        Scalar act = one << i;
        BOOST_CHECK_EQUAL(act.GetUint64(), exp);
        exp <<= 1;
    }
}

BOOST_AUTO_TEST_CASE(test_shift_right)
{
    Scalar eight(8);
    Scalar seven(7);
    Scalar six(6);
    Scalar five(5);
    Scalar four(4);
    Scalar three(3);
    Scalar two(2);
    Scalar one(1);
    Scalar zero(0);

    BOOST_CHECK((eight >> 1) == four);
    BOOST_CHECK((seven >> 1) == three);
    BOOST_CHECK((six >> 1) == three);
    BOOST_CHECK((five >> 1) == two);
    BOOST_CHECK((four >> 1) == two);
    BOOST_CHECK((three >> 1) == one);
    BOOST_CHECK((two >> 1) == one);
    BOOST_CHECK((one >> 1) == zero);
}

BOOST_AUTO_TEST_CASE(test_assign_int64)
{
    {
        int64_t n = INT64_MAX;
        Scalar a = n;
        BOOST_CHECK_EQUAL(a.GetUint64(), n);
    }
    {
        Scalar a(INT64_MIN);
        SCALAR_INT64_MIN(b);
        BOOST_CHECK_EQUAL(a.GetString(16), b.GetString(16));
    }
}


BOOST_AUTO_TEST_CASE(test_get_uint64)
{
    {
        // Scalar(int) operator takes int64_t, so let it take INT64_MAX
        Scalar a(INT64_MAX);
        uint64_t b = a.GetUint64();
        uint64_t c = 9223372036854775807ul; // is INT64_MAX
        BOOST_CHECK_EQUAL(b, c);
    }
    {
        // assignment operator takes int64_t
        Scalar base(0b1);
        int64_t n = 1;
        for (uint8_t i = 0; i < 63; ++i) { // test up to positive max of int64_t
            Scalar a = base << i;
            BOOST_CHECK_EQUAL(a.GetUint64(), n);
            n <<= 1;
        }
    }
    {
        int64_t int64_t_min = std::numeric_limits<int64_t>::min();
        Scalar s(int64_t_min);

        // int64_t minimum value maps to:
        // '0b111111111111111111111111111111100000000000000000000000000000001'
        // = 9223372032559808513
        uint64_t exp(9223372032559808513);

        BOOST_CHECK_EQUAL(s.GetUint64(), exp);
    }
}

BOOST_AUTO_TEST_CASE(test_hash_with_salt)
{
    // upon generating the digest, following data is added to the hasher:
    // - 1 byte storing the size of the following array
    // - 32-byte big-endian array representing the Scalar value
    // - 4-byte little-endian array representing the parameter of Hash function

    Scalar a(1);
    const int n = 51;
    uint256 digest = a.GetHashWithSalt(n);
    auto act = digest.GetHex();
    std::string exp("ccf3c17eeef9710908b3a66f5103cf97cc7313e0f49cf93429dc92d01f8fd5b2");
    BOOST_CHECK(act == exp);
}

BOOST_AUTO_TEST_CASE(test_getstring_base_2_10_16)
{
    Scalar a(0xffff);

    auto act16 = a.GetString(16);
    std::string exp16("ffff");
    BOOST_CHECK(act16 == exp16);

    auto act10 = a.GetString(10);
    std::string exp10("65535");
    BOOST_CHECK(act10 == exp10);

    auto act2 = a.GetString(2);
    std::string exp2("1111111111111111");
    BOOST_CHECK(act2 == exp2);

    int64_t n = INT64_MIN;
    std::string s("52435875175126190479447740508185965837690552500527637822594435327901726408705");
    Scalar b(n);
    auto act_int64_min = b.GetString(10);
    BOOST_CHECK(act_int64_min == s);
}

BOOST_AUTO_TEST_CASE(test_get_bits)
{
    // n is group order r minus 1
    std::vector<uint8_t> n_vec{
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
        0x00,
        0x00,
    };
    std::string n_hex("73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000");
    std::string n_bin("111001111101101101001110101001100101001100111010111110101001000001100110011100111011000000010000000100110100001110110000000010101010011101111011010010000000010111111111111111001011011111111101111111111111111111111111111111100000000000000000000000000000000");

    auto u = uint256(n_vec);
    Scalar s(u);

    std::string exp = n_bin;
    auto bs = s.ToBinaryVec();

    BOOST_CHECK(bs.size() == exp.size());
    for (size_t i = 0; i < bs.size(); ++i) {
        auto expC = exp[i];
        auto actC = bs[i] == true ? '1' : '0';
        BOOST_CHECK(expC == actC);
    }
}

BOOST_AUTO_TEST_CASE(test_get_bit)
{
    {
        Scalar a(0b100000001);
        BOOST_CHECK_EQUAL(a.GetSeriBit(0), true); // 1st byte
        BOOST_CHECK_EQUAL(a.GetSeriBit(1), false);
        BOOST_CHECK_EQUAL(a.GetSeriBit(2), false);
        BOOST_CHECK_EQUAL(a.GetSeriBit(3), false);
        BOOST_CHECK_EQUAL(a.GetSeriBit(4), false);
        BOOST_CHECK_EQUAL(a.GetSeriBit(5), false);
        BOOST_CHECK_EQUAL(a.GetSeriBit(6), false);
        BOOST_CHECK_EQUAL(a.GetSeriBit(7), false);
        BOOST_CHECK_EQUAL(a.GetSeriBit(8), true); // 2nd byte
        BOOST_CHECK_EQUAL(a.GetSeriBit(9), false);
    }
    {
        CURVE_ORDER_MINUS_1_SCALAR(a);
        auto v = a.GetVch();

        // 5th byte from the last is 255
        for (size_t i = 0; i < 8; ++i) {
            BOOST_CHECK_EQUAL(a.GetSeriBit(4 * 8 + i), true);
        }

        // 9th byte from the last is 254
        for (size_t i = 0; i < 8; ++i) {
            BOOST_CHECK_EQUAL(a.GetSeriBit(8 * 8 + i), i != 0);
        }

        // 13th byte from the last is 2
        for (size_t i = 0; i < 8; ++i) {
            BOOST_CHECK_EQUAL(a.GetSeriBit(12 * 8 + i), i == 1);
        }

        // 32nd byte from the last is 115
        // 115 = 0b01110011 and rightmost bit is index 0
        std::vector<bool> bits115 = {true, true, false, false, true, true, true, false};
        for (size_t i = 0; i < 8; ++i) {
            BOOST_CHECK_EQUAL(a.GetSeriBit(31 * 8 + i), bits115[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_create_64_bit_shift)
{
    // serialized excess based on message "spaghetti meatballs"
    // = 7370616765747469206d65617462616c6c730000000000000001
    // = 111001101110000011000010110011101100101011101000111010001101001001000000110110101100101011000010111010001100010011000010110110001101100011100110000000000000000000000000000000000000000000000000000000000000001
    std::vector<unsigned char> excess_ser{
        0,
        0,
        0,
        0,
        0,
        115,
        112,
        97,
        103,
        104,
        101,
        116,
        116,
        105,
        32,
        109,
        101,
        97,
        116,
        98,
        97,
        108,
        108,
        115,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1};
    Scalar excess;
    excess.SetVch(excess_ser);

    std::vector<unsigned char> vMsg = (excess >> 64).GetVch();
    std::vector<unsigned char> vMsgTrimmed(0);
    bool fFoundNonZero = false;
    for (auto& it : vMsg) {
        if (it != '\0')
            fFoundNonZero = true;
        if (fFoundNonZero)
            vMsgTrimmed.push_back(it);
    }
    vMsg = vMsgTrimmed;
    std::string s(vMsg.begin(), vMsg.end());
    BOOST_CHECK(s == "spaghetti meatballs");
}

BOOST_AUTO_TEST_CASE(test_setvch)
{
    {
        std::vector<uint8_t> vec{
            0,
            1,
            2,
            3,
            4,
            5,
            6,
            7,
            8,
            9,
            10,
            11,
            12,
            13,
            14,
            15,
            16,
            17,
            18,
            19,
            20,
            21,
            22,
            23,
            24,
            25,
            26,
            27,
            28,
            29,
            30,
            31,
        };
        Scalar a;
        a.SetVch(vec);

        Scalar b(vec);
        BOOST_CHECK(a == b);
    }
    {
        // setting curveOrder - 1 should succeed
        CURVE_ORDER_MINUS_1_VEC(vec);
        Scalar a;
        a.SetVch(vec);
        Scalar b(vec);
        BOOST_CHECK(a == b);
    }
    {
        // setting curve order should succeed, but Scalar should get the modulo value
        CURVE_ORDER_VEC(vec);
        Scalar a;
        a.SetVch(vec);
        BOOST_CHECK_EQUAL(a.GetString(), "0");
    }
    {
        std::vector<uint8_t> vec;
        Scalar a(100);
        a.SetVch(vec);
        BOOST_CHECK_EQUAL(a.GetUint64(), 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
