// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_THREADINTERRUPT_H
#define BITCOIN_UTIL_THREADINTERRUPT_H

#include <sync.h>
#include <threadsafety.h>

#include <atomic>
#include <chrono>
#include <condition_variable>

/**
 * A helper class for interruptible sleeps. Calling operator() will interrupt
 * any current sleep, and after that point operator bool() will return true
 * until reset.
 *
 * This class should not be used in a signal handler. It uses thread
 * synchronization primitives that are not safe to use with signals. If sending
 * an interrupt from a signal handler is necessary, the \ref SignalInterrupt
 * class can be used instead.
 */

class CThreadInterrupt
{
public:
    using Clock = std::chrono::steady_clock;
    CThreadInterrupt();
    explicit operator bool() const;
    void operator()() EXCLUSIVE_LOCKS_REQUIRED(!mut);
    void reset();
    bool sleep_for(Clock::duration rel_time) EXCLUSIVE_LOCKS_REQUIRED(!mut);

private:
    std::condition_variable cond;
    Mutex mut;
    std::atomic<bool> flag;
};

#endif // BITCOIN_UTIL_THREADINTERRUPT_H
