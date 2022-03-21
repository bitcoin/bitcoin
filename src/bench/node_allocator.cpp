// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <support/allocators/node_allocator/factory.h>

#include <cstring>
#include <unordered_map>

template <typename Map>
void BenchFillClearMap(benchmark::Bench& bench, Map& map)
{
    size_t batch_size = 5000;

    // make sure each iteration of the benchmark contains exactly 5000 inserts and one clear.
    // do this at least 10 times so we get reasonable accurate results

    bench.batch(batch_size).minEpochIterations(10).run([&] {
        uint64_t key = 0;
        for (size_t i = 0; i < batch_size; ++i) {
            // add a random number for better spread in the map
            key += 0x967f29d1;
            map[key];
        }
        map.clear();
    });
}

static void NodeAllocator_StdUnorderedMap(benchmark::Bench& bench)
{
    auto map = std::unordered_map<uint64_t, uint64_t>();
    BenchFillClearMap(bench, map);
}

static void NodeAllocator_StdUnorderedMapWithNodeAllocator(benchmark::Bench& bench)
{
    using Factory = node_allocator::Factory<std::unordered_map<uint64_t, uint64_t>>;
    Factory::MemoryResourceType memory_resource{};
    auto map = Factory::CreateContainer(&memory_resource);
    BenchFillClearMap(bench, map);
}

BENCHMARK(NodeAllocator_StdUnorderedMap);
BENCHMARK(NodeAllocator_StdUnorderedMapWithNodeAllocator);
