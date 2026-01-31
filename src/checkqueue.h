// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHECKQUEUE_H
#define BITCOIN_CHECKQUEUE_H

#include <batchverify.h>
#include <logging.h>
#include <sync.h>
#include <tinyformat.h>
#include <script/script_error.h>
#include <util/threadnames.h>

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using ScriptFailureResult = std::pair<ScriptError, std::string>;
using BatchableResult = std::variant<std::vector<SchnorrSignatureToVerify>, ScriptFailureResult>;

/**
 * Queue for verifications that have to be performed.
  * The verifications are represented by a type T, which must provide an
  * operator(), returning an std::optional<R>.
  *
  * The overall result of the computation is std::nullopt if all invocations
  * return std::nullopt, or one of the other results otherwise.
  *
  * One thread (the master) is assumed to push batches of verifications
  * onto the queue, where they are processed by N-1 worker threads. When
  * the master is done adding work, it temporarily joins the worker pool
  * as an N'th worker, until all jobs are done.
  *
  */
template <typename T, typename R = std::remove_cvref_t<decltype(std::declval<T>()().value())>>
class CCheckQueue
{
private:
    template <typename U>
    struct is_batchable_result : std::is_same<U, BatchableResult> {};

    template <typename U>
    inline static constexpr bool is_batchable_result_v = is_batchable_result<U>::value;

    //! Mutex to protect the inner state
    Mutex m_mutex;

    //! Worker threads block on this when out of work
    std::condition_variable m_worker_cv;

    //! Master thread blocks on this when out of work
    std::condition_variable m_master_cv;

    //! The queue of elements to be processed.
    //! As the order of booleans doesn't matter, it is used as a LIFO (stack)
    std::vector<T> queue GUARDED_BY(m_mutex);

    //! The number of workers (including the master) that are idle.
    int nIdle GUARDED_BY(m_mutex){0};

    //! The total number of workers (including the master).
    int nTotal GUARDED_BY(m_mutex){0};

    //! The temporary evaluation result.
    std::optional<R> m_result GUARDED_BY(m_mutex);

    //! All checks for the current block have been added.
    bool m_complete GUARDED_BY(m_mutex){false};

    //! Total number of threads that have verified their batch (including master).
    int m_total_verified GUARDED_BY(m_mutex){0};

    /**
     * Number of verifications that haven't completed yet.
     * This includes elements that are no longer queued, but still in the
     * worker's own batches.
     */
    unsigned int nTodo GUARDED_BY(m_mutex){0};

    //! The maximum number of elements to be processed in one batch
    const unsigned int nBatchSize;

    std::vector<std::thread> m_worker_threads;
    bool m_request_stop GUARDED_BY(m_mutex){false};

    /** Internal function that does bulk of the verification work. If fMaster, return the final result. */
    std::optional<R> Loop(bool fMaster) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        std::condition_variable& cond = fMaster ? m_master_cv : m_worker_cv;
        std::vector<T> vChecks;
        vChecks.reserve(nBatchSize);
        unsigned int nNow = 0;
        std::optional<R> local_result;
        bool do_work;
        bool do_verify_batch{false};
        BatchSchnorrVerifier batch{};
        do {
            {
                WAIT_LOCK(m_mutex, lock);
                // first do the clean-up of the previous loop run (allowing us to do it in the same critsect)
                if (do_verify_batch) {
                    if (local_result.has_value() && !m_result.has_value()) {
                        std::swap(local_result, m_result);
                    }
                } else {
                    if (nNow) {
                        if (local_result.has_value() && !m_result.has_value()) {
                            std::swap(local_result, m_result);
                        }
                        nTodo -= nNow;
                        if constexpr (std::is_same_v<R, ScriptFailureResult>) {
                            if (nTodo == 0 && m_complete) {
                                // All checks are done and no more will be added
                                // Notify all threads to verify their batches
                                m_worker_cv.notify_all();
                                m_master_cv.notify_one();
                            }
                        } else {
                            if (nTodo == 0 && !fMaster) {
                                // We processed the last element; inform the master it can exit and return the result
                                m_master_cv.notify_one();
                            }
                        }
                    } else {
                        // first iteration
                        nTotal++;
                    }
                }
                // logically, the do loop starts here
                while (queue.empty() && !m_request_stop) {
                    if constexpr (std::is_same_v<R, ScriptFailureResult>) {
                        if (m_complete && nTodo == 0) {
                            if (m_total_verified != nTotal) {
                                if (!do_verify_batch) {
                                    do_verify_batch = true;
                                    break;
                                } else {
                                    m_total_verified++;
                                    do_verify_batch = false;

                                    if (m_total_verified == nTotal && !fMaster) {
                                        m_master_cv.notify_one();
                                    }
                                }
                            }
                            if (fMaster && m_total_verified == nTotal) {
                                m_complete = false;
                                m_total_verified = 0;
                                nTotal--;
                                std::optional<R> to_return = std::move(m_result);
                                m_result = std::nullopt;
                                return to_return;
                            }
                        }
                    } else {
                        if (fMaster && nTodo == 0) {
                            nTotal--;
                            std::optional<R> to_return = std::move(m_result);
                            // reset the status for new work later
                            m_result = std::nullopt;
                            // return the current status
                            return to_return;
                        }
                    }
                    nIdle++;
                    cond.wait(lock); // wait
                    nIdle--;
                }
                if (m_request_stop) {
                    // return value does not matter, because m_request_stop is only set in the destructor.
                    return std::nullopt;
                }

                if (!do_verify_batch) {
                    // Decide how many work units to process now.
                    // * Do not try to do everything at once, but aim for increasingly smaller batches so
                    //   all workers finish approximately simultaneously.
                    // * Try to account for idle jobs which will instantly start helping.
                    // * Don't do batches smaller than 1 (duh), or larger than nBatchSize.
                    nNow = std::max(1U, std::min(nBatchSize, (unsigned int)queue.size() / (nTotal + nIdle + 1)));
                    auto start_it = queue.end() - nNow;
                    vChecks.assign(std::make_move_iterator(start_it), std::make_move_iterator(queue.end()));
                    queue.erase(start_it, queue.end());
                    // Check whether we need to do work at all
                    do_work = !m_result.has_value();
                } else {
                    nNow = 0;
                    do_work = false;
                }
            }
            // Verify Schnorr signatures in batch
            if constexpr (std::is_same_v<R, ScriptFailureResult>) {
                if (do_verify_batch) {
                    if (!batch.Verify()) {
                        local_result = ScriptFailureResult(SCRIPT_ERR_BATCH_VALIDATION_FAILED, "Schnorr batch validation failed");
                    }
                    batch.Reset();
                }
            }
            // execute work
            if (do_work) {
                for (T& check : vChecks) {
                    auto check_result = check();

                    if constexpr (is_batchable_result_v<decltype(check_result)>) {
                        if (std::holds_alternative<ScriptFailureResult>(check_result)) {
                            local_result = std::get<ScriptFailureResult>(check_result);
                            break;
                        }
                        // Check succeeded; add the signatures to thread local batch
                        const auto& signatures = std::get<std::vector<SchnorrSignatureToVerify>>(check_result);
                        for (const auto& sig : signatures) {
                            if (!batch.Add(sig.sig, sig.pubkey, sig.sighash)) {
                                local_result = ScriptFailureResult(SCRIPT_ERR_BATCH_VALIDATION_FAILED, "Schnorr batch validation failed");
                            }
                        }
                    } else {
                        local_result = check_result;
                        if (local_result.has_value()) break;
                    }
                }
            }
            vChecks.clear();
        } while (true);
    }

public:
    //! Mutex to ensure only one concurrent CCheckQueueControl
    Mutex m_control_mutex;

    //! Create a new check queue
    explicit CCheckQueue(unsigned int batch_size, int worker_threads_num)
        : nBatchSize(batch_size)
    {
        LogInfo("Script verification uses %d additional threads", worker_threads_num);
        m_worker_threads.reserve(worker_threads_num);
        for (int n = 0; n < worker_threads_num; ++n) {
            m_worker_threads.emplace_back([this, n]() {
                util::ThreadRename(strprintf("scriptch.%i", n));
                Loop(false /* worker thread */);
            });
        }
    }

    // Since this class manages its own resources, which is a thread
    // pool `m_worker_threads`, copy and move operations are not appropriate.
    CCheckQueue(const CCheckQueue&) = delete;
    CCheckQueue& operator=(const CCheckQueue&) = delete;
    CCheckQueue(CCheckQueue&&) = delete;
    CCheckQueue& operator=(CCheckQueue&&) = delete;

    //! Join the execution until completion. If at least one evaluation wasn't successful, return
    //! its error.
    std::optional<R> Complete() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        if constexpr (std::is_same_v<R, ScriptFailureResult>) {
            WITH_LOCK(m_mutex, m_complete = true);
            m_worker_cv.notify_all();
        }
        return Loop(true /* master thread */);
    }

    //! Add a batch of checks to the queue
    void Add(std::vector<T>&& vChecks) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        if (vChecks.empty()) {
            return;
        }

        {
            LOCK(m_mutex);
            queue.insert(queue.end(), std::make_move_iterator(vChecks.begin()), std::make_move_iterator(vChecks.end()));
            nTodo += vChecks.size();
        }

        if (vChecks.size() == 1) {
            m_worker_cv.notify_one();
        } else {
            m_worker_cv.notify_all();
        }
    }

    ~CCheckQueue()
    {
        WITH_LOCK(m_mutex, m_request_stop = true);
        m_worker_cv.notify_all();
        for (std::thread& t : m_worker_threads) {
            t.join();
        }
    }

    bool HasThreads() const { return !m_worker_threads.empty(); }
};

/**
 * RAII-style controller object for a CCheckQueue that guarantees the passed
 * queue is finished before continuing.
 */
template <typename T, typename R = std::remove_cvref_t<decltype(std::declval<T>()().value())>>
class SCOPED_LOCKABLE CCheckQueueControl
{
private:
    CCheckQueue<T, R>& m_queue;
    UniqueLock<Mutex> m_lock;
    bool fDone;

public:
    CCheckQueueControl() = delete;
    CCheckQueueControl(const CCheckQueueControl&) = delete;
    CCheckQueueControl& operator=(const CCheckQueueControl&) = delete;
    explicit CCheckQueueControl(CCheckQueue<T, R>& queueIn) EXCLUSIVE_LOCK_FUNCTION(queueIn.m_control_mutex) : m_queue(queueIn), m_lock(LOCK_ARGS(queueIn.m_control_mutex)), fDone(false) {}

    std::optional<R> Complete()
    {
        auto ret = m_queue.Complete();
        fDone = true;
        return ret;
    }

    void Add(std::vector<T>&& vChecks)
    {
        m_queue.Add(std::move(vChecks));
    }

    ~CCheckQueueControl() UNLOCK_FUNCTION()
    {
        if (!fDone)
            Complete();
    }
};

#endif // BITCOIN_CHECKQUEUE_H
