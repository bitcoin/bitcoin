// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/system.h>

#include <clientversion.h>
#include <hash.h> // For Hash()
#include <key.h> // For CKey
#include <optional.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <test/util/str.h>
#include <uint256.h>
#include <util/message.h> // For MessageSign(), MessageVerify(), MESSAGE_MAGIC
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <util/spanparsing.h>
#include <util/vector.h>

#include <array>
#include <stdint.h>
#include <thread>
#include <univalue.h>
#include <utility>
#include <vector>
#ifndef WIN32
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <boost/test/unit_test.hpp>

/* defined in logging.cpp */
namespace BCLog {
    std::string LogEscapeMessage(const std::string& str);
}

BOOST_FIXTURE_TEST_SUITE(util_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(util_datadir)
{
    ClearDatadirCache();
    const fs::path dd_norm = GetDataDir();

    gArgs.ForceSetArg("-datadir", dd_norm.string() + "/");
    ClearDatadirCache();
    BOOST_CHECK_EQUAL(dd_norm, GetDataDir());

    gArgs.ForceSetArg("-datadir", dd_norm.string() + "/.");
    ClearDatadirCache();
    BOOST_CHECK_EQUAL(dd_norm, GetDataDir());

    gArgs.ForceSetArg("-datadir", dd_norm.string() + "/./");
    ClearDatadirCache();
    BOOST_CHECK_EQUAL(dd_norm, GetDataDir());

    gArgs.ForceSetArg("-datadir", dd_norm.string() + "/.//");
    ClearDatadirCache();
    BOOST_CHECK_EQUAL(dd_norm, GetDataDir());
}

BOOST_AUTO_TEST_CASE(util_criticalsection)
{
    RecursiveMutex cs;

    do {
        LOCK(cs);
        break;

        BOOST_ERROR("break was swallowed!");
    } while(0);

    do {
        TRY_LOCK(cs, lockTest);
        if (lockTest) {
            BOOST_CHECK(true); // Needed to suppress "Test case [...] did not check any assertions"
            break;
        }

        BOOST_ERROR("break was swallowed!");
    } while(0);
}

static const unsigned char ParseHex_expected[65] = {
    0x04, 0x67, 0x8a, 0xfd, 0xb0, 0xfe, 0x55, 0x48, 0x27, 0x19, 0x67, 0xf1, 0xa6, 0x71, 0x30, 0xb7,
    0x10, 0x5c, 0xd6, 0xa8, 0x28, 0xe0, 0x39, 0x09, 0xa6, 0x79, 0x62, 0xe0, 0xea, 0x1f, 0x61, 0xde,
    0xb6, 0x49, 0xf6, 0xbc, 0x3f, 0x4c, 0xef, 0x38, 0xc4, 0xf3, 0x55, 0x04, 0xe5, 0x1e, 0xc1, 0x12,
    0xde, 0x5c, 0x38, 0x4d, 0xf7, 0xba, 0x0b, 0x8d, 0x57, 0x8a, 0x4c, 0x70, 0x2b, 0x6b, 0xf1, 0x1d,
    0x5f
};
BOOST_AUTO_TEST_CASE(util_ParseHex)
{
    std::vector<unsigned char> result;
    std::vector<unsigned char> expected(ParseHex_expected, ParseHex_expected + sizeof(ParseHex_expected));
    // Basic test vector
    result = ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f");
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());

    // Spaces between bytes must be supported
    result = ParseHex("12 34 56 78");
    BOOST_CHECK(result.size() == 4 && result[0] == 0x12 && result[1] == 0x34 && result[2] == 0x56 && result[3] == 0x78);

    // Leading space must be supported (used in BerkeleyEnvironment::Salvage)
    result = ParseHex(" 89 34 56 78");
    BOOST_CHECK(result.size() == 4 && result[0] == 0x89 && result[1] == 0x34 && result[2] == 0x56 && result[3] == 0x78);

    // Stop parsing at invalid value
    result = ParseHex("1234 invalid 1234");
    BOOST_CHECK(result.size() == 2 && result[0] == 0x12 && result[1] == 0x34);
}

BOOST_AUTO_TEST_CASE(util_HexStr)
{
    BOOST_CHECK_EQUAL(
        HexStr(ParseHex_expected, ParseHex_expected + sizeof(ParseHex_expected)),
        "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f");

    BOOST_CHECK_EQUAL(
        HexStr(ParseHex_expected + sizeof(ParseHex_expected),
               ParseHex_expected + sizeof(ParseHex_expected)),
        "");

    BOOST_CHECK_EQUAL(
        HexStr(ParseHex_expected, ParseHex_expected),
        "");

    std::vector<unsigned char> ParseHex_vec(ParseHex_expected, ParseHex_expected + 5);

    BOOST_CHECK_EQUAL(
        HexStr(ParseHex_vec.rbegin(), ParseHex_vec.rend()),
        "b0fd8a6704"
    );

    BOOST_CHECK_EQUAL(
        HexStr(std::reverse_iterator<const uint8_t *>(ParseHex_expected),
               std::reverse_iterator<const uint8_t *>(ParseHex_expected)),
        ""
    );

    BOOST_CHECK_EQUAL(
        HexStr(std::reverse_iterator<const uint8_t *>(ParseHex_expected + 1),
               std::reverse_iterator<const uint8_t *>(ParseHex_expected)),
        "04"
    );

    BOOST_CHECK_EQUAL(
        HexStr(std::reverse_iterator<const uint8_t *>(ParseHex_expected + 5),
               std::reverse_iterator<const uint8_t *>(ParseHex_expected)),
        "b0fd8a6704"
    );

    BOOST_CHECK_EQUAL(
        HexStr(std::reverse_iterator<const uint8_t *>(ParseHex_expected + 65),
               std::reverse_iterator<const uint8_t *>(ParseHex_expected)),
        "5f1df16b2b704c8a578d0bbaf74d385cde12c11ee50455f3c438ef4c3fbcf649b6de611feae06279a60939e028a8d65c10b73071a6f16719274855feb0fd8a6704"
    );
}

BOOST_AUTO_TEST_CASE(util_Join)
{
    // Normal version
    BOOST_CHECK_EQUAL(Join({}, ", "), "");
    BOOST_CHECK_EQUAL(Join({"foo"}, ", "), "foo");
    BOOST_CHECK_EQUAL(Join({"foo", "bar"}, ", "), "foo, bar");

    // Version with unary operator
    const auto op_upper = [](const std::string& s) { return ToUpper(s); };
    BOOST_CHECK_EQUAL(Join<std::string>({}, ", ", op_upper), "");
    BOOST_CHECK_EQUAL(Join<std::string>({"foo"}, ", ", op_upper), "FOO");
    BOOST_CHECK_EQUAL(Join<std::string>({"foo", "bar"}, ", ", op_upper), "FOO, BAR");
}

BOOST_AUTO_TEST_CASE(util_FormatParseISO8601DateTime)
{
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(1317425777), "2011-09-30T23:36:17Z");
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(0), "1970-01-01T00:00:00Z");

    BOOST_CHECK_EQUAL(ParseISO8601DateTime("1970-01-01T00:00:00Z"), 0);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("1960-01-01T00:00:00Z"), 0);
    BOOST_CHECK_EQUAL(ParseISO8601DateTime("2011-09-30T23:36:17Z"), 1317425777);

    auto time = GetSystemTimeInSeconds();
    BOOST_CHECK_EQUAL(ParseISO8601DateTime(FormatISO8601DateTime(time)), time);
}

BOOST_AUTO_TEST_CASE(util_FormatISO8601Date)
{
    BOOST_CHECK_EQUAL(FormatISO8601Date(1317425777), "2011-09-30");
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
        util::SettingsValue setting;
        bool default_string = false;
        bool default_int = false;
        bool default_bool = false;
        const char* string_value = nullptr;
        Optional<int64_t> int_value;
        Optional<bool> bool_value;
        Optional<std::vector<std::string>> list_value;
        const char* error = nullptr;

        Expect(util::SettingsValue s) : setting(std::move(s)) {}
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
        bool success = test.ParseParameters(arg ? 2 : 1, (char**)argv, error);

        BOOST_CHECK_EQUAL(test.GetSetting("-value").write(), expect.setting.write());
        auto settings_list = test.GetSettingsList("-value");
        if (expect.setting.isNull() || expect.setting.isFalse()) {
            BOOST_CHECK_EQUAL(settings_list.size(), 0);
        } else {
            BOOST_CHECK_EQUAL(settings_list.size(), 1);
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
            BOOST_CHECK_EQUAL(test.GetArg("-value", 99999), 99999);
        } else if (expect.int_value) {
            BOOST_CHECK_EQUAL(test.GetArg("-value", 99999), *expect.int_value);
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
    BOOST_CHECK(testArgs.ParseParameters(0, (char**)argv_test, error));
    BOOST_CHECK(testArgs.m_settings.command_line_options.empty() && testArgs.m_settings.ro_config.empty());

    BOOST_CHECK(testArgs.ParseParameters(1, (char**)argv_test, error));
    BOOST_CHECK(testArgs.m_settings.command_line_options.empty() && testArgs.m_settings.ro_config.empty());

    BOOST_CHECK(testArgs.ParseParameters(7, (char**)argv_test, error));
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
    BOOST_CHECK(test.ParseParameters(2, (char**)argv, error));
    BOOST_CHECK_EQUAL(error, "");

    argv[1] = "-unregistered";
    BOOST_CHECK(!test.ParseParameters(2, (char**)argv, error));
    BOOST_CHECK_EQUAL(error, "Invalid parameter -unregistered");

    // Make sure registered parameters prefixed with a chain name trigger errors.
    // (Previously, they were accepted and ignored.)
    argv[1] = "-test.registered";
    BOOST_CHECK(!test.ParseParameters(2, (char**)argv, error));
    BOOST_CHECK_EQUAL(error, "Invalid parameter -test.registered");
}

static void TestParse(const std::string& str, bool expected_bool, int64_t expected_int)
{
    TestArgsManager test;
    test.SetupArgs({{"-value", ArgsManager::ALLOW_ANY}});
    std::string arg = "-value=" + str;
    const char* argv[] = {"ignored", arg.c_str()};
    std::string error;
    BOOST_CHECK(test.ParseParameters(2, (char**)argv, error));
    BOOST_CHECK_EQUAL(test.GetBoolArg("-value", false), expected_bool);
    BOOST_CHECK_EQUAL(test.GetBoolArg("-value", true), expected_bool);
    BOOST_CHECK_EQUAL(test.GetArg("-value", 99998), expected_int);
    BOOST_CHECK_EQUAL(test.GetArg("-value", 99999), expected_int);
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
    BOOST_CHECK(testArgs.ParseParameters(7, (char**)argv_test, error));

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
    BOOST_CHECK(testArgs.ParseParameters(4, (char**)argv_test, error));

    // This was passed twice, second one overrides the negative setting.
    BOOST_CHECK(!testArgs.IsArgNegated("-foo"));
    BOOST_CHECK(testArgs.GetArg("-foo", "xxx") == "");

    // A double negative is a positive, and not marked as negated.
    BOOST_CHECK(!testArgs.IsArgNegated("-bar"));
    BOOST_CHECK(testArgs.GetArg("-bar", "xxx") == "1");

    // Config test
    const char *conf_test = "nofoo=1\nfoo=1\nnobar=0\n";
    BOOST_CHECK(testArgs.ParseParameters(1, (char**)argv_test, error));
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
    BOOST_CHECK(testArgs.ParseParameters(3, (char**)combo_test_args, error));
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

    BOOST_CHECK(test_args.m_settings.ro_config[""].count("a")
                && test_args.m_settings.ro_config[""].count("b")
                && test_args.m_settings.ro_config[""].count("ccc")
                && test_args.m_settings.ro_config[""].count("d")
                && test_args.m_settings.ro_config[""].count("fff")
                && test_args.m_settings.ro_config[""].count("ggg")
                && test_args.m_settings.ro_config[""].count("h")
                && test_args.m_settings.ro_config[""].count("i")
               );
    BOOST_CHECK(test_args.m_settings.ro_config["sec1"].count("ccc")
                && test_args.m_settings.ro_config["sec1"].count("h")
                && test_args.m_settings.ro_config["sec2"].count("ccc")
                && test_args.m_settings.ro_config["sec2"].count("iii")
               );

    BOOST_CHECK(test_args.IsArgSet("-a")
                && test_args.IsArgSet("-b")
                && test_args.IsArgSet("-ccc")
                && test_args.IsArgSet("-d")
                && test_args.IsArgSet("-fff")
                && test_args.IsArgSet("-ggg")
                && test_args.IsArgSet("-h")
                && test_args.IsArgSet("-i")
                && !test_args.IsArgSet("-zzz")
                && !test_args.IsArgSet("-iii")
               );

    BOOST_CHECK(test_args.GetArg("-a", "xxx") == ""
                && test_args.GetArg("-b", "xxx") == "1"
                && test_args.GetArg("-ccc", "xxx") == "argument"
                && test_args.GetArg("-d", "xxx") == "e"
                && test_args.GetArg("-fff", "xxx") == "0"
                && test_args.GetArg("-ggg", "xxx") == "1"
                && test_args.GetArg("-h", "xxx") == "0"
                && test_args.GetArg("-i", "xxx") == "1"
                && test_args.GetArg("-zzz", "xxx") == "xxx"
                && test_args.GetArg("-iii", "xxx") == "xxx"
               );

    for (const bool def : {false, true}) {
        BOOST_CHECK(test_args.GetBoolArg("-a", def)
                     && test_args.GetBoolArg("-b", def)
                     && !test_args.GetBoolArg("-ccc", def)
                     && !test_args.GetBoolArg("-d", def)
                     && !test_args.GetBoolArg("-fff", def)
                     && test_args.GetBoolArg("-ggg", def)
                     && !test_args.GetBoolArg("-h", def)
                     && test_args.GetBoolArg("-i", def)
                     && test_args.GetBoolArg("-zzz", def) == def
                     && test_args.GetBoolArg("-iii", def) == def
                   );
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
    BOOST_CHECK(test_args.GetArg("-a", "xxx") == ""
                && test_args.GetArg("-b", "xxx") == "1"
                && test_args.GetArg("-fff", "xxx") == "0"
                && test_args.GetArg("-ggg", "xxx") == "1"
                && test_args.GetArg("-zzz", "xxx") == "xxx"
                && test_args.GetArg("-iii", "xxx") == "xxx"
               );
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
    BOOST_CHECK(test_args.GetArg("-a", "xxx") == ""
                && test_args.GetArg("-b", "xxx") == "1"
                && test_args.GetArg("-d", "xxx") == "e"
                && test_args.GetArg("-fff", "xxx") == "0"
                && test_args.GetArg("-ggg", "xxx") == "1"
                && test_args.GetArg("-zzz", "xxx") == "xxx"
                && test_args.GetArg("-h", "xxx") == "0"
               );
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

    test_args.SelectConfigNetwork(CBaseChainParams::MAIN);
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
    BOOST_CHECK_EQUAL(testArgs.GetArg("inttest1", -1), 12345);
    BOOST_CHECK_EQUAL(testArgs.GetArg("inttest2", -1), 81985529216486895LL);
    BOOST_CHECK_EQUAL(testArgs.GetArg("inttest3", -1), -1);
    BOOST_CHECK_EQUAL(testArgs.GetBoolArg("booltest1", false), true);
    BOOST_CHECK_EQUAL(testArgs.GetBoolArg("booltest2", false), false);
    BOOST_CHECK_EQUAL(testArgs.GetBoolArg("booltest3", false), false);
    BOOST_CHECK_EQUAL(testArgs.GetBoolArg("booltest4", false), true);

    BOOST_CHECK_EQUAL(testArgs.GetArg("pritest1", "default"), "b");
    BOOST_CHECK_EQUAL(testArgs.GetArg("pritest2", "default"), "a");
    BOOST_CHECK_EQUAL(testArgs.GetArg("pritest3", "default"), "a");
    BOOST_CHECK_EQUAL(testArgs.GetArg("pritest4", "default"), "b");
}

BOOST_AUTO_TEST_CASE(util_GetChainName)
{
    TestArgsManager test_args;
    const auto testnet = std::make_pair("-testnet", ArgsManager::ALLOW_ANY);
    const auto regtest = std::make_pair("-regtest", ArgsManager::ALLOW_ANY);
    test_args.SetupArgs({testnet, regtest});

    const char* argv_testnet[] = {"cmd", "-testnet"};
    const char* argv_regtest[] = {"cmd", "-regtest"};
    const char* argv_test_no_reg[] = {"cmd", "-testnet", "-noregtest"};
    const char* argv_both[] = {"cmd", "-testnet", "-regtest"};

    // equivalent to "-testnet"
    // regtest in testnet section is ignored
    const char* testnetconf = "testnet=1\nregtest=0\n[test]\nregtest=1";
    std::string error;

    BOOST_CHECK(test_args.ParseParameters(0, (char**)argv_testnet, error));
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "main");

    BOOST_CHECK(test_args.ParseParameters(2, (char**)argv_testnet, error));
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "test");

    BOOST_CHECK(test_args.ParseParameters(2, (char**)argv_regtest, error));
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "regtest");

    BOOST_CHECK(test_args.ParseParameters(3, (char**)argv_test_no_reg, error));
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "test");

    BOOST_CHECK(test_args.ParseParameters(3, (char**)argv_both, error));
    BOOST_CHECK_THROW(test_args.GetChainName(), std::runtime_error);

    BOOST_CHECK(test_args.ParseParameters(0, (char**)argv_testnet, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "test");

    BOOST_CHECK(test_args.ParseParameters(2, (char**)argv_testnet, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "test");

    BOOST_CHECK(test_args.ParseParameters(2, (char**)argv_regtest, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainName(), std::runtime_error);

    BOOST_CHECK(test_args.ParseParameters(3, (char**)argv_test_no_reg, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "test");

    BOOST_CHECK(test_args.ParseParameters(3, (char**)argv_both, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainName(), std::runtime_error);

    // check setting the network to test (and thus making
    // [test] regtest=1 potentially relevant) doesn't break things
    test_args.SelectConfigNetwork("test");

    BOOST_CHECK(test_args.ParseParameters(0, (char**)argv_testnet, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "test");

    BOOST_CHECK(test_args.ParseParameters(2, (char**)argv_testnet, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "test");

    BOOST_CHECK(test_args.ParseParameters(2, (char**)argv_regtest, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainName(), std::runtime_error);

    BOOST_CHECK(test_args.ParseParameters(2, (char**)argv_test_no_reg, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_EQUAL(test_args.GetChainName(), "test");

    BOOST_CHECK(test_args.ParseParameters(3, (char**)argv_both, error));
    test_args.ReadConfigString(testnetconf);
    BOOST_CHECK_THROW(test_args.GetChainName(), std::runtime_error);
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
                        for (const std::string& section : {CBaseChainParams::MAIN, CBaseChainParams::TESTNET}) {
                            for (const std::string& network : {CBaseChainParams::MAIN, CBaseChainParams::TESTNET}) {
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

        out_sha.Write((const unsigned char*)desc.data(), desc.size());
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
    std::string out_sha_hex = HexStr(std::begin(out_sha_bytes), std::end(out_sha_bytes));

    // If check below fails, should manually dump the results with:
    //
    //   ARGS_MERGE_TEST_OUT=results.txt ./test_bitcoin --run_test=util_tests/util_ArgsMerge
    //
    // And verify diff against previous results to make sure the changes are expected.
    //
    // Results file is formatted like:
    //
    //   <input> || <IsArgSet/IsArgNegated/GetArg output> | <GetArgs output> | <GetUnsuitable output>
    BOOST_CHECK_EQUAL(out_sha_hex, "8fd4877bb8bf337badca950ede6c917441901962f160e52514e06a60dea46cde");
}

// Similar test as above, but for ArgsManager::GetChainName function.
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
            desc += parser.GetChainName();
        } catch (const std::runtime_error& e) {
            desc += "error: ";
            desc += e.what();
        }
        desc += "\n";

        out_sha.Write((const unsigned char*)desc.data(), desc.size());
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
    std::string out_sha_hex = HexStr(std::begin(out_sha_bytes), std::end(out_sha_bytes));

    // If check below fails, should manually dump the results with:
    //
    //   CHAIN_MERGE_TEST_OUT=results.txt ./test_bitcoin --run_test=util_tests/util_ChainMerge
    //
    // And verify diff against previous results to make sure the changes are expected.
    //
    // Results file is formatted like:
    //
    //   <input> || <output>
    BOOST_CHECK_EQUAL(out_sha_hex, "f0b3a3c29869edc765d579c928f7f1690a71fbb673b49ccf39cbc4de18156a0d");
}

BOOST_AUTO_TEST_CASE(util_FormatMoney)
{
    BOOST_CHECK_EQUAL(FormatMoney(0), "0.00");
    BOOST_CHECK_EQUAL(FormatMoney((COIN/10000)*123456789), "12345.6789");
    BOOST_CHECK_EQUAL(FormatMoney(-COIN), "-1.00");

    BOOST_CHECK_EQUAL(FormatMoney(COIN*100000000), "100000000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*10000000), "10000000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*1000000), "1000000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*100000), "100000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*10000), "10000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*1000), "1000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*100), "100.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*10), "10.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN), "1.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/10), "0.10");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/100), "0.01");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/1000), "0.001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/10000), "0.0001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/100000), "0.00001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/1000000), "0.000001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/10000000), "0.0000001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/100000000), "0.00000001");
}

BOOST_AUTO_TEST_CASE(util_ParseMoney)
{
    CAmount ret = 0;
    BOOST_CHECK(ParseMoney("0.0", ret));
    BOOST_CHECK_EQUAL(ret, 0);

    BOOST_CHECK(ParseMoney("12345.6789", ret));
    BOOST_CHECK_EQUAL(ret, (COIN/10000)*123456789);

    BOOST_CHECK(ParseMoney("100000000.00", ret));
    BOOST_CHECK_EQUAL(ret, COIN*100000000);
    BOOST_CHECK(ParseMoney("10000000.00", ret));
    BOOST_CHECK_EQUAL(ret, COIN*10000000);
    BOOST_CHECK(ParseMoney("1000000.00", ret));
    BOOST_CHECK_EQUAL(ret, COIN*1000000);
    BOOST_CHECK(ParseMoney("100000.00", ret));
    BOOST_CHECK_EQUAL(ret, COIN*100000);
    BOOST_CHECK(ParseMoney("10000.00", ret));
    BOOST_CHECK_EQUAL(ret, COIN*10000);
    BOOST_CHECK(ParseMoney("1000.00", ret));
    BOOST_CHECK_EQUAL(ret, COIN*1000);
    BOOST_CHECK(ParseMoney("100.00", ret));
    BOOST_CHECK_EQUAL(ret, COIN*100);
    BOOST_CHECK(ParseMoney("10.00", ret));
    BOOST_CHECK_EQUAL(ret, COIN*10);
    BOOST_CHECK(ParseMoney("1.00", ret));
    BOOST_CHECK_EQUAL(ret, COIN);
    BOOST_CHECK(ParseMoney("1", ret));
    BOOST_CHECK_EQUAL(ret, COIN);
    BOOST_CHECK(ParseMoney("   1", ret));
    BOOST_CHECK_EQUAL(ret, COIN);
    BOOST_CHECK(ParseMoney("1   ", ret));
    BOOST_CHECK_EQUAL(ret, COIN);
    BOOST_CHECK(ParseMoney("  1 ", ret));
    BOOST_CHECK_EQUAL(ret, COIN);
    BOOST_CHECK(ParseMoney("0.1", ret));
    BOOST_CHECK_EQUAL(ret, COIN/10);
    BOOST_CHECK(ParseMoney("0.01", ret));
    BOOST_CHECK_EQUAL(ret, COIN/100);
    BOOST_CHECK(ParseMoney("0.001", ret));
    BOOST_CHECK_EQUAL(ret, COIN/1000);
    BOOST_CHECK(ParseMoney("0.0001", ret));
    BOOST_CHECK_EQUAL(ret, COIN/10000);
    BOOST_CHECK(ParseMoney("0.00001", ret));
    BOOST_CHECK_EQUAL(ret, COIN/100000);
    BOOST_CHECK(ParseMoney("0.000001", ret));
    BOOST_CHECK_EQUAL(ret, COIN/1000000);
    BOOST_CHECK(ParseMoney("0.0000001", ret));
    BOOST_CHECK_EQUAL(ret, COIN/10000000);
    BOOST_CHECK(ParseMoney("0.00000001", ret));
    BOOST_CHECK_EQUAL(ret, COIN/100000000);
    BOOST_CHECK(ParseMoney(" 0.00000001 ", ret));
    BOOST_CHECK_EQUAL(ret, COIN/100000000);
    BOOST_CHECK(ParseMoney("0.00000001 ", ret));
    BOOST_CHECK_EQUAL(ret, COIN/100000000);
    BOOST_CHECK(ParseMoney(" 0.00000001", ret));
    BOOST_CHECK_EQUAL(ret, COIN/100000000);

    // Parsing amount that can not be represented in ret should fail
    BOOST_CHECK(!ParseMoney("0.000000001", ret));

    // Parsing empty string should fail
    BOOST_CHECK(!ParseMoney("", ret));
    BOOST_CHECK(!ParseMoney(" ", ret));
    BOOST_CHECK(!ParseMoney("  ", ret));

    // Parsing two numbers should fail
    BOOST_CHECK(!ParseMoney("1 2", ret));
    BOOST_CHECK(!ParseMoney(" 1 2 ", ret));
    BOOST_CHECK(!ParseMoney(" 1.2 3 ", ret));
    BOOST_CHECK(!ParseMoney(" 1 2.3 ", ret));

    // Attempted 63 bit overflow should fail
    BOOST_CHECK(!ParseMoney("92233720368.54775808", ret));

    // Parsing negative amounts must fail
    BOOST_CHECK(!ParseMoney("-1", ret));

    // Parsing strings with embedded NUL characters should fail
    BOOST_CHECK(!ParseMoney(std::string("\0-1", 3), ret));
    BOOST_CHECK(!ParseMoney(std::string("\01", 2), ret));
    BOOST_CHECK(!ParseMoney(std::string("1\0", 2), ret));
}

BOOST_AUTO_TEST_CASE(util_IsHex)
{
    BOOST_CHECK(IsHex("00"));
    BOOST_CHECK(IsHex("00112233445566778899aabbccddeeffAABBCCDDEEFF"));
    BOOST_CHECK(IsHex("ff"));
    BOOST_CHECK(IsHex("FF"));

    BOOST_CHECK(!IsHex(""));
    BOOST_CHECK(!IsHex("0"));
    BOOST_CHECK(!IsHex("a"));
    BOOST_CHECK(!IsHex("eleven"));
    BOOST_CHECK(!IsHex("00xx00"));
    BOOST_CHECK(!IsHex("0x0000"));
}

BOOST_AUTO_TEST_CASE(util_IsHexNumber)
{
    BOOST_CHECK(IsHexNumber("0x0"));
    BOOST_CHECK(IsHexNumber("0"));
    BOOST_CHECK(IsHexNumber("0x10"));
    BOOST_CHECK(IsHexNumber("10"));
    BOOST_CHECK(IsHexNumber("0xff"));
    BOOST_CHECK(IsHexNumber("ff"));
    BOOST_CHECK(IsHexNumber("0xFfa"));
    BOOST_CHECK(IsHexNumber("Ffa"));
    BOOST_CHECK(IsHexNumber("0x00112233445566778899aabbccddeeffAABBCCDDEEFF"));
    BOOST_CHECK(IsHexNumber("00112233445566778899aabbccddeeffAABBCCDDEEFF"));

    BOOST_CHECK(!IsHexNumber(""));   // empty string not allowed
    BOOST_CHECK(!IsHexNumber("0x")); // empty string after prefix not allowed
    BOOST_CHECK(!IsHexNumber("0x0 ")); // no spaces at end,
    BOOST_CHECK(!IsHexNumber(" 0x0")); // or beginning,
    BOOST_CHECK(!IsHexNumber("0x 0")); // or middle,
    BOOST_CHECK(!IsHexNumber(" "));    // etc.
    BOOST_CHECK(!IsHexNumber("0x0ga")); // invalid character
    BOOST_CHECK(!IsHexNumber("x0"));    // broken prefix
    BOOST_CHECK(!IsHexNumber("0x0x00")); // two prefixes not allowed

}

BOOST_AUTO_TEST_CASE(util_seed_insecure_rand)
{
    SeedInsecureRand(SeedRand::ZEROS);
    for (int mod=2;mod<11;mod++)
    {
        int mask = 1;
        // Really rough binomial confidence approximation.
        int err = 30*10000./mod*sqrt((1./mod*(1-1./mod))/10000.);
        //mask is 2^ceil(log2(mod))-1
        while(mask<mod-1)mask=(mask<<1)+1;

        int count = 0;
        //How often does it get a zero from the uniform range [0,mod)?
        for (int i = 0; i < 10000; i++) {
            uint32_t rval;
            do{
                rval=InsecureRand32()&mask;
            }while(rval>=(uint32_t)mod);
            count += rval==0;
        }
        BOOST_CHECK(count<=10000/mod+err);
        BOOST_CHECK(count>=10000/mod-err);
    }
}

BOOST_AUTO_TEST_CASE(util_TimingResistantEqual)
{
    BOOST_CHECK(TimingResistantEqual(std::string(""), std::string("")));
    BOOST_CHECK(!TimingResistantEqual(std::string("abc"), std::string("")));
    BOOST_CHECK(!TimingResistantEqual(std::string(""), std::string("abc")));
    BOOST_CHECK(!TimingResistantEqual(std::string("a"), std::string("aa")));
    BOOST_CHECK(!TimingResistantEqual(std::string("aa"), std::string("a")));
    BOOST_CHECK(TimingResistantEqual(std::string("abc"), std::string("abc")));
    BOOST_CHECK(!TimingResistantEqual(std::string("abc"), std::string("aba")));
}

/* Test strprintf formatting directives.
 * Put a string before and after to ensure sanity of element sizes on stack. */
#define B "check_prefix"
#define E "check_postfix"
BOOST_AUTO_TEST_CASE(strprintf_numbers)
{
    int64_t s64t = -9223372036854775807LL; /* signed 64 bit test value */
    uint64_t u64t = 18446744073709551615ULL; /* unsigned 64 bit test value */
    BOOST_CHECK(strprintf("%s %d %s", B, s64t, E) == B" -9223372036854775807 " E);
    BOOST_CHECK(strprintf("%s %u %s", B, u64t, E) == B" 18446744073709551615 " E);
    BOOST_CHECK(strprintf("%s %x %s", B, u64t, E) == B" ffffffffffffffff " E);

    size_t st = 12345678; /* unsigned size_t test value */
    ssize_t sst = -12345678; /* signed size_t test value */
    BOOST_CHECK(strprintf("%s %d %s", B, sst, E) == B" -12345678 " E);
    BOOST_CHECK(strprintf("%s %u %s", B, st, E) == B" 12345678 " E);
    BOOST_CHECK(strprintf("%s %x %s", B, st, E) == B" bc614e " E);

    ptrdiff_t pt = 87654321; /* positive ptrdiff_t test value */
    ptrdiff_t spt = -87654321; /* negative ptrdiff_t test value */
    BOOST_CHECK(strprintf("%s %d %s", B, spt, E) == B" -87654321 " E);
    BOOST_CHECK(strprintf("%s %u %s", B, pt, E) == B" 87654321 " E);
    BOOST_CHECK(strprintf("%s %x %s", B, pt, E) == B" 5397fb1 " E);
}
#undef B
#undef E

/* Check for mingw/wine issue #3494
 * Remove this test before time.ctime(0xffffffff) == 'Sun Feb  7 07:28:15 2106'
 */
BOOST_AUTO_TEST_CASE(gettime)
{
    BOOST_CHECK((GetTime() & ~0xFFFFFFFFLL) == 0);
}

BOOST_AUTO_TEST_CASE(util_time_GetTime)
{
    SetMockTime(111);
    // Check that mock time does not change after a sleep
    for (const auto& num_sleep : {0, 1}) {
        UninterruptibleSleep(std::chrono::milliseconds{num_sleep});
        BOOST_CHECK_EQUAL(111, GetTime()); // Deprecated time getter
        BOOST_CHECK_EQUAL(111, GetTime<std::chrono::seconds>().count());
        BOOST_CHECK_EQUAL(111000, GetTime<std::chrono::milliseconds>().count());
        BOOST_CHECK_EQUAL(111000000, GetTime<std::chrono::microseconds>().count());
    }

    SetMockTime(0);
    // Check that system time changes after a sleep
    const auto ms_0 = GetTime<std::chrono::milliseconds>();
    const auto us_0 = GetTime<std::chrono::microseconds>();
    UninterruptibleSleep(std::chrono::milliseconds{1});
    BOOST_CHECK(ms_0 < GetTime<std::chrono::milliseconds>());
    BOOST_CHECK(us_0 < GetTime<std::chrono::microseconds>());
}

BOOST_AUTO_TEST_CASE(test_IsDigit)
{
    BOOST_CHECK_EQUAL(IsDigit('0'), true);
    BOOST_CHECK_EQUAL(IsDigit('1'), true);
    BOOST_CHECK_EQUAL(IsDigit('8'), true);
    BOOST_CHECK_EQUAL(IsDigit('9'), true);

    BOOST_CHECK_EQUAL(IsDigit('0' - 1), false);
    BOOST_CHECK_EQUAL(IsDigit('9' + 1), false);
    BOOST_CHECK_EQUAL(IsDigit(0), false);
    BOOST_CHECK_EQUAL(IsDigit(1), false);
    BOOST_CHECK_EQUAL(IsDigit(8), false);
    BOOST_CHECK_EQUAL(IsDigit(9), false);
}

BOOST_AUTO_TEST_CASE(test_ParseInt32)
{
    int32_t n;
    // Valid values
    BOOST_CHECK(ParseInt32("1234", nullptr));
    BOOST_CHECK(ParseInt32("0", &n) && n == 0);
    BOOST_CHECK(ParseInt32("1234", &n) && n == 1234);
    BOOST_CHECK(ParseInt32("01234", &n) && n == 1234); // no octal
    BOOST_CHECK(ParseInt32("2147483647", &n) && n == 2147483647);
    BOOST_CHECK(ParseInt32("-2147483648", &n) && n == (-2147483647 - 1)); // (-2147483647 - 1) equals INT_MIN
    BOOST_CHECK(ParseInt32("-1234", &n) && n == -1234);
    // Invalid values
    BOOST_CHECK(!ParseInt32("", &n));
    BOOST_CHECK(!ParseInt32(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseInt32("1 ", &n));
    BOOST_CHECK(!ParseInt32("1a", &n));
    BOOST_CHECK(!ParseInt32("aap", &n));
    BOOST_CHECK(!ParseInt32("0x1", &n)); // no hex
    BOOST_CHECK(!ParseInt32("0x1", &n)); // no hex
    const char test_bytes[] = {'1', 0, '1'};
    std::string teststr(test_bytes, sizeof(test_bytes));
    BOOST_CHECK(!ParseInt32(teststr, &n)); // no embedded NULs
    // Overflow and underflow
    BOOST_CHECK(!ParseInt32("-2147483649", nullptr));
    BOOST_CHECK(!ParseInt32("2147483648", nullptr));
    BOOST_CHECK(!ParseInt32("-32482348723847471234", nullptr));
    BOOST_CHECK(!ParseInt32("32482348723847471234", nullptr));
}

BOOST_AUTO_TEST_CASE(test_ParseInt64)
{
    int64_t n;
    // Valid values
    BOOST_CHECK(ParseInt64("1234", nullptr));
    BOOST_CHECK(ParseInt64("0", &n) && n == 0LL);
    BOOST_CHECK(ParseInt64("1234", &n) && n == 1234LL);
    BOOST_CHECK(ParseInt64("01234", &n) && n == 1234LL); // no octal
    BOOST_CHECK(ParseInt64("2147483647", &n) && n == 2147483647LL);
    BOOST_CHECK(ParseInt64("-2147483648", &n) && n == -2147483648LL);
    BOOST_CHECK(ParseInt64("9223372036854775807", &n) && n == (int64_t)9223372036854775807);
    BOOST_CHECK(ParseInt64("-9223372036854775808", &n) && n == (int64_t)-9223372036854775807-1);
    BOOST_CHECK(ParseInt64("-1234", &n) && n == -1234LL);
    // Invalid values
    BOOST_CHECK(!ParseInt64("", &n));
    BOOST_CHECK(!ParseInt64(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseInt64("1 ", &n));
    BOOST_CHECK(!ParseInt64("1a", &n));
    BOOST_CHECK(!ParseInt64("aap", &n));
    BOOST_CHECK(!ParseInt64("0x1", &n)); // no hex
    const char test_bytes[] = {'1', 0, '1'};
    std::string teststr(test_bytes, sizeof(test_bytes));
    BOOST_CHECK(!ParseInt64(teststr, &n)); // no embedded NULs
    // Overflow and underflow
    BOOST_CHECK(!ParseInt64("-9223372036854775809", nullptr));
    BOOST_CHECK(!ParseInt64("9223372036854775808", nullptr));
    BOOST_CHECK(!ParseInt64("-32482348723847471234", nullptr));
    BOOST_CHECK(!ParseInt64("32482348723847471234", nullptr));
}

BOOST_AUTO_TEST_CASE(test_ParseUInt32)
{
    uint32_t n;
    // Valid values
    BOOST_CHECK(ParseUInt32("1234", nullptr));
    BOOST_CHECK(ParseUInt32("0", &n) && n == 0);
    BOOST_CHECK(ParseUInt32("1234", &n) && n == 1234);
    BOOST_CHECK(ParseUInt32("01234", &n) && n == 1234); // no octal
    BOOST_CHECK(ParseUInt32("2147483647", &n) && n == 2147483647);
    BOOST_CHECK(ParseUInt32("2147483648", &n) && n == (uint32_t)2147483648);
    BOOST_CHECK(ParseUInt32("4294967295", &n) && n == (uint32_t)4294967295);
    // Invalid values
    BOOST_CHECK(!ParseUInt32("", &n));
    BOOST_CHECK(!ParseUInt32(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseUInt32(" -1", &n));
    BOOST_CHECK(!ParseUInt32("1 ", &n));
    BOOST_CHECK(!ParseUInt32("1a", &n));
    BOOST_CHECK(!ParseUInt32("aap", &n));
    BOOST_CHECK(!ParseUInt32("0x1", &n)); // no hex
    BOOST_CHECK(!ParseUInt32("0x1", &n)); // no hex
    const char test_bytes[] = {'1', 0, '1'};
    std::string teststr(test_bytes, sizeof(test_bytes));
    BOOST_CHECK(!ParseUInt32(teststr, &n)); // no embedded NULs
    // Overflow and underflow
    BOOST_CHECK(!ParseUInt32("-2147483648", &n));
    BOOST_CHECK(!ParseUInt32("4294967296", &n));
    BOOST_CHECK(!ParseUInt32("-1234", &n));
    BOOST_CHECK(!ParseUInt32("-32482348723847471234", nullptr));
    BOOST_CHECK(!ParseUInt32("32482348723847471234", nullptr));
}

BOOST_AUTO_TEST_CASE(test_ParseUInt64)
{
    uint64_t n;
    // Valid values
    BOOST_CHECK(ParseUInt64("1234", nullptr));
    BOOST_CHECK(ParseUInt64("0", &n) && n == 0LL);
    BOOST_CHECK(ParseUInt64("1234", &n) && n == 1234LL);
    BOOST_CHECK(ParseUInt64("01234", &n) && n == 1234LL); // no octal
    BOOST_CHECK(ParseUInt64("2147483647", &n) && n == 2147483647LL);
    BOOST_CHECK(ParseUInt64("9223372036854775807", &n) && n == 9223372036854775807ULL);
    BOOST_CHECK(ParseUInt64("9223372036854775808", &n) && n == 9223372036854775808ULL);
    BOOST_CHECK(ParseUInt64("18446744073709551615", &n) && n == 18446744073709551615ULL);
    // Invalid values
    BOOST_CHECK(!ParseUInt64("", &n));
    BOOST_CHECK(!ParseUInt64(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseUInt64(" -1", &n));
    BOOST_CHECK(!ParseUInt64("1 ", &n));
    BOOST_CHECK(!ParseUInt64("1a", &n));
    BOOST_CHECK(!ParseUInt64("aap", &n));
    BOOST_CHECK(!ParseUInt64("0x1", &n)); // no hex
    const char test_bytes[] = {'1', 0, '1'};
    std::string teststr(test_bytes, sizeof(test_bytes));
    BOOST_CHECK(!ParseUInt64(teststr, &n)); // no embedded NULs
    // Overflow and underflow
    BOOST_CHECK(!ParseUInt64("-9223372036854775809", nullptr));
    BOOST_CHECK(!ParseUInt64("18446744073709551616", nullptr));
    BOOST_CHECK(!ParseUInt64("-32482348723847471234", nullptr));
    BOOST_CHECK(!ParseUInt64("-2147483648", &n));
    BOOST_CHECK(!ParseUInt64("-9223372036854775808", &n));
    BOOST_CHECK(!ParseUInt64("-1234", &n));
}

BOOST_AUTO_TEST_CASE(test_ParseDouble)
{
    double n;
    // Valid values
    BOOST_CHECK(ParseDouble("1234", nullptr));
    BOOST_CHECK(ParseDouble("0", &n) && n == 0.0);
    BOOST_CHECK(ParseDouble("1234", &n) && n == 1234.0);
    BOOST_CHECK(ParseDouble("01234", &n) && n == 1234.0); // no octal
    BOOST_CHECK(ParseDouble("2147483647", &n) && n == 2147483647.0);
    BOOST_CHECK(ParseDouble("-2147483648", &n) && n == -2147483648.0);
    BOOST_CHECK(ParseDouble("-1234", &n) && n == -1234.0);
    BOOST_CHECK(ParseDouble("1e6", &n) && n == 1e6);
    BOOST_CHECK(ParseDouble("-1e6", &n) && n == -1e6);
    // Invalid values
    BOOST_CHECK(!ParseDouble("", &n));
    BOOST_CHECK(!ParseDouble(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseDouble("1 ", &n));
    BOOST_CHECK(!ParseDouble("1a", &n));
    BOOST_CHECK(!ParseDouble("aap", &n));
    BOOST_CHECK(!ParseDouble("0x1", &n)); // no hex
    const char test_bytes[] = {'1', 0, '1'};
    std::string teststr(test_bytes, sizeof(test_bytes));
    BOOST_CHECK(!ParseDouble(teststr, &n)); // no embedded NULs
    // Overflow and underflow
    BOOST_CHECK(!ParseDouble("-1e10000", nullptr));
    BOOST_CHECK(!ParseDouble("1e10000", nullptr));
}

BOOST_AUTO_TEST_CASE(test_FormatParagraph)
{
    BOOST_CHECK_EQUAL(FormatParagraph("", 79, 0), "");
    BOOST_CHECK_EQUAL(FormatParagraph("test", 79, 0), "test");
    BOOST_CHECK_EQUAL(FormatParagraph(" test", 79, 0), " test");
    BOOST_CHECK_EQUAL(FormatParagraph("test test", 79, 0), "test test");
    BOOST_CHECK_EQUAL(FormatParagraph("test test", 4, 0), "test\ntest");
    BOOST_CHECK_EQUAL(FormatParagraph("testerde test", 4, 0), "testerde\ntest");
    BOOST_CHECK_EQUAL(FormatParagraph("test test", 4, 4), "test\n    test");

    // Make sure we don't indent a fully-new line following a too-long line ending
    BOOST_CHECK_EQUAL(FormatParagraph("test test\nabc", 4, 4), "test\n    test\nabc");

    BOOST_CHECK_EQUAL(FormatParagraph("This_is_a_very_long_test_string_without_any_spaces_so_it_should_just_get_returned_as_is_despite_the_length until it gets here", 79), "This_is_a_very_long_test_string_without_any_spaces_so_it_should_just_get_returned_as_is_despite_the_length\nuntil it gets here");

    // Test wrap length is exact
    BOOST_CHECK_EQUAL(FormatParagraph("a b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de f g h i j k l m n o p", 79), "a b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de\nf g h i j k l m n o p");
    BOOST_CHECK_EQUAL(FormatParagraph("x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de f g h i j k l m n o p", 79), "x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de\nf g h i j k l m n o p");
    // Indent should be included in length of lines
    BOOST_CHECK_EQUAL(FormatParagraph("x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9 a b c d e fg h i j k", 79, 4), "x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de\n    f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9 a b c d e fg\n    h i j k");

    BOOST_CHECK_EQUAL(FormatParagraph("This is a very long test string. This is a second sentence in the very long test string.", 79), "This is a very long test string. This is a second sentence in the very long\ntest string.");
    BOOST_CHECK_EQUAL(FormatParagraph("This is a very long test string.\nThis is a second sentence in the very long test string. This is a third sentence in the very long test string.", 79), "This is a very long test string.\nThis is a second sentence in the very long test string. This is a third\nsentence in the very long test string.");
    BOOST_CHECK_EQUAL(FormatParagraph("This is a very long test string.\n\nThis is a second sentence in the very long test string. This is a third sentence in the very long test string.", 79), "This is a very long test string.\n\nThis is a second sentence in the very long test string. This is a third\nsentence in the very long test string.");
    BOOST_CHECK_EQUAL(FormatParagraph("Testing that normal newlines do not get indented.\nLike here.", 79), "Testing that normal newlines do not get indented.\nLike here.");
}

BOOST_AUTO_TEST_CASE(test_FormatSubVersion)
{
    std::vector<std::string> comments;
    comments.push_back(std::string("comment1"));
    std::vector<std::string> comments2;
    comments2.push_back(std::string("comment1"));
    comments2.push_back(SanitizeString(std::string("Comment2; .,_?@-; !\"#$%&'()*+/<=>[]\\^`{|}~"), SAFE_CHARS_UA_COMMENT)); // Semicolon is discouraged but not forbidden by BIP-0014
    BOOST_CHECK_EQUAL(FormatSubVersion("Test", 99900, std::vector<std::string>()),std::string("/Test:0.9.99/"));
    BOOST_CHECK_EQUAL(FormatSubVersion("Test", 99900, comments),std::string("/Test:0.9.99(comment1)/"));
    BOOST_CHECK_EQUAL(FormatSubVersion("Test", 99900, comments2),std::string("/Test:0.9.99(comment1; Comment2; .,_?@-; )/"));
}

BOOST_AUTO_TEST_CASE(test_ParseFixedPoint)
{
    int64_t amount = 0;
    BOOST_CHECK(ParseFixedPoint("0", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 0LL);
    BOOST_CHECK(ParseFixedPoint("1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 100000000LL);
    BOOST_CHECK(ParseFixedPoint("0.0", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 0LL);
    BOOST_CHECK(ParseFixedPoint("-0.1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, -10000000LL);
    BOOST_CHECK(ParseFixedPoint("1.1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 110000000LL);
    BOOST_CHECK(ParseFixedPoint("1.10000000000000000", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 110000000LL);
    BOOST_CHECK(ParseFixedPoint("1.1e1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 1100000000LL);
    BOOST_CHECK(ParseFixedPoint("1.1e-1", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 11000000LL);
    BOOST_CHECK(ParseFixedPoint("1000", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 100000000000LL);
    BOOST_CHECK(ParseFixedPoint("-1000", 8, &amount));
    BOOST_CHECK_EQUAL(amount, -100000000000LL);
    BOOST_CHECK(ParseFixedPoint("0.00000001", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 1LL);
    BOOST_CHECK(ParseFixedPoint("0.0000000100000000", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 1LL);
    BOOST_CHECK(ParseFixedPoint("-0.00000001", 8, &amount));
    BOOST_CHECK_EQUAL(amount, -1LL);
    BOOST_CHECK(ParseFixedPoint("1000000000.00000001", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 100000000000000001LL);
    BOOST_CHECK(ParseFixedPoint("9999999999.99999999", 8, &amount));
    BOOST_CHECK_EQUAL(amount, 999999999999999999LL);
    BOOST_CHECK(ParseFixedPoint("-9999999999.99999999", 8, &amount));
    BOOST_CHECK_EQUAL(amount, -999999999999999999LL);

    BOOST_CHECK(!ParseFixedPoint("", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("a-1000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-a1000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-1000a", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-01000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("00.1", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint(".1", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("--0.1", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("0.000000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-0.000000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("0.00000001000000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-10000000000.00000000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("10000000000.00000000", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-10000000000.00000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("10000000000.00000001", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-10000000000.00000009", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("10000000000.00000009", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-99999999999.99999999", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("99999909999.09999999", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("92233720368.54775807", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("92233720368.54775808", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-92233720368.54775808", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("-92233720368.54775809", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("1.1e", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("1.1e-", 8, &amount));
    BOOST_CHECK(!ParseFixedPoint("1.", 8, &amount));
}

static void TestOtherThread(fs::path dirname, std::string lockname, bool *result)
{
    *result = LockDirectory(dirname, lockname);
}

#ifndef WIN32 // Cannot do this test on WIN32 due to lack of fork()
static constexpr char LockCommand = 'L';
static constexpr char UnlockCommand = 'U';
static constexpr char ExitCommand = 'X';

static void TestOtherProcess(fs::path dirname, std::string lockname, int fd)
{
    char ch;
    while (true) {
        int rv = read(fd, &ch, 1); // Wait for command
        assert(rv == 1);
        switch(ch) {
        case LockCommand:
            ch = LockDirectory(dirname, lockname);
            rv = write(fd, &ch, 1);
            assert(rv == 1);
            break;
        case UnlockCommand:
            ReleaseDirectoryLocks();
            ch = true; // Always succeeds
            rv = write(fd, &ch, 1);
            assert(rv == 1);
            break;
        case ExitCommand:
            close(fd);
            exit(0);
        default:
            assert(0);
        }
    }
}
#endif

BOOST_AUTO_TEST_CASE(test_LockDirectory)
{
    fs::path dirname = GetDataDir() / "lock_dir";
    const std::string lockname = ".lock";
#ifndef WIN32
    // Revert SIGCHLD to default, otherwise boost.test will catch and fail on
    // it: there is BOOST_TEST_IGNORE_SIGCHLD but that only works when defined
    // at build-time of the boost library
    void (*old_handler)(int) = signal(SIGCHLD, SIG_DFL);

    // Fork another process for testing before creating the lock, so that we
    // won't fork while holding the lock (which might be undefined, and is not
    // relevant as test case as that is avoided with -daemonize).
    int fd[2];
    BOOST_CHECK_EQUAL(socketpair(AF_UNIX, SOCK_STREAM, 0, fd), 0);
    pid_t pid = fork();
    if (!pid) {
        BOOST_CHECK_EQUAL(close(fd[1]), 0); // Child: close parent end
        TestOtherProcess(dirname, lockname, fd[0]);
    }
    BOOST_CHECK_EQUAL(close(fd[0]), 0); // Parent: close child end
#endif
    // Lock on non-existent directory should fail
    BOOST_CHECK_EQUAL(LockDirectory(dirname, lockname), false);

    fs::create_directories(dirname);

    // Probing lock on new directory should succeed
    BOOST_CHECK_EQUAL(LockDirectory(dirname, lockname, true), true);

    // Persistent lock on new directory should succeed
    BOOST_CHECK_EQUAL(LockDirectory(dirname, lockname), true);

    // Another lock on the directory from the same thread should succeed
    BOOST_CHECK_EQUAL(LockDirectory(dirname, lockname), true);

    // Another lock on the directory from a different thread within the same process should succeed
    bool threadresult;
    std::thread thr(TestOtherThread, dirname, lockname, &threadresult);
    thr.join();
    BOOST_CHECK_EQUAL(threadresult, true);
#ifndef WIN32
    // Try to acquire lock in child process while we're holding it, this should fail.
    char ch;
    BOOST_CHECK_EQUAL(write(fd[1], &LockCommand, 1), 1);
    BOOST_CHECK_EQUAL(read(fd[1], &ch, 1), 1);
    BOOST_CHECK_EQUAL((bool)ch, false);

    // Give up our lock
    ReleaseDirectoryLocks();
    // Probing lock from our side now should succeed, but not hold on to the lock.
    BOOST_CHECK_EQUAL(LockDirectory(dirname, lockname, true), true);

    // Try to acquire the lock in the child process, this should be successful.
    BOOST_CHECK_EQUAL(write(fd[1], &LockCommand, 1), 1);
    BOOST_CHECK_EQUAL(read(fd[1], &ch, 1), 1);
    BOOST_CHECK_EQUAL((bool)ch, true);

    // When we try to probe the lock now, it should fail.
    BOOST_CHECK_EQUAL(LockDirectory(dirname, lockname, true), false);

    // Unlock the lock in the child process
    BOOST_CHECK_EQUAL(write(fd[1], &UnlockCommand, 1), 1);
    BOOST_CHECK_EQUAL(read(fd[1], &ch, 1), 1);
    BOOST_CHECK_EQUAL((bool)ch, true);

    // When we try to probe the lock now, it should succeed.
    BOOST_CHECK_EQUAL(LockDirectory(dirname, lockname, true), true);

    // Re-lock the lock in the child process, then wait for it to exit, check
    // successful return. After that, we check that exiting the process
    // has released the lock as we would expect by probing it.
    int processstatus;
    BOOST_CHECK_EQUAL(write(fd[1], &LockCommand, 1), 1);
    BOOST_CHECK_EQUAL(write(fd[1], &ExitCommand, 1), 1);
    BOOST_CHECK_EQUAL(waitpid(pid, &processstatus, 0), pid);
    BOOST_CHECK_EQUAL(processstatus, 0);
    BOOST_CHECK_EQUAL(LockDirectory(dirname, lockname, true), true);

    // Restore SIGCHLD
    signal(SIGCHLD, old_handler);
    BOOST_CHECK_EQUAL(close(fd[1]), 0); // Close our side of the socketpair
#endif
    // Clean up
    ReleaseDirectoryLocks();
    fs::remove_all(dirname);
}

BOOST_AUTO_TEST_CASE(test_DirIsWritable)
{
    // Should be able to write to the data dir.
    fs::path tmpdirname = GetDataDir();
    BOOST_CHECK_EQUAL(DirIsWritable(tmpdirname), true);

    // Should not be able to write to a non-existent dir.
    tmpdirname = tmpdirname / fs::unique_path();
    BOOST_CHECK_EQUAL(DirIsWritable(tmpdirname), false);

    fs::create_directory(tmpdirname);
    // Should be able to write to it now.
    BOOST_CHECK_EQUAL(DirIsWritable(tmpdirname), true);
    fs::remove(tmpdirname);
}

BOOST_AUTO_TEST_CASE(test_ToLower)
{
    BOOST_CHECK_EQUAL(ToLower('@'), '@');
    BOOST_CHECK_EQUAL(ToLower('A'), 'a');
    BOOST_CHECK_EQUAL(ToLower('Z'), 'z');
    BOOST_CHECK_EQUAL(ToLower('['), '[');
    BOOST_CHECK_EQUAL(ToLower(0), 0);
    BOOST_CHECK_EQUAL(ToLower('\xff'), '\xff');

    BOOST_CHECK_EQUAL(ToLower(""), "");
    BOOST_CHECK_EQUAL(ToLower("#HODL"), "#hodl");
    BOOST_CHECK_EQUAL(ToLower("\x00\xfe\xff"), "\x00\xfe\xff");
}

BOOST_AUTO_TEST_CASE(test_ToUpper)
{
    BOOST_CHECK_EQUAL(ToUpper('`'), '`');
    BOOST_CHECK_EQUAL(ToUpper('a'), 'A');
    BOOST_CHECK_EQUAL(ToUpper('z'), 'Z');
    BOOST_CHECK_EQUAL(ToUpper('{'), '{');
    BOOST_CHECK_EQUAL(ToUpper(0), 0);
    BOOST_CHECK_EQUAL(ToUpper('\xff'), '\xff');

    BOOST_CHECK_EQUAL(ToUpper(""), "");
    BOOST_CHECK_EQUAL(ToUpper("#hodl"), "#HODL");
    BOOST_CHECK_EQUAL(ToUpper("\x00\xfe\xff"), "\x00\xfe\xff");
}

BOOST_AUTO_TEST_CASE(test_Capitalize)
{
    BOOST_CHECK_EQUAL(Capitalize(""), "");
    BOOST_CHECK_EQUAL(Capitalize("bitcoin"), "Bitcoin");
    BOOST_CHECK_EQUAL(Capitalize("\x00\xfe\xff"), "\x00\xfe\xff");
}

static std::string SpanToStr(Span<const char>& span)
{
    return std::string(span.begin(), span.end());
}

BOOST_AUTO_TEST_CASE(test_spanparsing)
{
    using namespace spanparsing;
    std::string input;
    Span<const char> sp;
    bool success;

    // Const(...): parse a constant, update span to skip it if successful
    input = "MilkToastHoney";
    sp = MakeSpan(input);
    success = Const("", sp); // empty
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "MilkToastHoney");

    success = Const("Milk", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "ToastHoney");

    success = Const("Bread", sp);
    BOOST_CHECK(!success);

    success = Const("Toast", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "Honey");

    success = Const("Honeybadger", sp);
    BOOST_CHECK(!success);

    success = Const("Honey", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "");

    // Func(...): parse a function call, update span to argument if successful
    input = "Foo(Bar(xy,z()))";
    sp = MakeSpan(input);

    success = Func("FooBar", sp);
    BOOST_CHECK(!success);

    success = Func("Foo(", sp);
    BOOST_CHECK(!success);

    success = Func("Foo", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "Bar(xy,z())");

    success = Func("Bar", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "xy,z()");

    success = Func("xy", sp);
    BOOST_CHECK(!success);

    // Expr(...): return expression that span begins with, update span to skip it
    Span<const char> result;

    input = "(n*(n-1))/2";
    sp = MakeSpan(input);
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "(n*(n-1))/2");
    BOOST_CHECK_EQUAL(SpanToStr(sp), "");

    input = "foo,bar";
    sp = MakeSpan(input);
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "foo");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ",bar");

    input = "(aaaaa,bbbbb()),c";
    sp = MakeSpan(input);
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "(aaaaa,bbbbb())");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ",c");

    input = "xyz)foo";
    sp = MakeSpan(input);
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "xyz");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ")foo");

    input = "((a),(b),(c)),xxx";
    sp = MakeSpan(input);
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "((a),(b),(c))");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ",xxx");

    // Split(...): split a string on every instance of sep, return vector
    std::vector<Span<const char>> results;

    input = "xxx";
    results = Split(MakeSpan(input), 'x');
    BOOST_CHECK_EQUAL(results.size(), 4);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[3]), "");

    input = "one#two#three";
    results = Split(MakeSpan(input), '-');
    BOOST_CHECK_EQUAL(results.size(), 1);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "one#two#three");

    input = "one#two#three";
    results = Split(MakeSpan(input), '#');
    BOOST_CHECK_EQUAL(results.size(), 3);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "one");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "two");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "three");

    input = "*foo*bar*";
    results = Split(MakeSpan(input), '*');
    BOOST_CHECK_EQUAL(results.size(), 4);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "foo");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "bar");
    BOOST_CHECK_EQUAL(SpanToStr(results[3]), "");
}

BOOST_AUTO_TEST_CASE(test_LogEscapeMessage)
{
    // ASCII and UTF-8 must pass through unaltered.
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage("Valid log message貓"), "Valid log message貓");
    // Newlines must pass through unaltered.
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage("Message\n with newlines\n"), "Message\n with newlines\n");
    // Other control characters are escaped in C syntax.
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage("\x01\x7f Corrupted log message\x0d"), R"(\x01\x7f Corrupted log message\x0d)");
    // Embedded NULL characters are escaped too.
    const std::string NUL("O\x00O", 3);
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage(NUL), R"(O\x00O)");
}

namespace {

struct Tracker
{
    //! Points to the original object (possibly itself) we moved/copied from
    const Tracker* origin;
    //! How many copies where involved between the original object and this one (moves are not counted)
    int copies;

    Tracker() noexcept : origin(this), copies(0) {}
    Tracker(const Tracker& t) noexcept : origin(t.origin), copies(t.copies + 1) {}
    Tracker(Tracker&& t) noexcept : origin(t.origin), copies(t.copies) {}
    Tracker& operator=(const Tracker& t) noexcept
    {
        origin = t.origin;
        copies = t.copies + 1;
        return *this;
    }
    Tracker& operator=(Tracker&& t) noexcept
    {
        origin = t.origin;
        copies = t.copies;
        return *this;
    }
};

}

BOOST_AUTO_TEST_CASE(test_tracked_vector)
{
    Tracker t1;
    Tracker t2;
    Tracker t3;

    BOOST_CHECK(t1.origin == &t1);
    BOOST_CHECK(t2.origin == &t2);
    BOOST_CHECK(t3.origin == &t3);

    auto v1 = Vector(t1);
    BOOST_CHECK_EQUAL(v1.size(), 1);
    BOOST_CHECK(v1[0].origin == &t1);
    BOOST_CHECK_EQUAL(v1[0].copies, 1);

    auto v2 = Vector(std::move(t2));
    BOOST_CHECK_EQUAL(v2.size(), 1);
    BOOST_CHECK(v2[0].origin == &t2);
    BOOST_CHECK_EQUAL(v2[0].copies, 0);

    auto v3 = Vector(t1, std::move(t2));
    BOOST_CHECK_EQUAL(v3.size(), 2);
    BOOST_CHECK(v3[0].origin == &t1);
    BOOST_CHECK(v3[1].origin == &t2);
    BOOST_CHECK_EQUAL(v3[0].copies, 1);
    BOOST_CHECK_EQUAL(v3[1].copies, 0);

    auto v4 = Vector(std::move(v3[0]), v3[1], std::move(t3));
    BOOST_CHECK_EQUAL(v4.size(), 3);
    BOOST_CHECK(v4[0].origin == &t1);
    BOOST_CHECK(v4[1].origin == &t2);
    BOOST_CHECK(v4[2].origin == &t3);
    BOOST_CHECK_EQUAL(v4[0].copies, 1);
    BOOST_CHECK_EQUAL(v4[1].copies, 1);
    BOOST_CHECK_EQUAL(v4[2].copies, 0);

    auto v5 = Cat(v1, v4);
    BOOST_CHECK_EQUAL(v5.size(), 4);
    BOOST_CHECK(v5[0].origin == &t1);
    BOOST_CHECK(v5[1].origin == &t1);
    BOOST_CHECK(v5[2].origin == &t2);
    BOOST_CHECK(v5[3].origin == &t3);
    BOOST_CHECK_EQUAL(v5[0].copies, 2);
    BOOST_CHECK_EQUAL(v5[1].copies, 2);
    BOOST_CHECK_EQUAL(v5[2].copies, 2);
    BOOST_CHECK_EQUAL(v5[3].copies, 1);

    auto v6 = Cat(std::move(v1), v3);
    BOOST_CHECK_EQUAL(v6.size(), 3);
    BOOST_CHECK(v6[0].origin == &t1);
    BOOST_CHECK(v6[1].origin == &t1);
    BOOST_CHECK(v6[2].origin == &t2);
    BOOST_CHECK_EQUAL(v6[0].copies, 1);
    BOOST_CHECK_EQUAL(v6[1].copies, 2);
    BOOST_CHECK_EQUAL(v6[2].copies, 1);

    auto v7 = Cat(v2, std::move(v4));
    BOOST_CHECK_EQUAL(v7.size(), 4);
    BOOST_CHECK(v7[0].origin == &t2);
    BOOST_CHECK(v7[1].origin == &t1);
    BOOST_CHECK(v7[2].origin == &t2);
    BOOST_CHECK(v7[3].origin == &t3);
    BOOST_CHECK_EQUAL(v7[0].copies, 1);
    BOOST_CHECK_EQUAL(v7[1].copies, 1);
    BOOST_CHECK_EQUAL(v7[2].copies, 1);
    BOOST_CHECK_EQUAL(v7[3].copies, 0);

    auto v8 = Cat(std::move(v2), std::move(v3));
    BOOST_CHECK_EQUAL(v8.size(), 3);
    BOOST_CHECK(v8[0].origin == &t2);
    BOOST_CHECK(v8[1].origin == &t1);
    BOOST_CHECK(v8[2].origin == &t2);
    BOOST_CHECK_EQUAL(v8[0].copies, 0);
    BOOST_CHECK_EQUAL(v8[1].copies, 1);
    BOOST_CHECK_EQUAL(v8[2].copies, 0);
}

BOOST_AUTO_TEST_CASE(message_sign)
{
    const std::array<unsigned char, 32> privkey_bytes = {
        // just some random data
        // derived address from this private key: 15CRxFdyRpGZLW9w8HnHvVduizdL5jKNbs
        0xD9, 0x7F, 0x51, 0x08, 0xF1, 0x1C, 0xDA, 0x6E,
        0xEE, 0xBA, 0xAA, 0x42, 0x0F, 0xEF, 0x07, 0x26,
        0xB1, 0xF8, 0x98, 0x06, 0x0B, 0x98, 0x48, 0x9F,
        0xA3, 0x09, 0x84, 0x63, 0xC0, 0x03, 0x28, 0x66
    };

    const std::string message = "Trust no one";

    const std::string expected_signature =
        "IPojfrX2dfPnH26UegfbGQQLrdK844DlHq5157/P6h57WyuS/Qsl+h/WSVGDF4MUi4rWSswW38oimDYfNNUBUOk=";

    CKey privkey;
    std::string generated_signature;

    BOOST_REQUIRE_MESSAGE(!privkey.IsValid(),
        "Confirm the private key is invalid");

    BOOST_CHECK_MESSAGE(!MessageSign(privkey, message, generated_signature),
        "Sign with an invalid private key");

    privkey.Set(privkey_bytes.begin(), privkey_bytes.end(), true);

    BOOST_REQUIRE_MESSAGE(privkey.IsValid(),
        "Confirm the private key is valid");

    BOOST_CHECK_MESSAGE(MessageSign(privkey, message, generated_signature),
        "Sign with a valid private key");

    BOOST_CHECK_EQUAL(expected_signature, generated_signature);
}

BOOST_AUTO_TEST_CASE(message_verify)
{
    BOOST_CHECK_EQUAL(
        MessageVerify(
            "invalid address",
            "signature should be irrelevant",
            "message too"),
        MessageVerificationResult::ERR_INVALID_ADDRESS);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "3B5fQsEXEaV8v6U3ejYc8XaKXAkyQj2MjV",
            "signature should be irrelevant",
            "message too"),
        MessageVerificationResult::ERR_ADDRESS_NO_KEY);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "1KqbBpLy5FARmTPD4VZnDDpYjkUvkr82Pm",
            "invalid signature, not in base64 encoding",
            "message should be irrelevant"),
        MessageVerificationResult::ERR_MALFORMED_SIGNATURE);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "1KqbBpLy5FARmTPD4VZnDDpYjkUvkr82Pm",
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=",
            "message should be irrelevant"),
        MessageVerificationResult::ERR_PUBKEY_NOT_RECOVERED);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "15CRxFdyRpGZLW9w8HnHvVduizdL5jKNbs",
            "IPojfrX2dfPnH26UegfbGQQLrdK844DlHq5157/P6h57WyuS/Qsl+h/WSVGDF4MUi4rWSswW38oimDYfNNUBUOk=",
            "I never signed this"),
        MessageVerificationResult::ERR_NOT_SIGNED);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "15CRxFdyRpGZLW9w8HnHvVduizdL5jKNbs",
            "IPojfrX2dfPnH26UegfbGQQLrdK844DlHq5157/P6h57WyuS/Qsl+h/WSVGDF4MUi4rWSswW38oimDYfNNUBUOk=",
            "Trust no one"),
        MessageVerificationResult::OK);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "11canuhp9X2NocwCq7xNrQYTmUgZAnLK3",
            "IIcaIENoYW5jZWxsb3Igb24gYnJpbmsgb2Ygc2Vjb25kIGJhaWxvdXQgZm9yIGJhbmtzIAaHRtbCeDZINyavx14=",
            "Trust me"),
        MessageVerificationResult::OK);
}

BOOST_AUTO_TEST_CASE(message_hash)
{
    const std::string unsigned_tx = "...";
    const std::string prefixed_message =
        std::string(1, (char)MESSAGE_MAGIC.length()) +
        MESSAGE_MAGIC +
        std::string(1, (char)unsigned_tx.length()) +
        unsigned_tx;

    const uint256 signature_hash = Hash(unsigned_tx.begin(), unsigned_tx.end());
    const uint256 message_hash1 = Hash(prefixed_message.begin(), prefixed_message.end());
    const uint256 message_hash2 = MessageHash(unsigned_tx);

    BOOST_CHECK_EQUAL(message_hash1, message_hash2);
    BOOST_CHECK_NE(message_hash1, signature_hash);
}

BOOST_AUTO_TEST_SUITE_END()
