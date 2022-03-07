// Copyright (c) 2012-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <util/system.h>

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(getarg_tests, BasicTestingSetup)

void ResetArgs(ArgsManager& local_args, const std::string& strArg)
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
    BOOST_CHECK(local_args.ParseParameters(vecChar.size(), vecChar.data(), error));
}

void SetupArgs(ArgsManager& local_args, const std::vector<std::pair<std::string, unsigned int>>& args)
{
    for (const auto& arg : args) {
        local_args.AddArg(arg.first, "", arg.second, OptionsCategory::OPTIONS);
    }
}

BOOST_AUTO_TEST_CASE(boolarg)
{
    ArgsManager local_args;

    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_ANY);
    SetupArgs(local_args, {foo});
    ResetArgs(local_args, "-foo");
    BOOST_CHECK(local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(local_args.GetBoolArg("-foo", true));

    BOOST_CHECK(!local_args.GetBoolArg("-fo", false));
    BOOST_CHECK(local_args.GetBoolArg("-fo", true));

    BOOST_CHECK(!local_args.GetBoolArg("-fooo", false));
    BOOST_CHECK(local_args.GetBoolArg("-fooo", true));

    ResetArgs(local_args, "-foo=0");
    BOOST_CHECK(!local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(!local_args.GetBoolArg("-foo", true));

    ResetArgs(local_args, "-foo=1");
    BOOST_CHECK(local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(local_args.GetBoolArg("-foo", true));

    // New 0.6 feature: auto-map -nosomething to !-something:
    ResetArgs(local_args, "-nofoo");
    BOOST_CHECK(!local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(!local_args.GetBoolArg("-foo", true));

    ResetArgs(local_args, "-nofoo=1");
    BOOST_CHECK(!local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(!local_args.GetBoolArg("-foo", true));

    ResetArgs(local_args, "-foo -nofoo"); // -nofoo should win
    BOOST_CHECK(!local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(!local_args.GetBoolArg("-foo", true));

    ResetArgs(local_args, "-foo=1 -nofoo=1"); // -nofoo should win
    BOOST_CHECK(!local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(!local_args.GetBoolArg("-foo", true));

    ResetArgs(local_args, "-foo=0 -nofoo=0"); // -nofoo=0 should win
    BOOST_CHECK(local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(local_args.GetBoolArg("-foo", true));

    // New 0.6 feature: treat -- same as -:
    ResetArgs(local_args, "--foo=1");
    BOOST_CHECK(local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(local_args.GetBoolArg("-foo", true));

    ResetArgs(local_args, "--nofoo=1");
    BOOST_CHECK(!local_args.GetBoolArg("-foo", false));
    BOOST_CHECK(!local_args.GetBoolArg("-foo", true));
}

BOOST_AUTO_TEST_CASE(stringarg)
{
    ArgsManager local_args;

    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_ANY);
    const auto bar = std::make_pair("-bar", ArgsManager::ALLOW_ANY);
    SetupArgs(local_args, {foo, bar});
    ResetArgs(local_args, "");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", ""), "");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", "eleven"), "eleven");

    ResetArgs(local_args, "-foo -bar");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", ""), "");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", "eleven"), "");

    ResetArgs(local_args, "-foo=");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", ""), "");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", "eleven"), "");

    ResetArgs(local_args, "-foo=11");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", ""), "11");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", "eleven"), "11");

    ResetArgs(local_args, "-foo=eleven");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", ""), "eleven");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", "eleven"), "eleven");
}

BOOST_AUTO_TEST_CASE(intarg)
{
    ArgsManager local_args;

    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_ANY);
    const auto bar = std::make_pair("-bar", ArgsManager::ALLOW_ANY);
    SetupArgs(local_args, {foo, bar});
    ResetArgs(local_args, "");
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-foo", 11), 11);
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-foo", 0), 0);

    ResetArgs(local_args, "-foo -bar");
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-foo", 11), 0);
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-bar", 11), 0);

    // Check under-/overflow behavior.
    ResetArgs(local_args, "-foo=-9223372036854775809 -bar=9223372036854775808");
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-foo", 0), std::numeric_limits<int64_t>::min());
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-bar", 0), std::numeric_limits<int64_t>::max());

    ResetArgs(local_args, "-foo=11 -bar=12");
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-foo", 0), 11);
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-bar", 11), 12);

    ResetArgs(local_args, "-foo=NaN -bar=NotANumber");
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-foo", 1), 0);
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-bar", 11), 0);
}

BOOST_AUTO_TEST_CASE(patharg)
{
    ArgsManager local_args;

    const auto dir = std::make_pair("-dir", ArgsManager::ALLOW_ANY);
    SetupArgs(local_args, {dir});
    ResetArgs(local_args, "");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), fs::path{});

    const fs::path root_path{"/"};
    ResetArgs(local_args, "-dir=/");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), root_path);

    ResetArgs(local_args, "-dir=/.");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), root_path);

    ResetArgs(local_args, "-dir=/./");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), root_path);

    ResetArgs(local_args, "-dir=/.//");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), root_path);

#ifdef WIN32
    const fs::path win_root_path{"C:\\"};
    ResetArgs(local_args, "-dir=C:\\");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), win_root_path);

    ResetArgs(local_args, "-dir=C:/");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), win_root_path);

    ResetArgs(local_args, "-dir=C:\\\\");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), win_root_path);

    ResetArgs(local_args, "-dir=C:\\.");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), win_root_path);

    ResetArgs(local_args, "-dir=C:\\.\\");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), win_root_path);

    ResetArgs(local_args, "-dir=C:\\.\\\\");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), win_root_path);
#endif

    const fs::path absolute_path{"/home/user/.bitcoin"};
    ResetArgs(local_args, "-dir=/home/user/.bitcoin");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), absolute_path);

    ResetArgs(local_args, "-dir=/root/../home/user/.bitcoin");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), absolute_path);

    ResetArgs(local_args, "-dir=/home/./user/.bitcoin");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), absolute_path);

    ResetArgs(local_args, "-dir=/home/user/.bitcoin/");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), absolute_path);

    ResetArgs(local_args, "-dir=/home/user/.bitcoin//");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), absolute_path);

    ResetArgs(local_args, "-dir=/home/user/.bitcoin/.");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), absolute_path);

    ResetArgs(local_args, "-dir=/home/user/.bitcoin/./");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), absolute_path);

    ResetArgs(local_args, "-dir=/home/user/.bitcoin/.//");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), absolute_path);

    const fs::path relative_path{"user/.bitcoin"};
    ResetArgs(local_args, "-dir=user/.bitcoin");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), relative_path);

    ResetArgs(local_args, "-dir=somewhere/../user/.bitcoin");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), relative_path);

    ResetArgs(local_args, "-dir=user/./.bitcoin");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), relative_path);

    ResetArgs(local_args, "-dir=user/.bitcoin/");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), relative_path);

    ResetArgs(local_args, "-dir=user/.bitcoin//");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), relative_path);

    ResetArgs(local_args, "-dir=user/.bitcoin/.");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), relative_path);

    ResetArgs(local_args, "-dir=user/.bitcoin/./");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), relative_path);

    ResetArgs(local_args, "-dir=user/.bitcoin/.//");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir"), relative_path);

    // Check negated and default argument handling. Specifying an empty argument
    // is the same as not specifying the argument. This is convenient for
    // scripting so later command line arguments can override earlier command
    // line arguments or bitcoin.conf values. Currently the -dir= case cannot be
    // distinguished from -dir case with no assignment, but #16545 would add the
    // ability to distinguish these in the future (and treat the no-assign case
    // like an imperative command or an error).
    ResetArgs(local_args, "");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir", "default"), fs::path{"default"});
    ResetArgs(local_args, "-dir=override");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir", "default"), fs::path{"override"});
    ResetArgs(local_args, "-dir=");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir", "default"), fs::path{"default"});
    ResetArgs(local_args, "-dir");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir", "default"), fs::path{"default"});
    ResetArgs(local_args, "-nodir");
    BOOST_CHECK_EQUAL(local_args.GetPathArg("-dir", "default"), fs::path{""});
}

BOOST_AUTO_TEST_CASE(doubledash)
{
    ArgsManager local_args;

    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_ANY);
    const auto bar = std::make_pair("-bar", ArgsManager::ALLOW_ANY);
    SetupArgs(local_args, {foo, bar});
    ResetArgs(local_args, "--foo");
    BOOST_CHECK_EQUAL(local_args.GetBoolArg("-foo", false), true);

    ResetArgs(local_args, "--foo=verbose --bar=1");
    BOOST_CHECK_EQUAL(local_args.GetArg("-foo", ""), "verbose");
    BOOST_CHECK_EQUAL(local_args.GetIntArg("-bar", 0), 1);
}

BOOST_AUTO_TEST_CASE(boolargno)
{
    ArgsManager local_args;

    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_ANY);
    const auto bar = std::make_pair("-bar", ArgsManager::ALLOW_ANY);
    SetupArgs(local_args, {foo, bar});
    ResetArgs(local_args, "-nofoo");
    BOOST_CHECK(!local_args.GetBoolArg("-foo", true));
    BOOST_CHECK(!local_args.GetBoolArg("-foo", false));

    ResetArgs(local_args, "-nofoo=1");
    BOOST_CHECK(!local_args.GetBoolArg("-foo", true));
    BOOST_CHECK(!local_args.GetBoolArg("-foo", false));

    ResetArgs(local_args, "-nofoo=0");
    BOOST_CHECK(local_args.GetBoolArg("-foo", true));
    BOOST_CHECK(local_args.GetBoolArg("-foo", false));

    ResetArgs(local_args, "-foo --nofoo"); // --nofoo should win
    BOOST_CHECK(!local_args.GetBoolArg("-foo", true));
    BOOST_CHECK(!local_args.GetBoolArg("-foo", false));

    ResetArgs(local_args, "-nofoo -foo"); // foo always wins:
    BOOST_CHECK(local_args.GetBoolArg("-foo", true));
    BOOST_CHECK(local_args.GetBoolArg("-foo", false));
}

BOOST_AUTO_TEST_CASE(logargs)
{
    ArgsManager local_args;

    const auto okaylog_bool = std::make_pair("-okaylog-bool", ArgsManager::ALLOW_ANY);
    const auto okaylog_negbool = std::make_pair("-okaylog-negbool", ArgsManager::ALLOW_ANY);
    const auto okaylog = std::make_pair("-okaylog", ArgsManager::ALLOW_ANY);
    const auto dontlog = std::make_pair("-dontlog", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE);
    SetupArgs(local_args, {okaylog_bool, okaylog_negbool, okaylog, dontlog});
    ResetArgs(local_args, "-okaylog-bool -nookaylog-negbool -okaylog=public -dontlog=private");

    // Everything logged to debug.log will also append to str
    std::string str;
    auto print_connection = LogInstance().PushBackCallback(
        [&str](const std::string& s) {
            str += s;
        });

    // Log the arguments
    local_args.LogArgs();

    LogInstance().DeleteCallback(print_connection);
    // Check that what should appear does, and what shouldn't doesn't.
    BOOST_CHECK(str.find("Command-line arg: okaylog-bool=\"\"") != std::string::npos);
    BOOST_CHECK(str.find("Command-line arg: okaylog-negbool=false") != std::string::npos);
    BOOST_CHECK(str.find("Command-line arg: okaylog=\"public\"") != std::string::npos);
    BOOST_CHECK(str.find("dontlog=****") != std::string::npos);
    BOOST_CHECK(str.find("private") == std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
