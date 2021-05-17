// Copyright (c) 2014-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <util/serfloat.h>

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
    BOOST_CHECK_EQUAL(TestDouble(0.0), 0);
    BOOST_CHECK_EQUAL(TestDouble(-0.0), 0x8000000000000000);
    BOOST_CHECK_EQUAL(TestDouble(std::numeric_limits<double>::infinity()), 0x7ff0000000000000);
    BOOST_CHECK_EQUAL(TestDouble(-std::numeric_limits<double>::infinity()), 0xfff0000000000000);

    if (std::numeric_limits<float>::is_iec559) {
        BOOST_CHECK_EQUAL(sizeof(double), 8);
        BOOST_CHECK_EQUAL(sizeof(uint64_t), 8);
        // Test extreme values
        TestDouble(std::numeric_limits<double>::min());
        TestDouble(-std::numeric_limits<double>::min());
        TestDouble(std::numeric_limits<double>::max());
        TestDouble(-std::numeric_limits<double>::max());
        TestDouble(std::numeric_limits<double>::lowest());
        TestDouble(-std::numeric_limits<double>::lowest());
        TestDouble(std::numeric_limits<double>::quiet_NaN());
        TestDouble(-std::numeric_limits<double>::quiet_NaN());
        TestDouble(std::numeric_limits<double>::signaling_NaN());
        TestDouble(-std::numeric_limits<double>::signaling_NaN());
        TestDouble(std::numeric_limits<double>::denorm_min());
        TestDouble(-std::numeric_limits<double>::denorm_min());
        // Test exact encoding: on currently supported platforms, EncodeDouble
        // should produce exactly the same as the in-memory representation for non-NaN.
        for (int j = 0; j < 1000; ++j) {
            // Iterate over 9 specific bits exhaustively; the others are chosen randomly.
            // These specific bits are the sign bit, and the 2 top and bottom bits of
            // exponent and mantissa in the IEEE754 binary64 format.
            for (int x = 0; x < 512; ++x) {
                uint64_t v = InsecureRandBits(64);
                v &= ~(uint64_t{1} << 0);
                if (x & 1) v |= (uint64_t{1} << 0);
                v &= ~(uint64_t{1} << 1);
                if (x & 2) v |= (uint64_t{1} << 1);
                v &= ~(uint64_t{1} << 50);
                if (x & 4) v |= (uint64_t{1} << 50);
                v &= ~(uint64_t{1} << 51);
                if (x & 8) v |= (uint64_t{1} << 51);
                v &= ~(uint64_t{1} << 52);
                if (x & 16) v |= (uint64_t{1} << 52);
                v &= ~(uint64_t{1} << 53);
                if (x & 32) v |= (uint64_t{1} << 53);
                v &= ~(uint64_t{1} << 61);
                if (x & 64) v |= (uint64_t{1} << 61);
                v &= ~(uint64_t{1} << 62);
                if (x & 128) v |= (uint64_t{1} << 62);
                v &= ~(uint64_t{1} << 63);
                if (x & 256) v |= (uint64_t{1} << 63);
                double f;
                memcpy(&f, &v, 8);
                uint64_t v2 = TestDouble(f);
                if (!std::isnan(f)) BOOST_CHECK_EQUAL(v, v2);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
