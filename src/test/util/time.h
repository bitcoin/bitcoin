// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_TIME_H
#define BITCOIN_TEST_UTIL_TIME_H

#include <util/check.h>
#include <util/time.h>


/// Helper to initialize the global MockableSteadyClock, let a duration elapse,
/// and reset it after use in a test.
class SteadyClockContext
{
    MockableSteadyClock::mock_time_point::duration t{MockableSteadyClock::INITIAL_MOCK_TIME};

public:
    /** Initialize with INITIAL_MOCK_TIME. */
    explicit SteadyClockContext() { (*this) += 0s; }

    /** Unset mocktime */
    ~SteadyClockContext() { MockableSteadyClock::ClearMockTime(); }

    SteadyClockContext(const SteadyClockContext&) = delete;
    SteadyClockContext& operator=(const SteadyClockContext&) = delete;

    /** Change mocktime by the given duration delta */
    void operator+=(std::chrono::milliseconds d)
    {
        Assert(d >= 0s); // Steady time can only increase monotonically.
        t += d;
        MockableSteadyClock::SetMockTime(t);
    }
};

#endif // BITCOIN_TEST_UTIL_TIME_H
