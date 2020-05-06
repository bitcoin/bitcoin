// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(base64_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(base64_testvectors)
{
    static const std::string vstrIn[]  = {"","f","fo","foo","foob","fooba","foobar"};
    static const std::string vstrOut[] = {"","Zg==","Zm8=","Zm9v","Zm9vYg==","Zm9vYmE=","Zm9vYmFy"};
    for (unsigned int i=0; i<sizeof(vstrIn)/sizeof(vstrIn[0]); i++)
    {
        std::string strEnc = EncodeBase64(vstrIn[i]);
        BOOST_CHECK_EQUAL(strEnc, vstrOut[i]);
        std::string strDec = DecodeBase64(strEnc);
        BOOST_CHECK_EQUAL(strDec, vstrIn[i]);
    }

    // Decoding strings with embedded NUL characters should fail
    bool failure;
    (void)DecodeBase64(std::string("invalid", 7), &failure);
    BOOST_CHECK_EQUAL(failure, true);
    (void)DecodeBase64(std::string("nQB/pZw=", 8), &failure);
    BOOST_CHECK_EQUAL(failure, false);
    (void)DecodeBase64(std::string("nQB/pZw=\0invalid", 16), &failure);
    BOOST_CHECK_EQUAL(failure, true);
    (void)DecodeBase64(std::string("nQB/pZw=invalid", 15), &failure);
    BOOST_CHECK_EQUAL(failure, true);
}

BOOST_AUTO_TEST_SUITE_END()
