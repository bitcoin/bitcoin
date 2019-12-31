// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THREADINTERRUPT_H
#define BITCOIN_THREADINTERRUPT_H

#include <sync.h>

#include <atomic>
#include <chrono>
#include <condition_variable>

/*
    A helper class for interruptible sleeps. Calling operator() will interrupt
    any current sleep, and after that point operator bool() will return true
    until reset.
*/
class CThreadInterrupt
{
public:
    using Clock = std::chrono::steady_clock;
    CThreadInterrupt();
    explicit operator bool() const;
    void operator()();
    void reset();
    bool sleep_for(Clock::duration rel_time) EXCLUSIVE_LOCKS_REQUIRED(!mut);

private:
    std::condition_variable cond;
    Mutex mut;
    std::atomic<bool> flag;
};

#endif //BITCOIN_THREADINTERRUPT_H
