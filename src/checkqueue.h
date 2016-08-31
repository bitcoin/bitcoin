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
template <typename T, bool TFE = false, bool TLE = false>
class CCheckQueue;
/** Forward Declaration on CCheckQueueControl */
template <typename Q>
class CCheckQueueControl;


/** CCheckQueue_Internals contains various components that otherwise could live inside
 * of CCheckQueue, but is separate for easier testability and modularity */
namespace CCheckQueue_Internals
{
/** job_array holds the atomic flags and the job data for the queue
     * and provides methods to assist in accessing or adding jobs.
     */
template <typename T>
class job_array
{
    /** the raw check type */
    std::vector<T> checks;
    /** atomic flags which are used to reserve a check from checks
             * C++11 standard guarantees that these are atomic on all platforms
             * */
    struct default_cleared_flag : std::atomic_flag {
        default_cleared_flag () : std::atomic_flag() { clear(); };
        default_cleared_flag (const default_cleared_flag & s) : std::atomic_flag() { clear(); };
    };
    std::vector<default_cleared_flag> flags;
    size_t RT_N_SCRIPTCHECK_THREADS;

public:
    job_array() :  RT_N_SCRIPTCHECK_THREADS(0)
    {
    }

    void init(const size_t MAX_JOBS, const size_t rt)
    {
        RT_N_SCRIPTCHECK_THREADS = rt;
        checks.reserve(MAX_JOBS);
        flags.resize(MAX_JOBS);
    }
    void emplace_back(T&& t)
    {
        checks.emplace_back(std::move(t));
    }

    size_t size()
    {
        return checks.size();
    }
    
    /** reserve tries to set a flag for an element 
             * and returns if it was successful */
    bool reserve(const size_t i)
    {
        return !flags[i].test_and_set();
    }

    void reset_flags_for(const size_t ID, const size_t to) 
    {
        for (size_t i = ID; i < to; i += RT_N_SCRIPTCHECK_THREADS)
            flags[i].clear();
    }
    /** eval runs a check at specified index */
    bool eval(const size_t i)
    {
        return checks[i]();
    }

    void clear_check_memory()
    {
        checks.clear();
    }

    decltype(&checks) TEST_get_checks()
    {
        return  &checks;
    }
};
/* barrier is used to communicate that a thread has finished
     * all work and reported any bad checks it might have seen.
     *
     * Results should normally be cached thread locally (once a thread is done, it
     * will not mark itself un-done so no need to read the atomic twice)
     */

class barrier
{
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
class PriorityWorkQueue
{
    std::vector<size_t> n_done;
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
    PriorityWorkQueue(size_t id_, size_t RT_N_SCRIPTCHECK_THREADS_) : n_done(), id(id_), RT_N_SCRIPTCHECK_THREADS(RT_N_SCRIPTCHECK_THREADS_), total(0), id2_cache((id_ + 1) % RT_N_SCRIPTCHECK_THREADS){
        n_done.resize(RT_N_SCRIPTCHECK_THREADS);
    };
    void reset() {
        for (auto& i : n_done)
            i = 0;
        total = 0;
        id2_cache = (id +1) % RT_N_SCRIPTCHECK_THREADS;
    }
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

template <typename T, bool TFE, bool TLE>
class CCheckQueue
{
public:
    typedef T JOB_TYPE;
    static const bool TEST_FUNCTIONS_ENABLE = TFE;
    static const bool TEST_LOGGING_ENABLE = TLE;

private:
    CCheckQueue_Internals::job_array<JOB_TYPE> jobs;
    CCheckQueue_Internals::barrier work;
    CCheckQueue_Internals::barrier cleanup;
    CCheckQueue_Internals::atomic_condition sleeper;
    size_t RT_N_SCRIPTCHECK_THREADS;
    size_t MAX_JOBS;
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
    mutable std::vector<std::unique_ptr<std::ostringstream>> test_log;

    void consume(CCheckQueue_Internals::PriorityWorkQueue& work_queue)
    {
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
                return;
        } 
    }
    /** Internal function that does bulk of the verification work. */
    bool Master() {
        const size_t ID = 0;
        static CCheckQueue_Internals::PriorityWorkQueue work_queue(ID, RT_N_SCRIPTCHECK_THREADS);
        work_queue.reset();
        masterJoined.store(true);
        TEST_log(ID, [](std::ostringstream& o) { o << "Master just set masterJoined\n"; });
        consume(work_queue);
        work.finished();
        work.wait_all_finished();
        TEST_log(ID, [](std::ostringstream& o) { o << "(Master) saw all threads finished\n"; });
        bool fRet = fAllOk.load();
        sleeper.sleep();
        masterJoined.store(false);
        return fRet;
    }
    void Loop(const size_t ID)
    {

        CCheckQueue_Internals::PriorityWorkQueue work_queue(ID, RT_N_SCRIPTCHECK_THREADS);
        while (sleeper.wait()) {
            TEST_log(ID, [](std::ostringstream& o) {o << "Round starting\n";});
            consume(work_queue);
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
            TEST_log(ID, [](std::ostringstream& o) { o << "Resetting nAvail and fAllOk\n"; });
            nAvail.store(0);
            fAllOk.store(true);
            if (ID == 1) {
                // Reset master flags too
                jobs.reset_flags_for(0, prev_total);
                jobs.clear_check_memory();
                cleanup.wait_all_finished();
                cleanup.reset();
                work.reset();
            }
            work_queue.reset();
        }
        LogPrintf("CCheckQueue @%#010x Worker %q shutting down\n", this, ID);
    }

public:
    CCheckQueue() : jobs(), work(), cleanup(), sleeper(), RT_N_SCRIPTCHECK_THREADS(0),
    nAvail(0), fAllOk(true), masterJoined(false), test_log_seq(0) {}

    //! Worker thread
    void Thread(size_t ID)
    {
        RenameThread("bitcoin-scriptcheck");
        LogPrintf("Starting CCheckQueue Worker %q on CCheckQueue %#010x\n", ID, this);
        Loop(ID);
    }

    void ControlLock() {
        control_mtx.lock();
        sleeper.wakeup();
        work.wait_reset();
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


    struct emplacer {
        CCheckQueue_Internals::job_array<JOB_TYPE>& j;
        std::atomic<size_t>& nAvail;
        emplacer(CCheckQueue_Internals::job_array<JOB_TYPE>& j_, std::atomic<size_t>& nAvail_) : j(j_), nAvail(nAvail_) {}
        void operator()(JOB_TYPE && t)
        {
            j.emplace_back(std::move(t));
        }
        ~emplacer()
        {
            nAvail.store(j.size());
        }

    };
    emplacer get_emplacer() {
        return emplacer(jobs, nAvail);
    }
    ~CCheckQueue()
    {
        quit();
    }

    void init(const size_t MAX_JOBS_, const size_t RT_N_SCRIPTCHECK_THREADS_)
    {
        std::lock_guard<std::mutex> l(control_mtx);
        MAX_JOBS = MAX_JOBS_;
        RT_N_SCRIPTCHECK_THREADS = RT_N_SCRIPTCHECK_THREADS_;
        for (auto i = 0; i < RT_N_SCRIPTCHECK_THREADS && TEST_LOGGING_ENABLE; ++i)
            test_log.push_back(std::unique_ptr<std::ostringstream>(new std::ostringstream()));
        work.init(RT_N_SCRIPTCHECK_THREADS);
        cleanup.init(RT_N_SCRIPTCHECK_THREADS-1);
        jobs.init(MAX_JOBS, RT_N_SCRIPTCHECK_THREADS);
        sleeper.resurrect();
        for (size_t id = 1; id < RT_N_SCRIPTCHECK_THREADS; ++id) {
            std::thread t([=]() {Thread(id); });
            threads.push_back(std::move(t));
        }
    }

    /** Various testing functionalities */

    void TEST_consume(const size_t ID)
    {
        static CCheckQueue_Internals::PriorityWorkQueue work_queue(ID, RT_N_SCRIPTCHECK_THREADS);
        if (TEST_FUNCTIONS_ENABLE)
            consume(work_queue);
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
            *test_log[ID] << "[[" << test_log_seq++ <<"]] ";
            c(*test_log[ID]);
        }
    }

    void TEST_dump_log() const
    {
        if (TEST_FUNCTIONS_ENABLE) {
            LogPrintf("\n#####################\n## Round Beginning ##\n#####################");
            for (auto& i : test_log)
                LogPrintf("\n------------------\n%s\n------------------\n\n", i->str());
        }
    }

    void TEST_erase_log() const
    {
        if (TEST_FUNCTIONS_ENABLE)
        for (auto& i : test_log) {
            i->str("");
            i->clear();
        }
    }

    CCheckQueue_Internals::job_array<JOB_TYPE>* TEST_introspect_jobs()
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

    typename Q::emplacer get_emplacer()
    {
        return pqueue->get_emplacer();
    }

    operator bool() {
        return (!!pqueue);
    }

    ~CCheckQueueControl()
    {
        Wait();
    }
};

#endif // BITCOIN_CHECKQUEUE_H
