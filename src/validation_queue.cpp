// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation_queue.h>

#include <kernel/cs_main.h>
#include <sync.h>
#include <util/check.h>
#include <util/expected.h>
#include <util/thread.h>

#include <exception>
#include <stdexcept>
#include <utility>

BlockProcessingQueue::~BlockProcessingQueue()
{
    Stop();
}

void BlockProcessingQueue::Start()
{
    LOCK(m_mutex);
    if (m_state != State::Inactive) throw std::logic_error("Block processing queue already started or stopped");
    m_worker = std::thread{util::TraceThread, "blkprocess", [this] { Work(); }};
    m_worker_id = m_worker.get_id();
    m_state = State::Running;
}

void BlockProcessingQueue::Interrupt()
{
    {
        LOCK(m_mutex);
        if (m_state == State::Inactive) m_state = State::Stopped;
        if (m_state == State::Running) m_state = State::Interrupted;
    }
    m_cv.notify_all();
}

void BlockProcessingQueue::Stop()
{
    std::thread worker;
    {
        WAIT_LOCK(m_mutex, lock);
        if (m_state == State::Stopped) return;
        if (m_state == State::Inactive) {
            m_state = State::Stopped;
            return;
        }
        AssertLockNotHeld(cs_main);
        assert(std::this_thread::get_id() != m_worker_id);
        if (m_state == State::Stopping) {
            m_cv.wait(lock, [this]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_state == State::Stopped; });
            return;
        }
        m_state = State::Stopping;
        worker = std::move(m_worker);
    }
    m_cv.notify_all();
    worker.join();
    {
        LOCK(m_mutex);
        Assume(m_jobs.empty());
        m_worker_id = {};
        m_state = State::Stopped;
    }
    m_cv.notify_all();
}

BlockProcessingQueue::Submission BlockProcessingQueue::Submit(const std::shared_ptr<const CBlock>& block, Processor process)
{
    Assert(block);
    Assert(process);
    std::future<BlockProcessingResult> future;
    {
        LOCK(m_mutex);
        if (m_state == State::Inactive) return util::Unexpected{SubmitError::Inactive};
        if (m_state != State::Running) return util::Unexpected{SubmitError::Stopping};

        std::promise<BlockProcessingResult> completion;
        future = completion.get_future();
        m_jobs.push_back({block, std::move(process), std::move(completion)});
    }
    m_cv.notify_one();
    return {std::move(future)};
}

void BlockProcessingQueue::Work()
{
    WAIT_LOCK(m_mutex, lock);
    for (;;) {
        m_cv.wait(lock, [this]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_state != State::Running || !m_jobs.empty(); });
        if (m_jobs.empty()) return;

        Job job{std::move(m_jobs.front())};
        m_jobs.pop_front();
        BlockProcessingResult result;
        std::exception_ptr exception;
        {
            REVERSE_LOCK(lock, m_mutex);
            try {
                result = job.process(job.block);
            } catch (...) {
                exception = std::current_exception();
            }
            // Release retained block/callback data before making the future ready.
            job.block.reset();
            job.process = {};
            if (exception) {
                job.completion.set_exception(exception);
            } else {
                job.completion.set_value(result);
            }
        }
    }
}
