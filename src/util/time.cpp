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

void UninterruptibleSleep(const std::chrono::microseconds& n) { std::this_thread::sleep_for(n); }

static std::atomic<std::chrono::seconds> g_mock_time{}; //!< For testing
std::atomic<bool> g_used_system_time{false};
static std::atomic<MockableSteadyClock::mock_time_point::duration> g_mock_steady_time{}; //!< For testing

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
void SetMockTime(std::chrono::time_point<NodeClock, std::chrono::seconds> mock) { SetMockTime(mock.time_since_epoch()); }
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

void MockableSteadyClock::SetMockTime(mock_time_point::duration mock_time_in)
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
