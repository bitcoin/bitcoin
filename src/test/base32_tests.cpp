#include <boost/test/unit_test.hpp>

#include "util.h"

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
