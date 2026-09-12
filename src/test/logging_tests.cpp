// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init/common.h>
#include <logging.h>
#include <logging/timer.h>
#include <scheduler.h>
#include <test/util/common.h>
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
#include <stdexcept>
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

static std::vector<std::string> ReadDebugLogLines(const fs::path& file_path = LogInstance().m_file_path)
{
    std::vector<std::string> lines;
    std::ifstream ifs{file_path.std_path()};
    for (std::string line; std::getline(ifs, line);) {
        lines.push_back(std::move(line));
    }
    return lines;
}

struct FileLogSetup : BasicTestingSetup {
    BCLog::Logger logger;
    const fs::path log_path{m_args.GetDataDirBase() / "file.log"};

    FileLogSetup() { logger.m_log_timestamps = false; }
    ~FileLogSetup() { logger.DisconnectTestLogger(); }

    void Log(std::string message)
    {
        logger.LogPrint({.category = BCLog::ALL, .level = BCLog::Level::Info, .source_loc = SourceLocation{__func__}, .message = std::move(message)});
    }

    std::vector<std::string> Lines() const
    {
        auto lines{ReadDebugLogLines(log_path)};
        std::erase(lines, std::string{}); // Ignore the session separators.
        return lines;
    }
};

BOOST_FIXTURE_TEST_CASE(logging_file_lifecycle, FileLogSetup)
{
    {
        std::ofstream file{log_path.std_path()};
        file << "existing\n";
    }
    Log("before opening");
    BOOST_REQUIRE(logger.StartFileLogging(log_path));
    Log("while open");
    logger.m_reopen_file = true;
    logger.StopFileLogging();
    BOOST_CHECK(!logger.m_print_to_file);
    BOOST_CHECK(logger.m_file_path.empty());
    BOOST_CHECK(!logger.m_reopen_file);
    BOOST_CHECK(logger.Enabled());
    BOOST_CHECK(Lines() == (std::vector<std::string>{"existing", "before opening", "while open"}));

    Log("after closing");
    BOOST_CHECK(Lines() == (std::vector<std::string>{"existing", "before opening", "while open"}));
    BOOST_REQUIRE(logger.StartFileLogging(log_path));
    Log("after reopening");
    logger.StopFileLogging();
    BOOST_CHECK(Lines() == (std::vector<std::string>{"existing", "before opening", "while open", "after closing", "after reopening"}));
}

BOOST_FIXTURE_TEST_CASE(logging_file_open_failure, FileLogSetup)
{
    Log("retained after failure");
    const auto missing_parent{m_args.GetDataDirBase() / "missing" / "file.log"};
    BOOST_CHECK(!logger.StartFileLogging(missing_parent));
    BOOST_CHECK(!fs::exists(missing_parent));
    BOOST_CHECK(!logger.m_print_to_file);
    BOOST_CHECK(logger.m_file_path.empty());
    BOOST_CHECK(!logger.m_reopen_file);
    BOOST_REQUIRE(logger.StartFileLogging(log_path));
    Log("after retry");
    logger.StopFileLogging();
    BOOST_CHECK(Lines() == (std::vector<std::string>{"retained after failure", "after retry"}));
}

BOOST_FIXTURE_TEST_CASE(logging_file_duplicate, FileLogSetup)
{
    BOOST_REQUIRE(logger.StartFileLogging(log_path));
    const auto rejected_path{m_args.GetDataDirBase() / "rejected.log"};
    logger.m_reopen_file = true;
    BOOST_CHECK(!logger.StartFileLogging(rejected_path));
    BOOST_CHECK(logger.m_reopen_file);
    BOOST_CHECK(logger.m_file_path == log_path);
    BOOST_CHECK(!fs::exists(rejected_path));
    {
        std::ofstream file{rejected_path.std_path()};
        file << "unchanged\n";
    }
    BOOST_CHECK(!logger.StartFileLogging(rejected_path));
    BOOST_CHECK(ReadDebugLogLines(rejected_path) == (std::vector<std::string>{"unchanged"}));
    Log("original output still active");
    logger.StopFileLogging();
    BOOST_CHECK(Lines() == (std::vector<std::string>{"original output still active"}));
}

BOOST_FIXTURE_TEST_CASE(logging_file_disabled, FileLogSetup)
{
    logger.DisableLogging();
    BOOST_CHECK(!logger.Enabled());
    BOOST_CHECK(!logger.StartFileLogging(log_path));
    BOOST_CHECK(!logger.Enabled());
    BOOST_CHECK(!fs::exists(log_path));
}

BOOST_FIXTURE_TEST_CASE(logging_file_existing_output, FileLogSetup)
{
    logger.m_print_to_file = true;
    logger.m_file_path = log_path;
    const auto rejected_path{m_args.GetDataDirBase() / "rejected.log"};
    BOOST_CHECK(!logger.StartFileLogging(rejected_path));
    BOOST_CHECK(logger.m_print_to_file);
    BOOST_CHECK(logger.m_file_path == log_path);
    BOOST_CHECK(!fs::exists(log_path));
    BOOST_CHECK(!fs::exists(rejected_path));

    BOOST_REQUIRE(logger.StartLogging());
    BOOST_CHECK(!logger.StartFileLogging(rejected_path));
    Log("node output still active");
    BOOST_CHECK(Lines() == (std::vector<std::string>{"node output still active"}));
    BOOST_CHECK(!fs::exists(rejected_path));

    BCLog::Logger console;
    console.m_print_to_console = true;
    BOOST_CHECK(!console.StartFileLogging(rejected_path));
    BOOST_CHECK(console.m_print_to_console);

    BCLog::Logger configured_path;
    configured_path.m_file_path = log_path;
    BOOST_CHECK(!configured_path.StartFileLogging(rejected_path));
    BOOST_CHECK(configured_path.m_file_path == log_path);
    BOOST_CHECK(!fs::exists(rejected_path));
}

BOOST_FIXTURE_TEST_CASE(logging_file_callbacks, FileLogSetup)
{
    std::vector<std::string> messages;
    const auto connection{logger.PushBackCallback([&](const std::string& message) { messages.push_back(message); })};
    Log("buffered");
    BOOST_CHECK(messages.empty());
    BOOST_REQUIRE(logger.StartFileLogging(log_path));
    Log("both outputs");
    logger.StopFileLogging();
    BOOST_CHECK_EQUAL(logger.NumConnections(), 1);
    Log("callback only");
    BOOST_CHECK(messages == (std::vector<std::string>{"buffered\n", "both outputs\n", "callback only\n"}));
    BOOST_CHECK(Lines() == (std::vector<std::string>{"buffered", "both outputs"}));
    const auto rejected_path{m_args.GetDataDirBase() / "rejected.log"};
    BOOST_CHECK(!logger.StartFileLogging(rejected_path));
    BOOST_CHECK(!fs::exists(rejected_path));
    logger.DeleteCallback(connection);
    BOOST_CHECK_EQUAL(logger.NumConnections(), 0);
    BOOST_CHECK(!logger.Enabled());
    BOOST_CHECK(!logger.StartFileLogging(rejected_path));
}

BOOST_FIXTURE_TEST_CASE(logging_file_console, FileLogSetup)
{
    BOOST_REQUIRE(logger.StartFileLogging(log_path));
    logger.m_print_to_console = true;
    logger.StopFileLogging();
    BOOST_CHECK(logger.m_print_to_console);
    std::string message;
    const auto connection{logger.PushBackCallback([&](const std::string& value) { message = value; })};
    Log("console output remains active");
    BOOST_CHECK_EQUAL(message, "console output remains active\n");
    BOOST_CHECK(Lines().empty());
    logger.DeleteCallback(connection);
}

BOOST_FIXTURE_TEST_CASE(logging_file_replay_exception, FileLogSetup)
{
    // Discard an oversized entry, then retain two entries well within the buffer limit.
    Log(std::string(2 * BCLog::DEFAULT_MAX_LOG_BUFFER, 'x'));
    const std::string first(BCLog::DEFAULT_MAX_LOG_BUFFER / 3, 'a');
    const std::string retained(BCLog::DEFAULT_MAX_LOG_BUFFER / 3, 'b');
    Log(first);
    Log(retained);
    bool throw_once{true};
    const auto connection{logger.PushBackCallback([&](const std::string& message) {
        if (throw_once && message.starts_with("aaa")) {
            throw_once = false;
            throw std::runtime_error("startup replay interrupted");
        }
    })};
    BOOST_CHECK_THROW(logger.StartFileLogging(log_path), std::runtime_error);
    BOOST_CHECK(!logger.m_print_to_file);
    BOOST_CHECK(logger.m_file_path.empty());
    BOOST_CHECK(!logger.m_reopen_file);
    BOOST_CHECK_EQUAL(logger.NumConnections(), 1);
    const auto partial{Lines()};
    BOOST_REQUIRE_EQUAL(partial.size(), 2);
    BOOST_CHECK(partial.front().starts_with("Early logging buffer overflowed, "));
    BOOST_CHECK_EQUAL(partial.back(), first);

    // This fits beside the retained entry only if the consumed entry's memory
    // has been removed from the accounting. The old discard notice must not recur.
    const std::string after_failure(BCLog::DEFAULT_MAX_LOG_BUFFER / 2, 'c');
    Log(after_failure);
    BOOST_REQUIRE(logger.StartFileLogging(log_path));
    logger.StopFileLogging();
    BOOST_CHECK(Lines() == (std::vector<std::string>{partial.front(), first, retained, after_failure}));
    logger.DeleteCallback(connection);
}

BOOST_FIXTURE_TEST_CASE(logging_startup_overflow, FileLogSetup)
{
    logger.m_file_path = log_path;
    logger.m_print_to_file = true;
    logger.m_log_sourcelocations = true;
    Log(std::string(2 * BCLog::DEFAULT_MAX_LOG_BUFFER, 'x'));
    Log("retained");
    BOOST_REQUIRE(logger.StartLogging());
    const auto lines{Lines()};
    BOOST_REQUIRE_EQUAL(lines.size(), 2);
    BOOST_CHECK(lines.front().find("[StartLogging] Early logging buffer overflowed, 1 log lines discarded.") != std::string::npos);
    BOOST_CHECK(lines.back().ends_with("retained"));
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

BOOST_FIXTURE_TEST_CASE(logging_LogPrint, LogSetup)
{
    LogInstance().m_log_sourcelocations = true;

    struct Case {
        std::string msg;
        BCLog::LogFlags category;
        BCLog::Level level;
        std::string prefix;
        SourceLocation loc;
    };

    std::vector<Case> cases = {
        {"foo1: bar1", BCLog::NET, BCLog::Level::Debug, "[net] ", SourceLocation{__func__}},
        {"foo2: bar2", BCLog::NET, BCLog::Level::Info, "[net:info] ", SourceLocation{__func__}},
        {"foo3: bar3", BCLog::ALL, BCLog::Level::Debug, "[debug] ", SourceLocation{__func__}},
        {"foo4: bar4", BCLog::ALL, BCLog::Level::Info, "", SourceLocation{__func__}},
        {"foo5: bar5", BCLog::NONE, BCLog::Level::Debug, "[debug] ", SourceLocation{__func__}},
        {"foo6: bar6", BCLog::NONE, BCLog::Level::Info, "", SourceLocation{__func__}},
    };

    std::vector<std::string> expected;
    for (auto& [msg, category, level, prefix, loc] : cases) {
        expected.push_back(tfm::format("[%s:%s] [%s] %s%s", util::RemovePrefix(loc.file_name(), "./"), loc.line(), loc.function_name_short(), prefix, msg));
        LogInstance().LogPrint({.category = category, .level = level, .should_ratelimit = false, .source_loc = std::move(loc), .message = msg});
    }
    std::vector<std::string> log_lines{ReadDebugLogLines()};
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
        const auto trimmed_category_name = TrimString(category_name);
        const auto category{*Assert(BCLog::Logger::GetLogCategory(trimmed_category_name))};
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
    LogInstance().SetLogLevel(BCLog::Level::Debug);
    LogInstance().EnableCategory(BCLog::LogFlags::ALL);
    LogInstance().SetCategoryLogLevel(/*category_str=*/"net", /*level_str=*/"info");

    // Global log level
    LogInfo("info_%s", 1);
    LogTrace(BCLog::HTTP, "trace_%s. This log level is lower than the global one.", 2);
    LogDebug(BCLog::HTTP, "debug_%s", 3);
    LogWarning("warn_%s", 4);
    LogError("err_%s", 5);

    // Category-specific log level
    LogDebug(BCLog::NET, "debug_%s. This log level is the same as the global one but lower than the category-specific one, which takes precedence.", 6);

    std::vector<std::string> expected = {
        "info_1",
        "[http] debug_3",
        "[warning] warn_4",
        "[error] err_5",
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

    // Removed categories (like "libevent") should not store a category-specific level
    {
        ResetLogger();
        BOOST_CHECK(LogInstance().SetCategoryLogLevel(/*category_str=*/"libevent", /*level_str=*/"trace"));
        BOOST_CHECK(LogInstance().CategoryLevels().empty());
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
    auto source_loc_1{SourceLocation{__func__}};
    auto source_loc_2{SourceLocation{__func__}};

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
        LogInfo(util::log::NO_RATE_LIMIT, "%s\n", message);
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
