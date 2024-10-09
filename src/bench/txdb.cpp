// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <coins.h>
#include <common/args.h>
#include <consensus/amount.h>
#include <dbwrapper.h>
#include <kernel/mempool_options.h>
#include <key.h>
#include <node/caches.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <test/util/setup_common.h>
#include <test/util/transaction_utils.h>
#include <test/util/coins.h>
#include <txdb.h>
#include <util/fs.h>

#include <cassert>

class CCoinsViewDBBench
{
public:
    // This is hard-coded in `CompleteChainstateInitialization()`
    static constexpr double init_cache_fraction = 0.2;
    fs::path m_db_path;
    size_t m_cache_size;
    CCoinsViewDB m_db;

    CCoinsViewDBBench(fs::path path, size_t cache_size_bytes, bool obfuscate = true) :
        m_db_path(path),
        m_cache_size(cache_size_bytes),
        m_db(
            DBParams{
                .path = path,
                .cache_bytes = m_cache_size,
                .memory_only = false,
                .wipe_data = true,
                .obfuscate = obfuscate,
                .options = DBOptions{}},
            CoinsViewOptions{}
        ){}
};

// Calculate large cache threshold, accounting for the mempool allocation during IBD
static constexpr size_t large_threshold(size_t cache_size_bytes)
{
    return (DEFAULT_MAX_MEMPOOL_SIZE_MB * 1'000'000 + cache_size_bytes);
}

// Tests a CCoinsViewCache being filled with random coins, CCoinsViewDB being flushed to by a CCoinsViewCache.
static void CCoinsViewDBFlush(benchmark::Bench& bench)
{
    auto test_setup = MakeNoLogFileContext<TestingSetup>();
    FastRandomContext random{/*fDeterministic=*/true};

    auto db_path = test_setup->m_path_root / "test_coinsdb";
    // This comes from `CompleteChainstateInitialization()`.
    size_t cache_size_bytes = node::CalculateCacheSizes(ArgsManager{},/*n_indexes=*/0).coins_db * CCoinsViewDBBench::init_cache_fraction;

    // Benchmark.
    bench.batch(large_threshold(cache_size_bytes)).unit("byte").run([&] {
        auto bench_db = CCoinsViewDBBench(db_path, cache_size_bytes);
        CCoinsViewCache coins(&bench_db.m_db);
        coins.SetBestBlock(/*hashBlock=*/random.rand256());

        // Add coins until the large threshold
        while (coins.DynamicMemoryUsage() < large_threshold(cache_size_bytes)) {
            AddTestCoin(random, coins);
        }
        coins.Flush();
    });
}

BENCHMARK(CCoinsViewDBFlush, benchmark::PriorityLevel::HIGH);
