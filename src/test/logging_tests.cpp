// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init/common.h>
#include <logging.h>
#include <logging/timer.h>
#include <scheduler.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/string.h>

#include <chrono>
#include <fstream>
#include <future>
#include <ios>
#include <iostream>
#include <source_location>
#include <string>
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

static std::vector<std::string> ReadDebugLogLines()
{
    std::vector<std::string> lines;
    std::ifstream ifs{LogInstance().m_file_path.std_path()};
    for (std::string line; std::getline(ifs, line);) {
        lines.push_back(std::move(line));
    }
    return lines;
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
    BCLog::CategoryMask prev_category_mask;

    LogSetup() : prev_log_path{LogInstance().m_file_path},
                 tmp_log_path{m_args.GetDataDirBase() / "tmp_debug.log"},
                 prev_reopen_file{LogInstance().m_reopen_file},
                 prev_print_to_file{LogInstance().m_print_to_file},
                 prev_log_timestamps{LogInstance().m_log_timestamps},
                 prev_log_threadnames{LogInstance().m_log_threadnames},
                 prev_log_sourcelocations{LogInstance().m_log_sourcelocations},
                 prev_category_levels{LogInstance().CategoryLevels()},
                 prev_log_level{LogInstance().LogLevel()},
                 prev_category_mask{LogInstance().GetCategoryMask()}
    {
        LogInstance().m_file_path = tmp_log_path;
        LogInstance().m_reopen_file = true;
        LogInstance().m_print_to_file = true;
        LogInstance().m_log_timestamps = false;
        LogInstance().m_log_threadnames = false;

        // Prevent tests from failing when the line number of the logs changes.
        LogInstance().m_log_sourcelocations = false;

        LogInstance().SetLogLevel(BCLog::Level::Debug);
        LogInstance().DisableCategory(BCLog::LogFlags::ALL);
        LogInstance().SetCategoryLogLevel({});
        LogInstance().SetRateLimiting(nullptr);
    }

    ~LogSetup()
    {
        LogInstance().m_file_path = prev_log_path;
        LogInfo("Sentinel log to reopen log file");
        LogInstance().m_print_to_file = prev_print_to_file;
        LogInstance().m_reopen_file = prev_reopen_file;
        LogInstance().m_log_timestamps = prev_log_timestamps;
        LogInstance().m_log_threadnames = prev_log_threadnames;
        LogInstance().m_log_sourcelocations = prev_log_sourcelocations;
        LogInstance().SetLogLevel(prev_log_level);
        LogInstance().SetCategoryLogLevel(prev_category_levels);
        LogInstance().SetRateLimiting(nullptr);
        LogInstance().DisableCategory(BCLog::LogFlags::ALL);
        LogInstance().EnableCategory(BCLog::LogFlags{prev_category_mask});
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

    struct Case {
        std::string msg;
        BCLog::LogFlags category;
        BCLog::Level level;
        std::string prefix;
        std::source_location loc;
    };

    std::vector<Case> cases = {
        {"foo1: bar1", BCLog::NET, BCLog::Level::Debug, "[net] ", std::source_location::current()},
        {"foo2: bar2", BCLog::NET, BCLog::Level::Info, "[net:info] ", std::source_location::current()},
        {"foo3: bar3", BCLog::ALL, BCLog::Level::Debug, "[debug] ", std::source_location::current()},
        {"foo4: bar4", BCLog::ALL, BCLog::Level::Info, "", std::source_location::current()},
        {"foo5: bar5", BCLog::NONE, BCLog::Level::Debug, "[debug] ", std::source_location::current()},
        {"foo6: bar6", BCLog::NONE, BCLog::Level::Info, "", std::source_location::current()},
    };

    std::vector<std::string> expected;
    for (auto& [msg, category, level, prefix, loc] : cases) {
        expected.push_back(tfm::format("[%s:%s] [%s] %s%s", util::RemovePrefix(loc.file_name(), "./"), loc.line(), loc.function_name(), prefix, msg));
        LogInstance().LogPrintStr(msg, std::move(loc), category, level, /*should_ratelimit=*/false);
    }
    std::vector<std::string> log_lines{ReadDebugLogLines()};
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintMacrosDeprecated, LogSetup)
{
    LogInstance().EnableCategory(BCLog::NET);
    LogPrintLevel(BCLog::NET, BCLog::Level::Trace, "foo4: %s\n", "bar4"); // not logged
    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "foo7: %s\n", "bar7");
    LogPrintLevel(BCLog::NET, BCLog::Level::Info, "foo8: %s\n", "bar8");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "foo9: %s\n", "bar9");
    LogPrintLevel(BCLog::NET, BCLog::Level::Error, "foo10: %s\n", "bar10");
    std::vector<std::string> log_lines{ReadDebugLogLines()};
    std::vector<std::string> expected{
        "[net] foo7: bar7",
        "[net:info] foo8: bar8",
        "[net:warning] foo9: bar9",
        "[net:error] foo10: bar10",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_LogPrintMacros, LogSetup)
{
    LogInstance().EnableCategory(BCLog::NET);
    LogTrace(BCLog::NET, "foo6: %s", "bar6"); // not logged
    LogDebug(BCLog::NET, "foo7: %s", "bar7");
    LogInfo("foo8: %s", "bar8");
    LogWarning("foo9: %s", "bar9");
    LogError("foo10: %s", "bar10");
    std::vector<std::string> log_lines{ReadDebugLogLines()};
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
        std::string expected_log = "[";
        expected_log += name;
        expected_log += "] foo: bar";
        expected.push_back(expected_log);
    }

    std::vector<std::string> log_lines{ReadDebugLogLines()};
    BOOST_CHECK_EQUAL_COLLECTIONS(log_lines.begin(), log_lines.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(logging_SeverityLevels, LogSetup)
{
    LogInstance().EnableCategory(BCLog::LogFlags::ALL);
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
    std::vector<std::string> log_lines{ReadDebugLogLines()};
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

struct ScopedScheduler {
    CScheduler scheduler{};

    ScopedScheduler()
    {
        scheduler.m_service_thread = std::thread([this] { scheduler.serviceQueue(); });
    }
    ~ScopedScheduler()
    {
        scheduler.stop();
    }
    void MockForwardAndSync(std::chrono::seconds duration)
    {
        scheduler.MockForward(duration);
        std::promise<void> promise;
        scheduler.scheduleFromNow([&promise] { promise.set_value(); }, 0ms);
        promise.get_future().wait();
    }
    std::shared_ptr<BCLog::LogRateLimiter> GetLimiter(size_t max_bytes, std::chrono::seconds window)
    {
        auto sched_func = [this](auto func, auto w) {
            scheduler.scheduleEvery(std::move(func), w);
        };
        return BCLog::LogRateLimiter::Create(sched_func, max_bytes, window);
    }
};

BOOST_AUTO_TEST_CASE(logging_log_rate_limiter)
{
    uint64_t max_bytes{1024};
    auto reset_window{1min};
    ScopedScheduler scheduler{};
    auto limiter_{scheduler.GetLimiter(max_bytes, reset_window)};
    auto& limiter{*Assert(limiter_)};

    using Status = BCLog::LogRateLimiter::Status;
    auto source_loc_1{std::source_location::current()};
    auto source_loc_2{std::source_location::current()};

    // A fresh limiter should not have any suppressions
    BOOST_CHECK(!limiter.SuppressionsActive());

    // Resetting an unused limiter is fine
    limiter.Reset();
    BOOST_CHECK(!limiter.SuppressionsActive());

    // No suppression should happen until more than max_bytes have been consumed
    BOOST_CHECK_EQUAL(limiter.Consume(source_loc_1, std::string(max_bytes - 1, 'a')), Status::UNSUPPRESSED);
    BOOST_CHECK_EQUAL(limiter.Consume(source_loc_1, "a"), Status::UNSUPPRESSED);
    BOOST_CHECK(!limiter.SuppressionsActive());
    BOOST_CHECK_EQUAL(limiter.Consume(source_loc_1, "a"), Status::NEWLY_SUPPRESSED);
    BOOST_CHECK(limiter.SuppressionsActive());
    BOOST_CHECK_EQUAL(limiter.Consume(source_loc_1, "a"), Status::STILL_SUPPRESSED);
    BOOST_CHECK(limiter.SuppressionsActive());

    // Location 2  should not be affected by location 1's suppression
    BOOST_CHECK_EQUAL(limiter.Consume(source_loc_2, std::string(max_bytes, 'a')), Status::UNSUPPRESSED);
    BOOST_CHECK_EQUAL(limiter.Consume(source_loc_2, "a"), Status::NEWLY_SUPPRESSED);
    BOOST_CHECK(limiter.SuppressionsActive());

    // After reset_window time has passed, all suppressions should be cleared.
    scheduler.MockForwardAndSync(reset_window);

    BOOST_CHECK(!limiter.SuppressionsActive());
    BOOST_CHECK_EQUAL(limiter.Consume(source_loc_1, std::string(max_bytes, 'a')), Status::UNSUPPRESSED);
    BOOST_CHECK_EQUAL(limiter.Consume(source_loc_2, std::string(max_bytes, 'a')), Status::UNSUPPRESSED);
}

BOOST_AUTO_TEST_CASE(logging_log_limit_stats)
{
    BCLog::LogRateLimiter::Stats stats(BCLog::RATELIMIT_MAX_BYTES);

    // Check that stats gets initialized correctly.
    BOOST_CHECK_EQUAL(stats.m_available_bytes, BCLog::RATELIMIT_MAX_BYTES);
    BOOST_CHECK_EQUAL(stats.m_dropped_bytes, uint64_t{0});

    const uint64_t MESSAGE_SIZE{BCLog::RATELIMIT_MAX_BYTES / 2};
    BOOST_CHECK(stats.Consume(MESSAGE_SIZE));
    BOOST_CHECK_EQUAL(stats.m_available_bytes, BCLog::RATELIMIT_MAX_BYTES - MESSAGE_SIZE);
    BOOST_CHECK_EQUAL(stats.m_dropped_bytes, uint64_t{0});

    BOOST_CHECK(stats.Consume(MESSAGE_SIZE));
    BOOST_CHECK_EQUAL(stats.m_available_bytes, BCLog::RATELIMIT_MAX_BYTES - MESSAGE_SIZE * 2);
    BOOST_CHECK_EQUAL(stats.m_dropped_bytes, uint64_t{0});

    // Consuming more bytes after already having consumed RATELIMIT_MAX_BYTES should fail.
    BOOST_CHECK(!stats.Consume(500));
    BOOST_CHECK_EQUAL(stats.m_available_bytes, uint64_t{0});
    BOOST_CHECK_EQUAL(stats.m_dropped_bytes, uint64_t{500});
}

namespace {

enum class Location {
    INFO_1,
    INFO_2,
    DEBUG_LOG,
    INFO_NOLIMIT,
};

void LogFromLocation(Location location, const std::string& message) {
    switch (location) {
    case Location::INFO_1:
        LogInfo("%s\n", message);
        return;
    case Location::INFO_2:
        LogInfo("%s\n", message);
        return;
    case Location::DEBUG_LOG:
        LogDebug(BCLog::LogFlags::HTTP, "%s\n", message);
        return;
    case Location::INFO_NOLIMIT:
        LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Info, /*should_ratelimit=*/false, "%s\n", message);
        return;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

/**
 * For a given `location` and `message`, ensure that the on-disk debug log behaviour resembles what
 * we'd expect it to be for `status` and `suppressions_active`.
 */
void TestLogFromLocation(Location location, const std::string& message,
                         BCLog::LogRateLimiter::Status status, bool suppressions_active,
                         std::source_location source = std::source_location::current())
{
    BOOST_TEST_INFO_SCOPE("TestLogFromLocation called from " << source.file_name() << ":" << source.line());
    using Status = BCLog::LogRateLimiter::Status;
    if (!suppressions_active) assert(status == Status::UNSUPPRESSED); // developer error

    std::ofstream ofs(LogInstance().m_file_path.std_path(), std::ios::out | std::ios::trunc); // clear debug log
    LogFromLocation(location, message);
    auto log_lines{ReadDebugLogLines()};
    BOOST_TEST_INFO_SCOPE(log_lines.size() << " log_lines read: \n" << util::Join(log_lines, "\n"));

    if (status == Status::STILL_SUPPRESSED) {
        BOOST_CHECK_EQUAL(log_lines.size(), 0);
        return;
    }

    if (status == Status::NEWLY_SUPPRESSED) {
        BOOST_REQUIRE_EQUAL(log_lines.size(), 2);
        BOOST_CHECK(log_lines[0].starts_with("[*] [warning] Excessive logging detected"));
        log_lines.erase(log_lines.begin());
    }
    BOOST_REQUIRE_EQUAL(log_lines.size(), 1);
    auto& payload{log_lines.back()};
    BOOST_CHECK_EQUAL(suppressions_active, payload.starts_with("[*]"));
    BOOST_CHECK(payload.ends_with(message));
}

} // namespace

BOOST_FIXTURE_TEST_CASE(logging_filesize_rate_limit, LogSetup)
{
    using Status = BCLog::LogRateLimiter::Status;
    LogInstance().m_log_timestamps = false;
    LogInstance().m_log_sourcelocations = false;
    LogInstance().m_log_threadnames = false;
    LogInstance().EnableCategory(BCLog::LogFlags::HTTP);

    constexpr int64_t line_length{1024};
    constexpr int64_t num_lines{10};
    constexpr int64_t bytes_quota{line_length * num_lines};
    constexpr auto time_window{1h};

    ScopedScheduler scheduler{};
    auto limiter{scheduler.GetLimiter(bytes_quota, time_window)};
    LogInstance().SetRateLimiting(limiter);

    const std::string log_message(line_length - 1, 'a'); // subtract one for newline

    for (int i = 0; i < num_lines; ++i) {
        TestLogFromLocation(Location::INFO_1, log_message, Status::UNSUPPRESSED, /*suppressions_active=*/false);
    }
    TestLogFromLocation(Location::INFO_1, "a", Status::NEWLY_SUPPRESSED, /*suppressions_active=*/true);
    TestLogFromLocation(Location::INFO_1, "b", Status::STILL_SUPPRESSED, /*suppressions_active=*/true);
    TestLogFromLocation(Location::INFO_2, "c", Status::UNSUPPRESSED, /*suppressions_active=*/true);
    {
        scheduler.MockForwardAndSync(time_window);
        BOOST_CHECK(ReadDebugLogLines().back().starts_with("[warning] Restarting logging"));
    }
    // Check that logging from previously suppressed location is unsuppressed again.
    TestLogFromLocation(Location::INFO_1, log_message, Status::UNSUPPRESSED, /*suppressions_active=*/false);
    // Check that conditional logging, and unconditional logging with should_ratelimit=false is
    // not being ratelimited.
    for (Location location : {Location::DEBUG_LOG, Location::INFO_NOLIMIT}) {
        for (int i = 0; i < num_lines + 2; ++i) {
            TestLogFromLocation(location, log_message, Status::UNSUPPRESSED, /*suppressions_active=*/false);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
