// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_TIME_H
#define BITCOIN_TEST_UTIL_TIME_H

#include <util/check.h>
#include <util/time.h>

#include <optional>

/// Helper to initialize the global MockableSteadyClock, let a duration elapse,
/// and reset it after use in a test.
class ElapseSteady
{
    MockableSteadyClock::mock_time_point::duration t{MockableSteadyClock::INITIAL_MOCK_TIME};

public:
    /** Initialize with INITIAL_MOCK_TIME. */
    explicit ElapseSteady() { (*this)(0s); }

    /** Unset mocktime */
    ~ElapseSteady() { MockableSteadyClock::ClearMockTime(); }

    ElapseSteady(const ElapseSteady&) = delete;
    ElapseSteady& operator=(const ElapseSteady&) = delete;

    /** Change mocktime by the given duration delta */
    void operator()(std::chrono::milliseconds d)
    {
        Assert(d >= 0s); // Steady time can only increase monotonically.
        t += d;
        MockableSteadyClock::SetMockTime(t);
    }
};

/// Helper to initialize the global NodeClock, let a duration elapse,
/// and reset it after use in a test.
class ElapseTime
{
    NodeSeconds m_t{std::chrono::seconds::max()};

public:
    /** Initialize with the given time. */
    explicit ElapseTime(NodeSeconds init_time) { set(init_time); }
    explicit ElapseTime(std::chrono::seconds init_time) { set(init_time); }
    /** Initialize with current time, using the next tick to avoid going back by rounding to seconds */
    explicit ElapseTime() { set(++Now<NodeSeconds>().time_since_epoch()); }

    /** Unset mocktime */
    ~ElapseTime() { set(0s); }

    ElapseTime(const ElapseTime&) = delete;
    ElapseTime& operator=(const ElapseTime&) = delete;

    /** Set mocktime */
    void set(NodeSeconds t) { SetMockTime(m_t = t); }
    void set(std::chrono::seconds t) { set(NodeSeconds{t}); }

    /** Change mocktime by the given duration delta */
    void operator()(std::chrono::seconds d) { set(m_t += d); }
};

#endif // BITCOIN_TEST_UTIL_TIME_H
