// Copyright (c) 2012-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/strencodings.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(base32_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(base32_testvectors)
{
    static const std::string vstrIn[]  = {"","f","fo","foo","foob","fooba","foobar"};
    static const std::string vstrOut[] = {"","my======","mzxq====","mzxw6===","mzxw6yq=","mzxw6ytb","mzxw6ytboi======"};
    for (unsigned int i=0; i<sizeof(vstrIn)/sizeof(vstrIn[0]); i++)
    {
        std::string strEnc = EncodeBase32(vstrIn[i]);
        BOOST_CHECK_EQUAL(strEnc, vstrOut[i]);
        std::string strDec = DecodeBase32(vstrOut[i]);
        BOOST_CHECK_EQUAL(strDec, vstrIn[i]);
    }

    // Decoding strings with embedded NUL characters should fail
    bool failure;
    (void)DecodeBase32(std::string("invalid", 7), &failure);
    BOOST_CHECK_EQUAL(failure, true);
    (void)DecodeBase32(std::string("AWSX3VPP", 8), &failure);
    BOOST_CHECK_EQUAL(failure, false);
    (void)DecodeBase32(std::string("AWSX3VPP\0invalid", 16), &failure);
    BOOST_CHECK_EQUAL(failure, true);
    (void)DecodeBase32(std::string("AWSX3VPPinvalid", 15), &failure);
    BOOST_CHECK_EQUAL(failure, true);
}

BOOST_AUTO_TEST_SUITE_END()
