// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"
#include "utiltime.h"
#include "main.h"

#include "test/test_bitcoin.h"
#include "checkqueue.h"
#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>

#include <unordered_set>
#include <memory>
#include "random.h"
BOOST_FIXTURE_TEST_SUITE(checkqueue_tests, TestingSetup)


class RAII_ThreadGroup
{
    std::vector<std::thread> threadGroup;

public:
    template <typename Callable>
    void create_thread(Callable c)
    {
        std::thread t(c);
        threadGroup.push_back(std::move(t));
    };
    void join_all()
    {
        for (auto& t : threadGroup)
            t.join();
        threadGroup.clear();
    };
    ~RAII_ThreadGroup()
    {
        join_all();
    };
};

struct FakeJob {
};
typedef CCheckQueue<FakeJob, true, false> Standard_Queue;
typedef typename Standard_Queue::JOB_TYPE JT; // This is a "recursive template" hack

BOOST_AUTO_TEST_CASE(test_CheckQueue_PriorityWorkQueue_basic)
{
    CCheckQueue_Internals::PriorityWorkQueue work(0, 16);
    auto m = 0;
    work.add(100);
    size_t x = 0;
    work.pop(x, true);
    BOOST_REQUIRE(x == 0);
    work.pop(x, true);
    BOOST_REQUIRE(x == 16);
    m = 2;
    while (work.pop(x, true)) {
        ++m;
    }
    BOOST_REQUIRE(m == 100);
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_PriorityWorkQueue_emits_all)
{
    CCheckQueue_Internals::PriorityWorkQueue work(0, 16);
    auto m = 0;
    work.add(200);
    std::unordered_multiset<size_t> results;
    size_t x;
    while (work.pop(x, true)) {
        results.insert(x);
        ++m;
    }
    bool b = true;
    for (auto i = 0; i < 200; ++i) {
        b = b && results.count(i) == 1;
        results.erase(i);
    }
    BOOST_REQUIRE(b);
    BOOST_REQUIRE(results.empty());
    BOOST_REQUIRE(m == 200);
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_PriorityWorkQueue_stealing)
{
    CCheckQueue_Internals::PriorityWorkQueue work(0, 16);
    auto m = 0;
    work.add(160);
    std::unordered_multiset<size_t> results;
    size_t x;
    while (work.pop(x, false)) {
        results.insert(x);
        ++m;
    }
    bool b = true;
    for (auto i = 0; i < 160; i+=16) {
        b = b &&results.count(i) == 1;
        results.erase(i);
    }
    BOOST_REQUIRE(b);
    BOOST_REQUIRE(results.empty());
    while (work.pop(x, true)) {
        results.insert(x);
        ++m;
    }
    bool b2 = true;
    for (auto i = 0; i < 160; ++i) {
        b2 = b2 && (results.count(i) == 1 || (i %16) == 0);
        results.erase(i);
    }
    BOOST_REQUIRE(b2);
}

typedef CCheckQueue_Internals::job_array<FakeJob> J;
BOOST_AUTO_TEST_CASE(test_CheckQueue_job_array)
{
    const size_t MAX_JOBS = 100000;
    auto jobs = std::shared_ptr<J>(new J());
    jobs->init(MAX_JOBS, 1);
    std::atomic<size_t> m;

    jobs->reset_flags_for(0, MAX_JOBS);

    m = 0;
    std::thread t([&](std::atomic<size_t>& m) {
        for (size_t i = 0; i < MAX_JOBS; ++i)
            m += jobs->reserve(i) ? 1 : 0;
    }, std::ref(m));


    std::thread t2([&](std::atomic<size_t>& m) {
        for (size_t i = 0; i < MAX_JOBS; ++i)
            m += jobs->reserve(i) ? 1 : 0;
    }, std::ref(m));
    t.join();
    t2.join();

    BOOST_REQUIRE(m == MAX_JOBS);
}

BOOST_AUTO_TEST_CASE(test_CheckQueue_round_barrier)
{
    RAII_ThreadGroup threadGroup;
    auto b = std::shared_ptr<CCheckQueue_Internals::barrier>(new CCheckQueue_Internals::barrier());
    b->init(nScriptCheckThreads);
    b->reset();

    for (int i = 0; i < nScriptCheckThreads; ++i)
        threadGroup.create_thread([&, i]() {
            b->finished();
            b->wait_all_finished();
        });

    threadGroup.join_all();
}

struct FakeJobNoWork {
    bool operator()()
    {
        return true;
    }
    void swap(FakeJobNoWork& x){};
};
typedef CCheckQueue<FakeJobNoWork, true, false> Consume_Queue;
BOOST_AUTO_TEST_CASE(test_CheckQueue_consume)
{
    auto fast_queue = std::shared_ptr<Consume_Queue>(new Consume_Queue());
    fast_queue->init(100000, nScriptCheckThreads);
    std::atomic<int> spawned{0};
    RAII_ThreadGroup threadGroup;

    for (int i = 0; i < nScriptCheckThreads; ++i) {
        threadGroup.create_thread([&, i]() {
            ++spawned;
            fast_queue->TEST_consume(i);
        });
    }

    while (spawned != nScriptCheckThreads);
    for (auto y = 0; y < 1000; ++y) {
        auto emplacer = fast_queue->get_emplacer();
        for (auto x = 0; x< 100; ++x) 
            emplacer(FakeJobNoWork{});
    }
    fast_queue->TEST_set_masterJoined(true);

    threadGroup.join_all();

    size_t count = fast_queue->TEST_count_set_flags();
    BOOST_REQUIRE(count == 100000);
}


struct FakeJobCheckCompletion {
    static std::atomic<size_t> n_calls;
    bool operator()()
    {
        ++n_calls;
        return true;
    }
    void swap(FakeJobCheckCompletion& x){};
};
std::atomic<size_t> FakeJobCheckCompletion::n_calls{0};
const bool print_Correct_Queue = false;
typedef CCheckQueue<FakeJobCheckCompletion,  true, print_Correct_Queue> Correct_Queue;
void Correct_Queue_range(std::vector<size_t> range)
{
    auto small_queue = std::shared_ptr<Correct_Queue>(new Correct_Queue);
    small_queue->init(100000, nScriptCheckThreads);
    for (auto i : range) {
        size_t total = i;
        FakeJobCheckCompletion::n_calls = 0;
        {
            CCheckQueueControl<Correct_Queue> control(small_queue.get());
            while (total) {
                size_t r = GetRand(10);
                auto emplacer = control.get_emplacer();
                for (size_t k = 0; k < r && total; k++) {
                    total--;
                    emplacer(FakeJobCheckCompletion{});
                }
            }
        }
        if (print_Correct_Queue) {
            small_queue->TEST_dump_log();
            small_queue->TEST_erase_log();
        }
        if (FakeJobCheckCompletion::n_calls != i) {
            BOOST_REQUIRE(FakeJobCheckCompletion::n_calls == i);
            BOOST_TEST_MESSAGE("Failure on trial " << i << " expected, got " << FakeJobCheckCompletion::n_calls);
        }
    }
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct_Zero)
{
    std::vector<size_t> range;
    range.push_back((size_t)0);
    Correct_Queue_range(range);
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct_One)
{
    std::vector<size_t> range;
    range.push_back((size_t)1);
    Correct_Queue_range(range);
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct_Max)
{
    std::vector<size_t> range;
    range.push_back(100000);
    Correct_Queue_range(range);
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct_Random) 
{
    std::vector<size_t> range;
    for (size_t i = 2; i < 100000; i += std::max((size_t)1,(size_t) GetRand(std::min((size_t) 1000, ((size_t) 100000)-i))))
        range.push_back(i);
    Correct_Queue_range(range);
}

struct FailingJob {
    bool fails;
    bool call_state;
    FailingJob(bool fails) : fails(fails), call_state(false){};
    FailingJob() : fails(true), call_state(false){};
    bool operator()()
    {
        call_state = true;
        return !fails;
    }
    void swap(FailingJob& x)
    {
        std::swap(fails, x.fails);
        std::swap(call_state, x.call_state);
    };
};
typedef CCheckQueue<FailingJob, true, false> Failing_Queue;
BOOST_AUTO_TEST_CASE(test_CheckQueue_Catches_Failure)
{
    auto fail_queue = std::unique_ptr<Failing_Queue>(new Failing_Queue());

    fail_queue->init(1000, nScriptCheckThreads);

    for (size_t i = 0; i < 1001; ++i) {
        CCheckQueueControl<Failing_Queue> control(fail_queue.get());
        size_t remaining = i;
        while (remaining) {
            size_t r = GetRand(10);

            auto emplacer = control.get_emplacer();
            for (size_t k = 0; k < r && remaining; k++, remaining--)
                emplacer( FailingJob{remaining == 1});
        }
        bool success = control.Wait();
        if (success && i > 0) {
            size_t nChecked = 0;
            std::vector<FailingJob>* jobs = fail_queue->TEST_introspect_jobs()->TEST_get_checks();
            for (auto j : *jobs)
                if (j.call_state)
                    nChecked++;
            fail_queue->TEST_dump_log();
            fail_queue->TEST_erase_log();
            BOOST_REQUIRE(!success);
        } else if (i == 0) {
            fail_queue->TEST_erase_log();
            BOOST_REQUIRE(success);
        }
        fail_queue->TEST_erase_log();
    }
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_Recovers_From_Failure)
{
    auto fail_queue = std::unique_ptr<Failing_Queue>(new Failing_Queue());
    std::array<FailingJob, 100> checks;
    fail_queue->init(100, nScriptCheckThreads);

    for (auto times = 0; times < 10; ++times) {
        std::array<bool, 2> result;
        for (bool end_fails : {true, false}) {
            CCheckQueueControl<Failing_Queue> control(fail_queue.get());
            {
                auto emplacer = control.get_emplacer();
                for (size_t k = 0; k < 100; ++k)
                    emplacer(FailingJob {k == 99 && end_fails});
            }
            result[end_fails ? 0 : 1] = control.Wait();
            fail_queue->TEST_dump_log();
            fail_queue->TEST_erase_log();
        }
        BOOST_CHECK(!result[0]);
        BOOST_CHECK(result[1]);
    }
}

struct UniqueJob {
    static std::mutex m;
    static std::unordered_multiset<size_t> results;
    size_t job_id;
    UniqueJob(size_t job_id_in) : job_id(job_id_in){};
    UniqueJob() : job_id(0){};
    bool operator()()
    {
        std::lock_guard<std::mutex> l(m);
        results.insert(job_id);
        return true;
    }
    void swap(UniqueJob& x) { std::swap(x.job_id, job_id); };
};
std::mutex UniqueJob::m;
std::unordered_multiset<size_t> UniqueJob::results;
typedef CCheckQueue<UniqueJob,  true, false> Unique_Queue;
BOOST_AUTO_TEST_CASE(test_CheckQueue_UniqueJob)
{
    auto queue = std::shared_ptr<Unique_Queue>(new Unique_Queue);
    queue->init(100000, nScriptCheckThreads);

    size_t COUNT = 100000;
    size_t total = COUNT;
    {
        CCheckQueueControl<Unique_Queue> control(queue.get());
        while (total) {
            size_t r = GetRand(10);
            auto emplacer = control.get_emplacer();
            for (size_t k = 0; k < r && total; k++)
                emplacer( UniqueJob{--total});
        }
    }
    bool r = true;
    for (size_t i = 0; i < COUNT; ++i)
        r = r && UniqueJob::results.count(i) == 1;
    BOOST_REQUIRE(r);

}


struct MemoryJob {
    std::vector<std::array<unsigned char, 1000000> > mb_memory;
    bool operator()()
    {
        return true;
    }
    MemoryJob(){};
    MemoryJob(bool b)
    {
        if (b)
            mb_memory.reserve(200);
    };
    void swap(MemoryJob& x) { mb_memory.swap(x.mb_memory); };
};
typedef CCheckQueue<MemoryJob> Memory_Queue;
BOOST_AUTO_TEST_CASE(test_CheckQueue_Memory)
{
    auto queue = std::shared_ptr<Memory_Queue>(new Memory_Queue{});
    queue->init(10000, nScriptCheckThreads);

    for (size_t i = 9999; i < 9999; --i) {
        size_t total = i;
        {
            CCheckQueueControl<Memory_Queue> control(queue.get());
            while (total) {
                size_t r = GetRand(10);
                auto emplacer = control.get_emplacer();
                for (size_t k = 0; k < r && total; k++) {
                    total--;
                    emplacer(MemoryJob{total == 0});
                }
            }
        }
    }
}

struct FrozenCleanupJob {
    static std::atomic<bool> frozen;
    bool operator()()
    {
        return true;
    }
    FrozenCleanupJob() {}
    ~FrozenCleanupJob()
    {
        while (frozen)
            ;
    }
    void swap(FrozenCleanupJob& x) {};
};

std::atomic<bool> FrozenCleanupJob::frozen {false};
typedef CCheckQueue<FrozenCleanupJob> FrozenCleanup_Queue;
BOOST_AUTO_TEST_CASE(test_CheckQueue_FrozenCleanup)
{
    auto queue = std::shared_ptr<FrozenCleanup_Queue>(new FrozenCleanup_Queue{});
    queue->init(1000, nScriptCheckThreads);
    std::thread t0([&](){
        CCheckQueueControl<FrozenCleanup_Queue> control(queue.get());
        {
            control.get_emplacer()(FrozenCleanupJob{});
        }
        FrozenCleanupJob::frozen = true;
        BOOST_REQUIRE(control.Wait());
    });
    std::atomic<bool> made_control{false};
    std::thread t1([&](){
        CCheckQueueControl<FrozenCleanup_Queue> control(queue.get());
        made_control = true;
    });
    std::thread t2([&](){
        bool b = true;
        for (auto i = 0; i < 3000; ++i) {
            b = b && !made_control;
            MilliSleep(1);
        }
        FrozenCleanupJob::frozen = false;
        while (!made_control){}


    });
    t1.join();
    t2.join();
    t0.join();

}

BOOST_AUTO_TEST_SUITE_END()
