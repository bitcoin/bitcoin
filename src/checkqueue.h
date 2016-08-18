// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHECKQUEUE_H
#define BITCOIN_CHECKQUEUE_H

#include <vector>
#include "util.h"
#include "utiltime.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <sstream>
#include <iostream>
#include <mutex>


#ifndef ATOMIC_BOOL_LOCK_FREE
#pragma message("std::atomic<bool> is not lock free")
#endif
#ifndef ATOMIC_LONG_LOCK_FREE
#pragma message("std::atomic<uint32_t> is not lock free")
#endif
#ifndef ATOMIC_LLONG_LOCK_FREE
#pragma message("std::atomic<uint64_t> is not lock free")
#endif


/** Forward Declaration on CCheckQueue. Note default no testing. */
template <typename T, size_t J, size_t W, bool TFE = false, bool TLE = false>
class CCheckQueue;
/** Forward Declaration on CCheckQueueControl */
template <typename Q>
class CCheckQueueControl;


/** CCheckQueue_Internals contains various components that otherwise could live inside
 * of CCheckQueue, but is separate for easier testability and modularity */
namespace CCheckQueue_Internals
{
template <typename Q>
class inserter {
    Q * const queue;
    typename Q::JOB_TYPE * start;
    typename Q::JOB_TYPE * free_index;
    public:
    inserter(Q * const queueIn) :
       queue(queueIn),
       start(queue ? queue->get_next_free_index() : nullptr),
       free_index(start) {}
    typename Q::JOB_TYPE * operator()()
    {
        return start ? free_index++ : nullptr;
    }
    ~inserter()
    {
        if (queue)
            queue->Flush(std::distance(start, free_index));
    }
};
/** job_array holds the atomic flags and the job data for the queue
     * and provides methods to assist in accessing or adding jobs.
     */
template <typename Q>
class job_array
{
    /** the raw check type */
    std::array<typename Q::JOB_TYPE, Q::MAX_JOBS> checks;
    /** atomic flags which are used to reserve a check from checks
             * C++11 standard guarantees that these are atomic on all platforms
             * */
    std::array<std::atomic_flag, Q::MAX_JOBS> flags;
    /** used as the insertion point into the array. */
    typename Q::JOB_TYPE* next_free_index;
    size_t RT_N_SCRIPTCHECK_THREADS;

public:
    job_array() :  next_free_index(checks.begin()), RT_N_SCRIPTCHECK_THREADS(0)
    {
        for (size_t i = 0; i < Q::MAX_JOBS; ++i) 
            checks[i] = typename Q::JOB_TYPE {};
        for (auto& i : flags)
            i.clear();
    }

    void init(const size_t rt)
    {
        RT_N_SCRIPTCHECK_THREADS = rt;
    }

    typename Q::JOB_TYPE* get_next_free_index()
    {
        return next_free_index;
    }
    void Add(ptrdiff_t n) 
    {
        next_free_index += n;
    }
    
    /** reserve tries to set a flag for an element 
             * and returns if it was successful */
    bool reserve(const size_t i)
    {
        return !flags[i].test_and_set();
    }

    /** reset_flag resets a flag */
    void reset_flag(const size_t i)
    {
        flags[i].clear();
    }

    void reset_flags_for(const size_t ID, const size_t to) 
    {
        for (size_t i = ID; i < to; i += RT_N_SCRIPTCHECK_THREADS)
            reset_flag(i);
    }
    /** eval runs a check at specified index */
    bool eval(const size_t i)
    {
        bool b = checks[i]();
        typename Q::JOB_TYPE tmp {};
        checks[i].swap(tmp);
        return b;
    }

    /** reset_jobs resets the insertion index only, so should only be run on master.
             *
             * The caller must ensure that forall i, checks[i] is destructed and flags[i] is
             * reset.
             *
             * NOTE: This cleanup done "for free" elsewhere
             *      - checksi] is destructed by master on swap
             *      - flags[i] is reset by each thread while waiting to be cleared for duty
             */
    void reset_jobs()
    {
        next_free_index = checks.begin();
    }
    void clear_check_memory()
    {
        for (auto it = checks.begin(); it != next_free_index; ++it) {
            typename Q::JOB_TYPE tmp {};
            it->swap(tmp);
        }
    }

    decltype(&checks) TEST_get_checks()
    {
        return Q::TEST_FUNCTIONS_ENABLE ? &checks : nullptr;
    }
};
/* barrier is used to communicate that a thread has finished
     * all work and reported any bad checks it might have seen.
     *
     * Results should normally be cached thread locally (once a thread is done, it
     * will not mark itself un-done so no need to read the atomic twice)
     */

template <typename Q>
class barrier
{
    std::array<std::atomic_bool, Q::MAX_WORKERS> state;
    size_t RT_N_SCRIPTCHECK_THREADS;
    std::atomic<size_t> count;

public:
    /** Default state is false so that first round looks like no prior round*/
    barrier() : RT_N_SCRIPTCHECK_THREADS(0), count(0) {
    }
    void init(const size_t rt)
    {
        RT_N_SCRIPTCHECK_THREADS = rt;
    }
    void finished()
    {
        ++count;
    }
    void wait_all_finished() const
    {
        while (count != RT_N_SCRIPTCHECK_THREADS)
            ;
    }
    void reset()
    {
        count.store(0);
    }
    void wait_reset() const 
    {
        while (count.load() != 0)
            ;
    }
};
/* PriorityWorkQueue exists to help threads select work 
     * to do in a cache friendly way. As long as all entries added are
     * popped it will be correct. Performance comes from intelligently
     * chosing the order.
     *
     * Each thread has a unique id, and preferentiall evaluates
     * jobs in an index i such that  i == id (mod RT_N_SCRIPTCHECK_THREADS) in increasing
     * order.
     *
     * After id aligned work is finished, the thread walks sequentially
     * through its neighbors (id +1%RT_N_SCRIPTCHECK_THREADS, id+2% RT_N_SCRIPTCHECK_THREADS) to find work.
     *
     * TODO: future optimizations
     *     - Abort (by clearing)
     *       remaining on backwards walk if one that is reserved
     *       already, because it means either the worker's stuff is done
     *       OR it already has 2 (or more) workers already who will finish it.
     *     - Use an interval set rather than a vector (maybe)
     *     - Select thread by most amount of work remaining 
     *       (requires coordination)
     *     - Preferentially help 0 (the master) as it joins last
     *     - have two levels of empty, priority_empty and all_empty
     *       (check for more work when priority_empty)
     *
     */
template <typename Q>
class PriorityWorkQueue
{
    std::array<size_t, Q::MAX_WORKERS> n_done;
    /** The Worker's ID */
    const size_t id;
    /** The number of workers that bitcoind started with, eg, RunTime Number ScriptCheck Threads  */
    const size_t RT_N_SCRIPTCHECK_THREADS;
    /** Stores the total inserted since the last reset (ignores pop) */
    size_t total;
    /** a cache of the last queue we were popping from, reset on adds and (circularly) incremented on pops 
             * Otherwise pops have an O(workers) term, this keeps pop amortized constant */
    size_t id2_cache;


public:
    PriorityWorkQueue(){};
    PriorityWorkQueue(size_t id_, size_t RT_N_SCRIPTCHECK_THREADS_) : n_done(), id(id_), RT_N_SCRIPTCHECK_THREADS(RT_N_SCRIPTCHECK_THREADS_), total(0), id2_cache((id_ + 1) % RT_N_SCRIPTCHECK_THREADS){};
    /** adds entries for execution [total, n)
             * Places entries in the proper bucket
             * Resets the next thread to help (id2_cache) if work was added
             */
    void add(const size_t n)
    {
        if (n > total) {
            total = n;
            id2_cache = (id + 1) % RT_N_SCRIPTCHECK_THREADS;
        }
    }
    size_t get_total() const
    {
        return total;
    }


    /* Get one first from out own work stack (take the first one) and then try from neighbors sequentially
             * (from the last one on that neighbors stack)
             */
    bool pop(size_t& val, const bool stealing)
    {
        val = (id + (n_done[id]) * RT_N_SCRIPTCHECK_THREADS);
        if (val < total) {
            ++n_done[id];
            return true;
        }
        if (!stealing)
            return false;
        // Iterate untill id2 wraps around to id.
        for (; id2_cache != id; id2_cache = (id2_cache + 1) % RT_N_SCRIPTCHECK_THREADS) {
            val = (id2_cache + (n_done[id2_cache]) * RT_N_SCRIPTCHECK_THREADS);
            if (val < total) {
                ++n_done[id2_cache];
                return true;
            }
        }
        return false;
    }
};

class atomic_condition {
    std::atomic<uint8_t> state;
    public:
    atomic_condition() : state(0) {};
    void wakeup()
    {
        ++state;
    }
    void sleep()
    {
        --state;
    }
    void kill()
    {
        state.store(3);
    }
    void resurrect()
    {
        state.store(0);
    }
    bool wait() const
    {
        for (;;) {
            switch (state.load()) {
                case 0:
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                    break;
                case 1:
                    return true;
                default:
                    return false;
            }
        }
    }
};
}


/** Queue for verifications that have to be performed.
 *
 * The verifications are represented by a type T, which must provide an
 * operator()(), returning a bool.
 *
 * One thread (the master) is assumed to push batches of verifications
 * onto the queue, where they are processed by N-1 worker threads. When
 * the master is done adding work, it temporarily joins the worker pool
 * as an N'th worker, until all jobs are done.
 *
 * @tparam T the type of callable check object
 * @tparam J the maximum number of jobs possible 
 * @tparam W the maximum number of workers possible
 */

template <typename T, size_t J, size_t W, bool TFE, bool TLE>
class CCheckQueue
{
public:
    typedef T JOB_TYPE;
    typedef CCheckQueue<T, J, W, TFE, TLE> SELF;
    static const size_t MAX_JOBS = J;
    static const size_t MAX_WORKERS = W;
    static const bool TEST_FUNCTIONS_ENABLE = TFE;
    static const bool TEST_LOGGING_ENABLE = TLE;

private:
    CCheckQueue_Internals::job_array<SELF> jobs;
    CCheckQueue_Internals::barrier<SELF> work;
    CCheckQueue_Internals::barrier<SELF> cleanup;
    CCheckQueue_Internals::atomic_condition sleeper;
    size_t RT_N_SCRIPTCHECK_THREADS;
    /** The number of checks put into the queue, done or not */
    std::atomic<size_t> nAvail;
    /** true if all checks were successful, false if any failure occurs */
    std::atomic<bool> fAllOk;
    /** true if the master has joined, false otherwise. A round may not terminate unless masterJoined */
    std::atomic<bool> masterJoined;
    /** hold handles for work */
    std::vector<std::thread> threads;
    /** Makes sure only one thread is using control functionality at a time*/
    std::mutex control_mtx;
    /** state only for testing */
    mutable std::atomic<size_t> test_log_seq;
    mutable std::array<std::ostringstream, MAX_WORKERS> test_log;

    void consume(const size_t ID)
    {
        TEST_log(ID, [](std::ostringstream& o) {o << "In consume\n";});
        CCheckQueue_Internals::PriorityWorkQueue<SELF> work_queue(ID, RT_N_SCRIPTCHECK_THREADS);
        for (;;) {
            // Note: Must check masterJoined before nAvail, otherwise
            // {Thread A: nAvail.load();} {Thread B:nAvail++; masterJoined = true;} {Thread A: masterJoined.load()}
            bool stealing = masterJoined.load();
            work_queue.add(nAvail.load());
            size_t job_id;
            bool got_data = work_queue.pop(job_id, stealing);
            if (got_data && jobs.reserve(job_id)) {
                if (!jobs.eval(job_id)) {
                    fAllOk.store(false);
                    return;
                }
            }
            else if (stealing && !got_data)
                break;

        } 
            
        TEST_log(ID, [&, this](std::ostringstream& o) {o << "Leaving consume. fAllOk was " << fAllOk.load() << "\n";});
    }
    /** Internal function that does bulk of the verification work. */
    bool Master() {
        const size_t ID = 0;
        work.wait_reset();
        masterJoined.store(true);
        TEST_log(ID, [](std::ostringstream& o) { o << "Master just set masterJoined\n"; });
        consume(ID);
        work.finished();
        work.wait_all_finished();
        TEST_log(ID, [](std::ostringstream& o) { o << "(Master) saw all threads finished\n"; });
        bool fRet = fAllOk;
        sleeper.sleep();
        masterJoined.store(false);
        return fRet;
    }
    void Loop(const size_t ID)
    {
        while (sleeper.wait()) {
            TEST_log(ID, [](std::ostringstream& o) {o << "Round starting\n";});
            consume(ID);
            // Only spins here if !fAllOk, otherwise consume finished all
            while (!masterJoined.load())
                ;
            size_t prev_total = nAvail.load();
            TEST_log(ID, [&, this](std::ostringstream& o) {
                o << "saw up to " << prev_total << " master was "
                << masterJoined.load() << " nAvail " << nAvail.load() << '\n';
            });
            work.finished();
            // We wait until the master reports leaving explicitly
            while (masterJoined.load())
                ;
            TEST_log(ID, [](std::ostringstream& o) { o << "Saw master leave\n"; });
            jobs.reset_flags_for(ID, prev_total);
            cleanup.finished();
            if (ID == 1) {
                // Reset master flags too
                jobs.reset_flags_for(0, prev_total);
                jobs.clear_check_memory();
                cleanup.wait_all_finished();
                cleanup.reset();
                work.reset();
            }
        }
        LogPrintf("CCheckQueue @%#010x Worker %q shutting down\n", this, ID);
    }

public:
    CCheckQueue() : jobs(), work(), cleanup(), sleeper(), RT_N_SCRIPTCHECK_THREADS(0),
    nAvail(0), fAllOk(true), masterJoined(false), test_log_seq(0) {}

    void reset_jobs()
    {
        jobs.reset_jobs();
    }
    //! Worker thread
    void Thread(size_t ID)
    {
        RenameThread("bitcoin-scriptcheck");
        LogPrintf("Starting CCheckQueue Worker %q on CCheckQueue %#010x\n", ID, this);
        Loop(ID);
    }

    void ControlLock() {
        control_mtx.lock();
        TEST_log(0, [](std::ostringstream& o) {
            o << "Resetting nAvail and fAllOk" << '\n';
        });
        nAvail.store(0);
        fAllOk.store(true);
        sleeper.wakeup();
        reset_jobs();
    }
    void ControlUnlock() {
        control_mtx.unlock();
    };

    //! Wait until execution finishes, and return whether all evaluations were successful.
    bool Wait()
    {
        return Master();
    }
    void quit()
    {
        std::lock_guard<std::mutex> l(control_mtx);
        sleeper.kill();
        for (auto& t : threads)
            t.join();
        threads.clear();
        jobs.clear_check_memory();
    }

    JOB_TYPE* get_next_free_index() {
        return jobs.get_next_free_index();
    }
    void Flush(std::ptrdiff_t n)
    {
        nAvail.fetch_add(n);
        jobs.Add(n);
        TEST_log(0, [&, this](std::ostringstream& o) {
            o << "Added " << n << " values. nAvail was " 
            << nAvail.load() - n << " now is " << nAvail.load() << " \n";
        });
    }
    ~CCheckQueue()
    {
        quit();
    }

    void init(const size_t RT_N_SCRIPTCHECK_THREADS_)
    {
        std::lock_guard<std::mutex> l(control_mtx);
        RT_N_SCRIPTCHECK_THREADS = RT_N_SCRIPTCHECK_THREADS_;
        work.init(RT_N_SCRIPTCHECK_THREADS);
        cleanup.init(RT_N_SCRIPTCHECK_THREADS-1);
        jobs.init(RT_N_SCRIPTCHECK_THREADS);
        sleeper.resurrect();
        for (size_t id = 1; id < RT_N_SCRIPTCHECK_THREADS; ++id) {
            std::thread t([=]() {Thread(id); });
            threads.push_back(std::move(t));
        }
    }

    /** Various testing functionalities */

    void TEST_consume(const size_t ID)
    {
        if (TEST_FUNCTIONS_ENABLE)
            consume(ID);
    }

    void TEST_set_masterJoined(const bool b)
    {
        if (TEST_FUNCTIONS_ENABLE)
            masterJoined.store(b);
    }

    size_t TEST_count_set_flags()
    {
        auto count = 0;
        for (auto t = 0; t < MAX_JOBS && TEST_FUNCTIONS_ENABLE; ++t)
            count += jobs.reserve(t) ? 0 : 1;
        return count;
    }

    void TEST_reset_all_flags()
    {
        for (auto t = 0; t < MAX_JOBS && TEST_FUNCTIONS_ENABLE; ++t)
            jobs.reset_flag(t);
    }

    template <typename Callable>
    void TEST_log(const size_t ID, Callable c) const
    {
        if (TEST_LOGGING_ENABLE) {
            test_log[ID] << "[[" << test_log_seq++ <<"]] ";
            c(test_log[ID]);
        }
    }

    void TEST_dump_log(const size_t upto) const
    {
        if (TEST_FUNCTIONS_ENABLE) {
            LogPrintf("\n#####################\n## Round Beginning ##\n#####################");
            for (auto i = 0; i < upto; ++i)
                LogPrintf("\n------------------\n%s\n------------------\n\n", test_log[i].str());
        }
    }

    void TEST_erase_log() const
    {
        for (auto i = 0; i < MAX_WORKERS && TEST_FUNCTIONS_ENABLE; ++i) {
            test_log[i].str("");
            test_log[i].clear();
        }
    }

    CCheckQueue_Internals::job_array<SELF>* TEST_introspect_jobs()
    {
        return TEST_FUNCTIONS_ENABLE ? &jobs : nullptr;
    }
};

/** 
 * RAII-style controller object for a CCheckQueue that guarantees the passed
 * queue is finished before continuing.
 */
template <typename Q>
class CCheckQueueControl
{
private:
    Q * const pqueue;
    bool fDone;

public:
    CCheckQueueControl(Q* pqueueIn) : pqueue(pqueueIn), fDone(false)
    {
        if (pqueue) 
            pqueue->ControlLock();
    }

    bool Wait()
    {
        if (!pqueue || fDone)
            return true;
        bool fRet = pqueue->Wait();
        fDone = true;
        pqueue->ControlUnlock();
        return fRet;
    }

    CCheckQueue_Internals::inserter<Q> get_inserter() {
        return CCheckQueue_Internals::inserter<Q>(pqueue);
    }

    ~CCheckQueueControl()
    {
        Wait();
    }
};

#endif // BITCOIN_CHECKQUEUE_H
