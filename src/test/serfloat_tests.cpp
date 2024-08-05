// Copyright (c) 2014-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/serfloat.h>
#include <serialize.h>
#include <streams.h>

#include <boost/test/unit_test.hpp>

#include <cmath>
#include <limits>

BOOST_FIXTURE_TEST_SUITE(serfloat_tests, BasicTestingSetup)

namespace {

uint64_t TestDouble(double f) {
    uint64_t i = EncodeDouble(f);
    double f2 = DecodeDouble(i);
    if (std::isnan(f)) {
        // NaN is not guaranteed to round-trip exactly.
        BOOST_CHECK(std::isnan(f2));
    } else {
        // Everything else is.
        BOOST_CHECK(!std::isnan(f2));
        uint64_t i2 = EncodeDouble(f2);
        BOOST_CHECK_EQUAL(f, f2);
        BOOST_CHECK_EQUAL(i, i2);
    }
    return i;
}

} // namespace

BOOST_AUTO_TEST_CASE(double_serfloat_tests) {
    // Test specific values against their expected encoding.
    BOOST_CHECK_EQUAL(TestDouble(0.0), 0U);
    BOOST_CHECK_EQUAL(TestDouble(-0.0), 0x8000000000000000);
    BOOST_CHECK_EQUAL(TestDouble(std::numeric_limits<double>::infinity()), 0x7ff0000000000000U);
    BOOST_CHECK_EQUAL(TestDouble(-std::numeric_limits<double>::infinity()), 0xfff0000000000000);
    BOOST_CHECK_EQUAL(TestDouble(0.5), 0x3fe0000000000000ULL);
    BOOST_CHECK_EQUAL(TestDouble(1.0), 0x3ff0000000000000ULL);
    BOOST_CHECK_EQUAL(TestDouble(2.0), 0x4000000000000000ULL);
    BOOST_CHECK_EQUAL(TestDouble(4.0), 0x4010000000000000ULL);
    BOOST_CHECK_EQUAL(TestDouble(785.066650390625), 0x4088888880000000ULL);
    BOOST_CHECK_EQUAL(TestDouble(3.7243058682384174), 0x400dcb60e0031440);
    BOOST_CHECK_EQUAL(TestDouble(91.64070592566159), 0x4056e901536d447a);
    BOOST_CHECK_EQUAL(TestDouble(-98.63087668642575), 0xc058a860489c007a);
    BOOST_CHECK_EQUAL(TestDouble(4.908737756962054), 0x4013a28c268b2b70);
    BOOST_CHECK_EQUAL(TestDouble(77.9247330021754), 0x40537b2ed3547804);
    BOOST_CHECK_EQUAL(TestDouble(40.24732825357566), 0x40441fa873c43dfc);
    BOOST_CHECK_EQUAL(TestDouble(71.39395607929222), 0x4051d936938f27b6);
    BOOST_CHECK_EQUAL(TestDouble(58.80100710817612), 0x404d668766a2bd70);
    BOOST_CHECK_EQUAL(TestDouble(-30.10665786964975), 0xc03e1b4dee1e01b8);
    BOOST_CHECK_EQUAL(TestDouble(60.15231509068704), 0x404e137f0f969814);
    BOOST_CHECK_EQUAL(TestDouble(-48.15848711335961), 0xc04814494e445bc6);
    BOOST_CHECK_EQUAL(TestDouble(26.68450101125353), 0x403aaf3b755169b0);
    BOOST_CHECK_EQUAL(TestDouble(-65.72071986604303), 0xc0506e2046378ede);
    BOOST_CHECK_EQUAL(TestDouble(17.95575825512381), 0x4031f4ac92b0a388);
    BOOST_CHECK_EQUAL(TestDouble(-35.27171863226279), 0xc041a2c7ad17a42a);
    BOOST_CHECK_EQUAL(TestDouble(-8.58810329425124), 0xc0212d1bdffef538);
    BOOST_CHECK_EQUAL(TestDouble(88.51393044338977), 0x405620e43c83b1c8);
    BOOST_CHECK_EQUAL(TestDouble(48.07224932612732), 0x4048093f77466ffc);
    BOOST_CHECK_EQUAL(TestDouble(9.867348871395659e+117), 0x586f4daeb2459b9f);
    BOOST_CHECK_EQUAL(TestDouble(-1.5166424385129721e+206), 0xeabe3bbc484bd458);
    BOOST_CHECK_EQUAL(TestDouble(-8.585156555624594e-275), 0x8707c76eee012429);
    BOOST_CHECK_EQUAL(TestDouble(2.2794371091628822e+113), 0x5777b2184458f4ee);
    BOOST_CHECK_EQUAL(TestDouble(-1.1290476594131867e+163), 0xe1c91893d3488bb0);
    BOOST_CHECK_EQUAL(TestDouble(9.143848423979275e-246), 0x0d0ff76e5f2620a3);
    BOOST_CHECK_EQUAL(TestDouble(-2.8366718125941117e+81), 0xd0d7ec7e754b394a);
    BOOST_CHECK_EQUAL(TestDouble(-1.2754409481684012e+229), 0xef80d32f8ec55342);
    BOOST_CHECK_EQUAL(TestDouble(6.000577060053642e-186), 0x197a1be7c8209b6a);
    BOOST_CHECK_EQUAL(TestDouble(2.0839423284378986e-302), 0x014c94f8689cb0a5);
    BOOST_CHECK_EQUAL(TestDouble(-1.422140051483753e+259), 0xf5bd99271d04bb35);
    BOOST_CHECK_EQUAL(TestDouble(-1.0593973991188853e+46), 0xc97db0cdb72d1046);
    BOOST_CHECK_EQUAL(TestDouble(2.62945125875249e+190), 0x67779b36366c993b);
    BOOST_CHECK_EQUAL(TestDouble(-2.920377657275094e+115), 0xd7e7b7b45908e23b);
    BOOST_CHECK_EQUAL(TestDouble(9.790289014855851e-118), 0x27a3c031cc428bcc);
    BOOST_CHECK_EQUAL(TestDouble(-4.629317182034961e-114), 0xa866ccf0b753705a);
    BOOST_CHECK_EQUAL(TestDouble(-1.7674605603846528e+279), 0xf9e8ed383ffc3e25);
    BOOST_CHECK_EQUAL(TestDouble(2.5308171727712605e+120), 0x58ef5cd55f0ec997);
    BOOST_CHECK_EQUAL(TestDouble(-1.05034156412799e+54), 0xcb25eea1b9350fa0);

    // Test extreme values
    BOOST_CHECK_EQUAL(TestDouble(std::numeric_limits<double>::min()), 0x10000000000000);
    BOOST_CHECK_EQUAL(TestDouble(-std::numeric_limits<double>::min()), 0x8010000000000000);
    BOOST_CHECK_EQUAL(TestDouble(std::numeric_limits<double>::max()), 0x7fefffffffffffff);
    BOOST_CHECK_EQUAL(TestDouble(-std::numeric_limits<double>::max()), 0xffefffffffffffff);
    BOOST_CHECK_EQUAL(TestDouble(std::numeric_limits<double>::lowest()), 0xffefffffffffffff);
    BOOST_CHECK_EQUAL(TestDouble(-std::numeric_limits<double>::lowest()), 0x7fefffffffffffff);
    BOOST_CHECK_EQUAL(TestDouble(std::numeric_limits<double>::denorm_min()), 0x1);
    BOOST_CHECK_EQUAL(TestDouble(-std::numeric_limits<double>::denorm_min()), 0x8000000000000001);
    // Note that all NaNs are encoded the same way.
    BOOST_CHECK_EQUAL(TestDouble(std::numeric_limits<double>::quiet_NaN()), 0x7ff8000000000000);
    BOOST_CHECK_EQUAL(TestDouble(-std::numeric_limits<double>::quiet_NaN()), 0x7ff8000000000000);
    BOOST_CHECK_EQUAL(TestDouble(std::numeric_limits<double>::signaling_NaN()), 0x7ff8000000000000);
    BOOST_CHECK_EQUAL(TestDouble(-std::numeric_limits<double>::signaling_NaN()), 0x7ff8000000000000);

    // Construct doubles to test from the encoding.
    static_assert(sizeof(double) == 8);
    static_assert(sizeof(uint64_t) == 8);
    for (int j = 0; j < 1000; ++j) {
        // Iterate over 9 specific bits exhaustively; the others are chosen randomly.
        // These specific bits are the sign bit, and the 2 top and bottom bits of
        // exponent and mantissa in the IEEE754 binary64 format.
        for (int x = 0; x < 512; ++x) {
            uint64_t v = InsecureRandBits(64);
            int x_pos = 0;
            for (int v_pos : {0, 1, 50, 51, 52, 53, 61, 62, 63}) {
                v &= ~(uint64_t{1} << v_pos);
                if ((x >> (x_pos++)) & 1) v |= (uint64_t{1} << v_pos);
            }
            double f;
            memcpy(&f, &v, 8);
            TestDouble(f);
        }
    }
}

/*
Python code to generate the below hashes:

    def reversed_hex(x):
        return bytes(reversed(x)).hex()

    def dsha256(x):
        return hashlib.sha256(hashlib.sha256(x).digest()).digest()

    reversed_hex(dsha256(b''.join(struct.pack('<d', x) for x in range(0,1000)))) == '43d0c82591953c4eafe114590d392676a01585d25b25d433557f0d7878b23f96'
*/
BOOST_AUTO_TEST_CASE(doubles)
{
    DataStream ss{};
    // encode
    for (int i = 0; i < 1000; i++) {
        ss << EncodeDouble(i);
    }
    BOOST_CHECK(Hash(ss) == uint256{"43d0c82591953c4eafe114590d392676a01585d25b25d433557f0d7878b23f96"});

    // decode
    for (int i = 0; i < 1000; i++) {
        uint64_t val;
        ss >> val;
        double j = DecodeDouble(val);
        BOOST_CHECK_MESSAGE(i == j, "decoded:" << j << " expected:" << i);
    }
}

BOOST_AUTO_TEST_SUITE_END()
