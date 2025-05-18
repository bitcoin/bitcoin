// Copyright (c) 2011-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <sync.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <test/util/str.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/fs.h>
#include <util/strencodings.h>

#include <array>
#include <optional>
#include <cstdint>
#include <cstring>
#include <vector>

#include <boost/test/unit_test.hpp>

using util::ToString;

BOOST_FIXTURE_TEST_SUITE(argsman_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(util_datadir)
{
    // Use local args variable instead of m_args to avoid making assumptions about test setup
    ArgsManager args;
    args.ForceSetArg("-datadir", fs::PathToString(m_path_root));

    const fs::path dd_norm = args.GetDataDirBase();

    args.ForceSetArg("-datadir", fs::PathToString(dd_norm) + "/");
    args.ClearPathCache();
    BOOST_CHECK_EQUAL(dd_norm, args.GetDataDirBase());

    args.ForceSetArg("-datadir", fs::PathToString(dd_norm) + "/.");
    args.ClearPathCache();
    BOOST_CHECK_EQUAL(dd_norm, args.GetDataDirBase());

    args.ForceSetArg("-datadir", fs::PathToString(dd_norm) + "/./");
    args.ClearPathCache();
    BOOST_CHECK_EQUAL(dd_norm, args.GetDataDirBase());

    args.ForceSetArg("-datadir", fs::PathToString(dd_norm) + "/.//");
    args.ClearPathCache();
    BOOST_CHECK_EQUAL(dd_norm, args.GetDataDirBase());
}

struct TestArgsManager : public ArgsManager
{
    TestArgsManager() { m_network_only_args.clear(); }
    void ReadConfigString(const std::string str_config)
    {
        std::istringstream streamConfig(str_config);
        {
            LOCK(cs_args);
            m_settings.ro_config.clear();
            m_config_sections.clear();
        }
        std::string error;
        BOOST_REQUIRE(ReadConfigStream(streamConfig, "", error));
    }
    void SetNetworkOnlyArg(const std::string arg)
    {
        LOCK(cs_args);
        m_network_only_args.insert(arg);
    }
    void SetupArgs(const std::vector<std::pair<std::string, unsigned int>>& args)
    {
        for (const auto& arg : args) {
            AddArg(arg.first, "", arg.second, OptionsCategory::OPTIONS);
        }
    }
    using ArgsManager::GetSetting;
    using ArgsManager::GetSettingsList;
    using ArgsManager::ReadConfigStream;
    using ArgsManager::cs_args;
    using ArgsManager::m_network;
    using ArgsManager::m_settings;
};

//! Test GetSetting and GetArg type coercion, negation, and default value handling.
class CheckValueTest : public TestChain100Setup
{
public:
    struct Expect {
        common::SettingsValue setting;
        bool default_string = false;
        bool default_int = false;
        bool default_bool = false;
        const char* string_value = nullptr;
        std::optional<int64_t> int_value;
        std::optional<bool> bool_value;
        std::optional<std::vector<std::string>> list_value;
        const char* error = nullptr;

        explicit Expect(common::SettingsValue s) : setting(std::move(s)) {}
        Expect& DefaultString() { default_string = true; return *this; }
        Expect& DefaultInt() { default_int = true; return *this; }
        Expect& DefaultBool() { default_bool = true; return *this; }
        Expect& String(const char* s) { string_value = s; return *this; }
        Expect& Int(int64_t i) { int_value = i; return *this; }
        Expect& Bool(bool b) { bool_value = b; return *this; }
        Expect& List(std::vector<std::string> m) { list_value = std::move(m); return *this; }
        Expect& Error(const char* e) { error = e; return *this; }
    };

    void CheckValue(unsigned int flags, const char* arg, const Expect& expect)
    {
        TestArgsManager test;
        test.SetupArgs({{"-value", flags}});
        const char* argv[] = {"ignored", arg};
        std::string error;
        bool success = test.ParseParameters(arg ? 2 : 1, argv, error);

        BOOST_CHECK_EQUAL(test.GetSetting("-value").write(), expect.setting.write());
        auto settings_list = test.GetSettingsList("-value");
        if (expect.setting.isNull() || expect.setting.isFalse()) {
            BOOST_CHECK_EQUAL(settings_list.size(), 0U);
        } else {
            BOOST_CHECK_EQUAL(settings_list.size(), 1U);
            BOOST_CHECK_EQUAL(settings_list[0].write(), expect.setting.write());
        }

        if (expect.error) {
            BOOST_CHECK(!success);
            BOOST_CHECK_NE(error.find(expect.error), std::string::npos);
        } else {
            BOOST_CHECK(success);
            BOOST_CHECK_EQUAL(error, "");
        }

        if (expect.default_string) {
            BOOST_CHECK_EQUAL(test.GetArg("-value", "zzzzz"), "zzzzz");
        } else if (expect.string_value) {
            BOOST_CHECK_EQUAL(test.GetArg("-value", "zzzzz"), expect.string_value);
        } else {
            BOOST_CHECK(!success);
        }

        if (expect.default_int) {
            BOOST_CHECK_EQUAL(test.GetIntArg("-value", 99999), 99999);
        } else if (expect.int_value) {
            BOOST_CHECK_EQUAL(test.GetIntArg("-value", 99999), *expect.int_value);
        } else {
            BOOST_CHECK(!success);
        }

        if (expect.default_bool) {
            BOOST_CHECK_EQUAL(test.GetBoolArg("-value", false), false);
            BOOST_CHECK_EQUAL(test.GetBoolArg("-value", true), true);
        } else if (expect.bool_value) {
            BOOST_CHECK_EQUAL(test.GetBoolArg("-value", false), *expect.bool_value);
            BOOST_CHECK_EQUAL(test.GetBoolArg("-value", true), *expect.bool_value);
        } else {
            BOOST_CHECK(!success);
        }

        if (expect.list_value) {
            auto l = test.GetArgs("-value");
            BOOST_CHECK_EQUAL_COLLECTIONS(l.begin(), l.end(), expect.list_value->begin(), expect.list_value->end());
        } else {
            BOOST_CHECK(!success);
        }
    }
};

BOOST_FIXTURE_TEST_CASE(util_CheckValue, CheckValueTest)
{
    using M = ArgsManager;

    CheckValue(M::ALLOW_ANY, nullptr, Expect{{}}.DefaultString().DefaultInt().DefaultBool().List({}));
    CheckValue(M::ALLOW_ANY, "-novalue", Expect{false}.String("0").Int(0).Bool(false).List({}));
    CheckValue(M::ALLOW_ANY, "-novalue=", Expect{false}.String("0").Int(0).Bool(false).List({}));
    CheckValue(M::ALLOW_ANY, "-novalue=0", Expect{true}.String("1").Int(1).Bool(true).List({"1"}));
    CheckValue(M::ALLOW_ANY, "-novalue=1", Expect{false}.String("0").Int(0).Bool(false).List({}));
    CheckValue(M::ALLOW_ANY, "-novalue=2", Expect{false}.String("0").Int(0).Bool(false).List({}));
    CheckValue(M::ALLOW_ANY, "-novalue=abc", Expect{true}.String("1").Int(1).Bool(true).List({"1"}));
    CheckValue(M::ALLOW_ANY, "-value", Expect{""}.String("").Int(0).Bool(true).List({""}));
    CheckValue(M::ALLOW_ANY, "-value=", Expect{""}.String("").Int(0).Bool(true).List({""}));
    CheckValue(M::ALLOW_ANY, "-value=0", Expect{"0"}.String("0").Int(0).Bool(false).List({"0"}));
    CheckValue(M::ALLOW_ANY, "-value=1", Expect{"1"}.String("1").Int(1).Bool(true).List({"1"}));
    CheckValue(M::ALLOW_ANY, "-value=2", Expect{"2"}.String("2").Int(2).Bool(true).List({"2"}));
    CheckValue(M::ALLOW_ANY, "-value=abc", Expect{"abc"}.String("abc").Int(0).Bool(false).List({"abc"}));
}

struct NoIncludeConfTest {
    std::string Parse(const char* arg)
    {
        TestArgsManager test;
        test.SetupArgs({{"-includeconf", ArgsManager::ALLOW_ANY}});
        std::array argv{"ignored", arg};
        std::string error;
        (void)test.ParseParameters(argv.size(), argv.data(), error);
        return error;
    }
};

BOOST_FIXTURE_TEST_CASE(util_NoIncludeConf, NoIncludeConfTest)
{
    BOOST_CHECK_EQUAL(Parse("-noincludeconf"), "");
    BOOST_CHECK_EQUAL(Parse("-includeconf"), "-includeconf cannot be used from commandline; -includeconf=\"\"");
    BOOST_CHECK_EQUAL(Parse("-includeconf=file"), "-includeconf cannot be used from commandline; -includeconf=\"file\"");
}

BOOST_AUTO_TEST_CASE(util_ParseParameters)
{
    TestArgsManager testArgs;
    const auto a = std::make_pair("-a", ArgsManager::ALLOW_ANY);
    const auto b = std::make_pair("-b", ArgsManager::ALLOW_ANY);
    const auto ccc = std::make_pair("-ccc", ArgsManager::ALLOW_ANY);
    const auto d = std::make_pair("-d", ArgsManager::ALLOW_ANY);

    const char *argv_test[] = {"-ignored", "-a", "-b", "-ccc=argument", "-ccc=multiple", "f", "-d=e"};

    std::string error;
    LOCK(testArgs.cs_args);
    testArgs.SetupArgs({a, b, ccc, d});
    BOOST_CHECK(testArgs.ParseParameters(0, argv_test, error));
    BOOST_CHECK(testArgs.m_settings.command_line_options.empty() && testArgs.m_settings.ro_config.empty());

    BOOST_CHECK(testArgs.ParseParameters(1, argv_test, error));
    BOOST_CHECK(testArgs.m_settings.command_line_options.empty() && testArgs.m_settings.ro_config.empty());

    BOOST_CHECK(testArgs.ParseParameters(7, argv_test, error));
    // expectation: -ignored is ignored (program name argument),
    // -a, -b and -ccc end up in map, -d ignored because it is after
    // a non-option argument (non-GNU option parsing)
    BOOST_CHECK(testArgs.m_settings.command_line_options.size() == 3 && testArgs.m_settings.ro_config.empty());
    BOOST_CHECK(testArgs.IsArgSet("-a") && testArgs.IsArgSet("-b") && testArgs.IsArgSet("-ccc")
                && !testArgs.IsArgSet("f") && !testArgs.IsArgSet("-d"));
    BOOST_CHECK(testArgs.m_settings.command_line_options.count("a") && testArgs.m_settings.command_line_options.count("b") && testArgs.m_settings.command_line_options.count("ccc")
                && !testArgs.m_settings.command_line_options.count("f") && !testArgs.m_settings.command_line_options.count("d"));

    BOOST_CHECK(testArgs.m_settings.command_line_options["a"].size() == 1);
    BOOST_CHECK(testArgs.m_settings.command_line_options["a"].front().get_str() == "");
    BOOST_CHECK(testArgs.m_settings.command_line_options["ccc"].size() == 2);
    BOOST_CHECK(testArgs.m_settings.command_line_options["ccc"].front().get_str() == "argument");
    BOOST_CHECK(testArgs.m_settings.command_line_options["ccc"].back().get_str() == "multiple");
    BOOST_CHECK(testArgs.GetArgs("-ccc").size() == 2);
}

BOOST_AUTO_TEST_CASE(util_ParseInvalidParameters)
{
    TestArgsManager test;
    test.SetupArgs({{"-registered", ArgsManager::ALLOW_ANY}});

    const char* argv[] = {"ignored", "-registered"};
    std::string error;
    BOOST_CHECK(test.ParseParameters(2, argv, error));
    BOOST_CHECK_EQUAL(error, "");

    argv[1] = "-unregistered";
    BOOST_CHECK(!test.ParseParameters(2, argv, error));
    BOOST_CHECK_EQUAL(error, "Invalid parameter -unregistered");

    // Make sure registered parameters prefixed with a chain type trigger errors.
    // (Previously, they were accepted and ignored.)
    argv[1] = "-test.registered";
    BOOST_CHECK(!test.ParseParameters(2, argv, error));
    BOOST_CHECK_EQUAL(error, "Invalid parameter -test.registered");
}

static void TestParse(const std::string& str, bool expected_bool, int64_t expected_int)
{
    TestArgsManager test;
    test.SetupArgs({{"-value", ArgsManager::ALLOW_ANY}});
    std::string arg = "-value=" + str;
    const char* argv[] = {"ignored", arg.c_str()};
    std::string error;
    BOOST_CHECK(test.ParseParameters(2, argv, error));
    BOOST_CHECK_EQUAL(test.GetBoolArg("-value", false), expected_bool);
    BOOST_CHECK_EQUAL(test.GetBoolArg("-value", true), expected_bool);
    BOOST_CHECK_EQUAL(test.GetIntArg("-value", 99998), expected_int);
    BOOST_CHECK_EQUAL(test.GetIntArg("-value", 99999), expected_int);
}

// Test bool and int parsing.
BOOST_AUTO_TEST_CASE(util_ArgParsing)
{
    // Some of these cases could be ambiguous or surprising to users, and might
    // be worth triggering errors or warnings in the future. But for now basic
    // test coverage is useful to avoid breaking backwards compatibility
    // unintentionally.
    TestParse("", true, 0);
    TestParse(" ", false, 0);
    TestParse("0", false, 0);
    TestParse("0 ", false, 0);
    TestParse(" 0", false, 0);
    TestParse("+0", false, 0);
    TestParse("-0", false, 0);
    TestParse("5", true, 5);
    TestParse("5 ", true, 5);
    TestParse(" 5", true, 5);
    TestParse("+5", true, 5);
    TestParse("-5", true, -5);
    TestParse("0 5", false, 0);
    TestParse("5 0", true, 5);
    TestParse("050", true, 50);
    TestParse("0.", false, 0);
    TestParse("5.", true, 5);
    TestParse("0.0", false, 0);
    TestParse("0.5", false, 0);
    TestParse("5.0", true, 5);
    TestParse("5.5", true, 5);
    TestParse("x", false, 0);
    TestParse("x0", false, 0);
    TestParse("x5", false, 0);
    TestParse("0x", false, 0);
    TestParse("5x", true, 5);
    TestParse("0x5", false, 0);
    TestParse("false", false, 0);
    TestParse("true", false, 0);
    TestParse("yes", false, 0);
    TestParse("no", false, 0);
}

BOOST_AUTO_TEST_CASE(util_GetBoolArg)
{
    TestArgsManager testArgs;
    const auto a = std::make_pair("-a", ArgsManager::ALLOW_ANY);
    const auto b = std::make_pair("-b", ArgsManager::ALLOW_ANY);
    const auto c = std::make_pair("-c", ArgsManager::ALLOW_ANY);
    const auto d = std::make_pair("-d", ArgsManager::ALLOW_ANY);
    const auto e = std::make_pair("-e", ArgsManager::ALLOW_ANY);
    const auto f = std::make_pair("-f", ArgsManager::ALLOW_ANY);

    const char *argv_test[] = {
        "ignored", "-a", "-nob", "-c=0", "-d=1", "-e=false", "-f=true"};
    std::string error;
    LOCK(testArgs.cs_args);
    testArgs.SetupArgs({a, b, c, d, e, f});
    BOOST_CHECK(testArgs.ParseParameters(7, argv_test, error));

    // Each letter should be set.
    for (const char opt : "abcdef")
        BOOST_CHECK(testArgs.IsArgSet({'-', opt}) || !opt);

    // Nothing else should be in the map
    BOOST_CHECK(testArgs.m_settings.command_line_options.size() == 6 &&
                testArgs.m_settings.ro_config.empty());

    // The -no prefix should get stripped on the way in.
    BOOST_CHECK(!testArgs.IsArgSet("-nob"));

    // The -b option is flagged as negated, and nothing else is
    BOOST_CHECK(testArgs.IsArgNegated("-b"));
    BOOST_CHECK(!testArgs.IsArgNegated("-a"));

    // Check expected values.
    BOOST_CHECK(testArgs.GetBoolArg("-a", false) == true);
    BOOST_CHECK(testArgs.GetBoolArg("-b", true) == false);
    BOOST_CHECK(testArgs.GetBoolArg("-c", true) == false);
    BOOST_CHECK(testArgs.GetBoolArg("-d", false) == true);
    BOOST_CHECK(testArgs.GetBoolArg("-e", true) == false);
    BOOST_CHECK(testArgs.GetBoolArg("-f", true) == false);
}

BOOST_AUTO_TEST_CASE(util_GetBoolArgEdgeCases)
{
    // Test some awful edge cases that hopefully no user will ever exercise.
    TestArgsManager testArgs;

    // Params test
    const auto foo = std::make_pair("-foo", ArgsManager::ALLOW_ANY);
    const auto bar = std::make_pair("-bar", ArgsManager::ALLOW_ANY);
    const char *argv_test[] = {"ignored", "-nofoo", "-foo", "-nobar=0"};
    testArgs.SetupArgs({foo, bar});
    std::string error;
    BOOST_CHECK(testArgs.ParseParameters(4, argv_test, error));

    // This was passed twice, second one overrides the negative setting.
    BOOST_CHECK(!testArgs.IsArgNegated("-foo"));
    BOOST_CHECK(testArgs.GetArg("-foo", "xxx") == "");

    // A double negative is a positive, and not marked as negated.
    BOOST_CHECK(!testArgs.IsArgNegated("-bar"));
    BOOST_CHECK(testArgs.GetArg("-bar", "xxx") == "1");

    // Config test
    const char *conf_test = "nofoo=1\nfoo=1\nnobar=0\n";
    BOOST_CHECK(testArgs.ParseParameters(1, argv_test, error));
    testArgs.ReadConfigString(conf_test);

    // This was passed twice, second one overrides the negative setting,
    // and the value.
    BOOST_CHECK(!testArgs.IsArgNegated("-foo"));
    BOOST_CHECK(testArgs.GetArg("-foo", "xxx") == "1");

    // A double negative is a positive, and does not count as negated.
    BOOST_CHECK(!testArgs.IsArgNegated("-bar"));
    BOOST_CHECK(testArgs.GetArg("-bar", "xxx") == "1");

    // Combined test
    const char *combo_test_args[] = {"ignored", "-nofoo", "-bar"};
    const char *combo_test_conf = "foo=1\nnobar=1\n";
    BOOST_CHECK(testArgs.ParseParameters(3, combo_test_args, error));
    testArgs.ReadConfigString(combo_test_conf);

    // Command line overrides, but doesn't erase old setting
    BOOST_CHECK(testArgs.IsArgNegated("-foo"));
    BOOST_CHECK(testArgs.GetArg("-foo", "xxx") == "0");
    BOOST_CHECK(testArgs.GetArgs("-foo").size() == 0);

    // Command line overrides, but doesn't erase old setting
    BOOST_CHECK(!testArgs.IsArgNegated("-bar"));
    BOOST_CHECK(testArgs.GetArg("-bar", "xxx") == "");
    BOOST_CHECK(testArgs.GetArgs("-bar").size() == 1
                && testArgs.GetArgs("-bar").front() == "");
}

BOOST_AUTO_TEST_CASE(util_ReadConfigStream)
{
    const char *str_config =
       "a=\n"
       "b=1\n"
       "ccc=argument\n"
       "ccc=multiple\n"
       "d=e\n"
       "nofff=1\n"
       "noggg=0\n"
       "h=1\n"
       "noh=1\n"
       "noi=1\n"
       "i=1\n"
       "sec1.ccc=extend1\n"
       "\n"
       "[sec1]\n"
       "ccc=extend2\n"
       "d=eee\n"
       "h=1\n"
       "[sec2]\n"
       "ccc=extend3\n"
       "iii=2\n";

    TestArgsManager test_args;
    LOCK(test_args.cs_args);
    const auto a = std::make_pair("-a", ArgsManager::ALLOW_ANY);
    const auto b = std::make_pair("-b", ArgsManager::ALLOW_ANY);
    const auto ccc = std::make_pair("-ccc", ArgsManager::ALLOW_ANY);
    const auto d = std::make_pair("-d", ArgsManager::ALLOW_ANY);
    const auto e = std::make_pair("-e", ArgsManager::ALLOW_ANY);
    const auto fff = std::make_pair("-fff", ArgsManager::ALLOW_ANY);
    const auto ggg = std::make_pair("-ggg", ArgsManager::ALLOW_ANY);
    const auto h = std::make_pair("-h", ArgsManager::ALLOW_ANY);
    const auto i = std::make_pair("-i", ArgsManager::ALLOW_ANY);
    const auto iii = std::make_pair("-iii", ArgsManager::ALLOW_ANY);
    test_args.SetupArgs({a, b, ccc, d, e, fff, ggg, h, i, iii});

    test_args.ReadConfigString(str_config);
    // expectation: a, b, ccc, d, fff, ggg, h, i end up in map
    // so do sec1.ccc, sec1.d, sec1.h, sec2.ccc, sec2.iii

    BOOST_CHECK(test_args.m_settings.command_line_options.empty());
    BOOST_CHECK(test_args.m_settings.ro_config.size() == 3);
    BOOST_CHECK(test_args.m_settings.ro_config[""].size() == 8);
    BOOST_CHECK(test_args.m_settings.ro_config["sec1"].size() == 3);
    BOOST_CHECK(test_args.m_settings.ro_config["sec2"].size() == 2);

    BOOST_CHECK(test_args.m_settings.ro_config[""].count("a"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].count("b"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].count("ccc"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].count("d"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].count("fff"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].count("ggg"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].count("h"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].count("i"));
    BOOST_CHECK(test_args.m_settings.ro_config["sec1"].count("ccc"));
    BOOST_CHECK(test_args.m_settings.ro_config["sec1"].count("h"));
    BOOST_CHECK(test_args.m_settings.ro_config["sec2"].count("ccc"));
    BOOST_CHECK(test_args.m_settings.ro_config["sec2"].count("iii"));

    BOOST_CHECK(test_args.IsArgSet("-a"));
    BOOST_CHECK(test_args.IsArgSet("-b"));
    BOOST_CHECK(test_args.IsArgSet("-ccc"));
    BOOST_CHECK(test_args.IsArgSet("-d"));
    BOOST_CHECK(test_args.IsArgSet("-fff"));
    BOOST_CHECK(test_args.IsArgSet("-ggg"));
    BOOST_CHECK(test_args.IsArgSet("-h"));
    BOOST_CHECK(test_args.IsArgSet("-i"));
    BOOST_CHECK(!test_args.IsArgSet("-zzz"));
    BOOST_CHECK(!test_args.IsArgSet("-iii"));

    BOOST_CHECK_EQUAL(test_args.GetArg("-a", "xxx"), "");
    BOOST_CHECK_EQUAL(test_args.GetArg("-b", "xxx"), "1");
    BOOST_CHECK_EQUAL(test_args.GetArg("-ccc", "xxx"), "argument");
    BOOST_CHECK_EQUAL(test_args.GetArg("-d", "xxx"), "e");
    BOOST_CHECK_EQUAL(test_args.GetArg("-fff", "xxx"), "0");
    BOOST_CHECK_EQUAL(test_args.GetArg("-ggg", "xxx"), "1");
    BOOST_CHECK_EQUAL(test_args.GetArg("-h", "xxx"), "0");
    BOOST_CHECK_EQUAL(test_args.GetArg("-i", "xxx"), "1");
    BOOST_CHECK_EQUAL(test_args.GetArg("-zzz", "xxx"), "xxx");
    BOOST_CHECK_EQUAL(test_args.GetArg("-iii", "xxx"), "xxx");

    for (const bool def : {false, true}) {
        BOOST_CHECK(test_args.GetBoolArg("-a", def));
        BOOST_CHECK(test_args.GetBoolArg("-b", def));
        BOOST_CHECK(!test_args.GetBoolArg("-ccc", def));
        BOOST_CHECK(!test_args.GetBoolArg("-d", def));
        BOOST_CHECK(!test_args.GetBoolArg("-fff", def));
        BOOST_CHECK(test_args.GetBoolArg("-ggg", def));
        BOOST_CHECK(!test_args.GetBoolArg("-h", def));
        BOOST_CHECK(test_args.GetBoolArg("-i", def));
        BOOST_CHECK(test_args.GetBoolArg("-zzz", def) == def);
        BOOST_CHECK(test_args.GetBoolArg("-iii", def) == def);
    }

    BOOST_CHECK(test_args.GetArgs("-a").size() == 1
                && test_args.GetArgs("-a").front() == "");
    BOOST_CHECK(test_args.GetArgs("-b").size() == 1
                && test_args.GetArgs("-b").front() == "1");
    BOOST_CHECK(test_args.GetArgs("-ccc").size() == 2
                && test_args.GetArgs("-ccc").front() == "argument"
                && test_args.GetArgs("-ccc").back() == "multiple");
    BOOST_CHECK(test_args.GetArgs("-fff").size() == 0);
    BOOST_CHECK(test_args.GetArgs("-nofff").size() == 0);
    BOOST_CHECK(test_args.GetArgs("-ggg").size() == 1
                && test_args.GetArgs("-ggg").front() == "1");
    BOOST_CHECK(test_args.GetArgs("-noggg").size() == 0);
    BOOST_CHECK(test_args.GetArgs("-h").size() == 0);
    BOOST_CHECK(test_args.GetArgs("-noh").size() == 0);
    BOOST_CHECK(test_args.GetArgs("-i").size() == 1
                && test_args.GetArgs("-i").front() == "1");
    BOOST_CHECK(test_args.GetArgs("-noi").size() == 0);
    BOOST_CHECK(test_args.GetArgs("-zzz").size() == 0);

    BOOST_CHECK(!test_args.IsArgNegated("-a"));
    BOOST_CHECK(!test_args.IsArgNegated("-b"));
    BOOST_CHECK(!test_args.IsArgNegated("-ccc"));
    BOOST_CHECK(!test_args.IsArgNegated("-d"));
    BOOST_CHECK(test_args.IsArgNegated("-fff"));
    BOOST_CHECK(!test_args.IsArgNegated("-ggg"));
    BOOST_CHECK(test_args.IsArgNegated("-h")); // last setting takes precedence
    BOOST_CHECK(!test_args.IsArgNegated("-i")); // last setting takes precedence
    BOOST_CHECK(!test_args.IsArgNegated("-zzz"));

    // Test sections work
    test_args.SelectConfigNetwork("sec1");

    // same as original
    BOOST_CHECK_EQUAL(test_args.GetArg("-a", "xxx"), "");
    BOOST_CHECK_EQUAL(test_args.GetArg("-b", "xxx"), "1");
    BOOST_CHECK_EQUAL(test_args.GetArg("-fff", "xxx"), "0");
    BOOST_CHECK_EQUAL(test_args.GetArg("-ggg", "xxx"), "1");
    BOOST_CHECK_EQUAL(test_args.GetArg("-zzz", "xxx"), "xxx");
    BOOST_CHECK_EQUAL(test_args.GetArg("-iii", "xxx"), "xxx");
    // d is overridden
    BOOST_CHECK(test_args.GetArg("-d", "xxx") == "eee");
    // section-specific setting
    BOOST_CHECK(test_args.GetArg("-h", "xxx") == "1");
    // section takes priority for multiple values
    BOOST_CHECK(test_args.GetArg("-ccc", "xxx") == "extend1");
    // check multiple values works
    const std::vector<std::string> sec1_ccc_expected = {"extend1","extend2","argument","multiple"};
    const auto& sec1_ccc_res = test_args.GetArgs("-ccc");
    BOOST_CHECK_EQUAL_COLLECTIONS(sec1_ccc_res.begin(), sec1_ccc_res.end(), sec1_ccc_expected.begin(), sec1_ccc_expected.end());

    test_args.SelectConfigNetwork("sec2");

    // same as original
    BOOST_CHECK(test_args.GetArg("-a", "xxx") == "");
    BOOST_CHECK(test_args.GetArg("-b", "xxx") == "1");
    BOOST_CHECK(test_args.GetArg("-d", "xxx") == "e");
    BOOST_CHECK(test_args.GetArg("-fff", "xxx") == "0");
    BOOST_CHECK(test_args.GetArg("-ggg", "xxx") == "1");
    BOOST_CHECK(test_args.GetArg("-zzz", "xxx") == "xxx");
    BOOST_CHECK(test_args.GetArg("-h", "xxx") == "0");
    // section-specific setting
    BOOST_CHECK(test_args.GetArg("-iii", "xxx") == "2");
    // section takes priority for multiple values
    BOOST_CHECK(test_args.GetArg("-ccc", "xxx") == "extend3");
    // check multiple values works
    const std::vector<std::string> sec2_ccc_expected = {"extend3","argument","multiple"};
    const auto& sec2_ccc_res = test_args.GetArgs("-ccc");
    BOOST_CHECK_EQUAL_COLLECTIONS(sec2_ccc_res.begin(), sec2_ccc_res.end(), sec2_ccc_expected.begin(), sec2_ccc_expected.end());

    // Test section only options

    test_args.SetNetworkOnlyArg("-d");
    test_args.SetNetworkOnlyArg("-ccc");
    test_args.SetNetworkOnlyArg("-h");

    test_args.SelectConfigNetwork(ChainTypeToString(ChainType::MAIN));
    BOOST_CHECK(test_args.GetArg("-d", "xxx") == "e");
    BOOST_CHECK(test_args.GetArgs("-ccc").size() == 2);
    BOOST_CHECK(test_args.GetArg("-h", "xxx") == "0");

    test_args.SelectConfigNetwork("sec1");
    BOOST_CHECK(test_args.GetArg("-d", "xxx") == "eee");
    BOOST_CHECK(test_args.GetArgs("-d").size() == 1);
    BOOST_CHECK(test_args.GetArgs("-ccc").size() == 2);
    BOOST_CHECK(test_args.GetArg("-h", "xxx") == "1");

    test_args.SelectConfigNetwork("sec2");
    BOOST_CHECK(test_args.GetArg("-d", "xxx") == "xxx");
    BOOST_CHECK(test_args.GetArgs("-d").size() == 0);
    BOOST_CHECK(test_args.GetArgs("-ccc").size() == 1);
    BOOST_CHECK(test_args.GetArg("-h", "xxx") == "0");
}

BOOST_AUTO_TEST_CASE(util_GetArg)
{
    TestArgsManager testArgs;
    LOCK(testArgs.cs_args);
    testArgs.m_settings.command_line_options.clear();
    testArgs.m_settings.command_line_options["strtest1"] = {"string..."};
    // strtest2 undefined on purpose
    testArgs.m_settings.command_line_options["inttest1"] = {"12345"};
    testArgs.m_settings.command_line_options["inttest2"] = {"81985529216486895"};
    // inttest3 undefined on purpose
    testArgs.m_settings.command_line_options["booltest1"] = {""};
    // booltest2 undefined on purpose
    testArgs.m_settings.command_line_options["booltest3"] = {"0"};
    testArgs.m_settings.command_line_options["booltest4"] = {"1"};

    // priorities
    testArgs.m_settings.command_line_options["pritest1"] = {"a", "b"};
    testArgs.m_settings.ro_config[""]["pritest2"] = {"a", "b"};
    testArgs.m_settings.command_line_options["pritest3"] = {"a"};
    testArgs.m_settings.ro_config[""]["pritest3"] = {"b"};
    testArgs.m_settings.command_line_options["pritest4"] = {"a","b"};
    testArgs.m_settings.ro_config[""]["pritest4"] = {"c","d"};

    BOOST_CHECK_EQUAL(testArgs.GetArg("strtest1", "default"), "string...");
    BOOST_CHECK_EQUAL(testArgs.GetArg("strtest2", "default"), "default");
    BOOST_CHECK_EQUAL(testArgs.GetIntArg("inttest1", -1), 12345);
    BOOST_CHECK_EQUAL(testArgs.GetIntArg("inttest2", -1), 81985529216486895LL);
    BOOST_CHECK_EQUAL(testArgs.GetIntArg("inttest3", -1), -1);
    BOOST_CHECK_EQUAL(testArgs.GetBoolArg("booltest1", false), true);
    BOOST_CHECK_EQUAL(testArgs.GetBoolArg("booltest2", false), false);
    BOOST_CHECK_EQUAL(testArgs.GetBoolArg("booltest3", false), false);
    BOOST_CHECK_EQUAL(testArgs.GetBoolArg("booltest4", false), true);

    BOOST_CHECK_EQUAL(testArgs.GetArg("pritest1", "default"), "b");
    BOOST_CHECK_EQUAL(testArgs.GetArg("pritest2", "default"), "a");
    BOOST_CHECK_EQUAL(testArgs.GetArg("pritest3", "default"), "a");
    BOOST_CHECK_EQUAL(testArgs.GetArg("pritest4", "default"), "b");
}

BOOST_AUTO_TEST_CASE(util_GetChainTypeString)
{
    TestArgsManager test_args;
    const auto testnet = std::make_pair("-testnet", ArgsManager::ALLOW_ANY);
    const auto testnet4 = std::make_pair("-testnet4", ArgsManager::ALLOW_ANY);
    const auto regtest = std::make_pair("-regtest", ArgsManager::ALLOW_ANY);
    test_args.SetupArgs({testnet, testnet4, regtest});

    const char* argv_testnet[] = {"cmd", "-testnet"};
    const char* argv_testnet4[] = {"cmd", "-testnet4"};
    const char* argv_regtest[] = {"cmd", "-regtest"};
    const char* argv_test_no_reg[] = {"cmd", "-testnet", "-noregtest"};
    const char* argv_both[] = {"cmd", "-testnet", "-regtest"};

    // equivalent to "-testnet"
    // regtest in testnet section is ignored
    const char* testnetconf = "testnet=1\nregtest=0\n[test]\nregtest=1";
    std::string error;

    BOOST_CHECK(test_args.ParseParameters(0, argv_testnet, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "main");

    BOOST_CHECK(test_args.ParseParameters(2, argv_testnet, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "test");

    BOOST_CHECK(test_args.ParseParameters(0, argv_testnet4, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "main");

    BOOST_CHECK(test_args.ParseParameters(2, argv_testnet4, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "testnet4");

    BOOST_CHECK(test_args.ParseParameters(2, argv_regtest, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "regtest");

    BOOST_CHECK(test_args.ParseParameters(3, argv_test_no_reg, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "test");

    BOOST_CHECK(test_args.ParseParameters(3, argv_both, error));
    BOOST_CHECK_THROW(test_args.GetChainTypeString(), std::runtime_error);

    BOOST_CHECK(test_args.ParseParameters(0, argv_testnet, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "test");

    BOOST_CHECK(test_args.ParseParameters(2, argv_testnet, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "test");

    BOOST_CHECK(test_args.ParseParameters(2, argv_regtest, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainTypeString(), std::runtime_error);

    BOOST_CHECK(test_args.ParseParameters(3, argv_test_no_reg, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "test");

    BOOST_CHECK(test_args.ParseParameters(3, argv_both, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainTypeString(), std::runtime_error);

    // check setting the network to test (and thus making
    // [test] regtest=1 potentially relevant) doesn't break things
    test_args.SelectConfigNetwork("test");

    BOOST_CHECK(test_args.ParseParameters(0, argv_testnet, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "test");

    BOOST_CHECK(test_args.ParseParameters(2, argv_testnet, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "test");

    BOOST_CHECK(test_args.ParseParameters(2, argv_regtest, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainTypeString(), std::runtime_error);

    BOOST_CHECK(test_args.ParseParameters(2, argv_test_no_reg, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "test");

    BOOST_CHECK(test_args.ParseParameters(3, argv_both, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainTypeString(), std::runtime_error);
}

// Test different ways settings can be merged, and verify results. This test can
// be used to confirm that updates to settings code don't change behavior
// unintentionally.
//
// The test covers:
//
// - Combining different setting actions. Possible actions are: configuring a
//   setting, negating a setting (adding "-no" prefix), and configuring/negating
//   settings in a network section (adding "main." or "test." prefixes).
//
// - Combining settings from command line arguments and a config file.
//
// - Combining SoftSet and ForceSet calls.
//
// - Testing "main" and "test" network values to make sure settings from network
//   sections are applied and to check for mainnet-specific behaviors like
//   inheriting settings from the default section.
//
// - Testing network-specific settings like "-wallet", that may be ignored
//   outside a network section, and non-network specific settings like "-server"
//   that aren't sensitive to the network.
//
struct ArgsMergeTestingSetup : public BasicTestingSetup {
    //! Max number of actions to sequence together. Can decrease this when
    //! debugging to make test results easier to understand.
    static constexpr int MAX_ACTIONS = 3;

    enum Action { NONE, SET, NEGATE, SECTION_SET, SECTION_NEGATE };
    using ActionList = Action[MAX_ACTIONS];

    //! Enumerate all possible test configurations.
    template <typename Fn>
    void ForEachMergeSetup(Fn&& fn)
    {
        ActionList arg_actions = {};
        // command_line_options do not have sections. Only iterate over SET and NEGATE
        ForEachNoDup(arg_actions, SET, NEGATE, [&] {
            ActionList conf_actions = {};
            ForEachNoDup(conf_actions, SET, SECTION_NEGATE, [&] {
                for (bool soft_set : {false, true}) {
                    for (bool force_set : {false, true}) {
                        for (const std::string& section : {ChainTypeToString(ChainType::MAIN), ChainTypeToString(ChainType::TESTNET), ChainTypeToString(ChainType::TESTNET4), ChainTypeToString(ChainType::SIGNET)}) {
                            for (const std::string& network : {ChainTypeToString(ChainType::MAIN), ChainTypeToString(ChainType::TESTNET), ChainTypeToString(ChainType::TESTNET4), ChainTypeToString(ChainType::SIGNET)}) {
                                for (bool net_specific : {false, true}) {
                                    fn(arg_actions, conf_actions, soft_set, force_set, section, network, net_specific);
                                }
                            }
                        }
                    }
                }
            });
        });
    }

    //! Translate actions into a list of <key>=<value> setting strings.
    std::vector<std::string> GetValues(const ActionList& actions,
        const std::string& section,
        const std::string& name,
        const std::string& value_prefix)
    {
        std::vector<std::string> values;
        int suffix = 0;
        for (Action action : actions) {
            if (action == NONE) break;
            std::string prefix;
            if (action == SECTION_SET || action == SECTION_NEGATE) prefix = section + ".";
            if (action == SET || action == SECTION_SET) {
                for (int i = 0; i < 2; ++i) {
                    values.push_back(prefix + name + "=" + value_prefix + ToString(++suffix));
                }
            }
            if (action == NEGATE || action == SECTION_NEGATE) {
                values.push_back(prefix + "no" + name + "=1");
            }
        }
        return values;
    }
};

// Regression test covering different ways config settings can be merged. The
// test parses and merges settings, representing the results as strings that get
// compared against an expected hash. To debug, the result strings can be dumped
// to a file (see comments below).
BOOST_FIXTURE_TEST_CASE(util_ArgsMerge, ArgsMergeTestingSetup)
{
    CHash256 out_sha;
    FILE* out_file = nullptr;
    if (const char* out_path = getenv("ARGS_MERGE_TEST_OUT")) {
        out_file = fsbridge::fopen(out_path, "w");
        if (!out_file) throw std::system_error(errno, std::generic_category(), "fopen failed");
    }

    ForEachMergeSetup([&](const ActionList& arg_actions, const ActionList& conf_actions, bool soft_set, bool force_set,
                          const std::string& section, const std::string& network, bool net_specific) {
        TestArgsManager parser;
        LOCK(parser.cs_args);

        std::string desc = "net=";
        desc += network;
        parser.m_network = network;

        const std::string& name = net_specific ? "wallet" : "server";
        const std::string key = "-" + name;
        parser.AddArg(key, name, ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
        if (net_specific) parser.SetNetworkOnlyArg(key);

        auto args = GetValues(arg_actions, section, name, "a");
        std::vector<const char*> argv = {"ignored"};
        for (auto& arg : args) {
            arg.insert(0, "-");
            desc += " ";
            desc += arg;
            argv.push_back(arg.c_str());
        }
        std::string error;
        BOOST_CHECK(parser.ParseParameters(argv.size(), argv.data(), error));
        BOOST_CHECK_EQUAL(error, "");

        std::string conf;
        for (auto& conf_val : GetValues(conf_actions, section, name, "c")) {
            desc += " ";
            desc += conf_val;
            conf += conf_val;
            conf += "\n";
        }
        std::istringstream conf_stream(conf);
        BOOST_CHECK(parser.ReadConfigStream(conf_stream, "filepath", error));
        BOOST_CHECK_EQUAL(error, "");

        if (soft_set) {
            desc += " soft";
            parser.SoftSetArg(key, "soft1");
            parser.SoftSetArg(key, "soft2");
        }

        if (force_set) {
            desc += " force";
            parser.ForceSetArg(key, "force1");
            parser.ForceSetArg(key, "force2");
        }

        desc += " || ";

        if (!parser.IsArgSet(key)) {
            desc += "unset";
            BOOST_CHECK(!parser.IsArgNegated(key));
            BOOST_CHECK_EQUAL(parser.GetArg(key, "default"), "default");
            BOOST_CHECK(parser.GetArgs(key).empty());
        } else if (parser.IsArgNegated(key)) {
            desc += "negated";
            BOOST_CHECK_EQUAL(parser.GetArg(key, "default"), "0");
            BOOST_CHECK(parser.GetArgs(key).empty());
        } else {
            desc += parser.GetArg(key, "default");
            desc += " |";
            for (const auto& arg : parser.GetArgs(key)) {
                desc += " ";
                desc += arg;
            }
        }

        std::set<std::string> ignored = parser.GetUnsuitableSectionOnlyArgs();
        if (!ignored.empty()) {
            desc += " | ignored";
            for (const auto& arg : ignored) {
                desc += " ";
                desc += arg;
            }
        }

        desc += "\n";

        out_sha.Write(MakeUCharSpan(desc));
        if (out_file) {
            BOOST_REQUIRE(fwrite(desc.data(), 1, desc.size(), out_file) == desc.size());
        }
    });

    if (out_file) {
        if (fclose(out_file)) throw std::system_error(errno, std::generic_category(), "fclose failed");
        out_file = nullptr;
    }

    unsigned char out_sha_bytes[CSHA256::OUTPUT_SIZE];
    out_sha.Finalize(out_sha_bytes);
    std::string out_sha_hex = HexStr(out_sha_bytes);

    // If check below fails, should manually dump the results with:
    //
    //   ARGS_MERGE_TEST_OUT=results.txt ./test_tortoisecoin --run_test=argsman_tests/util_ArgsMerge
    //
    // And verify diff against previous results to make sure the changes are expected.
    //
    // Results file is formatted like:
    //
    //   <input> || <IsArgSet/IsArgNegated/GetArg output> | <GetArgs output> | <GetUnsuitable output>
    BOOST_CHECK_EQUAL(out_sha_hex, "f1ee5ab094cc43d16a6086fa7f2c10389e0f99902616b31bbf29189972ad1473");
}

// Similar test as above, but for ArgsManager::GetChainTypeString function.
struct ChainMergeTestingSetup : public BasicTestingSetup {
    static constexpr int MAX_ACTIONS = 2;

    enum Action { NONE, ENABLE_TEST, DISABLE_TEST, NEGATE_TEST, ENABLE_REG, DISABLE_REG, NEGATE_REG };
    using ActionList = Action[MAX_ACTIONS];

    //! Enumerate all possible test configurations.
    template <typename Fn>
    void ForEachMergeSetup(Fn&& fn)
    {
        ActionList arg_actions = {};
        ForEachNoDup(arg_actions, ENABLE_TEST, NEGATE_REG, [&] {
            ActionList conf_actions = {};
            ForEachNoDup(conf_actions, ENABLE_TEST, NEGATE_REG, [&] { fn(arg_actions, conf_actions); });
        });
    }
};

BOOST_FIXTURE_TEST_CASE(util_ChainMerge, ChainMergeTestingSetup)
{
    CHash256 out_sha;
    FILE* out_file = nullptr;
    if (const char* out_path = getenv("CHAIN_MERGE_TEST_OUT")) {
        out_file = fsbridge::fopen(out_path, "w");
        if (!out_file) throw std::system_error(errno, std::generic_category(), "fopen failed");
    }

    ForEachMergeSetup([&](const ActionList& arg_actions, const ActionList& conf_actions) {
        TestArgsManager parser;
        LOCK(parser.cs_args);
        parser.AddArg("-regtest", "regtest", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
        parser.AddArg("-testnet", "testnet", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

        auto arg = [](Action action) { return action == ENABLE_TEST  ? "-testnet=1"   :
                                              action == DISABLE_TEST ? "-testnet=0"   :
                                              action == NEGATE_TEST  ? "-notestnet=1" :
                                              action == ENABLE_REG   ? "-regtest=1"   :
                                              action == DISABLE_REG  ? "-regtest=0"   :
                                              action == NEGATE_REG   ? "-noregtest=1" : nullptr; };

        std::string desc;
        std::vector<const char*> argv = {"ignored"};
        for (Action action : arg_actions) {
            const char* argstr = arg(action);
            if (!argstr) break;
            argv.push_back(argstr);
            desc += " ";
            desc += argv.back();
        }
        std::string error;
        BOOST_CHECK(parser.ParseParameters(argv.size(), argv.data(), error));
        BOOST_CHECK_EQUAL(error, "");

        std::string conf;
        for (Action action : conf_actions) {
            const char* argstr = arg(action);
            if (!argstr) break;
            desc += " ";
            desc += argstr + 1;
            conf += argstr + 1;
            conf += "\n";
        }
        std::istringstream conf_stream(conf);
        BOOST_CHECK(parser.ReadConfigStream(conf_stream, "filepath", error));
        BOOST_CHECK_EQUAL(error, "");

        desc += " || ";
        try {
            desc += parser.GetChainTypeString();
        } catch (const std::runtime_error& e) {
            desc += "error: ";
            desc += e.what();
        }
        desc += "\n";

        out_sha.Write(MakeUCharSpan(desc));
        if (out_file) {
            BOOST_REQUIRE(fwrite(desc.data(), 1, desc.size(), out_file) == desc.size());
        }
    });

    if (out_file) {
        if (fclose(out_file)) throw std::system_error(errno, std::generic_category(), "fclose failed");
        out_file = nullptr;
    }

    unsigned char out_sha_bytes[CSHA256::OUTPUT_SIZE];
    out_sha.Finalize(out_sha_bytes);
    std::string out_sha_hex = HexStr(out_sha_bytes);

    // If check below fails, should manually dump the results with:
    //
    //   CHAIN_MERGE_TEST_OUT=results.txt ./test_tortoisecoin --run_test=argsman_tests/util_ChainMerge
    //
    // And verify diff against previous results to make sure the changes are expected.
    //
    // Results file is formatted like:
    //
    //   <input> || <output>
    BOOST_CHECK_EQUAL(out_sha_hex, "9e60306e1363528bbc19a47f22bcede88e5d6815212f18ec8e6cdc4638dddab4");
}

BOOST_AUTO_TEST_CASE(util_ReadWriteSettings)
{
    // Test writing setting.
    TestArgsManager args1;
    args1.ForceSetArg("-datadir", fs::PathToString(m_path_root));
    args1.LockSettings([&](common::Settings& settings) { settings.rw_settings["name"] = "value"; });
    args1.WriteSettingsFile();

    // Test reading setting.
    TestArgsManager args2;
    args2.ForceSetArg("-datadir", fs::PathToString(m_path_root));
    args2.ReadSettingsFile();
    args2.LockSettings([&](common::Settings& settings) { BOOST_CHECK_EQUAL(settings.rw_settings["name"].get_str(), "value"); });

    // Test error logging, and remove previously written setting.
    {
        ASSERT_DEBUG_LOG("Failed renaming settings file");
        fs::remove(args1.GetDataDirBase() / "settings.json");
        fs::create_directory(args1.GetDataDirBase() / "settings.json");
        args2.WriteSettingsFile();
        fs::remove(args1.GetDataDirBase() / "settings.json");
    }
}

BOOST_AUTO_TEST_SUITE_END()
