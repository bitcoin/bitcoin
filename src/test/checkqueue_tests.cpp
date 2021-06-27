// Copyright (c) 2012-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sync.h>
#include <util/system.h>
#include <util/time.h>

#include <test/test_dash.h>
#include <checkqueue.h>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <unordered_set>
#include <memory>
#include <random.h>

BOOST_FIXTURE_TEST_SUITE(checkqueue_tests, TestingSetup)

static const unsigned int QUEUE_BATCH_SIZE = 128;
static const int SCRIPT_CHECK_THREADS = 3;

struct FakeCheck {
    bool operator()()
    {
        return true;
    }
    void swap(FakeCheck& x){};
};

struct FakeCheckCheckCompletion {
    static std::atomic<size_t> n_calls;
    bool operator()()
    {
        n_calls.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    void swap(FakeCheckCheckCompletion& x){};
};

struct FailingCheck {
    bool fails;
    FailingCheck(bool _fails) : fails(_fails){};
    FailingCheck() : fails(true){};
    bool operator()()
    {
        return !fails;
    }
    void swap(FailingCheck& x)
    {
        std::swap(fails, x.fails);
    };
};

struct UniqueCheck {
    static Mutex m;
    static std::unordered_multiset<size_t> results GUARDED_BY(m);
    size_t check_id;
    UniqueCheck(size_t check_id_in) : check_id(check_id_in){};
    UniqueCheck() : check_id(0){};
    bool operator()()
    {
        LOCK(m);
        results.insert(check_id);
        return true;
    }
    void swap(UniqueCheck& x) { std::swap(x.check_id, check_id); };
};


struct MemoryCheck {
    static std::atomic<size_t> fake_allocated_memory;
    bool b {false};
    bool operator()()
    {
        return true;
    }
    MemoryCheck(){};
    MemoryCheck(const MemoryCheck& x)
    {
        // We have to do this to make sure that destructor calls are paired
        //
        // Really, copy constructor should be deletable, but CCheckQueue breaks
        // if it is deleted because of internal push_back.
        fake_allocated_memory.fetch_add(b, std::memory_order_relaxed);
    };
    MemoryCheck(bool b_) : b(b_)
    {
        fake_allocated_memory.fetch_add(b, std::memory_order_relaxed);
    };
    ~MemoryCheck()
    {
        fake_allocated_memory.fetch_sub(b, std::memory_order_relaxed);
    };
    void swap(MemoryCheck& x) { std::swap(b, x.b); };
};

struct FrozenCleanupCheck {
    static std::atomic<uint64_t> nFrozen;
    static std::condition_variable cv;
    static std::mutex m;
    // Freezing can't be the default initialized behavior given how the queue
    // swaps in default initialized Checks.
    bool should_freeze {false};
    bool operator()()
    {
        return true;
    }
    FrozenCleanupCheck() {}
    ~FrozenCleanupCheck()
    {
        if (should_freeze) {
            std::unique_lock<std::mutex> l(m);
            nFrozen.store(1, std::memory_order_relaxed);
            cv.notify_one();
            cv.wait(l, []{ return nFrozen.load(std::memory_order_relaxed) == 0;});
        }
    }
    void swap(FrozenCleanupCheck& x){std::swap(should_freeze, x.should_freeze);};
};

// Static Allocations
std::mutex FrozenCleanupCheck::m{};
std::atomic<uint64_t> FrozenCleanupCheck::nFrozen{0};
std::condition_variable FrozenCleanupCheck::cv{};
Mutex UniqueCheck::m;
std::unordered_multiset<size_t> UniqueCheck::results;
std::atomic<size_t> FakeCheckCheckCompletion::n_calls{0};
std::atomic<size_t> MemoryCheck::fake_allocated_memory{0};

// Queue Typedefs
typedef CCheckQueue<FakeCheckCheckCompletion> Correct_Queue;
typedef CCheckQueue<FakeCheck> Standard_Queue;
typedef CCheckQueue<FailingCheck> Failing_Queue;
typedef CCheckQueue<UniqueCheck> Unique_Queue;
typedef CCheckQueue<MemoryCheck> Memory_Queue;
typedef CCheckQueue<FrozenCleanupCheck> FrozenCleanup_Queue;


/** This test case checks that the CCheckQueue works properly
 * with each specified size_t Checks pushed.
 */
static void Correct_Queue_range(std::vector<size_t> range)
{
    auto small_queue = std::unique_ptr<Correct_Queue>(new Correct_Queue {QUEUE_BATCH_SIZE});
    small_queue->StartWorkerThreads(SCRIPT_CHECK_THREADS);
    // Make vChecks here to save on malloc (this test can be slow...)
    std::vector<FakeCheckCheckCompletion> vChecks;
    for (auto i : range) {
        size_t total = i;
        FakeCheckCheckCompletion::n_calls = 0;
        CCheckQueueControl<FakeCheckCheckCompletion> control(small_queue.get());
        while (total) {
            vChecks.resize(std::min(total, (size_t) InsecureRandRange(10)));
            total -= vChecks.size();
            control.Add(vChecks);
        }
        BOOST_REQUIRE(control.Wait());
        if (FakeCheckCheckCompletion::n_calls != i) {
            BOOST_REQUIRE_EQUAL(FakeCheckCheckCompletion::n_calls, i);
            BOOST_TEST_MESSAGE("Failure on trial " << i << " expected, got " << FakeCheckCheckCompletion::n_calls);
        }
    }
    small_queue->StopWorkerThreads();
}

/** Test that 0 checks is correct
 */
BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct_Zero)
{
    std::vector<size_t> range;
    range.push_back((size_t)0);
    Correct_Queue_range(range);
}
/** Test that 1 check is correct
 */
BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct_One)
{
    std::vector<size_t> range;
    range.push_back((size_t)1);
    Correct_Queue_range(range);
}
/** Test that MAX check is correct
 */
BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct_Max)
{
    std::vector<size_t> range;
    range.push_back(100000);
    Correct_Queue_range(range);
}
/** Test that random numbers of checks are correct
 */
BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct_Random)
{
    std::vector<size_t> range;
    range.reserve(100000/1000);
    for (size_t i = 2; i < 100000; i += std::max((size_t)1, (size_t)InsecureRandRange(std::min((size_t)1000, ((size_t)100000) - i))))
        range.push_back(i);
    Correct_Queue_range(range);
}


/** Test that failing checks are caught */
BOOST_AUTO_TEST_CASE(test_CheckQueue_Catches_Failure)
{
    auto fail_queue = std::unique_ptr<Failing_Queue>(new Failing_Queue {QUEUE_BATCH_SIZE});
    fail_queue->StartWorkerThreads(SCRIPT_CHECK_THREADS);

    for (size_t i = 0; i < 1001; ++i) {
        CCheckQueueControl<FailingCheck> control(fail_queue.get());
        size_t remaining = i;
        while (remaining) {
            size_t r = InsecureRandRange(10);

            std::vector<FailingCheck> vChecks;
            vChecks.reserve(r);
            for (size_t k = 0; k < r && remaining; k++, remaining--)
                vChecks.emplace_back(remaining == 1);
            control.Add(vChecks);
        }
        bool success = control.Wait();
        if (i > 0) {
            BOOST_REQUIRE(!success);
        } else if (i == 0) {
            BOOST_REQUIRE(success);
        }
    }
    fail_queue->StopWorkerThreads();
}
// Test that a block validation which fails does not interfere with
// future blocks, ie, the bad state is cleared.
BOOST_AUTO_TEST_CASE(test_CheckQueue_Recovers_From_Failure)
{
    auto fail_queue = std::unique_ptr<Failing_Queue>(new Failing_Queue {QUEUE_BATCH_SIZE});
    fail_queue->StartWorkerThreads(SCRIPT_CHECK_THREADS);

    for (auto times = 0; times < 10; ++times) {
        for (bool end_fails : {true, false}) {
            CCheckQueueControl<FailingCheck> control(fail_queue.get());
            {
                std::vector<FailingCheck> vChecks;
                vChecks.resize(100, false);
                vChecks[99] = end_fails;
                control.Add(vChecks);
            }
            bool r =control.Wait();
            BOOST_REQUIRE(r != end_fails);
        }
    }
    fail_queue->StopWorkerThreads();
}

// Test that unique checks are actually all called individually, rather than
// just one check being called repeatedly. Test that checks are not called
// more than once as well
BOOST_AUTO_TEST_CASE(test_CheckQueue_UniqueCheck)
{
    auto queue = std::unique_ptr<Unique_Queue>(new Unique_Queue {QUEUE_BATCH_SIZE});
    queue->StartWorkerThreads(SCRIPT_CHECK_THREADS);

    size_t COUNT = 100000;
    size_t total = COUNT;
    {
        CCheckQueueControl<UniqueCheck> control(queue.get());
        while (total) {
            size_t r = InsecureRandRange(10);
            std::vector<UniqueCheck> vChecks;
            for (size_t k = 0; k < r && total; k++)
                vChecks.emplace_back(--total);
            control.Add(vChecks);
        }
    }
    {
        LOCK(UniqueCheck::m);
        bool r = true;
        BOOST_REQUIRE_EQUAL(UniqueCheck::results.size(), COUNT);
        for (size_t i = 0; i < COUNT; ++i) {
            r = r && UniqueCheck::results.count(i) == 1;
        }
        BOOST_REQUIRE(r);
    }
    queue->StopWorkerThreads();
}


// Test that blocks which might allocate lots of memory free their memory aggressively.
//
// This test attempts to catch a pathological case where by lazily freeing
// checks might mean leaving a check un-swapped out, and decreasing by 1 each
// time could leave the data hanging across a sequence of blocks.
BOOST_AUTO_TEST_CASE(test_CheckQueue_Memory)
{
    auto queue = std::unique_ptr<Memory_Queue>(new Memory_Queue {QUEUE_BATCH_SIZE});
    queue->StartWorkerThreads(SCRIPT_CHECK_THREADS);
    for (size_t i = 0; i < 1000; ++i) {
        size_t total = i;
        {
            CCheckQueueControl<MemoryCheck> control(queue.get());
            while (total) {
                size_t r = InsecureRandRange(10);
                std::vector<MemoryCheck> vChecks;
                for (size_t k = 0; k < r && total; k++) {
                    total--;
                    // Each iteration leaves data at the front, back, and middle
                    // to catch any sort of deallocation failure
                    vChecks.emplace_back(total == 0 || total == i || total == i/2);
                }
                control.Add(vChecks);
            }
        }
        BOOST_REQUIRE_EQUAL(MemoryCheck::fake_allocated_memory, 0U);
    }
    queue->StopWorkerThreads();
}

// Test that a new verification cannot occur until all checks
// have been destructed
BOOST_AUTO_TEST_CASE(test_CheckQueue_FrozenCleanup)
{
    auto queue = std::unique_ptr<FrozenCleanup_Queue>(new FrozenCleanup_Queue {QUEUE_BATCH_SIZE});
    bool fails = false;
    queue->StartWorkerThreads(SCRIPT_CHECK_THREADS);
    std::thread t0([&]() {
        CCheckQueueControl<FrozenCleanupCheck> control(queue.get());
        std::vector<FrozenCleanupCheck> vChecks(1);
        // Freezing can't be the default initialized behavior given how the queue
        // swaps in default initialized Checks (otherwise freezing destructor
        // would get called twice).
        vChecks[0].should_freeze = true;
        control.Add(vChecks);
        control.Wait(); // Hangs here
    });
    {
        std::unique_lock<std::mutex> l(FrozenCleanupCheck::m);
        // Wait until the queue has finished all jobs and frozen
        FrozenCleanupCheck::cv.wait(l, [](){return FrozenCleanupCheck::nFrozen == 1;});
    }
    // Try to get control of the queue a bunch of times
    for (auto x = 0; x < 100 && !fails; ++x) {
        fails = queue->m_control_mutex.try_lock();
    }
    {
        // Unfreeze (we need lock n case of spurious wakeup)
        std::unique_lock<std::mutex> l(FrozenCleanupCheck::m);
        FrozenCleanupCheck::nFrozen = 0;
    }
    // Awaken frozen destructor
    FrozenCleanupCheck::cv.notify_one();
    // Wait for control to finish
    t0.join();
    BOOST_REQUIRE(!fails);
    queue->StopWorkerThreads();
}


/** Test that CCheckQueueControl is threadsafe */
BOOST_AUTO_TEST_CASE(test_CheckQueueControl_Locks)
{
    auto queue = std::unique_ptr<Standard_Queue>(new Standard_Queue{QUEUE_BATCH_SIZE});
    {
        boost::thread_group tg;
        std::atomic<int> nThreads {0};
        std::atomic<int> fails {0};
        for (size_t i = 0; i < 3; ++i) {
            tg.create_thread(
                    [&]{
                    CCheckQueueControl<FakeCheck> control(queue.get());
                    // While sleeping, no other thread should execute to this point
                    auto observed = ++nThreads;
                    MilliSleep(10);
                    fails += observed  != nThreads;
                    });
        }
        tg.join_all();
        BOOST_REQUIRE_EQUAL(fails, 0);
    }
    {
        boost::thread_group tg;
        std::mutex m;
        std::condition_variable cv;
        bool has_lock{false};
        bool has_tried{false};
        bool done{false};
        bool done_ack{false};
        {
            std::unique_lock<std::mutex> l(m);
            tg.create_thread([&]{
                    CCheckQueueControl<FakeCheck> control(queue.get());
                    std::unique_lock<std::mutex> ll(m);
                    has_lock = true;
                    cv.notify_one();
                    cv.wait(ll, [&]{return has_tried;});
                    done = true;
                    cv.notify_one();
                    // Wait until the done is acknowledged
                    //
                    cv.wait(ll, [&]{return done_ack;});
                    });
            // Wait for thread to get the lock
            cv.wait(l, [&](){return has_lock;});
            bool fails = false;
            for (auto x = 0; x < 100 && !fails; ++x) {
                fails = queue->m_control_mutex.try_lock();
            }
            has_tried = true;
            cv.notify_one();
            cv.wait(l, [&](){return done;});
            // Acknowledge the done
            done_ack = true;
            cv.notify_one();
            BOOST_REQUIRE(!fails);
        }
        tg.join_all();
    }
}
BOOST_AUTO_TEST_SUITE_END()
