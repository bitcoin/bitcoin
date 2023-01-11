// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <blsct/eip_2333/bls12_381_keygen.h>

BOOST_FIXTURE_TEST_SUITE(bls12_381_keygen_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_os2ip)
{
    // Size zero is invalid
    {
        MclScalar s(31);
        BOOST_CHECK_THROW(BLS12_381_KeyGen::I2OSP(s, 0), std::runtime_error);
    }
    // 1 byte cases
    {
        MclScalar s(0);
        auto act = BLS12_381_KeyGen::I2OSP(s, 1);
        std::vector<uint8_t> exp = { 0x00 };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(1);
        auto act = BLS12_381_KeyGen::I2OSP(s, 1);
        std::vector<uint8_t> exp = { 0x01 };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(255);
        auto act = BLS12_381_KeyGen::I2OSP(s, 1);
        std::vector<uint8_t> exp = { 0xff };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(256);
        BOOST_CHECK_THROW(BLS12_381_KeyGen::I2OSP(s, 1), std::runtime_error);
    }
    // 2 byte cases
    {
        MclScalar s(0);
        auto act = BLS12_381_KeyGen::I2OSP(s, 2);
        std::vector<uint8_t> exp = { 0x00, 0x00 };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(1);
        auto act = BLS12_381_KeyGen::I2OSP(s, 2);
        std::vector<uint8_t> exp = { 0x00, 0x01 };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(255);
        auto act = BLS12_381_KeyGen::I2OSP(s, 2);
        std::vector<uint8_t> exp = { 0x00, 0xff };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(256);
        auto act = BLS12_381_KeyGen::I2OSP(s, 2);
        std::vector<uint8_t> exp = { 0x01, 0x00 };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(65535);
        auto act = BLS12_381_KeyGen::I2OSP(s, 2);
        std::vector<uint8_t> exp = { 0xff, 0xff };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(65536);
        BOOST_CHECK_THROW(BLS12_381_KeyGen::I2OSP(s, 2), std::runtime_error);
    }
    // 4 byte cases
    {
        MclScalar s(0);
        auto act = BLS12_381_KeyGen::I2OSP(s, 4);
        std::vector<uint8_t> exp = { 0x00, 0x00, 0x00, 0x00 };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(1);
        auto act = BLS12_381_KeyGen::I2OSP(s, 4);
        std::vector<uint8_t> exp = { 0x00, 0x00, 0x00, 0x01 };
        BOOST_CHECK(act == exp);
    }
    {
        MclScalar s(std::numeric_limits<uint32_t>::max());
        auto act = BLS12_381_KeyGen::I2OSP(s, 4);
        std::vector<uint8_t> exp = { 0xff, 0xff, 0xff, 0xff };
        BOOST_CHECK(act == exp);
    }
    {
        uint64_t uint32_t_max = std::numeric_limits<uint32_t>::max();
        MclScalar s(uint32_t_max + 1);
        BOOST_CHECK_THROW(BLS12_381_KeyGen::I2OSP(s, 4), std::runtime_error);
    }

    // 32 byte cases
    {
        MclScalar s(0);
        auto act = BLS12_381_KeyGen::I2OSP(s, 32);
        std::vector<uint8_t> exp = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };
        BOOST_CHECK(act == exp);
    }
    {
        std::vector<uint8_t> bls12_381_r = {
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
        MclScalar s(bls12_381_r);
        auto act = BLS12_381_KeyGen::I2OSP(s, 32);

        // r is the order and act should become zero
        std::vector<uint8_t> exp = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };
        BOOST_CHECK(act == exp);
    }
}

BOOST_AUTO_TEST_SUITE_END()