// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "scheduler.h"

#include <assert.h>
#include <boost/bind.hpp>
#include <utility>

CScheduler::CScheduler() : nThreadsServicingQueue(0)
{
}

CScheduler::~CScheduler()
{
    assert(nThreadsServicingQueue == 0);
}


#if BOOST_VERSION < 105000
static boost::system_time toPosixTime(const boost::chrono::system_clock::time_point& t)
{
    return boost::posix_time::from_time_t(boost::chrono::system_clock::to_time_t(t));
}
#endif

void CScheduler::serviceQueue()
{
    boost::unique_lock<boost::mutex> lock(newTaskMutex);
    ++nThreadsServicingQueue;

    // newTaskMutex is locked throughout this loop EXCEPT
    // when the thread is waiting or when the user's function
    // is called.
    while (1) {
        try {
            while (taskQueue.empty()) {
                // Wait until there is something to do.
                newTaskScheduled.wait(lock);
            }
// Wait until either there is a new task, or until
// the time of the first item on the queue:

// wait_until needs boost 1.50 or later; older versions have timed_wait:
#if BOOST_VERSION < 105000
            while (!taskQueue.empty() && newTaskScheduled.timed_wait(lock, toPosixTime(taskQueue.begin()->first))) {
                // Keep waiting until timeout
            }
#else
            while (!taskQueue.empty() && newTaskScheduled.wait_until(lock, taskQueue.begin()->first) != boost::cv_status::timeout) {
                // Keep waiting until timeout
            }
#endif
            // If there are multiple threads, the queue can empty while we're waiting (another
            // thread may service the task we were waiting on).
            if (taskQueue.empty())
                continue;

            Function f = taskQueue.begin()->second;
            taskQueue.erase(taskQueue.begin());

            // Unlock before calling f, so it can reschedule itself or another task
            // without deadlocking:
            lock.unlock();
            f();
            lock.lock();
        } catch (...) {
            --nThreadsServicingQueue;
            throw;
        }
    }
}

void CScheduler::schedule(CScheduler::Function f, boost::chrono::system_clock::time_point t)
{
    {
        boost::unique_lock<boost::mutex> lock(newTaskMutex);
        taskQueue.insert(std::make_pair(t, f));
    }
    newTaskScheduled.notify_one();
}

void CScheduler::scheduleFromNow(CScheduler::Function f, int64_t deltaSeconds)
{
    schedule(f, boost::chrono::system_clock::now() + boost::chrono::seconds(deltaSeconds));
}

static void Repeat(CScheduler* s, CScheduler::Function f, int64_t deltaSeconds)
{
    f();
    s->scheduleFromNow(boost::bind(&Repeat, s, f, deltaSeconds), deltaSeconds);
}

void CScheduler::scheduleEvery(CScheduler::Function f, int64_t deltaSeconds)
{
    scheduleFromNow(boost::bind(&Repeat, this, f, deltaSeconds), deltaSeconds);
}
