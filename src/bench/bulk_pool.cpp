// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <coins.h>
#include <support/allocators/bulk_pool.h>

#include <unordered_map>

template <typename Map>
void BenchFillClearMap(benchmark::State& state, Map& map)
{
    COutPoint p;
    while (state.KeepRunning()) {
        // modify hash. We don't need to check if big/little endian, just modify.
        ++*reinterpret_cast<uint64_t*>(p.hash.begin());
        map[p];
        if (map.size() > 50000) {
            map.clear();
        }
    }
}

static void BulkPool_StdUnorderedMap(benchmark::State& state)
{
    using Map = std::unordered_map<COutPoint, CCoinsCacheEntry, SaltedOutpointHasher>;
    Map map;
    BenchFillClearMap(state, map);
}

static void BulkPool_StdMap(benchmark::State& state)
{
    using Map = std::map<COutPoint, CCoinsCacheEntry>;
    Map map;
    BenchFillClearMap(state, map);
}

static void BulkPool_StdUnorderedMap_Enabled(benchmark::State& state)
{
    using Map = std::unordered_map<COutPoint, CCoinsCacheEntry, SaltedOutpointHasher, std::equal_to<COutPoint>, bulk_pool::Allocator<std::pair<const COutPoint, CCoinsCacheEntry>>>;
    bulk_pool::Pool pool;
    Map map(0, Map::hasher{}, Map::key_equal{}, Map::allocator_type{&pool});
    BenchFillClearMap(state, map);
}

static void BulkPool_StdMap_Enabled(benchmark::State& state)
{
    using Map = std::map<COutPoint, CCoinsCacheEntry, std::less<COutPoint>, bulk_pool::Allocator<std::pair<const COutPoint, CCoinsCacheEntry>>>;
    bulk_pool::Pool pool;
    Map map(Map::key_compare{}, Map::allocator_type{&pool});
    BenchFillClearMap(state, map);
}

BENCHMARK(BulkPool_StdUnorderedMap, 5000000);
BENCHMARK(BulkPool_StdMap, 5000000);
BENCHMARK(BulkPool_StdUnorderedMap_Enabled, 5000000);
BENCHMARK(BulkPool_StdMap_Enabled, 5000000);
