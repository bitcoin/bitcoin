// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <bench/bench.h>
#include <common/bloom.h>

static void RollingBloom(benchmark::Bench& bench)
{
    CRollingBloomFilter filter(120000, 0.000001);
    std::vector<unsigned char> data(32);
    uint32_t count = 0;
    bench.run([&] {
        count++;
        data[0] = count & 0xFF;
        data[1] = (count >> 8) & 0xFF;
        data[2] = (count >> 16) & 0xFF;
        data[3] = (count >> 24) & 0xFF;
        filter.insert(data);

        data[0] = (count >> 24) & 0xFF;
        data[1] = (count >> 16) & 0xFF;
        data[2] = (count >> 8) & 0xFF;
        data[3] = count & 0xFF;
        filter.contains(data);
    });
}

static void RollingBloomReset(benchmark::Bench& bench)
{
    CRollingBloomFilter filter(120000, 0.000001);
    bench.run([&] {
        filter.reset();
    });
}

BENCHMARK(RollingBloom);
BENCHMARK(RollingBloomReset);
