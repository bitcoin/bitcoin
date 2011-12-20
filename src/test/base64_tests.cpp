#include <boost/test/unit_test.hpp>

#include "../util.h"

BOOST_AUTO_TEST_SUITE(base64_tests)

BOOST_AUTO_TEST_CASE(base64_testvectors)
{
    static const string vstrIn[]  = {"","f","fo","foo","foob","fooba","foobar"};
    static const string vstrOut[] = {"","Zg==","Zm8=","Zm9v","Zm9vYg==","Zm9vYmE=","Zm9vYmFy"};
    for (int i=0; i<sizeof(vstrIn)/sizeof(vstrIn[0]); i++)
    {
        string strEnc = EncodeBase64(vstrIn[i]);
        BOOST_CHECK(strEnc == vstrOut[i]);
        string strDec = DecodeBase64(strEnc);
        BOOST_CHECK(strDec == vstrIn[i]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
