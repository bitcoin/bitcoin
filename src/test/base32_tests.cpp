// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <string>

using namespace std::literals;

BOOST_AUTO_TEST_SUITE(base32_tests)

BOOST_AUTO_TEST_CASE(base32_testvectors)
{
    static const std::string vstrIn[]  = {"","f","fo","foo","foob","fooba","foobar"};
    static const std::string vstrOut[] = {"","my======","mzxq====","mzxw6===","mzxw6yq=","mzxw6ytb","mzxw6ytboi======"};
    static const std::string vstrOutNoPadding[] = {"","my","mzxq","mzxw6","mzxw6yq","mzxw6ytb","mzxw6ytboi"};
    for (unsigned int i=0; i<std::size(vstrIn); i++)
    {
        std::string strEnc = EncodeBase32(vstrIn[i]);
        BOOST_CHECK_EQUAL(strEnc, vstrOut[i]);
        strEnc = EncodeBase32(vstrIn[i], false);
        BOOST_CHECK_EQUAL(strEnc, vstrOutNoPadding[i]);
        auto dec = DecodeBase32(vstrOut[i]);
        BOOST_REQUIRE(dec);
        BOOST_CHECK_MESSAGE(std::ranges::equal(*dec, vstrIn[i]), vstrOut[i]);
    }

    BOOST_CHECK(!DecodeBase32("AWSX3VPPinvalid")); // invalid size
    BOOST_CHECK( DecodeBase32("AWSX3VPP")); // valid

    // Decoding strings with embedded NUL characters should fail
    BOOST_CHECK(!DecodeBase32("invalid\0"sv)); // correct size, invalid due to \0
    BOOST_CHECK(!DecodeBase32("AWSX3VPP\0invalid"sv)); // correct size, invalid due to \0
}

BOOST_AUTO_TEST_CASE(base32_padding)
{
    // Is valid without padding
    BOOST_CHECK_EQUAL(EncodeBase32("fooba"), "mzxw6ytb");

    // Valid size
    BOOST_CHECK(!DecodeBase32("========"));
    BOOST_CHECK(!DecodeBase32("a======="));
    BOOST_CHECK( DecodeBase32("aa======"));
    BOOST_CHECK(!DecodeBase32("aaa====="));
    BOOST_CHECK( DecodeBase32("aaaa===="));
    BOOST_CHECK( DecodeBase32("aaaaa==="));
    BOOST_CHECK(!DecodeBase32("aaaaaa=="));
    BOOST_CHECK( DecodeBase32("aaaaaaa="));
}

BOOST_AUTO_TEST_SUITE_END()
