// Copyright (c) 2016-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THREADINTERRUPT_H
#define BITCOIN_THREADINTERRUPT_H

#include <sync.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>

/*
    A helper class for interruptible sleeps. Calling operator() will interrupt
    any current sleep, and after that point operator bool() will return true
    until reset.
*/
class CThreadInterrupt
{
public:
    CThreadInterrupt();
    explicit operator bool() const;
    void operator()();
    void reset();
    bool sleep_for(std::chrono::milliseconds rel_time);
    bool sleep_for(std::chrono::seconds rel_time);
    bool sleep_for(std::chrono::minutes rel_time);

private:
    std::condition_variable cond;
    Mutex mut;
    std::atomic<bool> flag;
};

struct InterruptFlag {
    Mutex m_mutex;
    std::condition_variable m_cond;
    bool m_interrupted GUARDED_BY(m_mutex) = false;
};

extern thread_local InterruptFlag* g_interrupt_flag;

class InterruptibleThread
{
public:
    template<typename Function, typename ... Args>
    InterruptibleThread(Function&& func, Args&& ...args) {
        InterruptFlag* flag_ptr = new InterruptFlag();
        m_interrupt_flag = std::unique_ptr<InterruptFlag>(flag_ptr);
        m_internal = std::thread([flag_ptr](typename std::decay<Function>::type&& func, typename std::decay<Args>::type&&...args){
            g_interrupt_flag = flag_ptr;
            func(std::forward<Args>(args)...);
        }, std::forward<Function>(func), std::forward<Args>(args)...);
    }

    void join();
    void interrupt();

private:
    std::thread m_internal;
    std::unique_ptr<InterruptFlag> m_interrupt_flag;
};

class ThreadInterrupted : public std::exception
{
};

void InterruptionPoint();

template <class Rep, class Period>
void InterruptibleSleep(const std::chrono::duration<Rep, Period>& sleep_duration)
{
    if (!g_interrupt_flag) { // Not interruptible thread
        std::this_thread::sleep_for(sleep_duration);
        return;
    }
    WAIT_LOCK(g_interrupt_flag->m_mutex, lock);
    if (g_interrupt_flag->m_interrupted) {
        throw ThreadInterrupted();
    }
    g_interrupt_flag->m_cond.wait_for(lock, sleep_duration);
    if (g_interrupt_flag->m_interrupted) {
        throw ThreadInterrupted();
    }
}

#endif //BITCOIN_THREADINTERRUPT_H
