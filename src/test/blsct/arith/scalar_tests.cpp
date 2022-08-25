// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <blsct/arith/mcl_initializer.h>
#include <blsct/arith/scalar.h>
#include <uint256.h>

#include <limits>

#define SCALAR_CURVE_ORDER_MINUS_1(x) Scalar x("73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000", 16)
#define SCALAR_INT64_MIN(x) Scalar x("52435875175126190479447740508185965837690552500527637822594435327901726408705", 10);

BOOST_FIXTURE_TEST_SUITE(scalar_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_scalar_constructors)
{
    // input vector modulo curve order r should be set to Scalar

    std::vector<uint8_t> one_zeros_be{
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };
    // input of curve order r - 1 should remain the same in Scalar
    std::vector<uint8_t> order_r_minus_1_be{
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
    // input of curve order r should become zero in Scalar
    std::vector<uint8_t> order_r_be{
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
        0x01,
    };

    //// uint256
    // uint256 constructor expects input vector to be big-endian
    {
        uint256 ui(one_zeros_be);
        Scalar a(ui);
        // Scalar::GetString drops preceding 0s
        BOOST_CHECK_EQUAL(a.GetString(), "100000000000000000000000000000000000000000000000000000000000000");
    }
    {
        uint256 ui(order_r_minus_1_be);
        Scalar a(ui);
        BOOST_CHECK_EQUAL(a.GetString(), "73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000");
    }
    {
        uint256 ui(order_r_be);
        Scalar a(order_r_be);
        BOOST_CHECK_EQUAL(a.GetString(), "0");
    }

    //// vector<uint8_t>
    // input vector is expected to be big-endian
    {
        Scalar a(one_zeros_be);
        BOOST_CHECK_EQUAL(a.GetString(), "100000000000000000000000000000000000000000000000000000000000000");
    }
    {
        Scalar a(order_r_minus_1_be);
        BOOST_CHECK_EQUAL(a.GetString(), "73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000");
    }
    {
        Scalar a(order_r_be);
        BOOST_CHECK_EQUAL(a.GetString(), "0");
    }

    //// int64_t
    {
        int64_t ui = 65535;
        Scalar a(ui);
        BOOST_CHECK_EQUAL(a.GetInt64(), ui);
    }
    {
        int64_t ui = std::numeric_limits<int64_t>::max();
        Scalar a(ui);
        BOOST_CHECK_EQUAL(a.GetInt64(), ui);
    }

    // TODO test negative input and possibly make the fixes
}

BOOST_AUTO_TEST_CASE(test_scalar_add)
{
    {
        Scalar a(1);
        Scalar b(2);
        Scalar c(3);
        BOOST_CHECK((a + b) == c);
    }
    {
        SCALAR_CURVE_ORDER_MINUS_1(a);
        Scalar b(1);
        Scalar c(0);
        BOOST_CHECK((a + b) == c);
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_sub)
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
        SCALAR_CURVE_ORDER_MINUS_1(c);
        BOOST_CHECK((a - b) == c);
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_mul)
{
    Scalar a(2);
    Scalar b(3);
    Scalar c(6);
    BOOST_CHECK((a * b) == c);
}

BOOST_AUTO_TEST_CASE(test_scalar_div)
{
    Scalar a(6);
    Scalar b(3);
    Scalar c(2);
    BOOST_CHECK((a / b) == c);
}

BOOST_AUTO_TEST_CASE(test_scalar_bitwise_or)
{
    {
        // there is no bit that has 1 in both a and b
        Scalar   a(0b0001000100010001);
        Scalar   b(0b1100110011001100);
        Scalar exp(0b1101110111011101);
        auto act = a | b;
        BOOST_CHECK(act == exp);
    }
    {
        // there are bits that have 1 in both aband b
        Scalar   a(0b0001000100010001);
        Scalar   b(0b1101110111001101);
        Scalar exp(0b1101110111011101);
        auto act = a | b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is shorter than b. expects big-endian merge
        Scalar           a(0b11111111);
        Scalar   b(0b1000100010001000);
        Scalar exp(0b1000100011111111);
        auto act = a | b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is longer than b. expects big-endian merge
        Scalar   a(0b1000100010001000);
        Scalar           b(0b11111111);
        Scalar exp(0b1000100011111111);
        auto act = a | b;
        BOOST_CHECK(act == exp);
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_bitwise_xor)
{
    {
        // there is no bit that has 1 in both a and b
        Scalar   a(0b0001000100010001);
        Scalar   b(0b1100110011001100);
        Scalar exp(0b1101110111011101);
        auto act = a ^ b;
        BOOST_CHECK(act == exp);
    }
    {
        // there are bits that have 1 in both aband b
        Scalar   a(0b0001000100010001);
        Scalar   b(0b1101110111001101);
        Scalar exp(0b1100110011011100);
        auto act = a ^ b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is shorter than b. expects big-endian merge
        Scalar           a(0b11111111);
        Scalar   b(0b1000100010001000);
        Scalar exp(0b1000100001110111);
        auto act = a ^ b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is longer than b. expects big-endian merge
        Scalar   a(0b1000100010001000);
        Scalar           b(0b11111111);
        Scalar exp(0b1000100001110111);
        auto act = a ^ b;
        BOOST_CHECK(act == exp);
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_bitwise_and)
{
    {
        // there is no bit that has 1 in both a and b
        Scalar   a(0b0001000100010001);
        Scalar   b(0b1100110011001100);
        Scalar                exp(0b0);
        auto act = a & b;
        BOOST_CHECK(act == exp);
    }
    {
        // there are bits that have 1 in both aband b
        Scalar   a(0b0001000100010001);
        Scalar   b(0b1101110111001101);
        Scalar    exp(0b1000100000001);
        auto act = a & b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is shorter than b. expects big-endian merge
        Scalar           a(0b11111111);
        Scalar   b(0b1000100010001000);
        Scalar         exp(0b10001000);
        auto act = a & b;
        BOOST_CHECK(act == exp);
    }
    {
        // a is longer than b. expects big-endian merge
        Scalar   a(0b1000100010001000);
        Scalar           b(0b11111111);
        Scalar         exp(0b10001000);
        auto act = a & b;
        BOOST_CHECK(act == exp);
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_bitwise_compl)
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

BOOST_AUTO_TEST_CASE(test_scalar_shift_left)
{
    Scalar base(0b1);
    int64_t exp = 1;
    for(unsigned int i=0; i<64; ++i) {
        Scalar a = base << i;
        BOOST_CHECK_EQUAL(a.GetInt64(), exp);
        exp <<= 1;
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_assign)
{
    {
        int64_t n = INT64_MAX;
        Scalar a = n;
        BOOST_CHECK_EQUAL(a.GetInt64(), n);
    }
    {
        Scalar a(INT64_MIN);
        SCALAR_INT64_MIN(b);
        BOOST_CHECK_EQUAL(a.GetString(16), b.GetString(16));
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_equal_or_not_equal_to_integer)
{
    Scalar a(6);
    int b = 6;
    int c = 5;
    BOOST_CHECK(a == b);
    BOOST_CHECK(a != c);
}

BOOST_AUTO_TEST_CASE(test_scalar_equal_or_not_equal_to_scalar)
{
    Scalar a(6);
    Scalar b(6);
    Scalar c(5);
    BOOST_CHECK(a == b);
    BOOST_CHECK(a != c);
}

BOOST_AUTO_TEST_CASE(test_scalar_invert)
{
    Scalar a(6);
    Scalar b = a.Invert();
    Scalar c = b.Invert();
    BOOST_CHECK(a == c);
}

BOOST_AUTO_TEST_CASE(test_scalar_invert_zero)
{
    Scalar a(0);
    BOOST_CHECK_THROW(a.Invert(), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_scalar_negate)
{
    Scalar a(6);
    Scalar b(-6);
    Scalar c = a.Negate();
    BOOST_CHECK(b == c);
}

BOOST_AUTO_TEST_CASE(test_scalar_square)
{
    Scalar a(9);
    Scalar b(81);
    Scalar c = a.Square();
    BOOST_CHECK(b == c);
}

BOOST_AUTO_TEST_CASE(test_scalar_cube)
{
    Scalar a(3);
    Scalar b(27);
    Scalar c = a.Cube();
    BOOST_CHECK(b == c);
}

BOOST_AUTO_TEST_CASE(test_scalar_pow)
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
        TestCase{195, 7, 10721172396796875},
    };
    for (auto tc: test_cases) {
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

BOOST_AUTO_TEST_CASE(test_scalar_rand)
{
    std::vector<bool> tf {true, false};
    for (auto exclude_zero : tf) {
        unsigned int num_tries = 1000000;
        unsigned int num_dups = 0;
        auto x = Scalar::Rand();

        for (size_t i = 0; i < num_tries; ++i) {
            auto y = Scalar::Rand(exclude_zero);
            if (exclude_zero && y == 0) BOOST_FAIL("expected non-zero");
            if (x == y) ++num_dups;
        }
        auto dup_ratio = num_dups / (float) num_tries;
        BOOST_CHECK(dup_ratio < 0.000001);
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_getint64)
{
    {
        Scalar a(INT64_MAX);
        int64_t b = a.GetInt64();
        int64_t c = 9223372036854775807;
        BOOST_CHECK_EQUAL(b, c);
    }
    {
        Scalar base(0b1);
        int64_t exp = 1;
        for (uint8_t i=0; i<64; ++i) {
            Scalar a = base << i;
            BOOST_CHECK_EQUAL(a.GetInt64(), exp);
            exp <<= 1;
        }
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_getvch)
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
    BOOST_CHECK(vec == a_vec);
}

BOOST_AUTO_TEST_CASE(test_scalar_setvch)
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
        std::vector<uint8_t> vec{
            115,
            237,
            167,
            83,
            41,
            157,
            125,
            72,
            51,
            57,
            216,
            8,
            9,
            161,
            216,
            5,
            83,
            189,
            164,
            2,
            255,
            254,
            91,
            254,
            255,
            255,
            255,
            255,
            0,
            0,
            0,
            0,
        };
        Scalar a;
        a.SetVch(vec);
        Scalar b(vec);
        BOOST_CHECK(a == b);
    }
    {
        // setting curveOrder should succeed, but Scalar should get the modulo value
        std::vector<uint8_t> vec{
            115,
            237,
            167,
            83,
            41,
            157,
            125,
            72,
            51,
            57,
            216,
            8,
            9,
            161,
            216,
            5,
            83,
            189,
            164,
            2,
            255,
            254,
            91,
            254,
            255,
            255,
            255,
            255,
            0,
            0,
            0,
            1,
        };
        Scalar a;
        BOOST_CHECK_EQUAL(a.GetString(), "0");
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_get_and_setvch)
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

BOOST_AUTO_TEST_CASE(test_scalar_setpow2)
{
    for (size_t i = 0; i < 10; ++i) {
        Scalar a;
        a.SetPow2(i);
        BOOST_CHECK_EQUAL(a.GetInt64(), std::pow(2, i));
    }
}

BOOST_AUTO_TEST_CASE(test_scalar_hash)
{
    // upon generating the digest, following data is added to the hasher:
    // - 1 byte storing the size of the following array
    // - 32-byte big-endian array representing the Scalar value
    // - 4-byte little-endian array representing the parameter of Hash function

    Scalar a(1);
    const int n = 42;
    uint256 digest = a.Hash(n);
    auto act = digest.GetHex();
    std::string exp("cd3f58bb5489460619c322a3b0ec0a21432a22a03ca345e458165b0aaf1202c0");
    BOOST_CHECK(act == exp);
}

// TODO fix this test
BOOST_AUTO_TEST_CASE(test_scalar_getstring)
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

BOOST_AUTO_TEST_CASE(test_scalar_get_bits)
{
    // n is group order r minus 1
    std::vector<uint8_t> n_vec{
        0x73, 0xed, 0xa7, 0x53, 0x29, 0x9d, 0x7d, 0x48, 0x33, 0x39,
        0xd8, 0x08, 0x09, 0xa1, 0xd8, 0x05, 0x53, 0xbd, 0xa4, 0x02,
        0xff, 0xfe, 0x5b, 0xfe, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00,
    };
    std::string n_hex("73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000");
    std::string n_bin("111001111101101101001110101001100101001100111010111110101001000001100110011100111011000000010000000100110100001110110000000010101010011101111011010010000000010111111111111111001011011111111101111111111111111111111111111111100000000000000000000000000000000");

    auto u = uint256(n_vec);
    Scalar s(u);

    std::string exp = n_bin;
    auto bs = s.GetBits();

    BOOST_CHECK(bs.size() == exp.size());
    for (size_t i = 0; i < bs.size(); ++i) {
        auto expC = exp[i];
        auto actC = bs[i] == true ? '1' : '0';
        BOOST_CHECK(expC == actC);
    }
}


BOOST_AUTO_TEST_CASE(test_scalar_get_bit)
{
    {
        Scalar a(0b100000001);
        BOOST_CHECK_EQUAL(a.GetBit(0), true); // 1st byte
        BOOST_CHECK_EQUAL(a.GetBit(1), false);
        BOOST_CHECK_EQUAL(a.GetBit(2), false);
        BOOST_CHECK_EQUAL(a.GetBit(3), false);
        BOOST_CHECK_EQUAL(a.GetBit(4), false);
        BOOST_CHECK_EQUAL(a.GetBit(5), false);
        BOOST_CHECK_EQUAL(a.GetBit(6), false);
        BOOST_CHECK_EQUAL(a.GetBit(7), false);
        BOOST_CHECK_EQUAL(a.GetBit(8), true); // 2nd byte
        BOOST_CHECK_EQUAL(a.GetBit(9), false);
    }
    {
        SCALAR_CURVE_ORDER_MINUS_1(a);
        auto v = a.GetVch();

        // 5th byte from the last is 255
        for (size_t i=0; i<8; ++i) {
            BOOST_CHECK_EQUAL(a.GetBit(4 * 8 + i), true);
        }

        // 9th byte from the last is 254
        for (size_t i=0; i<8; ++i) {
            BOOST_CHECK_EQUAL(a.GetBit(8 * 8 + i), i != 0);
        }

        // 13th byte from the last is 2
        for (size_t i=0; i<8; ++i) {
            BOOST_CHECK_EQUAL(a.GetBit(12 * 8 + i), i == 1);
        }

        // 32nd byte from the last is 115
        // 115 = 0b01110011 and rightmost bit is index 0
        std::vector<bool> bits115 = {true, true, false, false, true, true, true, false};
        for (size_t i = 0; i < 8; ++i) {
            BOOST_CHECK_EQUAL(a.GetBit(31 * 8 + i), bits115[i]);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
