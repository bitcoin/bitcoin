// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/base58_encode_decode.json.h>

#include <base58.h>
#include <test/test_dash.h>
#include <utilstrencodings.h>

#include <univalue.h>

#include <boost/test/unit_test.hpp>


extern UniValue read_json(const std::string& jsondata);

BOOST_FIXTURE_TEST_SUITE(base58_tests, BasicTestingSetup)

// Goal: test low-level base58 encoding functionality
BOOST_AUTO_TEST_CASE(base58_EncodeBase58)
{
    UniValue tests = read_json(std::string(json_tests::base58_encode_decode, json_tests::base58_encode_decode + sizeof(json_tests::base58_encode_decode)));
    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 2) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::vector<unsigned char> sourcedata = ParseHex(test[0].get_str());
        std::string base58string = test[1].get_str();
        BOOST_CHECK_MESSAGE(
                    EncodeBase58(sourcedata.data(), sourcedata.data() + sourcedata.size()) == base58string,
                    strTest);
    }
}

// Goal: test low-level base58 decoding functionality
BOOST_AUTO_TEST_CASE(base58_DecodeBase58)
{
    UniValue tests = read_json(std::string(json_tests::base58_encode_decode, json_tests::base58_encode_decode + sizeof(json_tests::base58_encode_decode)));
    std::vector<unsigned char> result;

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 2) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::vector<unsigned char> expected = ParseHex(test[0].get_str());
        std::string base58string = test[1].get_str();
        BOOST_CHECK_MESSAGE(DecodeBase58(base58string, result), strTest);
        BOOST_CHECK_MESSAGE(result.size() == expected.size() && std::equal(result.begin(), result.end(), expected.begin()), strTest);
    }

    BOOST_CHECK(!DecodeBase58("invalid", result));
    BOOST_CHECK(!DecodeBase58(std::string("invalid"), result));
    BOOST_CHECK(!DecodeBase58(std::string("\0invalid", 8), result));

    BOOST_CHECK(DecodeBase58(std::string("good", 4), result));
    BOOST_CHECK(!DecodeBase58(std::string("bad0IOl", 7), result));
    BOOST_CHECK(!DecodeBase58(std::string("goodbad0IOl", 11), result));
    BOOST_CHECK(!DecodeBase58(std::string("good\0bad0IOl", 12), result));

    // check that DecodeBase58 skips whitespace, but still fails with unexpected non-whitespace at the end.
    BOOST_CHECK(!DecodeBase58(" \t\n\v\f\r skip \r\f\v\n\t a", result));
    BOOST_CHECK( DecodeBase58(" \t\n\v\f\r skip \r\f\v\n\t ", result));
    std::vector<unsigned char> expected = ParseHex("971a55");
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());

    BOOST_CHECK(DecodeBase58Check(std::string("3vQB7B6MrGQZaxCuFg4oh", 21), result));
    BOOST_CHECK(!DecodeBase58Check(std::string("3vQB7B6MrGQZaxCuFg4oi", 21), result));
    BOOST_CHECK(!DecodeBase58Check(std::string("3vQB7B6MrGQZaxCuFg4oh0IOl", 25), result));
    BOOST_CHECK(!DecodeBase58Check(std::string("3vQB7B6MrGQZaxCuFg4oh\00IOl", 26), result));
}

BOOST_AUTO_TEST_SUITE_END()

