// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init/common.h>
#include <logging.h>
#include <logging/timer.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <util/string.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <source_location>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

using util::SplitString;
using util::TrimString;

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

BOOST_AUTO_TEST_CASE(logging_timer)
{
    auto micro_timer = BCLog::Timer<std::chrono::microseconds>("tests", "end_msg");
    const std::string_view result_prefix{"tests: msg ("};
    BOOST_CHECK_EQUAL(micro_timer.LogMsg("msg").substr(0, result_prefix.size()), result_prefix);
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintStr, LogSetup)
{
    LogInstance().m_log_sourcelocations = true;
    std::vector<std::source_location> source_locs = {
        std::source_location::current(),
        std::source_location::current(),
        std::source_location::current(),
        std::source_location::current(),
        std::source_location::current(),
        std::source_location::current(),
    };
    LogInstance().LogPrintStr("foo1: bar1", "fn1", source_locs[0], BCLog::LogFlags::NET, BCLog::Level::Debug);
    LogInstance().LogPrintStr("foo2: bar2", "fn2", source_locs[1], BCLog::LogFlags::NET, BCLog::Level::Info);
    LogInstance().LogPrintStr("foo3: bar3", "fn3", source_locs[2], BCLog::LogFlags::ALL, BCLog::Level::Debug);
    LogInstance().LogPrintStr("foo4: bar4", "fn4", source_locs[3], BCLog::LogFlags::ALL, BCLog::Level::Info);
    LogInstance().LogPrintStr("foo5: bar5", "fn5", source_locs[4], BCLog::LogFlags::NONE, BCLog::Level::Debug);
    LogInstance().LogPrintStr("foo6: bar6", "fn6", source_locs[5], BCLog::LogFlags::NONE, BCLog::Level::Info);
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    std::string file_name = source_locs[0].file_name();
    std::vector<std::string> expected = {
        "[" + file_name + ":" + std::to_string(source_locs[0].line()) + "] [fn1] [net] foo1: bar1",
        "[" + file_name + ":" + std::to_string(source_locs[1].line()) + "] [fn2] [net:info] foo2: bar2",
        "[" + file_name + ":" + std::to_string(source_locs[2].line()) + "] [fn3] [debug] foo3: bar3",
        "[" + file_name + ":" + std::to_string(source_locs[3].line()) + "] [fn4] foo4: bar4",
        "[" + file_name + ":" + std::to_string(source_locs[4].line()) + "] [fn5] [debug] foo5: bar5",
        "[" + file_name + ":" + std::to_string(source_locs[5].line()) + "] [fn6] foo6: bar6",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintMacrosDeprecated, LogSetup)
{
    LogPrintf("foo5: %s\n", "bar5");
    LogPrintLevel(BCLog::NET, BCLog::Level::Trace, "foo4: %s\n", "bar4"); // not logged
    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "foo7: %s\n", "bar7");
    LogPrintLevel(BCLog::NET, BCLog::Level::Info, "foo8: %s\n", "bar8");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "foo9: %s\n", "bar9");
    LogPrintLevel(BCLog::NET, BCLog::Level::Error, "foo10: %s\n", "bar10");
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    std::vector<std::string> expected = {
        "foo5: bar5",
        "[net] foo7: bar7",
        "[net:info] foo8: bar8",
        "[net:warning] foo9: bar9",
        "[net:error] foo10: bar10",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintMacros, LogSetup)
{
    LogTrace(BCLog::NET, "foo6: %s", "bar6"); // not logged
    LogDebug(BCLog::NET, "foo7: %s", "bar7");
    LogInfo("foo8: %s", "bar8");
    LogWarning("foo9: %s", "bar9");
    LogError("foo10: %s", "bar10");
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
        LogDebug(category, "foo: %s\n", "bar");
        if (category == BCLog::UNCONDITIONAL_ALWAYS || category == BCLog::UNCONDITIONAL_RATE_LIMITED) {
            expected.push_back("[debug] foo: bar");
        } else {
            expected.push_back("[" + name + "] foo: bar");
        }
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

BOOST_AUTO_TEST_CASE(logging_ratelimit_window)
{
    SetMockTime(std::chrono::hours{1});
    BCLog::LogRateLimiter window;

    // Check that window gets initialized correctly.
    BOOST_CHECK_EQUAL(window.GetAvailableBytes(), BCLog::LogRateLimiter::WINDOW_MAX_BYTES);
    BOOST_CHECK_EQUAL(window.GetDroppedBytes(), 0ull);

    const uint64_t MESSAGE_SIZE{512 * 1024};
    BOOST_CHECK(window.Consume(MESSAGE_SIZE));
    BOOST_CHECK_EQUAL(window.GetAvailableBytes(), BCLog::LogRateLimiter::WINDOW_MAX_BYTES - MESSAGE_SIZE);
    BOOST_CHECK_EQUAL(window.GetDroppedBytes(), 0ull);

    BOOST_CHECK(window.Consume(MESSAGE_SIZE));
    BOOST_CHECK_EQUAL(window.GetAvailableBytes(), BCLog::LogRateLimiter::WINDOW_MAX_BYTES - MESSAGE_SIZE * 2);
    BOOST_CHECK_EQUAL(window.GetDroppedBytes(), 0ull);

    // Consuming more bytes after already having consumed 1MB should fail.
    BOOST_CHECK(!window.Consume(500));
    BOOST_CHECK_EQUAL(window.GetAvailableBytes(), 0ull);
    BOOST_CHECK_EQUAL(window.GetDroppedBytes(), 500ull);

    // Advance time by one hour. This should trigger a window reset.
    SetMockTime(std::chrono::hours{2});

    // Check that the window resets as expected when new bytes are consumed.
    BOOST_CHECK(window.Consume(MESSAGE_SIZE));
    BOOST_CHECK_EQUAL(window.GetAvailableBytes(), BCLog::LogRateLimiter::WINDOW_MAX_BYTES - MESSAGE_SIZE);
    BOOST_CHECK_EQUAL(window.GetDroppedBytes(), 0ull);
}

void LogFromLocation(int location, std::string message)
{
    switch (location) {
    case 0:
        LogInfo("%s\n", message);
        break;
    case 1:
        LogInfo("%s\n", message);
        break;
    case 2:
        LogPrintLevel(BCLog::LogFlags::UNCONDITIONAL_ALWAYS, BCLog::Level::Info, "%s\n", message);
        break;
    case 3:
        LogPrintLevel(BCLog::LogFlags::ALL, BCLog::Level::Info, "%s\n", message);
        break;
    }
}

void LogFromLocationAndExpect(int location, std::string message, std::string expect)
{
    ASSERT_DEBUG_LOG(expect);
    LogFromLocation(location, message);
}

BOOST_AUTO_TEST_CASE(rate_limiting)
{
    bool prev_log_timestamps = LogInstance().m_log_timestamps;
    LogInstance().m_log_timestamps = false;
    bool prev_log_sourcelocations = LogInstance().m_log_sourcelocations;
    LogInstance().m_log_sourcelocations = false;
    bool prev_log_threadnames = LogInstance().m_log_threadnames;
    LogInstance().m_log_threadnames = false;

    // Log 1024-character lines (1023 plus newline) to make the math simple.
    std::string log_message(1023, 'a');

    SetMockTime(std::chrono::hours{1});

    size_t log_file_size = std::filesystem::file_size(LogInstance().m_file_path);

    // Logging 1 MiB should be allowed.
    for (int i = 0; i < 1024; ++i) {
        LogFromLocation(0, log_message);
    }
    BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "should be able to log 1 MiB from location 0");

    log_file_size = std::filesystem::file_size(LogInstance().m_file_path);

    BOOST_CHECK_NO_THROW(LogFromLocationAndExpect(0, log_message, "Excessive logging detected"));
    BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "the start of the suppression period should be logged");

    log_file_size = std::filesystem::file_size(LogInstance().m_file_path);
    for (int i = 0; i < 1024; ++i) {
        LogFromLocation(0, log_message);
    }
    BOOST_CHECK_MESSAGE(log_file_size == std::filesystem::file_size(LogInstance().m_file_path), "all further logs from location 0 should be dropped");

    BOOST_CHECK_THROW(LogFromLocationAndExpect(1, log_message, "Excessive logging detected"), std::runtime_error);
    BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "location 1 should be unaffected by other locations");

    SetMockTime(std::chrono::hours{2});

    BOOST_CHECK_NO_THROW(LogFromLocationAndExpect(0, log_message, "Restarting logging"));
    BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "the end of the suppression period should be logged");

    BOOST_CHECK_THROW(LogFromLocationAndExpect(1, log_message, "Restarting logging"), std::runtime_error);

    // Attempt to log 2 MiB to disk.
    // The exempt locations 2 and 3 should be allowed to log without limit.
    for (int i = 0; i < 2048; ++i) {
        log_file_size = std::filesystem::file_size(LogInstance().m_file_path);
        BOOST_CHECK_THROW(LogFromLocationAndExpect(2, log_message, "Excessive logging detected"), std::runtime_error);
        BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "location 2 should be exempt from rate limiting");

        log_file_size = std::filesystem::file_size(LogInstance().m_file_path);
        BOOST_CHECK_THROW(LogFromLocationAndExpect(3, log_message, "Excessive logging detected"), std::runtime_error);
        BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "location 3 should be exempt from rate limiting");
    }

    LogInstance().m_log_timestamps = prev_log_timestamps;
    LogInstance().m_log_sourcelocations = prev_log_sourcelocations;
    LogInstance().m_log_threadnames = prev_log_threadnames;
    SetMockTime(std::chrono::seconds{0});
}

BOOST_AUTO_TEST_SUITE_END()
