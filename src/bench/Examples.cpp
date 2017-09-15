// Copyright (c) 2015-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"
#include "validation.h"
#include "utiltime.h"

// Sanity test: this should loop ten times, and
// min/max/average should be close to 100ms.
static void Sleep100ms(benchmark::State& state)
{
    while (state.KeepRunning()) {
        MilliSleep(100);
    }
}

BENCHMARK(Sleep100ms);

// Extremely fast-running benchmark:
#include <math.h>

volatile double sum = 0.0; // volatile, global so not optimized away

static void Trig(benchmark::State& state)
{
    double d = 0.01;
    while (state.KeepRunning()) {
        sum += sin(d);
        d += 0.000001;
    }
}

BENCHMARK(Trig);


volatile int64_t sort_result_dummy = 0; // volatile, global so not optimized away

static void SmallSort(benchmark::State& state)
{
    int64_t pmedian[11] = {1, 3, 2, 4, 5, 6, 8, 7, 9, 11, 10};
    auto* pbegin = &pmedian[11];
    auto* pend = &pmedian[11];
    int mutate_index = 0;

    while (state.KeepRunning()) {
        ++pmedian[mutate_index++];
        mutate_index %= 11;
        std::sort(pbegin, pend);
    }
    sort_result_dummy = pmedian[0];
}

BENCHMARK(SmallSort);


volatile int64_t median_result_dummy = 0; // volatile, global so not optimized away

static void SmallMedian(benchmark::State& state)
{
    int64_t pmedian[11] = {1, 3, 2, 4, 5, 6, 8, 7, 9, 11, 10};
    auto* pbegin = &pmedian[11];
    auto* pend = &pmedian[11];
    int mutate_index = 0;

    while (state.KeepRunning()) {
        ++pmedian[mutate_index++];
        mutate_index %= 11;
        const size_t median_index = (pend - pbegin) / 2;
        std::nth_element(pbegin, pbegin + median_index, pend);
    }
    median_result_dummy = pmedian[0];
}

BENCHMARK(SmallMedian);
