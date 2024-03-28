// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init/common.h>
#include <logging.h>
#include <logging/timer.h>
#include <test/util/setup_common.h>
#include <util/string.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(logging_tests, BasicTestingSetup)

static void ResetLogger()
{
    LogInstance().SetLogLevel(BCLog::DEFAULT_LOG_LEVEL);
    LogInstance().SetCategoryLogLevel({});
}

struct LogSetup : public BasicTestingSetup {
    fs::path prev_log_path;
    fs::path tmp_log_path;
    bool prev_reopen_file;
    bool prev_print_to_file;
    bool prev_log_timestamps;
    bool prev_log_threadnames;
    bool prev_log_sourcelocations;
    std::unordered_map<BCLog::LogFlags, BCLog::Level> prev_category_levels;
    BCLog::Level prev_log_level;

    LogSetup() : prev_log_path{LogInstance().m_file_path},
                 tmp_log_path{m_args.GetDataDirBase() / "tmp_debug.log"},
                 prev_reopen_file{LogInstance().m_reopen_file},
                 prev_print_to_file{LogInstance().m_print_to_file},
                 prev_log_timestamps{LogInstance().m_log_timestamps},
                 prev_log_threadnames{LogInstance().m_log_threadnames},
                 prev_log_sourcelocations{LogInstance().m_log_sourcelocations},
                 prev_category_levels{LogInstance().CategoryLevels()},
                 prev_log_level{LogInstance().LogLevel()}
    {
        LogInstance().m_file_path = tmp_log_path;
        LogInstance().m_reopen_file = true;
        LogInstance().m_print_to_file = true;
        LogInstance().m_log_timestamps = false;
        LogInstance().m_log_threadnames = false;

        // Prevent tests from failing when the line number of the logs changes.
        LogInstance().m_log_sourcelocations = false;

        LogInstance().SetLogLevel(BCLog::Level::Debug);
        LogInstance().SetCategoryLogLevel({});
    }

    ~LogSetup()
    {
        LogInstance().m_file_path = prev_log_path;
        LogPrintf("Sentinel log to reopen log file\n");
        LogInstance().m_print_to_file = prev_print_to_file;
        LogInstance().m_reopen_file = prev_reopen_file;
        LogInstance().m_log_timestamps = prev_log_timestamps;
        LogInstance().m_log_threadnames = prev_log_threadnames;
        LogInstance().m_log_sourcelocations = prev_log_sourcelocations;
        LogInstance().SetLogLevel(prev_log_level);
        LogInstance().SetCategoryLogLevel(prev_category_levels);
    }
};

//! Test logging to local logger.
BOOST_AUTO_TEST_CASE(logging_local_logger)
{
    BCLog::Logger logger;
    logger.m_log_timestamps = false;
    logger.EnableCategory(BCLog::LogFlags::ALL);
    logger.SetLogLevel(BCLog::Level::Trace);
    logger.StartLogging();

    std::vector<std::string> messages;
    logger.PushBackCallback([&](const std::string& s) { messages.push_back(s); });

    BCLog::Source log{BCLog::NET, logger};
    LogError(log, "error %s\n", "arg");
    LogWarning(log, "warning %s\n", "arg");
    LogInfo(log, "info %s\n", "arg");
    LogDebug(log, "debug %s\n", "arg");
    LogTrace(log, "trace %s\n", "arg");

    std::vector<std::string> expected = {
        "[net:error] error arg\n",
        "[net:warning] warning arg\n",
        "[net:info] info arg\n",
        "[net] debug arg\n",
        "[net:trace] trace arg\n",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(messages.begin(), messages.end(), expected.begin(), expected.end());
}

//! Test logging to global logger with different types of source arguments.
BOOST_FIXTURE_TEST_CASE(logging_source_args, LogSetup)
{
    LogInstance().EnableCategory(BCLog::LogFlags::ALL);
    LogInstance().SetLogLevel(BCLog::Level::Trace);

    // Test logging with no source arguments.
    LogError("error\n");
    LogWarning("warning\n");
    LogInfo("info\n");
    LogDebug("debug\n");
    LogTrace("trace\n");
    LogError("error %s\n", "arg");
    LogWarning("warning %s\n", "arg");
    LogInfo("info %s\n", "arg");
    LogDebug("debug %s\n", "arg");
    LogTrace("trace %s\n", "arg");

    // Test logging with category arguments.
    LogError(BCLog::NET, "error\n");
    LogWarning(BCLog::NET, "warning\n");
    LogInfo(BCLog::NET, "info\n");
    LogDebug(BCLog::NET, "debug\n");
    LogTrace(BCLog::NET, "trace\n");
    LogError(BCLog::NET, "error %s\n", "arg");
    LogWarning(BCLog::NET, "warning %s\n", "arg");
    LogInfo(BCLog::NET, "info %s\n", "arg");
    LogDebug(BCLog::NET, "debug %s\n", "arg");
    LogTrace(BCLog::NET, "trace %s\n", "arg");

    // Test logging with source object.
    BCLog::Source log{BCLog::TOR};
    LogError(log, "error\n");
    LogWarning(log, "warning\n");
    LogInfo(log, "info\n");
    LogDebug(log, "debug\n");
    LogTrace(log, "trace\n");
    LogError(log, "error %s\n", "arg");
    LogWarning(log, "warning %s\n", "arg");
    LogInfo(log, "info %s\n", "arg");
    LogDebug(log, "debug %s\n", "arg");
    LogTrace(log, "trace %s\n", "arg");

    std::vector<std::string> expected = {
        "[error] error",
        "[warning] warning",
        "info",
        "[debug] debug",
        "[trace] trace",

        "[error] error arg",
        "[warning] warning arg",
        "info arg",
        "[debug] debug arg",
        "[trace] trace arg",

        "[net:error] error",
        "[net:warning] warning",
        "[net:info] info",
        "[net] debug",
        "[net:trace] trace",

        "[net:error] error arg",
        "[net:warning] warning arg",
        "[net:info] info arg",
        "[net] debug arg",
        "[net:trace] trace arg",

        "[tor:error] error",
        "[tor:warning] warning",
        "[tor:info] info",
        "[tor] debug",
        "[tor:trace] trace",

        "[tor:error] error arg",
        "[tor:warning] warning arg",
        "[tor:info] info arg",
        "[tor] debug arg",
        "[tor:trace] trace arg",
    };
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

struct CustomLogSource {
    static constexpr bool log_source{true};
    BCLog::LogFlags category{BCLog::VALIDATION};
    BCLog::Logger& logger{LogInstance()};
    int& m_counter;

    CustomLogSource(int& counter) : m_counter{counter} {}

    template <typename... Args>
    std::string Format(const char* fmt, const Args&... args) const
    {
        return tfm::format(("Custom #%d " + std::string{fmt}), ++m_counter, args...);
    }
};

BOOST_FIXTURE_TEST_CASE(logging_CustomSource, LogSetup)
{
    int counter{0};
    CustomLogSource log{counter};
    LogTrace(log, "foo0: %s\n", "bar0"); // not logged
    LogDebug(log, "foo1: %s\n", "bar1");
    LogInfo(log, "foo2: %s\n", "bar2");
    LogWarning(log, "foo3: %s\n", "bar3");
    LogError(log, "foo4: %s\n", "bar4");
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    std::vector<std::string> expected = {
        "[validation] Custom #1 foo1: bar1",
        "[validation:info] Custom #2 foo2: bar2",
        "[validation:warning] Custom #3 foo3: bar3",
        "[validation:error] Custom #4 foo4: bar4",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(logging_timer)
{
    auto micro_timer = BCLog::Timer<std::chrono::microseconds>("tests", "end_msg");
    const std::string_view result_prefix{"tests: msg ("};
    BOOST_CHECK_EQUAL(micro_timer.LogMsg("msg").substr(0, result_prefix.size()), result_prefix);
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintf_, LogSetup)
{
    LogInstance().m_log_sourcelocations = true;
    auto LogPrintf_ = [](const std::string&fun, const std::string&file, int line, BCLog::LogFlags category, BCLog::Level level, const auto&... args) {
        LogInstance().LogPrintStr(strprintf(args...), fun, file, line, category, level);
    };
    LogPrintf_("fn1", "src1", 1, BCLog::LogFlags::NET, BCLog::Level::Debug, "foo1: %s\n", "bar1");
    LogPrintf_("fn2", "src2", 2, BCLog::LogFlags::NET, BCLog::Level::Info, "foo2: %s\n", "bar2");
    LogPrintf_("fn3", "src3", 3, BCLog::LogFlags::ALL, BCLog::Level::Debug, "foo3: %s\n", "bar3");
    LogPrintf_("fn4", "src4", 4, BCLog::LogFlags::ALL, BCLog::Level::Info, "foo4: %s\n", "bar4");
    LogPrintf_("fn5", "src5", 5, BCLog::LogFlags::NONE, BCLog::Level::Debug, "foo5: %s\n", "bar5");
    LogPrintf_("fn6", "src6", 6, BCLog::LogFlags::NONE, BCLog::Level::Info, "foo6: %s\n", "bar6");
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    std::vector<std::string> expected = {
        "[src1:1] [fn1] [net] foo1: bar1",
        "[src2:2] [fn2] [net:info] foo2: bar2",
        "[src3:3] [fn3] [debug] foo3: bar3",
        "[src4:4] [fn4] foo4: bar4",
        "[src5:5] [fn5] [debug] foo5: bar5",
        "[src6:6] [fn6] foo6: bar6",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintMacrosDeprecated, LogSetup)
{
    LogPrintf("foo5: %s\n", "bar5");
    LogPrint(BCLog::NET, "foo6: %s\n", "bar6");
    LogPrintLevel(BCLog::NET, BCLog::Level::Trace, "foo4: %s\n", "bar4"); // not logged
    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "foo7: %s\n", "bar7");
    LogPrintLevel(BCLog::NET, BCLog::Level::Info, "foo8: %s\n", "bar8");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "foo9: %s\n", "bar9");
    LogPrintLevel(BCLog::NET, BCLog::Level::Error, "foo10: %s\n", "bar10");
    LogPrintfCategory(BCLog::VALIDATION, "foo11: %s\n", "bar11");
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    std::vector<std::string> expected = {
        "foo5: bar5",
        "[net] foo6: bar6",
        "[net] foo7: bar7",
        "[net:info] foo8: bar8",
        "[net:warning] foo9: bar9",
        "[net:error] foo10: bar10",
        "[validation:info] foo11: bar11",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintMacros, LogSetup)
{
    LogTrace(BCLog::NET, "foo6: %s\n", "bar6"); // not logged
    LogDebug(BCLog::NET, "foo7: %s\n", "bar7");
    LogInfo("foo8: %s\n", "bar8");
    LogWarning("foo9: %s\n", "bar9");
    LogError("foo10: %s\n", "bar10");
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    std::vector<std::string> expected = {
        "[net] foo7: bar7",
        "foo8: bar8",
        "[warning] foo9: bar9",
        "[error] foo10: bar10",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintMacros_CategoryName, LogSetup)
{
    LogInstance().EnableCategory(BCLog::LogFlags::ALL);
    const auto concatenated_category_names = LogInstance().LogCategoriesString();
    std::vector<std::pair<BCLog::LogFlags, std::string>> expected_category_names;
    const auto category_names = SplitString(concatenated_category_names, ',');
    for (const auto& category_name : category_names) {
        BCLog::LogFlags category;
        const auto trimmed_category_name = TrimString(category_name);
        BOOST_REQUIRE(GetLogCategory(category, trimmed_category_name));
        expected_category_names.emplace_back(category, trimmed_category_name);
    }

    std::vector<std::string> expected;
    for (const auto& [category, name] : expected_category_names) {
        LogPrint(category, "foo: %s\n", "bar");
        std::string expected_log = "[";
        expected_log += name;
        expected_log += "] foo: bar";
        expected.push_back(expected_log);
    }

    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_SeverityLevels, LogSetup)
{
    LogInstance().EnableCategory(BCLog::LogFlags::ALL);

    LogInstance().SetLogLevel(BCLog::Level::Debug);
    LogInstance().SetCategoryLogLevel(/*category_str=*/"net", /*level_str=*/"info");

    // Global log level
    LogPrintLevel(BCLog::HTTP, BCLog::Level::Info, "foo1: %s\n", "bar1");
    LogPrintLevel(BCLog::MEMPOOL, BCLog::Level::Trace, "foo2: %s. This log level is lower than the global one.\n", "bar2");
    LogPrintLevel(BCLog::VALIDATION, BCLog::Level::Warning, "foo3: %s\n", "bar3");
    LogPrintLevel(BCLog::RPC, BCLog::Level::Error, "foo4: %s\n", "bar4");

    // Category-specific log level
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "foo5: %s\n", "bar5");
    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "foo6: %s. This log level is the same as the global one but lower than the category-specific one, which takes precedence. \n", "bar6");
    LogPrintLevel(BCLog::NET, BCLog::Level::Error, "foo7: %s\n", "bar7");

    std::vector<std::string> expected = {
        "[http:info] foo1: bar1",
        "[validation:warning] foo3: bar3",
        "[rpc:error] foo4: bar4",
        "[net:warning] foo5: bar5",
        "[net:error] foo7: bar7",
    };
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_Conf, LogSetup)
{
    // Set global log level
    {
        ResetLogger();
        ArgsManager args;
        args.AddArg("-loglevel", "...", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
        const char* argv_test[] = {"bitcoind", "-loglevel=debug"};
        std::string err;
        BOOST_REQUIRE(args.ParseParameters(2, argv_test, err));

        auto result = init::SetLoggingLevel(args);
        BOOST_REQUIRE(result);
        BOOST_CHECK_EQUAL(LogInstance().LogLevel(), BCLog::Level::Debug);
    }

    // Set category-specific log level
    {
        ResetLogger();
        ArgsManager args;
        args.AddArg("-loglevel", "...", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
        const char* argv_test[] = {"bitcoind", "-loglevel=net:trace"};
        std::string err;
        BOOST_REQUIRE(args.ParseParameters(2, argv_test, err));

        auto result = init::SetLoggingLevel(args);
        BOOST_REQUIRE(result);
        BOOST_CHECK_EQUAL(LogInstance().LogLevel(), BCLog::DEFAULT_LOG_LEVEL);

        const auto& category_levels{LogInstance().CategoryLevels()};
        const auto net_it{category_levels.find(BCLog::LogFlags::NET)};
        BOOST_REQUIRE(net_it != category_levels.end());
        BOOST_CHECK_EQUAL(net_it->second, BCLog::Level::Trace);
    }

    // Set both global log level and category-specific log level
    {
        ResetLogger();
        ArgsManager args;
        args.AddArg("-loglevel", "...", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
        const char* argv_test[] = {"bitcoind", "-loglevel=debug", "-loglevel=net:trace", "-loglevel=http:info"};
        std::string err;
        BOOST_REQUIRE(args.ParseParameters(4, argv_test, err));

        auto result = init::SetLoggingLevel(args);
        BOOST_REQUIRE(result);
        BOOST_CHECK_EQUAL(LogInstance().LogLevel(), BCLog::Level::Debug);

        const auto& category_levels{LogInstance().CategoryLevels()};
        BOOST_CHECK_EQUAL(category_levels.size(), 2);

        const auto net_it{category_levels.find(BCLog::LogFlags::NET)};
        BOOST_CHECK(net_it != category_levels.end());
        BOOST_CHECK_EQUAL(net_it->second, BCLog::Level::Trace);

        const auto http_it{category_levels.find(BCLog::LogFlags::HTTP)};
        BOOST_CHECK(http_it != category_levels.end());
        BOOST_CHECK_EQUAL(http_it->second, BCLog::Level::Info);
    }
}

BOOST_AUTO_TEST_SUITE_END()
