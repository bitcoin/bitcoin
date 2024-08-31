// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/base58_encode_decode.json.h>

#include <base58.h>
#include <test/util/json.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <util/vector.h>

#include <univalue.h>

#include <boost/test/unit_test.hpp>
#include <string>

using namespace std::literals;
using namespace util::hex_literals;

BOOST_FIXTURE_TEST_SUITE(base58_tests, BasicTestingSetup)

// Goal: test low-level base58 encoding functionality
BOOST_AUTO_TEST_CASE(base58_EncodeBase58)
{
    UniValue tests = read_json(json_tests::base58_encode_decode);
    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 2) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::vector<unsigned char> sourcedata = ParseHex(test[0].get_str());
        std::string base58string = test[1].get_str();
        BOOST_CHECK_MESSAGE(
                    EncodeBase58(sourcedata) == base58string,
                    strTest);
    }
}

// Goal: test low-level base58 decoding functionality
BOOST_AUTO_TEST_CASE(base58_DecodeBase58)
{
    UniValue tests = read_json(json_tests::base58_encode_decode);
    std::vector<unsigned char> result;

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 2) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::vector<unsigned char> expected = ParseHex(test[0].get_str());
        std::string base58string = test[1].get_str();
        BOOST_CHECK_MESSAGE(DecodeBase58(base58string, result, 256), strTest);
        BOOST_CHECK_MESSAGE(result.size() == expected.size() && std::equal(result.begin(), result.end(), expected.begin()), strTest);
    }

    BOOST_CHECK(!DecodeBase58("invalid"s, result, 100));
    BOOST_CHECK(!DecodeBase58("invalid\0"s, result, 100));
    BOOST_CHECK(!DecodeBase58("\0invalid"s, result, 100));

    BOOST_CHECK(DecodeBase58("good"s, result, 100));
    BOOST_CHECK(!DecodeBase58("bad0IOl"s, result, 100));
    BOOST_CHECK(!DecodeBase58("goodbad0IOl"s, result, 100));
    BOOST_CHECK(!DecodeBase58("good\0bad0IOl"s, result, 100));

    // check that DecodeBase58 skips whitespace, but still fails with unexpected non-whitespace at the end.
    BOOST_CHECK(!DecodeBase58(" \t\n\v\f\r skip \r\f\v\n\t a", result, 3));
    BOOST_CHECK( DecodeBase58(" \t\n\v\f\r skip \r\f\v\n\t ", result, 3));
    constexpr auto expected{"971a55"_hex_u8};
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());

    BOOST_CHECK(DecodeBase58Check("3vQB7B6MrGQZaxCuFg4oh"s, result, 100));
    BOOST_CHECK(!DecodeBase58Check("3vQB7B6MrGQZaxCuFg4oi"s, result, 100));
    BOOST_CHECK(!DecodeBase58Check("3vQB7B6MrGQZaxCuFg4oh0IOl"s, result, 100));
    BOOST_CHECK(!DecodeBase58Check("3vQB7B6MrGQZaxCuFg4oh\0" "0IOl"s, result, 100));
}

BOOST_AUTO_TEST_CASE(base58_random_encode_decode)
{
    for (int n = 0; n < 1000; ++n) {
        unsigned int len = 1 + m_rng.randbits(8);
        unsigned int zeroes = m_rng.randbool() ? m_rng.randrange(len + 1) : 0;
        auto data = Cat(std::vector<unsigned char>(zeroes, '\000'), m_rng.randbytes(len - zeroes));
        auto encoded = EncodeBase58Check(data);
        std::vector<unsigned char> decoded;
        auto ok_too_small = DecodeBase58Check(encoded, decoded, m_rng.randrange(len));
        BOOST_CHECK(!ok_too_small);
        auto ok = DecodeBase58Check(encoded, decoded, len + m_rng.randrange(257 - len));
        BOOST_CHECK(ok);
        BOOST_CHECK(data == decoded);
    }
}

BOOST_AUTO_TEST_SUITE_END()
