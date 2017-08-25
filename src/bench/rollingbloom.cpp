// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

#include <bench/bench.h>
#include <bloom.h>
#include <utiltime.h>

static void RollingBloom(benchmark::State& state)
{
    CRollingBloomFilter filter(120000, 0.000001);
    std::vector<unsigned char> data(32);
    uint32_t count = 0;
    uint32_t nEntriesPerGeneration = (120000 + 1) / 2;
    uint32_t countnow = 0;
    uint64_t match = 0;
    while (state.KeepRunning()) {
        count++;
        data[0] = count;
        data[1] = count >> 8;
        data[2] = count >> 16;
        data[3] = count >> 24;
        if (countnow == nEntriesPerGeneration) {
            int64_t b = GetTimeMicros();
            filter.insert(data);
            int64_t e = GetTimeMicros();
            std::cout << "RollingBloom-refresh,1," << (e-b)*0.000001 << "," << (e-b)*0.000001 << "," << (e-b)*0.000001 << "\n";
            countnow = 0;
        } else {
            filter.insert(data);
        }
        countnow++;
        data[0] = count >> 24;
        data[1] = count >> 16;
        data[2] = count >> 8;
        data[3] = count;
        match += filter.contains(data);
    }
}

BENCHMARK(RollingBloom);
