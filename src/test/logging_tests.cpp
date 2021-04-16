// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <logging/timer.h>
#include <test/util/setup_common.h>

#include <chrono>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(logging_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(logging_timer)
{
    SetMockTime(1);
    auto sec_timer = BCLog::Timer<std::chrono::seconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(sec_timer.LogMsg("test secs"), "tests: test secs (1.00s)");

    SetMockTime(1);
    auto ms_timer = BCLog::Timer<std::chrono::milliseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(ms_timer.LogMsg("test ms"), "tests: test ms (1000.00ms)");

    SetMockTime(1);
    auto micro_timer = BCLog::Timer<std::chrono::microseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(micro_timer.LogMsg("test micros"), "tests: test micros (1000000.00μs)");
}

void GetLogFileSize(size_t& size)
{
    boost::system::error_code ec;
    size = fs::file_size(LogInstance().m_file_path, ec);
    if (ec) LogPrintf("%s: %s %s\n", __func__, ec.message(), LogInstance().m_file_path);
    BOOST_CHECK(!ec);
}

void LogFromFixedLocation(const std::string& str)
{
    LogPrintf("%s\n", str);
}

BOOST_AUTO_TEST_CASE(rate_limiting)
{
#if defined(_WIN32)
    // TODO
    // Since windows prints \r\n to file instead of \n, the log file size
    // does not match up with the internal "bytes logged" count.
    // This test relies on matching file sizes with expected values.
    return;
#endif

    // This allows us to check for exact size differences in the log file.
    bool prev_log_timestamps = LogInstance().m_log_sourcelocations;
    LogInstance().m_log_timestamps = false;
    bool prev_log_sourcelocations = LogInstance().m_log_sourcelocations;
    LogInstance().m_log_sourcelocations = false;
    bool prev_log_threadnames = LogInstance().m_log_threadnames;
    LogInstance().m_log_threadnames = false;

    std::string log_message(1023, 'a');

    SetMockTime(std::chrono::seconds{1});

    LogInstance().ResetRateLimitingTime();

    size_t prev_log_file_size, curr_log_file_size;
    GetLogFileSize(prev_log_file_size);

    // Log 1 MiB, this should be allowed.
    for (int i = 0; i < 1024; ++i) {
        LogFromFixedLocation(log_message);
    }
    GetLogFileSize(curr_log_file_size);
    BOOST_CHECK(curr_log_file_size - prev_log_file_size == 1024 * 1024);

    LogFromFixedLocation("This should trigger rate limiting");
    GetLogFileSize(prev_log_file_size);

    // Log 0.5 MiB, this should not be allowed and all messages should be dropped.
    for (int i = 0; i < 512; ++i) {
        LogFromFixedLocation(log_message);
    }
    GetLogFileSize(curr_log_file_size);
    BOOST_CHECK(curr_log_file_size - prev_log_file_size == 0);

    // Let one hour pass.
    SetMockTime(std::chrono::seconds{60 * 60 + 2});
    LogFromFixedLocation("This should trigger the quota usage reset");
    GetLogFileSize(prev_log_file_size);

    // Log 1 MiB, this should be allowed since the usage was reset.
    for (int i = 0; i < 1024; ++i) {
        LogFromFixedLocation(log_message);
    }
    GetLogFileSize(curr_log_file_size);
    BOOST_CHECK(curr_log_file_size - prev_log_file_size == 1024 * 1024);

    LogFromFixedLocation("This should trigger rate limiting");
    GetLogFileSize(prev_log_file_size);

    // Log 1 MiB, this should not be allowed and all messages should be dropped.
    for (int i = 0; i < 1024; ++i) {
        LogFromFixedLocation(log_message);
    }
    GetLogFileSize(curr_log_file_size);
    BOOST_CHECK(curr_log_file_size - prev_log_file_size == 0);

    // Let another hour pass
    SetMockTime(std::chrono::seconds{2 * 60 * 60 + 3});
    LogFromFixedLocation("This should trigger the quota usage reset");
    GetLogFileSize(prev_log_file_size);

    // Log 1 MiB, this should be allowed since the usage was reset.
    for (int i = 0; i < 1024; ++i) {
        LogFromFixedLocation(log_message);
    }
    GetLogFileSize(curr_log_file_size);
    BOOST_CHECK(curr_log_file_size - prev_log_file_size == 1024 * 1024);

    LogInstance().m_log_timestamps = prev_log_timestamps;
    LogInstance().m_log_sourcelocations = prev_log_sourcelocations;
    LogInstance().m_log_threadnames = prev_log_threadnames;
    SetMockTime(std::chrono::seconds{0});
}

BOOST_AUTO_TEST_SUITE_END()
