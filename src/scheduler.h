// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCHEDULER_H
#define BITCOIN_SCHEDULER_H

#include <condition_variable>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>

//
// Simple class for background tasks that should be run
// periodically or once "after a while"
//
// Usage:
//
//   CScheduler s;
//   s.scheduleFromNow(doSomething, 11); // Assuming a: void doSomething() { }
//   s.scheduleFromNow(std::bind(Class::func, this, argument), 3);
//   std::thread* t = new std::thread(std::bind(CScheduler::serviceQueue, &s));
//
// then at program shutdown, stop the scheduler (optionally draining the queue):
//
//   s.stop(/*drain?*/);
//
// and join the queue servicing thread:
//
//   t->join();
//   delete t;
//

class CScheduler
{
public:
    CScheduler();
    ~CScheduler();

    typedef std::function<void(void)> Function;

    // Call func at/after time t
    void schedule(Function f, std::chrono::system_clock::time_point t);

    // Convenience method: call f once deltaMilliSeconds from now
    void scheduleFromNow(Function f, int64_t deltaMilliSeconds);

    // Another convenience method: call f approximately
    // every deltaMilliSeconds forever, starting deltaMilliSeconds from now.
    // To be more precise: every time f is finished, it
    // is rescheduled to run deltaMilliSeconds later. If you
    // need more accurate scheduling, don't use this method.
    void scheduleEvery(Function f, int64_t deltaMilliSeconds);

    // To keep things as simple as possible, there is no unschedule.

    // Service items in the queue until stop() is called.
    void serviceQueue();

    // Tell any threads running serviceQueue to stop as soon as they're
    // done servicing whatever task they're currently servicing (drain=false)
    // or when there is no work left to be done (drain=true)
    void stop(bool drain=false);

    // Returns number of tasks waiting to be serviced,
    // and first and last task times
    size_t getQueueInfo(std::chrono::system_clock::time_point &first,
                        std::chrono::system_clock::time_point &last) const;

private:
    std::multimap<std::chrono::system_clock::time_point, Function> taskQueue;
    std::condition_variable newTaskScheduled;
    mutable std::mutex newTaskMutex;
    int nThreadsServicingQueue;
    bool stopRequested;
    bool stopWhenEmpty;
    bool shouldStop() const { return stopRequested || (stopWhenEmpty && taskQueue.empty()); }
};

#endif
