// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"
#include "util.h"
#include "validation.h"
#include "checkqueue.h"
#include "prevector.h"
#include <vector>
#include <boost/thread/thread.hpp>
#include "random.h"


// This Benchmark tests the CheckQueue with the lightest
// weight Checks, so it should make any lock contention
// particularly visible
static void CCheckQueueSpeed(benchmark::State& state)
{
    struct FakeJobNoWork {
        bool operator()()
        {
            return true;
        }
        void swap(FakeJobNoWork& x){};
    };
    CCheckQueue<FakeJobNoWork> queue {128};
    boost::thread_group tg;
    for (auto x = 0; x < std::max(2, GetNumCores()); ++x) {
       tg.create_thread([&]{queue.Thread();});
    }
    while (state.KeepRunning()) {
        CCheckQueueControl<FakeJobNoWork> control(&queue);
        // We can make vChecks out of the loop because calling Add doesn't
        // change the size of the vector.
        std::vector<FakeJobNoWork> vChecks;
        vChecks.resize(30);

        // We call Add a number of times to simulate the behavior of adding
        // a block of transactions at once.
        for (size_t j = 0; j < 101; ++j) {
            control.Add(vChecks);
        }
        // control waits for completion by RAII, but
        // it is done explicitly here for clarity
        control.Wait();
    }
    tg.interrupt_all();
    tg.join_all();
}

// This Benchmark tests the CheckQueue with a slightly realistic workload,
// where checks all contain a prevector that is indirect 50% of the time
// and there is a little bit of work done between calls to Add.
static void CCheckQueueSpeedPrevectorJob(benchmark::State& state)
{
    struct PrevectorJob {
        prevector<28, uint8_t> p;
        PrevectorJob(){
        }
        PrevectorJob(FastRandomContext& insecure_rand){
            p.resize(insecure_rand.rand32() % 56);
        }
        bool operator()()
        {
            return true;
        }
        void swap(PrevectorJob& x){p.swap(x.p);};
    };
    CCheckQueue<PrevectorJob> queue {128};
    boost::thread_group tg;
    for (auto x = 0; x < std::max(2, GetNumCores()); ++x) {
       tg.create_thread([&]{queue.Thread();});
    }
    while (state.KeepRunning()) {
        // Make insecure_rand here so that each iteration is identical.
        FastRandomContext insecure_rand(true);
        CCheckQueueControl<PrevectorJob> control(&queue);
        for (size_t j = 0; j < 101; ++j) {
            std::vector<PrevectorJob> vChecks;
            vChecks.reserve(30);
            for (auto x = 0; x < 30; ++x)
                vChecks.emplace_back(insecure_rand);
            control.Add(vChecks);
        }
        // control waits for completion by RAII, but
        // it is done explicitly here for clarity
        control.Wait();
    }
    tg.interrupt_all();
    tg.join_all();
}
BENCHMARK(CCheckQueueSpeed);
BENCHMARK(CCheckQueueSpeedPrevectorJob);
