// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <logging/timer.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <util/string.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(logging_tests, BasicTestingSetup)

struct LogSetup : public BasicTestingSetup {
    fs::path prev_log_path;
    fs::path tmp_log_path;
    bool prev_reopen_file;
    bool prev_print_to_file;
    bool prev_log_timestamps;
    bool prev_log_threadnames;
    bool prev_log_sourcelocations;

    LogSetup() : prev_log_path{LogInstance().m_file_path},
                 tmp_log_path{m_args.GetDataDirBase() / "tmp_debug.log"},
                 prev_reopen_file{LogInstance().m_reopen_file},
                 prev_print_to_file{LogInstance().m_print_to_file},
                 prev_log_timestamps{LogInstance().m_log_timestamps},
                 prev_log_threadnames{LogInstance().m_log_threadnames},
                 prev_log_sourcelocations{LogInstance().m_log_sourcelocations}
    {
        LogInstance().m_file_path = tmp_log_path;
        LogInstance().m_reopen_file = true;
        LogInstance().m_print_to_file = true;
        LogInstance().m_log_timestamps = false;
        LogInstance().m_log_threadnames = false;
        LogInstance().m_log_sourcelocations = true;
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
    }
};

BOOST_AUTO_TEST_CASE(logging_timer)
{
    SetMockTime(1);
    auto micro_timer = BCLog::Timer<std::chrono::microseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(micro_timer.LogMsg("test micros"), "tests: test micros (1000000μs)");

    SetMockTime(1);
    auto ms_timer = BCLog::Timer<std::chrono::milliseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(ms_timer.LogMsg("test ms"), "tests: test ms (1000.00ms)");

    SetMockTime(1);
    auto sec_timer = BCLog::Timer<std::chrono::seconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(sec_timer.LogMsg("test secs"), "tests: test secs (1.00s)");
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintf_, LogSetup)
{
    LogPrintf_("fn1", "src1", 1, BCLog::LogFlags::NET, BCLog::Level::Debug, "foo1: %s", "bar1\n");
    LogPrintf_("fn2", "src2", 2, BCLog::LogFlags::NET, BCLog::Level::None, "foo2: %s", "bar2\n");
    LogPrintf_("fn3", "src3", 3, BCLog::LogFlags::NONE, BCLog::Level::Debug, "foo3: %s", "bar3\n");
    LogPrintf_("fn4", "src4", 4, BCLog::LogFlags::NONE, BCLog::Level::None, "foo4: %s", "bar4\n");
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    std::vector<std::string> expected = {
        "[src1:1] [fn1] [net:debug] foo1: bar1",
        "[src2:2] [fn2] [net] foo2: bar2",
        "[src3:3] [fn3] [debug] foo3: bar3",
        "[src4:4] [fn4] foo4: bar4",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintMacros, LogSetup)
{
    // Prevent tests from failing when the line number of the following log calls changes.
    LogInstance().m_log_sourcelocations = false;

    LogPrintf("foo5: %s\n", "bar5");
    LogPrint(BCLog::NET, "foo6: %s\n", "bar6");
    LogPrintLevel(BCLog::Level::Debug, BCLog::NET, "foo7: %s\n", "bar7");
    LogPrintLevel(BCLog::Level::Info, BCLog::NET, "foo8: %s\n", "bar8");
    LogPrintLevel(BCLog::Level::Warning, BCLog::NET, "foo9: %s\n", "bar9");
    LogPrintLevel(BCLog::Level::Error, BCLog::NET, "foo10: %s\n", "bar10");
    std::ifstream file{tmp_log_path};
    std::vector<std::string> log_lines;
    for (std::string log; std::getline(file, log);) {
        log_lines.push_back(log);
    }
    std::vector<std::string> expected = {
        "foo5: bar5",
        "[net] foo6: bar6",
        "[net:debug] foo7: bar7",
        "[net:info] foo8: bar8",
        "[net:warning] foo9: bar9",
        "[net:error] foo10: bar10"};
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintMacros_CategoryName, LogSetup)
{
    // Prevent tests from failing when the line number of the following log calls changes.
    LogInstance().m_log_sourcelocations = false;
    LogInstance().EnableCategory(BCLog::LogFlags::ALL);
    const auto concated_categery_names = LogInstance().LogCategoriesString();
    std::vector<std::pair<BCLog::LogFlags, std::string>> expected_category_names;
    const auto category_names = SplitString(concated_categery_names, ',');
    for (const auto& category_name : category_names) {
        BCLog::LogFlags category = BCLog::NONE;
        const auto trimmed_category_name = TrimString(category_name);
        BOOST_TEST(GetLogCategory(category, trimmed_category_name));
        expected_category_names.emplace_back(category, trimmed_category_name);
    }

    std::vector<std::string> expected;
    for (const auto& [category, name] : expected_category_names) {
        LogPrint(category, "foo: %s\n", "bar");
        if (category == BCLog::UNCONDITIONAL_ALWAYS || category == BCLog::UNCONDITIONAL_RATE_LIMITED) {
            expected.push_back("foo: bar");
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

BOOST_AUTO_TEST_CASE(logging_ratelimit_window)
{
    SetMockTime(std::chrono::hours{1});
    BCLog::LogRateLimiter window;

    // Check that window gets initialised correctly.
    BOOST_CHECK_EQUAL(window.GetAvailableBytes(), BCLog::LogRateLimiter::WINDOW_MAX_BYTES);
    BOOST_CHECK_EQUAL(window.GetDroppedBytes(), 0ull);

    const uint64_t MESSAGE_SIZE{512 * 1024};
    BOOST_CHECK(window.Consume(MESSAGE_SIZE));
    BOOST_CHECK_EQUAL(window.GetAvailableBytes(), BCLog::LogRateLimiter::WINDOW_MAX_BYTES - MESSAGE_SIZE);
    BOOST_CHECK_EQUAL(window.GetDroppedBytes(), 0ull);

    BOOST_CHECK(window.Consume(MESSAGE_SIZE));
    BOOST_CHECK_EQUAL(window.GetAvailableBytes(), BCLog::LogRateLimiter::WINDOW_MAX_BYTES - MESSAGE_SIZE * 2);
    BOOST_CHECK_EQUAL(window.GetDroppedBytes(), 0ull);

    // Consuming more bytes after already having consumed a 1MB should fail.
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
        LogPrint(BCLog::UNCONDITIONAL_RATE_LIMITED, "%s\n", message);
        break;
    case 1:
        LogPrint(BCLog::UNCONDITIONAL_RATE_LIMITED, "%s\n", message);
        break;
    case 2:
        LogPrint(BCLog::UNCONDITIONAL_ALWAYS, "%s\n", message);
        break;
    case 3:
        LogPrint(BCLog::ALL, "%s\n", message);
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
    bool prev_log_timestamps = LogInstance().m_log_sourcelocations;
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
    BOOST_CHECK_NO_THROW(
        LogFromLocationAndExpect(0, log_message, "Excessive logging detected"));
    BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "the start of the supression period should be logged");

    log_file_size = std::filesystem::file_size(LogInstance().m_file_path);
    for (int i = 0; i < 1024; ++i) {
        LogFromLocation(0, log_message);
    }
    BOOST_CHECK_MESSAGE(log_file_size == std::filesystem::file_size(LogInstance().m_file_path), "all further logs from location 0 should be dropped");

    BOOST_CHECK_THROW(
        LogFromLocationAndExpect(1, log_message, "Excessive logging detected"), std::runtime_error);
    BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "location 1 should be unaffected by other locations");

    SetMockTime(std::chrono::hours{2});

    log_file_size = std::filesystem::file_size(LogInstance().m_file_path);
    BOOST_CHECK_NO_THROW(
        LogFromLocationAndExpect(0, log_message, "Restarting logging"));
    BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "the end of the supression period should be logged");

    BOOST_CHECK_THROW(
        LogFromLocationAndExpect(1, log_message, "Restarting logging"), std::runtime_error);

    // Attempt to log 2 MiB to disk.
    // The exempt locations 2 and 3 should be allowed to log without limit.
    for (int i = 0; i < 2048; ++i) {
        log_file_size = std::filesystem::file_size(LogInstance().m_file_path);
        BOOST_CHECK_THROW(
            LogFromLocationAndExpect(2, log_message, "Excessive logging detected"), std::runtime_error);
        BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "location 2 should be exempt from rate limiting");

        log_file_size = std::filesystem::file_size(LogInstance().m_file_path);
        BOOST_CHECK_THROW(
            LogFromLocationAndExpect(3, log_message, "Excessive logging detected"), std::runtime_error);
        BOOST_CHECK_MESSAGE(log_file_size < std::filesystem::file_size(LogInstance().m_file_path), "location 3 should be exempt from rate limiting");
    }

    LogInstance().m_log_timestamps = prev_log_timestamps;
    LogInstance().m_log_sourcelocations = prev_log_sourcelocations;
    LogInstance().m_log_threadnames = prev_log_threadnames;
    SetMockTime(std::chrono::seconds{0});
}

BOOST_AUTO_TEST_SUITE_END()
