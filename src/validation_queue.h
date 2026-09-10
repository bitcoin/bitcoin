// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_QUEUE_H
#define BITCOIN_VALIDATION_QUEUE_H

#include <sync.h>
#include <util/expected.h>

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <thread>

class CBlock;

/** ProcessNewBlock outcomes; neither implies full consensus validity. */
struct BlockProcessingResult {
    /** Whether initial processing and chain activation returned success. */
    bool processing_success{false};
    /** Set before storing newly received block data; can be true even if processing fails. */
    bool new_block{false};
};

/**
 * A FIFO of owned blocks and processing callbacks, serviced by one worker.
 * Callers must apply block admission checks before submitting.
 *
 * Start explicitly after processing dependencies have been initialized. Interrupt
 * closes admission and lets accepted jobs finish. Stop additionally joins the
 * worker, and must precede destruction of anything used by the callbacks. The
 * destructor calls Stop as a fallback. An interrupted/stopped queue cannot restart.
 * Callbacks may submit more work, but must not wait on that work or stop the queue.
 */
class BlockProcessingQueue
{
public:
    using Processor = std::function<BlockProcessingResult(const std::shared_ptr<const CBlock>&)>;
    enum class SubmitError { Inactive, Stopping };
    using Submission = util::Expected<std::future<BlockProcessingResult>, SubmitError>;

    BlockProcessingQueue() = default;
    ~BlockProcessingQueue();
    BlockProcessingQueue(const BlockProcessingQueue&) = delete;
    BlockProcessingQueue& operator=(const BlockProcessingQueue&) = delete;

    /** Start once; throws if already started, interrupted, or stopped. */
    void Start() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /** Stop accepting jobs without waiting for accepted work. */
    void Interrupt() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /**
     * Finish accepted jobs and join, without running jobs on the calling thread.
     * May be called repeatedly or concurrently. Joining must not hold cs_main.
     */
    void Stop() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Enqueue without waiting for validation while running. An inactive or
     * stopping queue neither retains the block nor executes the callback.
     * Processor exceptions are delivered through the accepted job's future.
     */
    [[nodiscard]] Submission Submit(const std::shared_ptr<const CBlock>& block, Processor process) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    struct Job {
        std::shared_ptr<const CBlock> block;
        Processor process;
        std::promise<BlockProcessingResult> completion;
    };
    enum class State { Inactive, Running, Interrupted, Stopping, Stopped };

    Mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Job> m_jobs GUARDED_BY(m_mutex);
    State m_state GUARDED_BY(m_mutex){State::Inactive};
    std::thread m_worker GUARDED_BY(m_mutex);
    std::thread::id m_worker_id GUARDED_BY(m_mutex);

    void Work() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

#endif // BITCOIN_VALIDATION_QUEUE_H
