// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <sync.h>
#include <test/argsman_tests_settings.h>
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
    void ReadConfigString(const std::string& str_config)
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
    void SetNetworkOnlyArg(const std::string& arg)
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
            BOOST_CHECK_EQUAL(ValueSettingStr::Get(test, "zzzzz"), "zzzzz");
        } else if (expect.string_value) {
            BOOST_CHECK_EQUAL(ValueSettingStr::Get(test, "zzzzz"), expect.string_value);
        } else {
            BOOST_CHECK(!success);
        }

        if (expect.default_int) {
            BOOST_CHECK_EQUAL(ValueSettingInt::Get(test, 99999), 99999);
        } else if (expect.int_value) {
            BOOST_CHECK_EQUAL(ValueSettingInt::Get(test, 99999), *expect.int_value);
        } else {
            BOOST_CHECK(!success);
        }

        if (expect.default_bool) {
            BOOST_CHECK_EQUAL(ValueSettingBool::Get(test, false), false);
            BOOST_CHECK_EQUAL(ValueSettingBool::Get(test, true), true);
        } else if (expect.bool_value) {
            BOOST_CHECK_EQUAL(ValueSettingBool::Get(test, false), *expect.bool_value);
            BOOST_CHECK_EQUAL(ValueSettingBool::Get(test, true), *expect.bool_value);
        } else {
            BOOST_CHECK(!success);
        }

        if (expect.list_value) {
            auto l = ValueSetting::Get(test);
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
    BOOST_CHECK(!ASetting::Value(testArgs).isNull() && !BSetting::Value(testArgs).isNull() && !CccSetting::Value(testArgs).isNull()
                && FSetting::Value(testArgs).isNull() && DSetting::Value(testArgs).isNull());
    BOOST_CHECK(testArgs.m_settings.command_line_options.contains("a") && testArgs.m_settings.command_line_options.contains("b") && testArgs.m_settings.command_line_options.contains("ccc")
                && !testArgs.m_settings.command_line_options.contains("f") && !testArgs.m_settings.command_line_options.contains("d"));

    BOOST_CHECK(testArgs.m_settings.command_line_options["a"].size() == 1);
    BOOST_CHECK(testArgs.m_settings.command_line_options["a"].front().get_str() == "");
    BOOST_CHECK(testArgs.m_settings.command_line_options["ccc"].size() == 2);
    BOOST_CHECK(testArgs.m_settings.command_line_options["ccc"].front().get_str() == "argument");
    BOOST_CHECK(testArgs.m_settings.command_line_options["ccc"].back().get_str() == "multiple");
    BOOST_CHECK(CccSetting::Get(testArgs).size() == 2);
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
    BOOST_CHECK_EQUAL(ValueSettingBool::Get(test, false), expected_bool);
    BOOST_CHECK_EQUAL(ValueSettingBool::Get(test, true), expected_bool);
    BOOST_CHECK_EQUAL(ValueSettingInt::Get(test, 99998), expected_int);
    BOOST_CHECK_EQUAL(ValueSettingInt::Get(test, 99999), expected_int);
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
    BOOST_CHECK(NobSetting::Value(testArgs).isNull());

    // The -b option is flagged as negated, and nothing else is
    BOOST_CHECK(BSetting::Value(testArgs).isFalse());
    BOOST_CHECK(!ASetting::Value(testArgs).isFalse());

    // Check expected values.
    BOOST_CHECK(ASettingBool::Get(testArgs, false) == true);
    BOOST_CHECK(BSettingBool::Get(testArgs, true) == false);
    BOOST_CHECK(CSetting::Get(testArgs) == false);
    BOOST_CHECK(DSettingBool::Get(testArgs, false) == true);
    BOOST_CHECK(ESetting::Get(testArgs) == false);
    BOOST_CHECK(FSetting2::Get(testArgs) == false);
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
    BOOST_CHECK(!FooSetting::Value(testArgs).isFalse());
    BOOST_CHECK(FooSettingStr::Get(testArgs, "xxx") == "");

    // A double negative is a positive, and not marked as negated.
    BOOST_CHECK(!BarSetting::Value(testArgs).isFalse());
    BOOST_CHECK(BarSettingStr::Get(testArgs, "xxx") == "1");

    // Config test
    const char *conf_test = "nofoo=1\nfoo=1\nnobar=0\n";
    BOOST_CHECK(testArgs.ParseParameters(1, argv_test, error));
    testArgs.ReadConfigString(conf_test);

    // This was passed twice, second one overrides the negative setting,
    // and the value.
    BOOST_CHECK(!FooSetting::Value(testArgs).isFalse());
    BOOST_CHECK(FooSettingStr::Get(testArgs, "xxx") == "1");

    // A double negative is a positive, and does not count as negated.
    BOOST_CHECK(!BarSetting::Value(testArgs).isFalse());
    BOOST_CHECK(BarSettingStr::Get(testArgs, "xxx") == "1");

    // Combined test
    const char *combo_test_args[] = {"ignored", "-nofoo", "-bar"};
    const char *combo_test_conf = "foo=1\nnobar=1\n";
    BOOST_CHECK(testArgs.ParseParameters(3, combo_test_args, error));
    testArgs.ReadConfigString(combo_test_conf);

    // Command line overrides, but doesn't erase old setting
    BOOST_CHECK(FooSetting::Value(testArgs).isFalse());
    BOOST_CHECK(FooSettingStr::Get(testArgs, "xxx") == "0");
    BOOST_CHECK(FooSetting::Get(testArgs).size() == 0);

    // Command line overrides, but doesn't erase old setting
    BOOST_CHECK(!BarSetting::Value(testArgs).isFalse());
    BOOST_CHECK(BarSettingStr::Get(testArgs, "xxx") == "");
    BOOST_CHECK(BarSetting::Get(testArgs).size() == 1
                && BarSetting::Get(testArgs).front() == "");
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

    BOOST_CHECK(test_args.m_settings.ro_config[""].contains("a"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].contains("b"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].contains("ccc"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].contains("d"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].contains("fff"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].contains("ggg"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].contains("h"));
    BOOST_CHECK(test_args.m_settings.ro_config[""].contains("i"));
    BOOST_CHECK(test_args.m_settings.ro_config["sec1"].contains("ccc"));
    BOOST_CHECK(test_args.m_settings.ro_config["sec1"].contains("h"));
    BOOST_CHECK(test_args.m_settings.ro_config["sec2"].contains("ccc"));
    BOOST_CHECK(test_args.m_settings.ro_config["sec2"].contains("iii"));

    BOOST_CHECK(!ASetting::Value(test_args).isNull());
    BOOST_CHECK(!BSetting::Value(test_args).isNull());
    BOOST_CHECK(!CccSetting::Value(test_args).isNull());
    BOOST_CHECK(!DSetting::Value(test_args).isNull());
    BOOST_CHECK(!FffSetting::Value(test_args).isNull());
    BOOST_CHECK(!GggSetting::Value(test_args).isNull());
    BOOST_CHECK(!HSetting::Value(test_args).isNull());
    BOOST_CHECK(!ISetting::Value(test_args).isNull());
    BOOST_CHECK(ZzzSetting::Value(test_args).isNull());
    BOOST_CHECK(IiiSetting::Value(test_args).isNull());

    BOOST_CHECK_EQUAL(ASettingStr::Get(test_args, "xxx"), "");
    BOOST_CHECK_EQUAL(BSettingStr::Get(test_args, "xxx"), "1");
    BOOST_CHECK_EQUAL(CccSettingStr::Get(test_args, "xxx"), "argument");
    BOOST_CHECK_EQUAL(DSettingStr::Get(test_args, "xxx"), "e");
    BOOST_CHECK_EQUAL(FffSettingStr::Get(test_args, "xxx"), "0");
    BOOST_CHECK_EQUAL(GggSettingStr::Get(test_args, "xxx"), "1");
    BOOST_CHECK_EQUAL(HSettingStr::Get(test_args, "xxx"), "0");
    BOOST_CHECK_EQUAL(ISettingStr::Get(test_args, "xxx"), "1");
    BOOST_CHECK_EQUAL(ZzzSettingStr::Get(test_args, "xxx"), "xxx");
    BOOST_CHECK_EQUAL(IiiSetting::Get(test_args, "xxx"), "xxx");

    for (const bool def : {false, true}) {
        BOOST_CHECK(ASettingBool::Get(test_args, def));
        BOOST_CHECK(BSettingBool::Get(test_args, def));
        BOOST_CHECK(!CccSettingBool::Get(test_args, def));
        BOOST_CHECK(!DSettingBool::Get(test_args, def));
        BOOST_CHECK(!FffSettingBool::Get(test_args, def));
        BOOST_CHECK(GggSettingBool::Get(test_args, def));
        BOOST_CHECK(!HSettingBool::Get(test_args, def));
        BOOST_CHECK(ISettingBool::Get(test_args, def));
        BOOST_CHECK(ZzzSettingBool::Get(test_args, def) == def);
        BOOST_CHECK(IiiSettingBool::Get(test_args, def) == def);
    }

    BOOST_CHECK(ASetting::Get(test_args).size() == 1
                && ASetting::Get(test_args).front() == "");
    BOOST_CHECK(BSetting::Get(test_args).size() == 1
                && BSetting::Get(test_args).front() == "1");
    BOOST_CHECK(CccSetting::Get(test_args).size() == 2
                && CccSetting::Get(test_args).front() == "argument"
                && CccSetting::Get(test_args).back() == "multiple");
    BOOST_CHECK(FffSetting::Get(test_args).size() == 0);
    BOOST_CHECK(NofffSetting::Get(test_args).size() == 0);
    BOOST_CHECK(GggSetting::Get(test_args).size() == 1
                && GggSetting::Get(test_args).front() == "1");
    BOOST_CHECK(NogggSetting::Get(test_args).size() == 0);
    BOOST_CHECK(HSetting::Get(test_args).size() == 0);
    BOOST_CHECK(NohSetting::Get(test_args).size() == 0);
    BOOST_CHECK(ISetting::Get(test_args).size() == 1
                && ISetting::Get(test_args).front() == "1");
    BOOST_CHECK(NoiSetting::Get(test_args).size() == 0);
    BOOST_CHECK(ZzzSetting::Get(test_args).size() == 0);

    BOOST_CHECK(!ASetting::Value(test_args).isFalse());
    BOOST_CHECK(!BSetting::Value(test_args).isFalse());
    BOOST_CHECK(!CccSetting::Value(test_args).isFalse());
    BOOST_CHECK(!DSetting::Value(test_args).isFalse());
    BOOST_CHECK(FffSetting::Value(test_args).isFalse());
    BOOST_CHECK(!GggSetting::Value(test_args).isFalse());
    BOOST_CHECK(HSetting::Value(test_args).isFalse()); // last setting takes precedence
    BOOST_CHECK(!ISetting::Value(test_args).isFalse()); // last setting takes precedence
    BOOST_CHECK(!ZzzSetting::Value(test_args).isFalse());

    // Test sections work
    test_args.SelectConfigNetwork("sec1");

    // same as original
    BOOST_CHECK_EQUAL(ASettingStr::Get(test_args, "xxx"), "");
    BOOST_CHECK_EQUAL(BSettingStr::Get(test_args, "xxx"), "1");
    BOOST_CHECK_EQUAL(FffSettingStr::Get(test_args, "xxx"), "0");
    BOOST_CHECK_EQUAL(GggSettingStr::Get(test_args, "xxx"), "1");
    BOOST_CHECK_EQUAL(ZzzSettingStr::Get(test_args, "xxx"), "xxx");
    BOOST_CHECK_EQUAL(IiiSetting::Get(test_args, "xxx"), "xxx");
    // d is overridden
    BOOST_CHECK(DSettingStr::Get(test_args, "xxx") == "eee");
    // section-specific setting
    BOOST_CHECK(HSettingStr::Get(test_args, "xxx") == "1");
    // section takes priority for multiple values
    BOOST_CHECK(CccSettingStr::Get(test_args, "xxx") == "extend1");
    // check multiple values works
    const std::vector<std::string> sec1_ccc_expected = {"extend1","extend2","argument","multiple"};
    const auto& sec1_ccc_res = CccSetting::Get(test_args);
    BOOST_CHECK_EQUAL_COLLECTIONS(sec1_ccc_res.begin(), sec1_ccc_res.end(), sec1_ccc_expected.begin(), sec1_ccc_expected.end());

    test_args.SelectConfigNetwork("sec2");

    // same as original
    BOOST_CHECK(ASettingStr::Get(test_args, "xxx") == "");
    BOOST_CHECK(BSettingStr::Get(test_args, "xxx") == "1");
    BOOST_CHECK(DSettingStr::Get(test_args, "xxx") == "e");
    BOOST_CHECK(FffSettingStr::Get(test_args, "xxx") == "0");
    BOOST_CHECK(GggSettingStr::Get(test_args, "xxx") == "1");
    BOOST_CHECK(ZzzSettingStr::Get(test_args, "xxx") == "xxx");
    BOOST_CHECK(HSettingStr::Get(test_args, "xxx") == "0");
    // section-specific setting
    BOOST_CHECK(IiiSetting::Get(test_args, "xxx") == "2");
    // section takes priority for multiple values
    BOOST_CHECK(CccSettingStr::Get(test_args, "xxx") == "extend3");
    // check multiple values works
    const std::vector<std::string> sec2_ccc_expected = {"extend3","argument","multiple"};
    const auto& sec2_ccc_res = CccSetting::Get(test_args);
    BOOST_CHECK_EQUAL_COLLECTIONS(sec2_ccc_res.begin(), sec2_ccc_res.end(), sec2_ccc_expected.begin(), sec2_ccc_expected.end());

    // Test section only options

    test_args.SetNetworkOnlyArg("-d");
    test_args.SetNetworkOnlyArg("-ccc");
    test_args.SetNetworkOnlyArg("-h");

    test_args.SelectConfigNetwork(ChainTypeToString(ChainType::MAIN));
    BOOST_CHECK(DSettingStr::Get(test_args, "xxx") == "e");
    BOOST_CHECK(CccSetting::Get(test_args).size() == 2);
    BOOST_CHECK(HSettingStr::Get(test_args, "xxx") == "0");

    test_args.SelectConfigNetwork("sec1");
    BOOST_CHECK(DSettingStr::Get(test_args, "xxx") == "eee");
    BOOST_CHECK(DSetting::Get(test_args).size() == 1);
    BOOST_CHECK(CccSetting::Get(test_args).size() == 2);
    BOOST_CHECK(HSettingStr::Get(test_args, "xxx") == "1");

    test_args.SelectConfigNetwork("sec2");
    BOOST_CHECK(DSettingStr::Get(test_args, "xxx") == "xxx");
    BOOST_CHECK(DSetting::Get(test_args).size() == 0);
    BOOST_CHECK(CccSetting::Get(test_args).size() == 1);
    BOOST_CHECK(HSettingStr::Get(test_args, "xxx") == "0");
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

    BOOST_CHECK_EQUAL(StrTest1Setting::Get(testArgs, "default"), "string...");
    BOOST_CHECK_EQUAL(StrTest2Setting::Get(testArgs, "default"), "default");
    BOOST_CHECK_EQUAL(IntTest1Setting::Get(testArgs), 12345);
    BOOST_CHECK_EQUAL(IntTest2Setting::Get(testArgs), 81985529216486895LL);
    BOOST_CHECK_EQUAL(IntTest3Setting::Get(testArgs), -1);
    BOOST_CHECK_EQUAL(BoolTest1Setting::Get(testArgs), true);
    BOOST_CHECK_EQUAL(BoolTest2Setting::Get(testArgs), false);
    BOOST_CHECK_EQUAL(BoolTest3Setting::Get(testArgs), false);
    BOOST_CHECK_EQUAL(BoolTest4Setting::Get(testArgs), true);

    BOOST_CHECK_EQUAL(PriTest1Setting::Get(testArgs, "default"), "b");
    BOOST_CHECK_EQUAL(PriTest2Setting::Get(testArgs, "default"), "a");
    BOOST_CHECK_EQUAL(PriTest3Setting::Get(testArgs, "default"), "a");
    BOOST_CHECK_EQUAL(PriTest4Setting::Get(testArgs, "default"), "b");
}

BOOST_AUTO_TEST_CASE(util_GetChainTypeString)
{
    TestArgsManager test_args;
    const auto testnet = std::make_pair("-testnet", ArgsManager::ALLOW_ANY);
    const auto testnet4 = std::make_pair("-testnet4", ArgsManager::ALLOW_ANY);
    const auto regtest = std::make_pair("-regtest", ArgsManager::ALLOW_ANY);
    test_args.SetupArgs({testnet, testnet4, regtest});

    const char* argv_testnet4[] = {"cmd", "-testnet4"};
    const char* argv_regtest[] = {"cmd", "-regtest"};
    const char* argv_test_no_reg[] = {"cmd", "-testnet4", "-noregtest"};
    const char* argv_both[] = {"cmd", "-testnet4", "-regtest"};

    // regtest in test network section is ignored
    const char* testnetconf = "testnet4=1\nregtest=0\n[testnet4]\nregtest=1";
    std::string error;

    BOOST_CHECK(test_args.ParseParameters(0, argv_testnet4, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "main");

    BOOST_CHECK(test_args.ParseParameters(0, argv_testnet4, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "main");

    BOOST_CHECK(test_args.ParseParameters(2, argv_testnet4, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "testnet4");

    BOOST_CHECK(test_args.ParseParameters(2, argv_regtest, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "regtest");

    BOOST_CHECK(test_args.ParseParameters(3, argv_test_no_reg, error));
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "testnet4");

    BOOST_CHECK(test_args.ParseParameters(3, argv_both, error));
    BOOST_CHECK_THROW(test_args.GetChainTypeString(), std::runtime_error);

    BOOST_CHECK(test_args.ParseParameters(0, argv_testnet4, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "testnet4");

    BOOST_CHECK(test_args.ParseParameters(2, argv_testnet4, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "testnet4");

    BOOST_CHECK(test_args.ParseParameters(2, argv_regtest, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainTypeString(), std::runtime_error);

    BOOST_CHECK(test_args.ParseParameters(3, argv_test_no_reg, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "testnet4");

    BOOST_CHECK(test_args.ParseParameters(3, argv_both, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainTypeString(), std::runtime_error);

    // check setting the network to testnet4 (and thus making
    // [testnet4] regtest=1 potentially relevant) doesn't break things
    test_args.SelectConfigNetwork("testnet4");

    BOOST_CHECK(test_args.ParseParameters(0, argv_testnet4, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "testnet4");

    BOOST_CHECK(test_args.ParseParameters(2, argv_testnet4, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "testnet4");

    BOOST_CHECK(test_args.ParseParameters(2, argv_regtest, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainTypeString(), std::runtime_error);

    BOOST_CHECK(test_args.ParseParameters(2, argv_test_no_reg, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainTypeString(), "testnet4");

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
// - Testing "main" and "testnet4" network values to make sure settings from network
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
    //   ARGS_MERGE_TEST_OUT=results.txt ./test_bitcoin --run_test=argsman_tests/util_ArgsMerge
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
        RegTestSetting::Register(parser);
        TestNet4Setting::Register(parser);

        auto arg = [](Action action) { return action == ENABLE_TEST  ? "-testnet4=1"   :
                                              action == DISABLE_TEST ? "-testnet4=0"   :
                                              action == NEGATE_TEST  ? "-notestnet4=1" :
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
    //   CHAIN_MERGE_TEST_OUT=results.txt ./test_bitcoin --run_test=argsman_tests/util_ChainMerge
    //
    // And verify diff against previous results to make sure the changes are expected.
    //
    // Results file is formatted like:
    //
    //   <input> || <output>
    BOOST_CHECK_EQUAL(out_sha_hex, "c0e33aab0c74e040ddcee9edad59e8148d8e1cacb3cccd9ea1a1f485cb6bad21");
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
