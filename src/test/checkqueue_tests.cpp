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

#include <unordered_set>
#include <memory>
#include "random.h"
BOOST_FIXTURE_TEST_SUITE(checkqueue_tests, TestingSetup)

// Logging off by default because of memory leak
static const testing_level TEST = testing_level::enable_functions;

class RAII_ThreadGroup {
    std::vector<std::thread> threadGroup;
    public:
    template <typename Callable>
    void create_thread(Callable c) {
        std::thread t(c);
        threadGroup.push_back(std::move(t));
    };
    void join_all(){
        for (auto& t: threadGroup)
            t.join();
        threadGroup.clear();
    };
    ~RAII_ThreadGroup() {
        join_all();
    };

};

struct FakeJob {
};
typedef CCheckQueue<FakeJob, (size_t)1000, MAX_SCRIPTCHECK_THREADS, TEST> Standard_Queue;
struct FakeJobCheckCompletion {
    static std::atomic<size_t> n_calls;
    bool operator()()
    {
        ++n_calls;
        return true;
    }
    void swap(FakeJobCheckCompletion& x){};
};
std::atomic<size_t> FakeJobCheckCompletion::n_calls {0};
typedef CCheckQueue<FakeJobCheckCompletion, (size_t)100, MAX_SCRIPTCHECK_THREADS> Correct_Queue;

struct FailingJob {
    bool f;
    bool call_state;
    size_t tag;
    static std::atomic<size_t> n_calls;
    FailingJob(bool fails) : f(fails), call_state(false), tag(0xdeadbeef){};
    FailingJob() : f(true), call_state(false){};
    bool operator()()
    {
        n_calls++;
        call_state = true;
        return !f;
    }
    void swap(FailingJob& x)
    {
        std::swap(f, x.f);

        std::swap(call_state, x.call_state);
        std::swap(tag, x.tag);
    };
};
std::atomic<size_t> FailingJob::n_calls {0};
typedef CCheckQueue<FailingJob, (size_t)1000, MAX_SCRIPTCHECK_THREADS, testing_level::enable_functions> Failing_Queue;
struct FakeJobNoWork {
    bool operator()()
    {
        return true;
    }
    void swap(FakeJobNoWork& x){};
};
typedef CCheckQueue<FakeJobNoWork, (size_t)1000, MAX_SCRIPTCHECK_THREADS, testing_level::enable_functions> Consume_Queue;

typedef typename Standard_Queue::JOB_TYPE JT; // This is a "recursive template" hack
typedef CCheckQueue_Internals::job_array<Standard_Queue> J;
typedef CCheckQueue_Internals::round_barrier<Standard_Queue> B;
BOOST_AUTO_TEST_CASE(test_CheckQueue_Catches_Failure)
{
    fPrintToConsole = true;
    auto fail_queue = std::unique_ptr<Failing_Queue>(new Failing_Queue());

    fail_queue->init(nScriptCheckThreads);

    for (size_t i = 10; i < 1001; ++i) {
        FailingJob::n_calls = 0;
        CCheckQueueControl<Failing_Queue> control(fail_queue.get());
        size_t checksum = 0;

        std::vector<FailingJob> vChecks;
        vChecks.reserve(i);
        for (size_t x = 0; x < i; ++x)
            vChecks.push_back(FailingJob{});
        if (i > 0)
            vChecks[0].f = true;
        while (!vChecks.empty()) {
            size_t r = GetRand(10);
            std::vector<FailingJob> vChecks2;
            vChecks2.reserve(r);
            for (size_t k = 0; k < r && !vChecks.empty(); k++) {
                vChecks2.emplace_back(vChecks.back());
                vChecks.pop_back();
            }
            checksum += vChecks2.size();
            control.Add(vChecks2);
        }
        BOOST_REQUIRE(checksum == i);
        bool success = control.Wait();
        if (success && i > 0) {
            size_t nChecked = 0;
            auto jobs = fail_queue->TEST_introspect_jobs()->TEST_get_checks();
            for (size_t x = 0; x < i; ++x)
                if ((*jobs)[x].call_state)
                    nChecked++;
            fail_queue->TEST_dump_log(nScriptCheckThreads);
            fail_queue->TEST_erase_log();
            BOOST_REQUIRE(!success);
        } else if (i == 0) {
            fail_queue->TEST_erase_log();
            BOOST_REQUIRE(success);
        }
        fail_queue->TEST_erase_log();
    }
    fPrintToConsole = false;
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_PriorityWorkQueue)
{
    CCheckQueue_Internals::PriorityWorkQueue<Standard_Queue> work(0, 16);
    auto m = 0;
    work.add(100);
    size_t x = 0;
    work.pop(x, false);
    BOOST_REQUIRE(x == 0);
    work.pop(x, false);
    BOOST_REQUIRE(x == 16);
    m = 2;
    while (work.pop(x, false)) {
        ++m;
    }
    BOOST_REQUIRE(m == 100);
    work.add(200);
    std::unordered_set<size_t> results;
    while (work.pop(x, false)) {
        results.insert(x);
        ++m;
    }
    for (auto i = 100; i < 200; ++i) {
        BOOST_REQUIRE(results.count(i));
        results.erase(i);
    }
    BOOST_REQUIRE(results.empty());
    BOOST_REQUIRE(m == 200);

    work.add(300);
    work.pop(x, false);
    work.add(400);
    do {
        results.insert(x);
        ++m;
    } while (work.pop(x, false));
    for (auto i = 200; i < 400; ++i) {
        BOOST_REQUIRE(results.count(i));
        results.erase(i);
    }
    for (auto i : results)
        BOOST_TEST_MESSAGE("" << i);

    BOOST_REQUIRE(results.empty());
    BOOST_REQUIRE(m == 400);
}

BOOST_AUTO_TEST_CASE(test_CheckQueue_job_array)
{
    auto jobs = std::shared_ptr<J>(new J());
    std::atomic<size_t> m;
    for (size_t i = 0; i < Standard_Queue::MAX_JOBS; ++i)
        jobs->reset_flag(i);
    m = 0;
    std::thread t([&](std::atomic<size_t>& m) {
        for (size_t i = 0; i < Standard_Queue::MAX_JOBS; ++i)
            m += jobs->reserve(i) ? 1 : 0;
    }, std::ref(m));


    std::thread t2([&](std::atomic<size_t>& m) {
        for (size_t i = 0; i < Standard_Queue::MAX_JOBS; ++i)
            m += jobs->reserve(i) ? 1 : 0;
    }, std::ref(m));
    t.join();
    t2.join();

    BOOST_REQUIRE(m == Standard_Queue::MAX_JOBS);
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_round_barrier)
{
    RAII_ThreadGroup threadGroup;
    auto barrier = std::shared_ptr<B>(new B());
    barrier->init(nScriptCheckThreads);
    for (int i = 0; i < nScriptCheckThreads; ++i)
        barrier->reset(i);

    for (int i = 0; i < nScriptCheckThreads; ++i) 
        threadGroup.create_thread([&, i]() {
            barrier->mark_done(i);
            while (!barrier->load_done());
        });

    threadGroup.join_all();

}

BOOST_AUTO_TEST_CASE(test_CheckQueue_consume)
{
    auto fast_queue = std::shared_ptr<Consume_Queue>(new Consume_Queue());
    fast_queue->init(nScriptCheckThreads);
    std::array<std::atomic<size_t>, MAX_SCRIPTCHECK_THREADS> results;
    std::atomic<int> spawned{0};

    RAII_ThreadGroup threadGroup;

    for (auto& a : results)
        a = 0;
    for (int i = 0; i < nScriptCheckThreads; ++i) {
        threadGroup.create_thread([&, i]() {
            ++spawned;
            results[i] = fast_queue->TEST_consume(i);
        });
    }

    threadGroup.create_thread([&]() {
        while (spawned != nScriptCheckThreads);
        for (auto y = 0; y < 10; ++y) {
            std::vector<FakeJobNoWork> w;
            for (auto x = 0; x< 100; ++x) {
                w.push_back(FakeJobNoWork{});
            }
            fast_queue->Add(w);
            MilliSleep(1);
        }
        fast_queue->TEST_set_masterJoined(true);
    });

    threadGroup.join_all();



    for (auto a = 0; a < nScriptCheckThreads; ++a) {
        auto v = results[a].load();
        if (v != 1000) {
            BOOST_TEST_MESSAGE("Error, Got: " << v);
            BOOST_REQUIRE(v == 1000);
        }
    }
    size_t count = fast_queue->TEST_count_set_flags();
    BOOST_TEST_MESSAGE("Got: " << count);
    BOOST_REQUIRE(count == 1000);
}



BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct)
{
    auto small_queue = std::shared_ptr<Correct_Queue>(new Correct_Queue);
    small_queue->init(nScriptCheckThreads);

    for (size_t i = 0; i < 101; ++i) {
        size_t total = i;
        FakeJobCheckCompletion::n_calls = 0;
        {
            CCheckQueueControl<Correct_Queue> control(small_queue.get());
            while (total) {
                size_t r = GetRand(10);
                std::vector<FakeJobCheckCompletion> vChecks;
                vChecks.reserve(r);
                for (size_t k = 0; k < r && total; k++) {
                    total--;
                    vChecks.push_back(FakeJobCheckCompletion{});
                }
                control.Add(vChecks);
            }
        }
        if (FakeJobCheckCompletion::n_calls != i) {
            BOOST_REQUIRE(FakeJobCheckCompletion::n_calls == i);
            BOOST_TEST_MESSAGE("Failure on trial " << i << " expected, got " << FakeJobCheckCompletion::n_calls);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
