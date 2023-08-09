// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

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
