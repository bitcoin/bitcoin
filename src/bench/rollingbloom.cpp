// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

#include <bench/bench.h>
#include <bloom.h>

static void RollingBloom(benchmark::State& state)
{
    CRollingBloomFilter filter(120000, 0.000001);
    std::vector<unsigned char> data(32);
    uint32_t count = 0;
    while (state.KeepRunning()) {
        count++;
        data[0] = count;
        data[1] = count >> 8;
        data[2] = count >> 16;
        data[3] = count >> 24;
        filter.insert(data);

        data[0] = count >> 24;
        data[1] = count >> 16;
        data[2] = count >> 8;
        data[3] = count;
        filter.contains(data);
    }
}

static void RollingBloomReset(benchmark::State& state)
{
    CRollingBloomFilter filter(120000, 0.000001);
    while (state.KeepRunning()) {
        filter.reset();
    }
}

BENCHMARK(RollingBloom, 1500 * 1000);
BENCHMARK(RollingBloomReset, 20000);
