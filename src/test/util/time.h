// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_TIME_H
#define BITCOIN_TEST_UTIL_TIME_H

#include <util/check.h>
#include <util/time.h>

#include <optional>

class ElapseSteady
{
    MockableSteadyClock::mock_time_point::duration t{MockableSteadyClock::INITIAL_MOCK_TIME};

public:
    /** Initialize with INITIAL_MOCK_TIME. */
    ElapseSteady() { (*this)(0s); }

    /** Unset mocktime */
    ~ElapseSteady() { MockableSteadyClock::SetMockTime(0s); }

    ElapseSteady(const ElapseSteady&) = delete;
    ElapseSteady& operator=(const ElapseSteady&) = delete;

    /** Change mocktime by the given duration delta */
    void operator()(std::chrono::milliseconds d)
    {
        Assert(d >= 0s); // Steady time can only move forward.
        t += d;
        MockableSteadyClock::SetMockTime(t);
    }
};

class ElapseTime
{
    NodeSeconds m_t{std::chrono::seconds::max()};

public:
    /** Initialize with initial time */
    ElapseTime(NodeSeconds init_time) { set(init_time); }
    ElapseTime(std::chrono::seconds init_time) { set(init_time); }
    /** Initialize with current time, using the next tick to avoid going back by rounding to seconds */
    ElapseTime() { set(++Now<NodeSeconds>().time_since_epoch()); }

    /** Unset mocktime */
    ~ElapseTime() { SetMockTime(0s); }

    ElapseTime(const ElapseTime&) = delete;
    ElapseTime& operator=(const ElapseTime&) = delete;

    /** Set mocktime */
    void set(NodeSeconds t) { SetMockTime(m_t = t); }
    void set(std::chrono::seconds t) { set(NodeSeconds{t}); }

    /** Change mocktime by the given duration delta */
    void operator()(std::chrono::seconds d) { SetMockTime(m_t += d); }
};

#endif // BITCOIN_TEST_UTIL_TIME_H
