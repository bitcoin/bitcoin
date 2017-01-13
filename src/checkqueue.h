// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHECKQUEUE_H
#define BITCOIN_CHECKQUEUE_H

#include <algorithm>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

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
    //! Mutex to ensure that sleeping threads are woken.
    boost::mutex mutex;

    //! Worker threads block on this when out of work
    boost::condition_variable condWorker;

    //! The temporary evaluation result.
    std::atomic<uint8_t> fAllOk;

    /**
     * Number of verification threads that aren't in stand-by. When a thread is
     * awake it may have a job that will return false, but is yet to report the
     * result through fAllOk.
     */
    std::atomic<unsigned int> nAwake;

    /** If there is presently a master process either in the queue or adding jobs */
    std::atomic<bool> fMasterPresent;

    //! Whether we're shutting down.
    bool fQuit;

    //! The maximum number of elements to be processed in one batch
    unsigned int nBatchSize;

    //! A pointer to contiguous memory that contains all checks
    T* check_mem {nullptr};

    /** The begin and end offsets into check_mem. 128 bytes of padding is
     * inserted before and after check_mem_top to eliminate false sharing*/
    std::atomic<uint32_t> check_mem_bot {0};
    unsigned char _padding[128];
    std::atomic<uint32_t> check_mem_top {0};
    unsigned char _padding2[128];

    /** Internal function that does bulk of the verification work. */
    bool Loop(bool fMaster = false)
    {

        uint8_t fOk = 1;
        // first iteration, only count non-master threads
        if (!fMaster)
            ++nAwake;
        for (;;) {
            uint32_t top_cache = check_mem_top;
            uint32_t bottom_cache = check_mem_bot;
            // Try to increment bottom_cache by 1 if our version of bottom_cache
            // indicates that there is work to be done.
            // E.g., if bottom_cache = top_cache, don't attempt to exchange.
            //       if  bottom_cache < top_cache, then do attempt to exchange 
            //
            // compare_exchange_weak, on failure, updates bottom_cache to latest
            while (top_cache > bottom_cache && 
                    !check_mem_bot.compare_exchange_weak( bottom_cache, bottom_cache+1));
            // If our loop terminated because of no_work_left...
            if (top_cache <= bottom_cache)
            {
                if (fMaster) {
                    fMasterPresent = false;
                    // There's no harm to the master holding the lock
                    // at this point because all the jobs are taken.
                    // so busy spin until no one else is awake
                    while (nAwake) {}
                    bool fRet = fAllOk;
                    // reset the status for new work later
                    fAllOk = 1;
                    // return the current status
                    return fRet;
                } else  if (!fMasterPresent) { // Read once outside the lock and once inside
                    --nAwake; 
                    // Unfortunately we need this lock for this to be safe
                    // We hold it for the min time possible
                    {
                        boost::unique_lock<boost::mutex> lock(mutex);
                        condWorker.wait(lock, [&]{ return fMasterPresent.load();});
                    }
                    ++nAwake;
                }
            } else {
                // We compute using bottom_cache (not bottom_cache + 1 as above) because it is 0-indexed
                T * pT = check_mem + bottom_cache;
                // Check whether we need to do work at all (can be read outside
                // of lock because it's fine if a worker executes checks
                // anyways)
                fOk = fAllOk;
                // execute work
                fOk &= fOk && (*pT)();
                // We swap in a default constructed value onto pT before freeing
                // so that we don't accidentally double free when check_mem is
                // freed. We don't strictly need to free here, but it's good
                // practice in case T uses a lot of memory.
                auto t = T();
                pT->swap(t);
                // Can't reveal result until after swapped, otherwise
                // the master thread might exit and we'd double free pT
                fAllOk &= fOk;
            }
        }
    }

public:
    //! Mutex to ensure only one concurrent CCheckQueueControl
    boost::mutex ControlMutex;

    //! Create a new check queue
    CCheckQueue(unsigned int nBatchSizeIn) :  fAllOk(1), nAwake(0), fMasterPresent(false),  fQuit(false), nBatchSize(nBatchSizeIn), check_mem(nullptr), check_mem_bot(0), check_mem_top(0) {}

    //! Worker thread
    void Thread()
    {
        Loop();
    }

    //! Wait until execution finishes, and return whether all evaluations were successful.
    bool Wait()
    {
        return Loop(true);
    }

    //! Setup is called once per batch to point the CheckQueue to the
    // checks & restart the counters.
    void Setup(T* check_mem_in)
    {
        check_mem = check_mem_in;
        check_mem_top = 0;
        check_mem_bot = 0;
        fMasterPresent = true;
        boost::unique_lock<boost::mutex> lock(mutex);
        condWorker.notify_all();
    }

    //! Add a batch of checks to the queue
    void Add(size_t size)
    {
        check_mem_top += size;
    }

    ~CCheckQueue()
    {
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
    std::vector<T> check_mem;
    CCheckQueue<T>* pqueue;
    bool fDone;

public:
    CCheckQueueControl() = delete;
    CCheckQueueControl(const CCheckQueueControl&) = delete;
    CCheckQueueControl& operator=(const CCheckQueueControl&) = delete;
    explicit CCheckQueueControl(CCheckQueue<T> * const pqueueIn, const unsigned int size) : check_mem(), pqueue(pqueueIn), fDone(false)
    {
        // passed queue is supposed to be unused, or NULL
        if (pqueue != NULL) {
            ENTER_CRITICAL_SECTION(pqueue->ControlMutex);
            check_mem.reserve(size);
            pqueue->Setup(check_mem.data());
        }
    }

    bool Wait()
    {
        if (pqueue == NULL)
            return true;
        bool fRet = pqueue->Wait();
        fDone = true;
        return fRet;
    }

    //! Deprecated. emplacement Add + Flush are the preferred method for adding
    // checks to the queue.
    void Add(std::vector<T>& vChecks)
    {
        if (pqueue != NULL) {
            auto s = vChecks.size();
            for (T& x : vChecks) {
                check_mem.emplace_back();
                check_mem.back().swap(x);
            }
            pqueue->Add(s);
        }
    }

    //! Add directly constructs a check on the Controller's memory
    // Checks created via emplacement Add won't be executed
    // until a subsequent Flush call.
    template<typename ... Args>
    void Add(Args && ... args)
    {
        if (pqueue != NULL) {
            check_mem.emplace_back(std::forward<Args>(args)...);
        }
    }

    //! FLush is called to inform the worker of new jobs
    void Flush(size_t s)
    {
        if (pqueue != NULL) {
            pqueue->Add(s);
        }
    }

    ~CCheckQueueControl()
    {
        if (!fDone)
            Wait();
        if (pqueue != NULL) {
            LEAVE_CRITICAL_SECTION(pqueue->ControlMutex);
        }
    }
};

#endif // BITCOIN_CHECKQUEUE_H
