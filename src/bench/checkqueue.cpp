// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <checkqueue.h>
#include <common/system.h>
#include <key.h>
#include <prevector.h>
#include <pubkey.h>
#include <random.h>

#include <vector>

static const size_t BATCHES = 101;
static const size_t BATCH_SIZE = 30;
static const int PREVECTOR_SIZE = 28;
static const unsigned int QUEUE_BATCH_SIZE = 128;

// This Benchmark tests the CheckQueue with a slightly realistic workload,
// where checks all contain a prevector that is indirect 50% of the time
// and there is a little bit of work done between calls to Add.
static void CCheckQueueSpeedPrevectorJob(benchmark::Bench& bench)
{
    // We shouldn't ever be running with the checkqueue on a single core machine.
    if (GetNumCores() <= 1) return;

    ECC_Context ecc_context{};

    struct PrevectorJob {
        prevector<PREVECTOR_SIZE, uint8_t> p;
        explicit PrevectorJob(FastRandomContext& insecure_rand){
            p.resize(insecure_rand.randrange(PREVECTOR_SIZE*2));
        }
        bool operator()()
        {
            return true;
        }
    };

    // The main thread should be counted to prevent thread oversubscription, and
    // to decrease the variance of benchmark results.
    int worker_threads_num{GetNumCores() - 1};
    CCheckQueue<PrevectorJob> queue{QUEUE_BATCH_SIZE, worker_threads_num};

    // create all the data once, then submit copies in the benchmark.
    FastRandomContext insecure_rand(true);
    std::vector<std::vector<PrevectorJob>> vBatches(BATCHES);
    for (auto& vChecks : vBatches) {
        vChecks.reserve(BATCH_SIZE);
        for (size_t x = 0; x < BATCH_SIZE; ++x)
            vChecks.emplace_back(insecure_rand);
    }

    bench.minEpochIterations(10).batch(BATCH_SIZE * BATCHES).unit("job").run([&] {
        // Make insecure_rand here so that each iteration is identical.
        CCheckQueueControl<PrevectorJob> control(&queue);
        for (auto vChecks : vBatches) {
            control.Add(std::move(vChecks));
        }
        // control waits for completion by RAII, but
        // it is done explicitly here for clarity
        control.Wait();
    });
}
BENCHMARK(CCheckQueueSpeedPrevectorJob, benchmark::PriorityLevel::HIGH);
