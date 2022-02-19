// Copyright (c) 2016-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <bench/bench.h>
#include <common/bloom.h>
#include <crypto/common.h>

static void RollingBloom(benchmark::Bench& bench)
{
    CRollingBloomFilter filter(120000, 0.000001);
    std::vector<unsigned char> data(32);
    unsigned char* dataPtr = data.data();
    uint32_t count = 0;
    bench.run([&] {
        count++;
        WriteLE32(dataPtr, count);
        filter.insert(data);

        WriteBE32(dataPtr, count);
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
