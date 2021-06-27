// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHECKQUEUE_H
#define BITCOIN_CHECKQUEUE_H

#include <sync.h>
#include <tinyformat.h>
#include <util/threadnames.h>

#include <algorithm>
#include <vector>

template <typename T>
class CCheckQueueControl;

/**
 * Queue for verifications that have to be performed.
  * The verifications are represented by a type T, which must provide an
  * operator(), returning a bool.
  *
  * One thread (the master) is assumed to push batches of verifications
  * onto the queue, where they are processed by N-1 worker threads. When
  * the master is done adding work, it temporarily joins the worker pool
  * as an N'th worker, until all jobs are done.
  */
template <typename T>
class CCheckQueue
{
private:
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
    bool fAllOk GUARDED_BY(m_mutex){true};

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

    /** Internal function that does bulk of the verification work. */
    bool Loop(bool fMaster)
    {
        std::condition_variable& cond = fMaster ? m_master_cv : m_worker_cv;
        std::vector<T> vChecks;
        vChecks.reserve(nBatchSize);
        unsigned int nNow = 0;
        bool fOk = true;
        do {
            {
                WAIT_LOCK(m_mutex, lock);
                // first do the clean-up of the previous loop run (allowing us to do it in the same critsect)
                if (nNow) {
                    fAllOk &= fOk;
                    nTodo -= nNow;
                    if (nTodo == 0 && !fMaster)
                        // We processed the last element; inform the master it can exit and return the result
                        m_master_cv.notify_one();
                } else {
                    // first iteration
                    nTotal++;
                }
                // logically, the do loop starts here
                while (queue.empty() && !m_request_stop) {
                    if (fMaster && nTodo == 0) {
                        nTotal--;
                        bool fRet = fAllOk;
                        // reset the status for new work later
                        if (fMaster)
                            fAllOk = true;
                        // return the current status
                        return fRet;
                    }
                    nIdle++;
                    cond.wait(lock); // wait
                    nIdle--;
                }
                if (m_request_stop) {
                    return false;
                }

                // Decide how many work units to process now.
                // * Do not try to do everything at once, but aim for increasingly smaller batches so
                //   all workers finish approximately simultaneously.
                // * Try to account for idle jobs which will instantly start helping.
                // * Don't do batches smaller than 1 (duh), or larger than nBatchSize.
                nNow = std::max(1U, std::min(nBatchSize, (unsigned int)queue.size() / (nTotal + nIdle + 1)));
                vChecks.resize(nNow);
                for (unsigned int i = 0; i < nNow; i++) {
                    // We want the lock on the m_mutex to be as short as possible, so swap jobs from the global
                    // queue to the local batch vector instead of copying.
                    vChecks[i].swap(queue.back());
                    queue.pop_back();
                }
                // Check whether we need to do work at all
                fOk = fAllOk;
            }
            // execute work
            for (T& check : vChecks)
                if (fOk)
                    fOk = check();
            vChecks.clear();
        } while (true);
    }

public:
    //! Mutex to ensure only one concurrent CCheckQueueControl
    Mutex m_control_mutex;

    //! Create a new check queue
    explicit CCheckQueue(unsigned int nBatchSizeIn)
        : nBatchSize(nBatchSizeIn)
    {
    }

    //! Create a pool of new worker threads.
    void StartWorkerThreads(const int threads_num)
    {
        {
            LOCK(m_mutex);
            nIdle = 0;
            nTotal = 0;
            fAllOk = true;
        }
        assert(m_worker_threads.empty());
        for (int n = 0; n < threads_num; ++n) {
            m_worker_threads.emplace_back([this, n]() {
                util::ThreadRename(strprintf("scriptch.%i", n));
                Loop(false /* worker thread */);
            });
        }
    }

    //! Wait until execution finishes, and return whether all evaluations were successful.
    bool Wait()
    {
        return Loop(true /* master thread */);
    }

    //! Add a batch of checks to the queue
    void Add(std::vector<T>& vChecks)
    {
        LOCK(m_mutex);
        for (T& check : vChecks) {
            queue.push_back(T());
            check.swap(queue.back());
        }
        nTodo += vChecks.size();
        if (vChecks.size() == 1)
            m_worker_cv.notify_one();
        else if (vChecks.size() > 1)
            m_worker_cv.notify_all();
    }

    //! Stop all of the worker threads.
    void StopWorkerThreads()
    {
        WITH_LOCK(m_mutex, m_request_stop = true);
        m_worker_cv.notify_all();
        for (std::thread& t : m_worker_threads) {
            t.join();
        }
        m_worker_threads.clear();
        WITH_LOCK(m_mutex, m_request_stop = false);
    }

    ~CCheckQueue()
    {
        assert(m_worker_threads.empty());
    }

};

/**
 * RAII-style controller object for a CCheckQueue that guarantees the passed
 * queue is finished before continuing.
 */
template <typename T>
class CCheckQueueControl
{
private:
    CCheckQueue<T> * const pqueue;
    bool fDone;

public:
    CCheckQueueControl() = delete;
    CCheckQueueControl(const CCheckQueueControl&) = delete;
    CCheckQueueControl& operator=(const CCheckQueueControl&) = delete;
    explicit CCheckQueueControl(CCheckQueue<T> * const pqueueIn) : pqueue(pqueueIn), fDone(false)
    {
        // passed queue is supposed to be unused, or nullptr
        if (pqueue != nullptr) {
            ENTER_CRITICAL_SECTION(pqueue->m_control_mutex);
        }
    }

    bool Wait()
    {
        if (pqueue == nullptr)
            return true;
        bool fRet = pqueue->Wait();
        fDone = true;
        return fRet;
    }

    void Add(std::vector<T>& vChecks)
    {
        if (pqueue != nullptr)
            pqueue->Add(vChecks);
    }

    ~CCheckQueueControl()
    {
        if (!fDone)
            Wait();
        if (pqueue != nullptr) {
            LEAVE_CRITICAL_SECTION(pqueue->m_control_mutex);
        }
    }
};

#endif // BITCOIN_CHECKQUEUE_H
