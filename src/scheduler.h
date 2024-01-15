// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCHEDULER_H
#define BITCOIN_SCHEDULER_H

#include <attributes.h>
#include <sync.h>
#include <threadsafety.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <thread>
#include <utility>

/**
 * Simple class for background tasks that should be run
 * periodically or once "after a while"
 *
 * Usage:
 *
 * CScheduler* s = new CScheduler();
 * s->scheduleFromNow(doSomething, std::chrono::milliseconds{11}); // Assuming a: void doSomething() { }
 * s->scheduleFromNow([=] { this->func(argument); }, std::chrono::milliseconds{3});
 * std::thread* t = new std::thread([&] { s->serviceQueue(); });
 *
 * ... then at program shutdown, make sure to call stop() to clean up the thread(s) running serviceQueue:
 * s->stop();
 * t->join();
 * delete t;
 * delete s; // Must be done after thread is interrupted/joined.
 */
class CScheduler
{
public:
    CScheduler();
    ~CScheduler();

    std::thread m_service_thread;

    typedef std::function<void()> Function;

    /** Call func at/after time t */
    void schedule(Function f, std::chrono::system_clock::time_point t);

    /** Call f once after the delta has passed */
    void scheduleFromNow(Function f, std::chrono::milliseconds delta)
    {
        schedule(std::move(f), std::chrono::system_clock::now() + delta);
    }

    /**
     * Repeat f until the scheduler is stopped. First run is after delta has passed once.
     *
     * The timing is not exact: Every time f is finished, it is rescheduled to run again after delta. If you need more
     * accurate scheduling, don't use this method.
     */
    void scheduleEvery(Function f, std::chrono::milliseconds delta);

    /**
     * Mock the scheduler to fast forward in time.
     * Iterates through items on taskQueue and reschedules them
     * to be delta_seconds sooner.
     */
    void MockForward(std::chrono::seconds delta_seconds);

    /**
     * Services the queue 'forever'. Should be run in a thread.
     */
    void serviceQueue();

    /** Tell any threads running serviceQueue to stop as soon as the current task is done */
    void stop()
    {
        WITH_LOCK(newTaskMutex, stopRequested = true);
        newTaskScheduled.notify_all();
        if (m_service_thread.joinable()) m_service_thread.join();
    }
    /** Tell any threads running serviceQueue to stop when there is no work left to be done */
    void StopWhenDrained()
    {
        WITH_LOCK(newTaskMutex, stopWhenEmpty = true);
        newTaskScheduled.notify_all();
        if (m_service_thread.joinable()) m_service_thread.join();
    }

    /**
     * Returns number of tasks waiting to be serviced,
     * and first and last task times
     */
    size_t getQueueInfo(std::chrono::system_clock::time_point& first,
                        std::chrono::system_clock::time_point& last) const;

    /** Returns true if there are threads actively running in serviceQueue() */
    bool AreThreadsServicingQueue() const;

private:
    mutable Mutex newTaskMutex;
    std::condition_variable newTaskScheduled;
    std::multimap<std::chrono::system_clock::time_point, Function> taskQueue GUARDED_BY(newTaskMutex);
    int nThreadsServicingQueue GUARDED_BY(newTaskMutex){0};
    bool stopRequested GUARDED_BY(newTaskMutex){false};
    bool stopWhenEmpty GUARDED_BY(newTaskMutex){false};
    bool shouldStop() const EXCLUSIVE_LOCKS_REQUIRED(newTaskMutex) { return stopRequested || (stopWhenEmpty && taskQueue.empty()); }
};

/**
 * Class used by CScheduler clients which may schedule multiple jobs
 * which are required to be run serially. Jobs may not be run on the
 * same thread, but no two jobs will be executed
 * at the same time and memory will be release-acquire consistent
 * (the scheduler will internally do an acquire before invoking a callback
 * as well as a release at the end). In practice this means that a callback
 * B() will be able to observe all of the effects of callback A() which executed
 * before it.
 */
class SingleThreadedSchedulerClient
{
private:
    CScheduler& m_scheduler;

    Mutex m_callbacks_mutex;
    std::list<std::function<void()>> m_callbacks_pending GUARDED_BY(m_callbacks_mutex);
    bool m_are_callbacks_running GUARDED_BY(m_callbacks_mutex) = false;

    void MaybeScheduleProcessQueue();
    void ProcessQueue();

public:
    explicit SingleThreadedSchedulerClient(CScheduler& scheduler LIFETIMEBOUND) : m_scheduler{scheduler} {}

    /**
     * Add a callback to be executed. Callbacks are executed serially
     * and memory is release-acquire consistent between callback executions.
     * Practically, this means that callbacks can behave as if they are executed
     * in order by a single thread.
     */
    void AddToProcessQueue(std::function<void()> func);

    /**
     * Processes all remaining queue members on the calling thread, blocking until queue is empty
     * Must be called after the CScheduler has no remaining processing threads!
     */
    void EmptyQueue();

    size_t CallbacksPending();
};

#endif
