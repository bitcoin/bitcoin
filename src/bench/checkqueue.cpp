
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
    typedef CCheckQueue<FakeJobNoWork, (size_t)100000, MAX_SCRIPTCHECK_THREADS> T;
    auto fast_queue = std::unique_ptr<T>(new T());
    fast_queue->init(GetNumCores());
    while (state.KeepRunning()) {
        size_t total = 0;
        {
            CCheckQueueControl<T> control(fast_queue.get());
            for (size_t j = 0; j < 101; ++j) {
                size_t r = 30;
                total += r;
                auto inserter = control.get_inserter();
                for (size_t k = 0; k < r; ++k)
                    new (inserter()) FakeJobNoWork{};
            }
        }
    }
}
BENCHMARK(CCheckQueueSpeed);
