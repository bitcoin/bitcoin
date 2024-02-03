// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TIME_H
#define BITCOIN_UTIL_TIME_H

#include <compat.h>

#include <chrono>
#include <stdint.h>
#include <string>

using namespace std::chrono_literals;

using SteadyClock = std::chrono::steady_clock;
using SteadySeconds = std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>;
using SteadyMilliseconds = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;
using SteadyMicroseconds = std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>;

void UninterruptibleSleep(const std::chrono::microseconds& n);

/**
 * Helper to count the seconds of a duration.
 *
 * All durations should be using std::chrono and calling this should generally
 * be avoided in code. Though, it is still preferred to an inline t.count() to
 * protect against a reliance on the exact type of t.
 *
 * This helper is used to convert durations before passing them over an
 * interface that doesn't support std::chrono (e.g. RPC, debug log, or the GUI)
 */
inline int64_t count_seconds(std::chrono::seconds t) { return t.count(); }
inline int64_t count_milliseconds(std::chrono::milliseconds t) { return t.count(); }
inline int64_t count_microseconds(std::chrono::microseconds t) { return t.count(); }

/**
 * DEPRECATED
 * Use either GetSystemTimeInSeconds (not mockable) or GetTime<T> (mockable)
 */
int64_t GetTime();

/** Returns the system time (not mockable) */
int64_t GetTimeMillis();
/** Returns the system time (not mockable) */
int64_t GetTimeMicros();
/** Returns the system time (not mockable) */
int64_t GetSystemTimeInSeconds(); // Like GetTime(), but not mockable

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

/** Return system time (or mocked time, if set) */
template <typename T>
T GetTime();
/**
 * Return the current time point cast to the given precicion. Only use this
 * when an exact precicion is needed, otherwise use T::clock::now() directly.
 */
template <typename T>
T Now()
{
    return std::chrono::time_point_cast<typename T::duration>(T::clock::now());
}

/**
 * ISO 8601 formatting is preferred. Use the FormatISO8601{DateTime,Date,Time}
 * helper functions if possible.
 */
std::string FormatISO8601DateTime(int64_t nTime);
std::string FormatISO8601Date(int64_t nTime);
std::string FormatISO8601Time(int64_t nTime);
int64_t ParseISO8601DateTime(const std::string& str);

/**
 * Convert milliseconds to a struct timeval for e.g. select.
 */
struct timeval MillisToTimeval(int64_t nTimeout);

/**
 * Convert milliseconds to a struct timeval for e.g. select.
 */
struct timeval MillisToTimeval(std::chrono::milliseconds ms);

/** Sanity check epoch match normal Unix epoch */
bool ChronoSanityCheck();

#endif // BITCOIN_UTIL_TIME_H
