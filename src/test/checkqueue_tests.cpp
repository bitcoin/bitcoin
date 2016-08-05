// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"
#include "utiltime.h"

#include "test/test_bitcoin.h"
#include "checkqueue.h"

#include <boost/test/unit_test.hpp>
#include <atomic>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <unordered_set>

#include "random.h"
BOOST_FIXTURE_TEST_SUITE(checkqueue_tests, BasicTestingSetup)

// Logging off by default because of memory leak
static const testing_level TEST = testing_level::enable_functions;

struct FakeJob {
};

typedef CCheckQueue<FakeJob, (size_t)100000, 16, TEST> big_queue;
typedef CCheckQueue<FakeJob, (size_t)2000, 16, TEST> medium_queue;

struct big_queue_proto {
    typedef big_queue::JOB_TYPE JOB_TYPE;
    static const size_t MAX_JOBS = big_queue::MAX_JOBS;
    static const size_t MAX_WORKERS = big_queue::MAX_WORKERS;
};

BOOST_AUTO_TEST_CASE(test_CheckQueue_Catches_Failure)
{
    fPrintToConsole = true;
    static std::atomic<size_t> n_calls;
    struct FailingJob {
        bool f;
        bool call_state;
        size_t tag;
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
    static CCheckQueue<FailingJob, (size_t)1000, 16, testing_level::enable_functions> fail_queue;
    size_t nThreads = 8;
    fail_queue.init(nThreads);

    for (size_t i = 10; i < 1001; ++i) {
        n_calls = 0;
        CCheckQueueControl<decltype(fail_queue)> control(&fail_queue);
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
            auto jobs = fail_queue.TEST_introspect_jobs()->TEST_get_checks();
            for (size_t x = 0; x < i; ++x)
                if ((*jobs)[x].call_state)
                    nChecked++;
            fail_queue.TEST_dump_log(nThreads);
            fail_queue.TEST_erase_log();
            BOOST_MESSAGE("Failed to see failure on round " << i << " only counted " << nChecked << " flags (by job_array).");
            BOOST_MESSAGE("Failed to see failure on round " << i << " only counted " << n_calls << " flags (by atomic).");
            BOOST_REQUIRE(!success);
        } else if (i == 0) {
            fail_queue.TEST_erase_log();
            BOOST_REQUIRE(success);
        }
        fail_queue.TEST_erase_log();
    }
    fPrintToConsole = false;
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_PriorityWorkQueue)
{
    CCheckQueue_Internals::PriorityWorkQueue<medium_queue> work(0, 16);
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
    boost::thread_group threadGroup;
    static CCheckQueue_Internals::job_array<big_queue_proto> jobs;
    static std::atomic<size_t> m;
    for (size_t i = 0; i < big_queue::MAX_JOBS; ++i)
        jobs.reset_flag(i);
    m = 0;
    threadGroup.create_thread([]() {
            for (size_t i = 0; i < big_queue::MAX_JOBS; ++i)
            m += jobs.reserve(i) ? 1 : 0;
    });

    threadGroup.create_thread([]() {
            for (size_t i = 0; i < big_queue::MAX_JOBS; ++i)
            m += jobs.reserve(i) ? 1 : 0;
    });
    threadGroup.join_all();

    BOOST_REQUIRE(m == big_queue::MAX_JOBS);
}
BOOST_AUTO_TEST_CASE(test_CheckQueue_round_barrier)
{
    boost::thread_group threadGroup;
    static CCheckQueue_Internals::round_barrier<big_queue> barrier;
    size_t nThreads = 8;
    barrier.init(nThreads);
    for (size_t i = 0; i < nThreads; ++i)
        barrier.reset(i);
    for (size_t i = 0; i < nThreads; ++i)
        threadGroup.create_thread([=]() {
            barrier.mark_done(i);
            while (!barrier.load_done());
        });

    threadGroup.join_all();
}

BOOST_AUTO_TEST_CASE(test_CheckQueue_consume)
{
    struct FakeJobNoWork {
        bool operator()()
        {
            return true;
        }
        void swap(FakeJobNoWork& x){};
    };
    static CCheckQueue<FakeJobNoWork, (size_t)100000, 10, testing_level::enable_functions> fast_queue{};
    size_t nThreads = 8;
    fast_queue.init(nThreads);
    std::array<std::atomic<size_t>, 8> results;
    std::atomic<size_t> spawned{0};

    boost::thread_group threadGroup;

    for (auto& a : results)
        a = 0;
    for (size_t i = 0; i < nThreads; ++i)
        threadGroup.create_thread([&, i]() {
            ++spawned;
            results[i] = fast_queue.TEST_consume(i);
        });

    threadGroup.create_thread([&]() {
        while (spawned != nThreads);
        for (auto y = 0; y < 10; ++y) {
            std::vector<FakeJobNoWork> w;
            for (auto x = 0; x< 100; ++x) {
                w.push_back(FakeJobNoWork{});
            }
            fast_queue.Add(w);
            MilliSleep(1);
        }
        fast_queue.TEST_set_masterJoined(true);
    });

    threadGroup.join_all();


    for (auto& a : results) {
        if (a != 1000) {
            BOOST_TEST_MESSAGE("Error, Got: " << a);
            BOOST_REQUIRE(a == 1000);
        }
    }
    size_t count = fast_queue.TEST_count_set_flags();
    BOOST_TEST_MESSAGE("Got: " << count);
    BOOST_REQUIRE(count == 1000);
}


BOOST_AUTO_TEST_CASE(test_CheckQueue_Performance)
{
    struct FakeJobNoWork {
        bool operator()()
        {
            return true;
        }
        void swap(FakeJobNoWork& x){};
    };
    static CCheckQueue<FakeJobNoWork, (size_t)100000, 16> fast_queue;
    size_t nThreads = 8;
    fast_queue.init(nThreads);

    std::vector<FakeJobNoWork> vChecks;
    vChecks.reserve(100);
    auto start_time = GetTimeMicros();
    size_t ROUNDS = 10000;
    for (size_t i = 0; i < ROUNDS; ++i) {
        size_t total = 0;
        {
            CCheckQueueControl<decltype(fast_queue)> control(&fast_queue);
            for (size_t j = 0; j < 101; ++j) {
                size_t r = 30;
                total += r;
                vChecks.clear();
                for (size_t k = 0; k < r; ++k)
                    vChecks.push_back(FakeJobNoWork{});
                control.Add(vChecks);
            }
        }
    }
    auto end_time = GetTimeMicros();
    BOOST_TEST_MESSAGE("Perf Test took " << end_time - start_time << " microseconds for " << ROUNDS << " rounds, " << (ROUNDS * 1000000.0) / (end_time - start_time) << "rps");
}

BOOST_AUTO_TEST_CASE(test_CheckQueue_Correct)
{
    static std::atomic<size_t> n_calls;
    struct FakeJobCheckCompletion {
        bool operator()()
        {
            ++n_calls;
            return true;
        }
        void swap(FakeJobCheckCompletion& x){};
    };
    static CCheckQueue<FakeJobCheckCompletion, (size_t)100, 16> small_queue;
    size_t nThreads = 8;
    small_queue.init(nThreads);

    for (size_t i = 0; i < 101; ++i) {
        size_t total = i;
        n_calls = 0;
        {
            CCheckQueueControl<decltype(small_queue)> control(&small_queue);
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
        if (n_calls != i) {
            BOOST_REQUIRE(n_calls == i);
            BOOST_TEST_MESSAGE("Failure on trial " << i << " expected, got " << n_calls);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
