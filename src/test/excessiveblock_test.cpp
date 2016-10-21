#include "unlimited.h"

#include "test/test_bitcoin.h"

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

// Defined in rpc_tests.cpp not bitcoin-cli.cpp
extern UniValue CallRPC(string strMethod);

BOOST_FIXTURE_TEST_SUITE(excessiveblock_test, TestingSetup)

BOOST_AUTO_TEST_CASE(rpc_excessive)
{
    BOOST_CHECK_NO_THROW(CallRPC("getexcessiveblock"));

    BOOST_CHECK_NO_THROW(CallRPC("getminingmaxblock"));

    BOOST_CHECK_THROW(CallRPC("setexcessiveblock not_uint"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("setexcessiveblock 1000000 not_uint"), boost::bad_lexical_cast);
    BOOST_CHECK_THROW(CallRPC("setexcessiveblock 1000000 -1"), boost::bad_lexical_cast);
    BOOST_CHECK_THROW(CallRPC("setexcessiveblock -1 0"), boost::bad_lexical_cast);

    BOOST_CHECK_THROW(CallRPC("setexcessiveblock 1000 1"), runtime_error);
    BOOST_CHECK_NO_THROW(CallRPC("setminingmaxblock 1000"));
    BOOST_CHECK_NO_THROW(CallRPC("setexcessiveblock 1000 1"));

    BOOST_CHECK_THROW(CallRPC("setexcessiveblock 1000 0 0"), runtime_error);

    BOOST_CHECK_THROW(CallRPC("setminingmaxblock"), runtime_error);
    BOOST_CHECK_NO_THROW(CallRPC("setminingmaxblock 100000"));
    BOOST_CHECK_THROW(CallRPC("setminingmaxblock not_uint"),  boost::bad_lexical_cast);
    BOOST_CHECK_THROW(CallRPC("setminingmaxblock -1"),  boost::bad_lexical_cast);
    BOOST_CHECK_THROW(CallRPC("setminingmaxblock 0"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("setminingmaxblock 0 0"), runtime_error);
}

BOOST_AUTO_TEST_CASE(buip005)
{
    string exceptedEB;
    string exceptedAD;
    excessiveBlockSize = 1000000;
    excessiveAcceptDepth = 9999999;
    exceptedEB = "EB1";
    exceptedAD = "AD9999999";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    BOOST_CHECK_MESSAGE(BUComments.back() == exceptedAD,
                        "AD ought to have been " << exceptedAD << " when excessiveBlockSize = " << excessiveAcceptDepth);
    excessiveBlockSize = 100000;
    excessiveAcceptDepth = 9999999 + 1;
    exceptedEB = "EB0.1";
    exceptedAD = "AD9999999";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    BOOST_CHECK_MESSAGE(BUComments.back() == exceptedAD,
                        "AD ought to have been " << exceptedAD << " when excessiveBlockSize = " << excessiveAcceptDepth);
    excessiveBlockSize = 10000;
    exceptedEB = "EB0";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    excessiveBlockSize = 150000;
    exceptedEB = "EB0.1";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been rounded to " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    excessiveBlockSize = 150000;
    exceptedEB = "EB0.1";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been rounded to " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
}

BOOST_AUTO_TEST_SUITE_END()
