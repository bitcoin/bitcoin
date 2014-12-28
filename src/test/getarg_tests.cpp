// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(getarg_tests)

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

    ParseParameters(vecChar.size(), &vecChar[0]);
}

BOOST_AUTO_TEST_CASE(boolarg)
{
    ResetArgs("-foo");
    BOOST_CHECK(GetBoolArg("-foo", false));
    BOOST_CHECK(GetBoolArg("-foo", true));

    BOOST_CHECK(!GetBoolArg("-fo", false));
    BOOST_CHECK(GetBoolArg("-fo", true));

    BOOST_CHECK(!GetBoolArg("-fooo", false));
    BOOST_CHECK(GetBoolArg("-fooo", true));

    ResetArgs("-foo=0");
    BOOST_CHECK(!GetBoolArg("-foo", false));
    BOOST_CHECK(!GetBoolArg("-foo", true));

    ResetArgs("-foo=1");
    BOOST_CHECK(GetBoolArg("-foo", false));
    BOOST_CHECK(GetBoolArg("-foo", true));

    // New 0.6 feature: auto-map -nosomething to !-something:
    ResetArgs("-nofoo");
    BOOST_CHECK(!GetBoolArg("-foo", false));
    BOOST_CHECK(!GetBoolArg("-foo", true));

    ResetArgs("-nofoo=1");
    BOOST_CHECK(!GetBoolArg("-foo", false));
    BOOST_CHECK(!GetBoolArg("-foo", true));

    ResetArgs("-foo -nofoo");  // -foo should win
    BOOST_CHECK(GetBoolArg("-foo", false));
    BOOST_CHECK(GetBoolArg("-foo", true));

    ResetArgs("-foo=1 -nofoo=1");  // -foo should win
    BOOST_CHECK(GetBoolArg("-foo", false));
    BOOST_CHECK(GetBoolArg("-foo", true));

    ResetArgs("-foo=0 -nofoo=0");  // -foo should win
    BOOST_CHECK(!GetBoolArg("-foo", false));
    BOOST_CHECK(!GetBoolArg("-foo", true));

    // New 0.6 feature: treat -- same as -:
    ResetArgs("--foo=1");
    BOOST_CHECK(GetBoolArg("-foo", false));
    BOOST_CHECK(GetBoolArg("-foo", true));

    ResetArgs("--nofoo=1");
    BOOST_CHECK(!GetBoolArg("-foo", false));
    BOOST_CHECK(!GetBoolArg("-foo", true));

}

BOOST_AUTO_TEST_CASE(stringarg)
{
    ResetArgs("");
    BOOST_CHECK_EQUAL(GetArg("-foo", ""), "");
    BOOST_CHECK_EQUAL(GetArg("-foo", "eleven"), "eleven");

    ResetArgs("-foo -bar");
    BOOST_CHECK_EQUAL(GetArg("-foo", ""), "");
    BOOST_CHECK_EQUAL(GetArg("-foo", "eleven"), "");

    ResetArgs("-foo=");
    BOOST_CHECK_EQUAL(GetArg("-foo", ""), "");
    BOOST_CHECK_EQUAL(GetArg("-foo", "eleven"), "");

    ResetArgs("-foo=11");
    BOOST_CHECK_EQUAL(GetArg("-foo", ""), "11");
    BOOST_CHECK_EQUAL(GetArg("-foo", "eleven"), "11");

    ResetArgs("-foo=eleven");
    BOOST_CHECK_EQUAL(GetArg("-foo", ""), "eleven");
    BOOST_CHECK_EQUAL(GetArg("-foo", "eleven"), "eleven");

}

BOOST_AUTO_TEST_CASE(intarg)
{
    ResetArgs("");
    BOOST_CHECK_EQUAL(GetArg("-foo", 11), 11);
    BOOST_CHECK_EQUAL(GetArg("-foo", 0), 0);

    ResetArgs("-foo -bar");
    BOOST_CHECK_EQUAL(GetArg("-foo", 11), 0);
    BOOST_CHECK_EQUAL(GetArg("-bar", 11), 0);

    ResetArgs("-foo=11 -bar=12");
    BOOST_CHECK_EQUAL(GetArg("-foo", 0), 11);
    BOOST_CHECK_EQUAL(GetArg("-bar", 11), 12);

    ResetArgs("-foo=NaN -bar=NotANumber");
    BOOST_CHECK_EQUAL(GetArg("-foo", 1), 0);
    BOOST_CHECK_EQUAL(GetArg("-bar", 11), 0);
}

BOOST_AUTO_TEST_CASE(doubledash)
{
    ResetArgs("--foo");
    BOOST_CHECK_EQUAL(GetBoolArg("-foo", false), true);

    ResetArgs("--foo=verbose --bar=1");
    BOOST_CHECK_EQUAL(GetArg("-foo", ""), "verbose");
    BOOST_CHECK_EQUAL(GetArg("-bar", 0), 1);
}

BOOST_AUTO_TEST_CASE(boolargno)
{
    ResetArgs("-nofoo");
    BOOST_CHECK(!GetBoolArg("-foo", true));
    BOOST_CHECK(!GetBoolArg("-foo", false));

    ResetArgs("-nofoo=1");
    BOOST_CHECK(!GetBoolArg("-foo", true));
    BOOST_CHECK(!GetBoolArg("-foo", false));

    ResetArgs("-nofoo=0");
    BOOST_CHECK(GetBoolArg("-foo", true));
    BOOST_CHECK(GetBoolArg("-foo", false));

    ResetArgs("-foo --nofoo");
    BOOST_CHECK(GetBoolArg("-foo", true));
    BOOST_CHECK(GetBoolArg("-foo", false));

    ResetArgs("-nofoo -foo"); // foo always wins:
    BOOST_CHECK(GetBoolArg("-foo", true));
    BOOST_CHECK(GetBoolArg("-foo", false));
}

BOOST_AUTO_TEST_SUITE_END()
