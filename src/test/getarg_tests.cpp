// Copyright (c) 2012-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/strencodings.h>
#include <util/system.h>
#include <test/setup_common.h>

#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(getarg_tests, BasicTestingSetup)

static void ResetArgs(const std::string& strArg)
{
    std::vector<std::string> vecArg;
    if (strArg.size())
      boost::split(vecArg, strArg, IsSpace, boost::token_compress_on);

    // Insert dummy executable name:
    vecArg.insert(vecArg.begin(), "testbitcoin");

    // Convert to char*:
    std::vector<const char*> vecChar;
    for (const std::string& s : vecArg)
        vecChar.push_back(s.c_str());

    std::string error;
    BOOST_CHECK(gArgs.ParseParameters(vecChar.size(), vecChar.data(), error));
}

static void SetupArgs(const std::vector<std::pair<std::string, unsigned int>>& args)
{
    gArgs.ClearArgs();
    for (const auto& arg : args) {
        gArgs.AddArg(arg.first, "", arg.second, OptionsCategory::OPTIONS);
    }
}

BOOST_AUTO_TEST_CASE(boolarg)
{
    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_BOOL);
    SetupArgs({foo});
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
    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_STRING);
    const auto bar = std::make_pair("-bar", ArgsManager::ALLOW_STRING);
    SetupArgs({foo, bar});
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
    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_INT);
    const auto bar = std::make_pair("-bar", ArgsManager::ALLOW_INT);
    SetupArgs({foo, bar});
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
    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_ANY);
    const auto bar = std::make_pair("-bar", ArgsManager::ALLOW_ANY);
    SetupArgs({foo, bar});
    ResetArgs("--foo");
    BOOST_CHECK_EQUAL(gArgs.GetBoolArg("-foo", false), true);

    ResetArgs("--foo=verbose --bar=1");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-foo", ""), "verbose");
    BOOST_CHECK_EQUAL(gArgs.GetArg("-bar", 0), 1);
}

BOOST_AUTO_TEST_CASE(boolargno)
{
    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_BOOL);
    const auto bar = std::make_pair("-bar", ArgsManager::ALLOW_BOOL);
    SetupArgs({foo, bar});
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
