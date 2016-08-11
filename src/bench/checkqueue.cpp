
// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"
#include "util.h"
#include "main.h"
#include <vector>
#include "checkqueue.h"

static void CCheckQueueSpeed(benchmark::State& state)
{
    struct FakeJobNoWork {
        bool operator()()
        {
            return true;
        }
        void swap(FakeJobNoWork& x){};
    };
    static CCheckQueue<FakeJobNoWork, (size_t)100000, MAX_SCRIPTCHECK_THREADS> fast_queue;
    fast_queue.init(GetNumCores());
    std::vector<FakeJobNoWork> vChecks;
    vChecks.reserve(100);
    while (state.KeepRunning()) {
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
}
BENCHMARK(CCheckQueueSpeed);
