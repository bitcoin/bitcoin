// Copyright (c) 2012-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"
#include "test/test_bitcoin.h"

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(getarg_tests, BasicTestingSetup)

static void ResetArgs(const std::string& strArg)
{
    std::vector<std::string> vecArg;
    if (strArg.size())
      boost::split(vecArg, strArg, boost::is_space(), boost::token_compress_on);

    // Insert dummy executable name:
    vecArg.insert(vecArg.begin(), "testbitcoin");

    // Convert to char*:
    std::vector<const char*> vecChar;
    BOOST_FOREACH(std::string& s, vecArg)
        vecChar.push_back(s.c_str());

    ParseParameters(vecChar.size(), &vecChar[0], AllowedArgs::Bitcoind());
}

BOOST_AUTO_TEST_CASE(boolarg)
{
    ResetArgs("-listen");
    BOOST_CHECK(GetBoolArg("-listen", false));
    BOOST_CHECK(GetBoolArg("-listen", true));

    BOOST_CHECK(!GetBoolArg("-fo", false));
    BOOST_CHECK(GetBoolArg("-fo", true));

    BOOST_CHECK(!GetBoolArg("-fooo", false));
    BOOST_CHECK(GetBoolArg("-fooo", true));

    ResetArgs("-listen=0");
    BOOST_CHECK(!GetBoolArg("-listen", false));
    BOOST_CHECK(!GetBoolArg("-listen", true));

    ResetArgs("-listen=1");
    BOOST_CHECK(GetBoolArg("-listen", false));
    BOOST_CHECK(GetBoolArg("-listen", true));

    // New 0.6 feature: auto-map -nosomething to !-something:
    ResetArgs("-nolisten");
    BOOST_CHECK(!GetBoolArg("-listen", false));
    BOOST_CHECK(!GetBoolArg("-listen", true));

    ResetArgs("-nolisten=1");
    BOOST_CHECK(!GetBoolArg("-listen", false));
    BOOST_CHECK(!GetBoolArg("-listen", true));

    ResetArgs("-listen -nolisten");  // -nolisten should win
    BOOST_CHECK(!GetBoolArg("-listen", false));
    BOOST_CHECK(!GetBoolArg("-listen", true));

    ResetArgs("-listen=1 -nolisten=1");  // -nolisten should win
    BOOST_CHECK(!GetBoolArg("-listen", false));
    BOOST_CHECK(!GetBoolArg("-listen", true));

    ResetArgs("-listen=0 -nolisten=0");  // -nolisten=0 should win
    BOOST_CHECK(GetBoolArg("-listen", false));
    BOOST_CHECK(GetBoolArg("-listen", true));

    // New 0.6 feature: treat -- same as -:
    ResetArgs("--listen=1");
    BOOST_CHECK(GetBoolArg("-listen", false));
    BOOST_CHECK(GetBoolArg("-listen", true));

    ResetArgs("--nolisten=1");
    BOOST_CHECK(!GetBoolArg("-listen", false));
    BOOST_CHECK(!GetBoolArg("-listen", true));

}

BOOST_AUTO_TEST_CASE(stringarg)
{
    ResetArgs("");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", ""), "");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", "eleven"), "eleven");

    ResetArgs("-uacomment -listen");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", ""), "");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", "eleven"), "");

    ResetArgs("-uacomment=");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", ""), "");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", "eleven"), "");

    ResetArgs("-uacomment=11");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", ""), "11");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", "eleven"), "11");

    ResetArgs("-uacomment=eleven");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", ""), "eleven");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", "eleven"), "eleven");

}

BOOST_AUTO_TEST_CASE(intarg)
{
    ResetArgs("");
    BOOST_CHECK_EQUAL(GetArg("-maxconnections", 11), 11);
    BOOST_CHECK_EQUAL(GetArg("-maxconnections", 0), 0);

    ResetArgs("-maxconnections -maxreceivebuffer");
    BOOST_CHECK_EQUAL(GetArg("-maxconnections", 11), 0);
    BOOST_CHECK_EQUAL(GetArg("-maxreceivebuffer", 11), 0);

    ResetArgs("-maxconnections=11 -maxreceivebuffer=12");
    BOOST_CHECK_EQUAL(GetArg("-maxconnections", 0), 11);
    BOOST_CHECK_EQUAL(GetArg("-maxreceivebuffer", 11), 12);

    ResetArgs("-maxconnections=NaN -maxreceivebuffer=NotANumber");
    BOOST_CHECK_EQUAL(GetArg("-maxconnections", 1), 0);
    BOOST_CHECK_EQUAL(GetArg("-maxreceivebuffer", 11), 0);
}

BOOST_AUTO_TEST_CASE(doubledash)
{
    ResetArgs("--listen");
    BOOST_CHECK_EQUAL(GetBoolArg("-listen", false), true);

    ResetArgs("--uacomment=verbose --maxconnections=1");
    BOOST_CHECK_EQUAL(GetArg("-uacomment", ""), "verbose");
    BOOST_CHECK_EQUAL(GetArg("-maxconnections", 0), 1);
}

BOOST_AUTO_TEST_CASE(boolargno)
{
    ResetArgs("-nolisten");
    BOOST_CHECK(!GetBoolArg("-listen", true));
    BOOST_CHECK(!GetBoolArg("-listen", false));

    ResetArgs("-nolisten=1");
    BOOST_CHECK(!GetBoolArg("-listen", true));
    BOOST_CHECK(!GetBoolArg("-listen", false));

    ResetArgs("-nolisten=0");
    BOOST_CHECK(GetBoolArg("-listen", true));
    BOOST_CHECK(GetBoolArg("-listen", false));

    ResetArgs("-listen --nolisten"); // --nolisten should win
    BOOST_CHECK(!GetBoolArg("-listen", true));
    BOOST_CHECK(!GetBoolArg("-listen", false));

    ResetArgs("-nolisten -listen"); // -listen always wins:
    BOOST_CHECK(GetBoolArg("-listen", true));
    BOOST_CHECK(GetBoolArg("-listen", false));
}

BOOST_AUTO_TEST_SUITE_END()
