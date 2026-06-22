// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_TIME_H
#define BITCOIN_TEST_UTIL_TIME_H

#include <util/check.h>
#include <util/time.h>

/// CRTP Helper to limit a class to at most one at a time.
template <class T>
class LimitOne
{
public:
    LimitOne() { Assert(g_T_available) = false; }
    ~LimitOne() { g_T_available = true; }
    LimitOne(const LimitOne&) = delete;
    LimitOne& operator=(const LimitOne&) = delete;

private:
    static inline bool g_T_available{true};
};


/// Helper to initialize the global MockableSteadyClock, let a duration elapse,
/// and reset it after use in a test.
class FakeSteadyClock : public LimitOne<FakeSteadyClock>
{
    MockableSteadyClock::mock_time_point::duration t{MockableSteadyClock::INITIAL_MOCK_TIME};

public:
    /** Initialize with INITIAL_MOCK_TIME. */
    explicit FakeSteadyClock() { (*this) += 0s; }

    /** Unset mocktime */
    ~FakeSteadyClock() { MockableSteadyClock::ClearMockTime(); }

    FakeSteadyClock(const FakeSteadyClock&) = delete;
    FakeSteadyClock& operator=(const FakeSteadyClock&) = delete;

    /** Change mocktime by the given duration delta */
    void operator+=(std::chrono::milliseconds d)
    {
        Assert(d >= 0s); // Steady time can only increase monotonically.
        t += d;
        MockableSteadyClock::SetMockTime(t);
    }
};

/// Helper to initialize the global NodeClock, let a duration elapse,
/// and reset it after use in a test.
class FakeNodeClock : public LimitOne<FakeNodeClock>
{
    NodeSeconds m_t{std::chrono::seconds::max()};

public:
    /// Initialize with the given time.
    explicit FakeNodeClock(NodeSeconds init_time) { set(init_time); }
    explicit FakeNodeClock(std::chrono::seconds init_time) { set(init_time); }
    /// Initialize with current time.
    explicit FakeNodeClock() { set(Now<NodeSeconds>()); }

    /// Unset mocktime.
    ~FakeNodeClock() { set(0s); }

    FakeNodeClock(const FakeNodeClock&) = delete;
    FakeNodeClock& operator=(const FakeNodeClock&) = delete;

    /// Set mocktime.
    void set(NodeSeconds t) { SetMockTime(m_t = t); }
    void set(std::chrono::seconds t) { set(NodeSeconds{t}); }

    /// Change mocktime by the given duration delta.
    void operator+=(std::chrono::seconds d) { set(m_t += d); }
    void operator-=(std::chrono::seconds d) { set(m_t -= d); }
};

#endif // BITCOIN_TEST_UTIL_TIME_H
