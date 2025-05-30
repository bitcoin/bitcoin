// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <scheduler.h>

#include <sync.h>
#include <util/time.h>

#include <cassert>
#include <functional>
#include <utility>

CScheduler::CScheduler() = default;

CScheduler::~CScheduler()
{
    assert(nThreadsServicingQueue == 0);
    if (stopWhenEmpty) assert(taskQueue.empty());
}


void CScheduler::serviceQueue()
{
    WAIT_LOCK(newTaskMutex, lock);
    ++nThreadsServicingQueue;

    // newTaskMutex is locked throughout this loop EXCEPT
    // when the thread is waiting or when the user's function
    // is called.
    while (!shouldStop()) {
        try {
            while (!shouldStop() && taskQueue.empty()) {
                // Wait until there is something to do.
                newTaskScheduled.wait(lock);
            }

            // Wait until either there is a new task, or until
            // the time of the first item on the queue:

            while (!shouldStop() && !taskQueue.empty()) {
                std::chrono::steady_clock::time_point timeToWaitFor = taskQueue.begin()->first;
                if (newTaskScheduled.wait_until(lock, timeToWaitFor) == std::cv_status::timeout) {
                    break; // Exit loop after timeout, it means we reached the time of the event
                }
            }

            // If there are multiple threads, the queue can empty while we're waiting (another
            // thread may service the task we were waiting on).
            if (shouldStop() || taskQueue.empty())
                continue;

            Function f = taskQueue.begin()->second;
            taskQueue.erase(taskQueue.begin());

            {
                // Unlock before calling f, so it can reschedule itself or another task
                // without deadlocking:
                REVERSE_LOCK(lock);
                f();
            }
        } catch (...) {
            --nThreadsServicingQueue;
            throw;
        }
    }
    --nThreadsServicingQueue;
    newTaskScheduled.notify_one();
}

void CScheduler::schedule(CScheduler::Function f, std::chrono::steady_clock::time_point t)
{
    {
        LOCK(newTaskMutex);
        taskQueue.insert(std::make_pair(t, f));
    }
    newTaskScheduled.notify_one();
}

void CScheduler::MockForward(std::chrono::seconds delta_seconds)
{
    assert(delta_seconds > 0s && delta_seconds <= 1h);

    {
        LOCK(newTaskMutex);

        // use temp_queue to maintain updated schedule
        std::multimap<std::chrono::steady_clock::time_point, Function> temp_queue;

        for (const auto& element : taskQueue) {
            temp_queue.emplace_hint(temp_queue.cend(), element.first - delta_seconds, element.second);
        }

        // point taskQueue to temp_queue
        taskQueue = std::move(temp_queue);
    }

    // notify that the taskQueue needs to be processed
    newTaskScheduled.notify_one();
}

static void Repeat(CScheduler& s, CScheduler::Function f, std::chrono::milliseconds delta)
{
    f();
    s.scheduleFromNow([=, &s] { Repeat(s, f, delta); }, delta);
}

void CScheduler::scheduleEvery(CScheduler::Function f, std::chrono::milliseconds delta)
{
    scheduleFromNow([this, f, delta] { Repeat(*this, f, delta); }, delta);
}

size_t CScheduler::getQueueInfo(std::chrono::steady_clock::time_point& first,
                                std::chrono::steady_clock::time_point& last) const
{
    LOCK(newTaskMutex);
    size_t result = taskQueue.size();
    if (!taskQueue.empty()) {
        first = taskQueue.begin()->first;
        last = taskQueue.rbegin()->first;
    }
    return result;
}

bool CScheduler::AreThreadsServicingQueue() const
{
    LOCK(newTaskMutex);
    return nThreadsServicingQueue;
}


void SerialTaskRunner::MaybeScheduleProcessQueue()
{
    {
        LOCK(m_callbacks_mutex);
        // Try to avoid scheduling too many copies here, but if we
        // accidentally have two ProcessQueue's scheduled at once its
        // not a big deal.
        if (m_are_callbacks_running) return;
        if (m_callbacks_pending.empty()) return;
    }
    m_scheduler.schedule([this] { this->ProcessQueue(); }, std::chrono::steady_clock::now());
}

void SerialTaskRunner::ProcessQueue()
{
    std::function<void()> callback;
    {
        LOCK(m_callbacks_mutex);
        if (m_are_callbacks_running) return;
        if (m_callbacks_pending.empty()) return;
        m_are_callbacks_running = true;

        callback = std::move(m_callbacks_pending.front());
        m_callbacks_pending.pop_front();
    }

    // RAII the setting of fCallbacksRunning and calling MaybeScheduleProcessQueue
    // to ensure both happen safely even if callback() throws.
    struct RAIICallbacksRunning {
        SerialTaskRunner* instance;
        explicit RAIICallbacksRunning(SerialTaskRunner* _instance) : instance(_instance) {}
        ~RAIICallbacksRunning()
        {
            {
                LOCK(instance->m_callbacks_mutex);
                instance->m_are_callbacks_running = false;
            }
            instance->MaybeScheduleProcessQueue();
        }
    } raiicallbacksrunning(this);

    callback();
}

void SerialTaskRunner::insert(std::function<void()> func)
{
    {
        LOCK(m_callbacks_mutex);
        m_callbacks_pending.emplace_back(std::move(func));
    }
    MaybeScheduleProcessQueue();
}

void SerialTaskRunner::flush()
{
    assert(!m_scheduler.AreThreadsServicingQueue());
    bool should_continue = true;
    while (should_continue) {
        ProcessQueue();
        LOCK(m_callbacks_mutex);
        should_continue = !m_callbacks_pending.empty();
    }
}

size_t SerialTaskRunner::size()
{
    LOCK(m_callbacks_mutex);
    return m_callbacks_pending.size();
}
