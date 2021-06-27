// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <util/system.h>
#include <validation.h>
#include <checkqueue.h>
#include <prevector.h>
#include <vector>
#include <random.h>


static const int MIN_CORES = 2;
static const size_t BATCHES = 101;
static const size_t BATCH_SIZE = 30;
static const int PREVECTOR_SIZE = 28;
static const unsigned int QUEUE_BATCH_SIZE = 128;

// This Benchmark tests the CheckQueue with a slightly realistic workload,
// where checks all contain a prevector that is indirect 50% of the time
// and there is a little bit of work done between calls to Add.
static void CCheckQueueSpeedPrevectorJob(benchmark::State& state)
{
    struct PrevectorJob {
        prevector<PREVECTOR_SIZE, uint8_t> p;
        PrevectorJob(){
        }
        explicit PrevectorJob(FastRandomContext& insecure_rand){
            p.resize(insecure_rand.randrange(PREVECTOR_SIZE*2));
        }
        bool operator()()
        {
            return true;
        }
        void swap(PrevectorJob& x){p.swap(x.p);};
    };
    CCheckQueue<PrevectorJob> queue {QUEUE_BATCH_SIZE};

    // The main thread should be counted to prevent thread oversubscription, and
    // to decrease the variance of benchmark results.
    queue.StartWorkerThreads(GetNumCores() - 1);

    while (state.KeepRunning()) {
        // Make insecure_rand here so that each iteration is identical.
        FastRandomContext insecure_rand(true);
        CCheckQueueControl<PrevectorJob> control(&queue);
        std::vector<std::vector<PrevectorJob>> vBatches(BATCHES);
        for (auto& vChecks : vBatches) {
            vChecks.reserve(BATCH_SIZE);
            for (size_t x = 0; x < BATCH_SIZE; ++x)
                vChecks.emplace_back(insecure_rand);
            control.Add(vChecks);
        }
        // control waits for completion by RAII, but
        // it is done explicitly here for clarity
        control.Wait();
    }
    queue.StopWorkerThreads();
}
BENCHMARK(CCheckQueueSpeedPrevectorJob, 1400);
