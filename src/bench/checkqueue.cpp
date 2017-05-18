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
static const int MIN_CORES = 2;
static const size_t BATCHES = 101;
static const size_t BATCH_SIZE = 30;
static const int PREVECTOR_SIZE = 28;
static const int QUEUE_BATCH_SIZE = 128;
static const int QUEUE_SIZE = BATCHES*BATCH_SIZE*5;
static void CCheckQueueSpeed(benchmark::State& state)
{
    struct FakeJobNoWork {
        bool operator()()
        {
            return true;
        }
        void swap(FakeJobNoWork& x){};
    };
    CCheckQueue<FakeJobNoWork> queue {QUEUE_BATCH_SIZE};
    boost::thread_group tg;
    for (auto x = 0; x < std::max(MIN_CORES, GetNumCores()); ++x) {
       tg.create_thread([&]{queue.Thread();});
    }
    while (state.KeepRunning()) {
        CCheckQueueControl<FakeJobNoWork> control(&queue, QUEUE_SIZE);

        // We call Add a number of times to simulate the behavior of adding
        // a block of transactions at once.

        for (size_t x = 0; x < BATCHES; ++x) {
            for (size_t y = 0; y < BATCH_SIZE; ++y) {
                control.Add();
            }
            control.Flush(BATCH_SIZE);
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
        prevector<PREVECTOR_SIZE, uint8_t> p;
        PrevectorJob(){
        }
        PrevectorJob(FastRandomContext& insecure_rand){
            p.resize(insecure_rand.randrange(PREVECTOR_SIZE*2));
        }
        bool operator()()
        {
            return true;
        }
        void swap(PrevectorJob& x){p.swap(x.p);};
    };
    CCheckQueue<PrevectorJob> queue {QUEUE_BATCH_SIZE};
    boost::thread_group tg;
    for (auto x = 0; x < std::max(MIN_CORES, GetNumCores()); ++x) {
       tg.create_thread([&]{queue.Thread();});
    }
    while (state.KeepRunning()) {
        // Make insecure_rand here so that each iteration is identical.
        FastRandomContext insecure_rand(true);
        CCheckQueueControl<PrevectorJob> control(&queue, QUEUE_SIZE);
        for (size_t x = 0; x < BATCHES; ++x) {
            for (size_t y = 0; y < BATCH_SIZE; ++y) {
                control.Add(insecure_rand);
            }
            control.Flush(BATCH_SIZE);
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
