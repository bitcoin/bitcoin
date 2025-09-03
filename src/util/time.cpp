// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/time.h>

#include <compat/compat.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#ifdef WIN32
#include <windows.h>
#include <winnt.h>

#include <processthreadsapi.h>
#else
#include <ctime>
#endif


void UninterruptibleSleep(const std::chrono::microseconds& n) { std::this_thread::sleep_for(n); }

static std::atomic<std::chrono::seconds> g_mock_time{}; //!< For testing
std::atomic<bool> g_used_system_time{false};
static std::atomic<std::chrono::milliseconds> g_mock_steady_time{}; //!< For testing

NodeClock::time_point NodeClock::now() noexcept
{
    const auto mocktime{g_mock_time.load(std::memory_order_relaxed)};
    if (!mocktime.count()) {
        g_used_system_time = true;
    }
    const auto ret{
        mocktime.count() ?
            mocktime :
            std::chrono::system_clock::now().time_since_epoch()};
    assert(ret > 0s);
    return time_point{ret};
};

void SetMockTime(int64_t nMockTimeIn) { SetMockTime(std::chrono::seconds{nMockTimeIn}); }
void SetMockTime(std::chrono::seconds mock_time_in)
{
    Assert(mock_time_in >= 0s);
    g_mock_time.store(mock_time_in, std::memory_order_relaxed);
}

std::chrono::seconds GetMockTime()
{
    return g_mock_time.load(std::memory_order_relaxed);
}

MockableSteadyClock::time_point MockableSteadyClock::now() noexcept
{
    const auto mocktime{g_mock_steady_time.load(std::memory_order_relaxed)};
    if (!mocktime.count()) {
        g_used_system_time = true;
    }
    const auto ret{
        mocktime.count() ?
            mocktime :
            std::chrono::steady_clock::now().time_since_epoch()};
    return time_point{ret};
};

void MockableSteadyClock::SetMockTime(std::chrono::milliseconds mock_time_in)
{
    Assert(mock_time_in >= 0s);
    g_mock_steady_time.store(mock_time_in, std::memory_order_relaxed);
}

void MockableSteadyClock::ClearMockTime()
{
    g_mock_steady_time.store(0ms, std::memory_order_relaxed);
}

int64_t GetTime() { return GetTime<std::chrono::seconds>().count(); }

std::string FormatISO8601DateTime(int64_t nTime)
{
    const std::chrono::sys_seconds secs{std::chrono::seconds{nTime}};
    const auto days{std::chrono::floor<std::chrono::days>(secs)};
    const std::chrono::year_month_day ymd{days};
    const std::chrono::hh_mm_ss hms{secs - days};
    return strprintf("%04i-%02u-%02uT%02i:%02i:%02iZ", signed{ymd.year()}, unsigned{ymd.month()}, unsigned{ymd.day()}, hms.hours().count(), hms.minutes().count(), hms.seconds().count());
}

std::string FormatISO8601Date(int64_t nTime)
{
    const std::chrono::sys_seconds secs{std::chrono::seconds{nTime}};
    const auto days{std::chrono::floor<std::chrono::days>(secs)};
    const std::chrono::year_month_day ymd{days};
    return strprintf("%04i-%02u-%02u", signed{ymd.year()}, unsigned{ymd.month()}, unsigned{ymd.day()});
}

std::optional<int64_t> ParseISO8601DateTime(std::string_view str)
{
    constexpr auto FMT_SIZE{std::string_view{"2000-01-01T01:01:01Z"}.size()};
    if (str.size() != FMT_SIZE || str[4] != '-' || str[7] != '-' || str[10] != 'T' || str[13] != ':' || str[16] != ':' || str[19] != 'Z') {
        return {};
    }
    const auto year{ToIntegral<uint16_t>(str.substr(0, 4))};
    const auto month{ToIntegral<uint8_t>(str.substr(5, 2))};
    const auto day{ToIntegral<uint8_t>(str.substr(8, 2))};
    const auto hour{ToIntegral<uint8_t>(str.substr(11, 2))};
    const auto min{ToIntegral<uint8_t>(str.substr(14, 2))};
    const auto sec{ToIntegral<uint8_t>(str.substr(17, 2))};
    if (!year || !month || !day || !hour || !min || !sec) {
        return {};
    }
    const std::chrono::year_month_day ymd{std::chrono::year{*year}, std::chrono::month{*month}, std::chrono::day{*day}};
    if (!ymd.ok()) {
        return {};
    }
    const auto time{std::chrono::hours{*hour} + std::chrono::minutes{*min} + std::chrono::seconds{*sec}};
    const auto tp{std::chrono::sys_days{ymd} + time};
    return int64_t{TicksSinceEpoch<std::chrono::seconds>(tp)};
}

std::string FormatISO8601Time(int64_t nTime)
{
    const std::chrono::sys_seconds secs{std::chrono::seconds{nTime}};
    const auto days{std::chrono::floor<std::chrono::days>(secs)};
    const std::chrono::hh_mm_ss hms{secs - days};
    return strprintf("%02i:%02i:%02iZ", hms.hours().count(), hms.minutes().count(), hms.seconds().count());
}

struct timeval MillisToTimeval(int64_t nTimeout)
{
    struct timeval timeout;
    timeout.tv_sec  = nTimeout / 1000;
    timeout.tv_usec = (nTimeout % 1000) * 1000;
    return timeout;
}

struct timeval MillisToTimeval(std::chrono::milliseconds ms)
{
    return MillisToTimeval(count_milliseconds(ms));
}

std::chrono::nanoseconds ThreadCpuTime()
{
#ifdef CLOCK_THREAD_CPUTIME_ID
    timespec t;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t) == -1) {
        return std::chrono::nanoseconds{0};
    }
    return std::chrono::seconds{t.tv_sec} + std::chrono::nanoseconds{t.tv_nsec};
#elif defined(WIN32)
    FILETIME creation;
    FILETIME exit;
    FILETIME kernel;
    FILETIME user;
    // GetThreadTimes():
    // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadtimes
    if (GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user) == 0) {
        return std::chrono::nanoseconds{0};
    }

    // https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
    // "... you should copy the low- and high-order parts of the file time to a
    // ULARGE_INTEGER structure, perform 64-bit arithmetic on the QuadPart
    // member ..."

    ULARGE_INTEGER kernel_;
    kernel_.LowPart = kernel.dwLowDateTime;
    kernel_.HighPart = kernel.dwHighDateTime;

    ULARGE_INTEGER user_;
    user_.LowPart = user.dwLowDateTime;
    user_.HighPart = user.dwHighDateTime;

    // The units of the returned values from GetThreadTimes() are "100-nanosecond periods".
    // So, we multiply by 100 to get nanoseconds.
    return std::chrono::nanoseconds{(kernel_.QuadPart + user_.QuadPart) * 100};
#else
    return std::chrono::nanoseconds{0};
#endif
}

std::chrono::nanoseconds operator+=(std::atomic<std::chrono::nanoseconds>& a, std::chrono::nanoseconds b)
{
    std::chrono::nanoseconds expected;
    std::chrono::nanoseconds desired;
    do {
        expected = a.load();
        desired = expected + b;
    } while (!a.compare_exchange_weak(expected, desired));
    return desired;
}
