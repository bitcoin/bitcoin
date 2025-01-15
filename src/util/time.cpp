// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <util/time.h>

#include <compat/compat.h>
#include <tinyformat.h>
#include <util/check.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#ifdef HAVE_CLOCK_GETTIME
#include <time.h>
#elif defined(HAVE_GETTHREADTIMES)
#include <windows.h>
#include <winnt.h>

#include <processthreadsapi.h>
#endif


void UninterruptibleSleep(const std::chrono::microseconds& n) { std::this_thread::sleep_for(n); }

static std::atomic<std::chrono::seconds> g_mock_time{}; //!< For testing

NodeClock::time_point NodeClock::now() noexcept
{
    const auto mocktime{g_mock_time.load(std::memory_order_relaxed)};
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
#ifdef HAVE_CLOCK_GETTIME
    // An alternative to clock_gettime() is getrusage().

    timespec t;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t) == -1) {
        return std::chrono::nanoseconds{0};
    }
    return std::chrono::seconds{t.tv_sec} + std::chrono::nanoseconds{t.tv_nsec};
#elif defined(HAVE_GETTHREADTIMES)
    // An alternative to GetThreadTimes() is QueryThreadCycleTime() but it
    // counts CPU cycles.

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

    // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadtimes
    // "Thread kernel mode and user mode times are amounts of time. For example,
    // if a thread has spent one second in kernel mode, this function will fill
    // the FILETIME structure specified by lpKernelTime with a 64-bit value of
    // ten million. That is the number of 100-nanosecond units in one second."
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
