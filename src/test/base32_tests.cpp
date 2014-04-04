// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(base32_tests)

BOOST_AUTO_TEST_CASE(base32_testvectors)
{
    static const std::string vstrIn[]  = {"","f","fo","foo","foob","fooba","foobar"};
    static const std::string vstrOut[] = {"","my======","mzxq====","mzxw6===","mzxw6yq=","mzxw6ytb","mzxw6ytboi======"};
    for (unsigned int i=0; i<sizeof(vstrIn)/sizeof(vstrIn[0]); i++)
    {
        std::string strEnc = EncodeBase32(vstrIn[i]);
        BOOST_CHECK(strEnc == vstrOut[i]);
        std::string strDec = DecodeBase32(vstrOut[i]);
        BOOST_CHECK(strDec == vstrIn[i]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
