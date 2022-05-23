// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <timedata.h>

#include <bench/bench.h>

// 256 element max
// 1000 iteration
static void ComputeMedian256_1000(benchmark::Bench& bench)
{
    //Precompute random numbers
    std::vector<int> rand_num;
    rand_num.reserve(1000);
    for (size_t i = 0; i < 1000; ++i) {
        rand_num.emplace_back(rand());
    }

    bench.run([&rand_num] {
        CMedianFilter<int, 256> median_filter(15);

        for (size_t i = 0; i < 1000; ++i) {
            median_filter.input(rand_num[i]);
        }
    });
}

// 20 element max
// 1000 iteration
static void ComputeMedian20_1000(benchmark::Bench& bench)
{
    //Precompute random numbers
    std::vector<int> rand_num;
    rand_num.reserve(1000);
    for (size_t i = 0; i < 1000; ++i) {
        rand_num.emplace_back(rand());
    }

    bench.run([&rand_num] {
        CMedianFilter<int, 20> median_filter(15);

        for (size_t i = 0; i < 1000; ++i) {
            median_filter.input(rand_num[i]);
        }
    });
}

// 10 element max
// 1000 iteration
static void ComputeMedian10_1000(benchmark::Bench& bench)
{
    //Precompute random numbers
    std::vector<int> rand_num;
    rand_num.reserve(1000);
    for (size_t i = 0; i < 1000; ++i) {
        rand_num.emplace_back(rand());
    }

    bench.run([&rand_num] {
        CMedianFilter<int, 10> median_filter(15);

        for (size_t i = 0; i < 1000; ++i) {
            median_filter.input(rand_num[i]);
        }
    });
}

BENCHMARK(ComputeMedian256_1000);
BENCHMARK(ComputeMedian20_1000);
BENCHMARK(ComputeMedian10_1000);

