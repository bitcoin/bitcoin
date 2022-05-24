// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <logging/timer.h>
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

BOOST_AUTO_TEST_SUITE_END()
