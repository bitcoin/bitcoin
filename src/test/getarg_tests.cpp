// Copyright (c) 2012-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util.h>
#include <test/test_bitcoin.h>

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
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
    for (std::string& s : vecArg)
        vecChar.push_back(s.c_str());

    gArgs.ParseParameters(vecChar.size(), vecChar.data());
}

BOOST_AUTO_TEST_CASE(configfile)
{
  fs::path expectedConfigPath(fs::system_complete("test/data/bitcoin.conf"));
  ClearDatadirCache();
  ResetArgs(strprintf("-datadir=%s", (fs::path("test") / "data").string()));
  BOOST_CHECK(fs::exists(GetConfigFile("bitcoin.conf")));
  BOOST_CHECK_EQUAL(GetConfigFile("bitcoin.conf"), expectedConfigPath);
  gArgs.ReadConfigFile("bitcoin.conf");

  // boolarg
  BOOST_CHECK(gArgs.GetBoolArg("-foo", false));
  BOOST_CHECK(gArgs.GetBoolArg("-foo", true));

  BOOST_CHECK(!gArgs.GetBoolArg("-bar", false));
  BOOST_CHECK(!gArgs.GetBoolArg("-bar", true));

  // stringarg
  BOOST_CHECK_EQUAL(gArgs.GetArg("-baz", ""), "11");
  BOOST_CHECK_EQUAL(gArgs.GetArg("-baz", "eleven"), "11");

  BOOST_CHECK_EQUAL(gArgs.GetArg("-qux", ""), "eleven");
  BOOST_CHECK_EQUAL(gArgs.GetArg("-qux", "eleven"), "eleven");

  // intarg
  BOOST_CHECK_EQUAL(gArgs.GetArg("-baz", 0), 11);
  BOOST_CHECK_EQUAL(gArgs.GetArg("-qux", 11), 0);

  // boolargno
  // also, prefers the first over later settings
  BOOST_CHECK(!gArgs.GetBoolArg("-quux", true));
  BOOST_CHECK(!gArgs.GetBoolArg("-quux", false));

  // command line settings override bitcoin.conf
  ResetArgs(strprintf("-nofoo -datadir=%s", (fs::path("test") / "data").string()));
  gArgs.ReadConfigFile("bitcoin.conf");

  BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));
  BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));

  // throws for missing datadir
  ResetArgs(strprintf("-datadir=%s", (fs::path("test") / "nonexistent").string()));
  BOOST_CHECK_THROW(gArgs.ReadConfigFile("bitcoin.conf"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(boolarg)
{
    ResetArgs("-foo");
    BOOST_CHECK(gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(gArgs.GetBoolArg("-foo", true));

    BOOST_CHECK(!gArgs.GetBoolArg("-fo", false));
    BOOST_CHECK(gArgs.GetBoolArg("-fo", true));

    BOOST_CHECK(!gArgs.GetBoolArg("-fooo", false));
    BOOST_CHECK(gArgs.GetBoolArg("-fooo", true));

    ResetArgs("-foo=0");
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));

    ResetArgs("-foo=1");
    BOOST_CHECK(gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(gArgs.GetBoolArg("-foo", true));

    // New 0.6 feature: auto-map -nosomething to !-something:
    ResetArgs("-nofoo");
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));

    ResetArgs("-nofoo=1");
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));

    ResetArgs("-foo -nofoo");  // -nofoo should win
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));

    ResetArgs("-foo=1 -nofoo=1");  // -nofoo should win
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));

    ResetArgs("-foo=0 -nofoo=0");  // -nofoo=0 should win
    BOOST_CHECK(gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(gArgs.GetBoolArg("-foo", true));

    // New 0.6 feature: treat -- same as -:
    ResetArgs("--foo=1");
    BOOST_CHECK(gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(gArgs.GetBoolArg("-foo", true));

    ResetArgs("--nofoo=1");
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));

}

BOOST_AUTO_TEST_CASE(stringarg)
{
    ResetArgs("");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", ""), "");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", "eleven"), "eleven");

    ResetArgs("-foo -bar");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", ""), "");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", "eleven"), "");

    ResetArgs("-foo=");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", ""), "");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", "eleven"), "");

    ResetArgs("-foo=11");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", ""), "11");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", "eleven"), "11");

    ResetArgs("-foo=eleven");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", ""), "eleven");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", "eleven"), "eleven");

}

BOOST_AUTO_TEST_CASE(intarg)
{
    ResetArgs("");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", 11), 11);
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", 0), 0);

    ResetArgs("-foo -bar");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", 11), 0);
    BOOST_CHECK_EQUAL(gArgs.GetArg("-bar", 11), 0);

    ResetArgs("-foo=11 -bar=12");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", 0), 11);
    BOOST_CHECK_EQUAL(gArgs.GetArg("-bar", 11), 12);

    ResetArgs("-foo=NaN -bar=NotANumber");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", 1), 0);
    BOOST_CHECK_EQUAL(gArgs.GetArg("-bar", 11), 0);
}

BOOST_AUTO_TEST_CASE(doubledash)
{
    ResetArgs("--foo");
    BOOST_CHECK_EQUAL(gArgs.GetBoolArg("-foo", false), true);

    ResetArgs("--foo=verbose --bar=1");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", ""), "verbose");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-bar", 0), 1);
}

BOOST_AUTO_TEST_CASE(boolargno)
{
    ResetArgs("-nofoo");
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));

    ResetArgs("-nofoo=1");
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));

    ResetArgs("-nofoo=0");
    BOOST_CHECK(gArgs.GetBoolArg("-foo", true));
    BOOST_CHECK(gArgs.GetBoolArg("-foo", false));

    ResetArgs("-foo --nofoo"); // --nofoo should win
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", true));
    BOOST_CHECK(!gArgs.GetBoolArg("-foo", false));

    ResetArgs("-nofoo -foo"); // foo always wins:
    BOOST_CHECK(gArgs.GetBoolArg("-foo", true));
    BOOST_CHECK(gArgs.GetBoolArg("-foo", false));
}

BOOST_AUTO_TEST_SUITE_END()
