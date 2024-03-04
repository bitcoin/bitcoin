// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>
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
        bool invalid;
        std::string strDec = DecodeBase32(vstrOut[i], &invalid);
        BOOST_CHECK(!invalid);
        BOOST_CHECK_EQUAL(strDec, vstrIn[i]);
    }

    // Decoding strings with embedded NUL characters should fail
    bool failure;
    (void)DecodeBase32("invalid\0"s, &failure); // correct size, invalid due to \0
    BOOST_CHECK(failure);
    (void)DecodeBase32("AWSX3VPP"s, &failure); // valid
    BOOST_CHECK(!failure);
    (void)DecodeBase32("AWSX3VPP\0invalid"s, &failure); // correct size, invalid due to \0
    BOOST_CHECK(failure);
    (void)DecodeBase32("AWSX3VPPinvalid"s, &failure); // invalid size
    BOOST_CHECK(failure);
}

BOOST_AUTO_TEST_SUITE_END()
