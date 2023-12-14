// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TIME_H
#define BITCOIN_UTIL_TIME_H

#include <chrono> // IWYU pragma: export
#include <cstdint>
#include <string>

using namespace std::chrono_literals;

/** Mockable clock in the context of tests, otherwise the system clock */
struct NodeClock : public std::chrono::system_clock {
    using time_point = std::chrono::time_point<NodeClock>;
    /** Return current system time or mocked time, if set */
    static time_point now() noexcept;
    static std::time_t to_time_t(const time_point&) = delete; // unused
    static time_point from_time_t(std::time_t) = delete;      // unused
};
using NodeSeconds = std::chrono::time_point<NodeClock, std::chrono::seconds>;

using SteadyClock = std::chrono::steady_clock;
using SteadySeconds = std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>;
using SteadyMilliseconds = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;
using SteadyMicroseconds = std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>;

using SystemClock = std::chrono::system_clock;

void UninterruptibleSleep(const std::chrono::microseconds& n);

/**
 * Helper to count the seconds of a duration/time_point.
 *
 * All durations/time_points should be using std::chrono and calling this should generally
 * be avoided in code. Though, it is still preferred to an inline t.count() to
 * protect against a reliance on the exact type of t.
 *
 * This helper is used to convert durations/time_points before passing them over an
 * interface that doesn't support std::chrono (e.g. RPC, debug log, or the GUI)
 */
template <typename Dur1, typename Dur2>
constexpr auto Ticks(Dur2 d)
{
    return std::chrono::duration_cast<Dur1>(d).count();
}
template <typename Duration, typename Timepoint>
constexpr auto TicksSinceEpoch(Timepoint t)
{
    return Ticks<Duration>(t.time_since_epoch());
}
constexpr int64_t count_seconds(std::chrono::seconds t) { return t.count(); }
constexpr int64_t count_milliseconds(std::chrono::milliseconds t) { return t.count(); }
constexpr int64_t count_microseconds(std::chrono::microseconds t) { return t.count(); }

using HoursDouble = std::chrono::duration<double, std::chrono::hours::period>;
using SecondsDouble = std::chrono::duration<double, std::chrono::seconds::period>;
using MillisecondsDouble = std::chrono::duration<double, std::chrono::milliseconds::period>;

/**
 * DEPRECATED
 * Use either ClockType::now() or Now<TimePointType>() if a cast is needed.
 * ClockType is
 * - SteadyClock/std::chrono::steady_clock for steady time
 * - SystemClock/std::chrono::system_clock for system time
 * - NodeClock                             for mockable system time
 */
int64_t GetTime();

/**
 * DEPRECATED
 * Use SetMockTime with chrono type
 *
 * @param[in] nMockTimeIn Time in seconds.
 */
void SetMockTime(int64_t nMockTimeIn);

/** For testing. Set e.g. with the setmocktime rpc, or -mocktime argument */
void SetMockTime(std::chrono::seconds mock_time_in);

/** For testing */
std::chrono::seconds GetMockTime();

/**
 * Return the current time point cast to the given precision. Only use this
 * when an exact precision is needed, otherwise use T::clock::now() directly.
 */
template <typename T>
T Now()
{
    return std::chrono::time_point_cast<typename T::duration>(T::clock::now());
}
/** DEPRECATED, see GetTime */
template <typename T>
T GetTime()
{
    return Now<std::chrono::time_point<NodeClock, T>>().time_since_epoch();
}

/**
 * ISO 8601 formatting is preferred. Use the FormatISO8601{DateTime,Date}
 * helper functions if possible.
 */
std::string FormatISO8601DateTime(int64_t nTime);
std::string FormatISO8601Date(int64_t nTime);

/**
 * Convert milliseconds to a struct timeval for e.g. select.
 */
struct timeval MillisToTimeval(int64_t nTimeout);

/**
 * Convert milliseconds to a struct timeval for e.g. select.
 */
struct timeval MillisToTimeval(std::chrono::milliseconds ms);

#endif // BITCOIN_UTIL_TIME_H
