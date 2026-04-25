// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dbwrapper.h>
#include <logging.h>
#include <random.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <util/threadpool.h>

#include <leveldb/env.h>
#include <leveldb/helpers/memenv/memenv.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <latch>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <vector>

namespace {

/**
 * A leveldb::Env that wraps a memenv and captures scheduled background
 * work (compaction) instead of dispatching to a real thread. The fuzz
 * harness calls RunOne() or DrainWork() at fuzzer-chosen points to
 * execute it, giving deterministic control over when compaction
 * interleaves with foreground operations.
 *
 * Deadlock prevention: LevelDB's MakeRoomForWrite blocks on a condition
 * variable when the previous immutable memtable is still awaiting compaction,
 * or when the L0 file count hits kL0_StopWritesTrigger. Since both conditions
 * can only be resolved by the (deferred) background work, the harness drains
 * all pending work before every write to avoid a single-threaded deadlock.
 */
class DeterministicEnv final : public leveldb::EnvWrapper
{
    struct Work {
        void (*function)(void*);
        void* arg;
    };

    mutable Mutex m_mutex;
    std::deque<Work> m_queue GUARDED_BY(m_mutex);

public:
    explicit DeterministicEnv(leveldb::Env* base) : EnvWrapper(base) {}

    void Schedule(void (*function)(void* arg), void* arg) override EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        m_queue.push_back({function, arg});
    }

    bool HasPending() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        return !m_queue.empty();
    }

    /** Execute one pending background task. The task may schedule a
     *  successor which is left pending for a later call. */
    void RunOne() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        Work work{nullptr, nullptr};
        {
            LOCK(m_mutex);
            if (m_queue.empty()) return;
            work = m_queue.front();
            m_queue.pop_front();
        }
        work.function(work.arg);
    }

    /** Execute pending background tasks until none remain. */
    void DrainWork() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        while (HasPending()) {
            RunOne();
        }
    }
};

constexpr size_t MAX_VALUE_SIZE{4096};

/** Generate a deterministic value from key and length. The fuzz input
 *  controls only the key and a 16-bit length; the actual content is
 *  derived from a fixed pattern and doubled so that fuzz bytes aren't
 *  wasted on bulk data. */
std::vector<uint8_t> MakeValue(uint16_t key, uint16_t len)
{
    std::vector<uint8_t> v(static_cast<size_t>(len) * 2);
    std::iota(v.begin(), v.end(), static_cast<uint8_t>(key));
    return v;
}

/** Comparator matching LevelDB's lexicographic order on little-endian
 *  serialized uint16_t keys (low byte first, then high byte). */
struct SerializedU16Cmp {
    bool operator()(uint16_t a, uint16_t b) const
    {
        const auto swap{[](uint16_t v) -> uint16_t { return static_cast<uint16_t>((v << 8) | (v >> 8)); }};
        return swap(a) < swap(b);
    }
};

void initialize_dbwrapper()
{
    LogInstance().DisableLogging();
}

constexpr size_t NUM_WORKERS{16};
ThreadPool g_pool{"dbfuzz"};

void initialize_dbwrapper_concurrent_reads()
{
    LogInstance().DisableLogging();
    // Will not work with fork. Different threads will cause deadlock via latch.
    g_pool.Start(NUM_WORKERS);
}

} // namespace

FUZZ_TARGET(dbwrapper, .init = initialize_dbwrapper)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    const auto memenv{std::unique_ptr<leveldb::Env>{leveldb::NewMemEnv(leveldb::Env::Default())}};
    DeterministicEnv det_env{memenv.get()};

    const bool obfuscate{provider.ConsumeBool()};
    const size_t cache_bytes{provider.ConsumeIntegralInRange<size_t>(64 << 10, 1_MiB)};
    const size_t max_file_size{provider.ConsumeIntegralInRange<size_t>(1_MiB, 4_MiB)};

    CDBWrapper dbw{{
        .path = "",
        .cache_bytes = cache_bytes,
        .obfuscate = obfuscate,
        .testing_env = &det_env,
        .max_file_size = max_file_size,
    }};

    // Oracle: key → value length. Content is reconstructed via MakeValue().
    std::map<uint16_t, uint16_t, SerializedU16Cmp> oracle;

    LIMITED_WHILE(provider.ConsumeBool(), 10000)
    {
        CallOneOf(
            provider,
            // --- Mutations ---
            [&] {
                const uint16_t key{provider.ConsumeIntegral<uint16_t>()};
                const uint16_t len{provider.ConsumeIntegralInRange<uint16_t>(0, MAX_VALUE_SIZE)};
                const bool sync{provider.ConsumeBool()};
                det_env.DrainWork();
                dbw.Write(key, MakeValue(key, len), sync);
                oracle[key] = len;
            },
            [&] {
                const uint16_t key{provider.ConsumeIntegral<uint16_t>()};
                const bool sync{provider.ConsumeBool()};
                det_env.DrainWork();
                dbw.Erase(key, sync);
                oracle.erase(key);
            },
            [&] {
                CDBBatch batch{dbw};
                std::map<uint16_t, uint16_t> batch_writes;
                std::set<uint16_t> batch_erases;
                const auto fill{[&] {
                    LIMITED_WHILE(provider.ConsumeBool(), 20)
                    {
                        if (provider.ConsumeBool()) {
                            const uint16_t key{provider.ConsumeIntegral<uint16_t>()};
                            const uint16_t len{provider.ConsumeIntegralInRange<uint16_t>(0, MAX_VALUE_SIZE)};
                            batch.Write(key, MakeValue(key, len));
                            batch_writes[key] = len;
                            batch_erases.erase(key);
                        } else {
                            const uint16_t key{provider.ConsumeIntegral<uint16_t>()};
                            batch.Erase(key);
                            batch_erases.insert(key);
                            batch_writes.erase(key);
                        }
                    }
                }};
                fill();
                if (provider.ConsumeBool()) {
                    (void)batch.ApproximateSize();
                    batch.Clear();
                    batch_writes.clear();
                    batch_erases.clear();
                    fill();
                }
                const bool sync{provider.ConsumeBool()};
                det_env.DrainWork();
                dbw.WriteBatch(batch, sync);
                for (const auto& [k, v] : batch_writes) oracle[k] = v;
                for (const auto& k : batch_erases) oracle.erase(k);
            },
            // --- Reads ---
            [&] {
                const uint16_t key{provider.ConsumeIntegral<uint16_t>()};
                std::vector<uint8_t> value;
                const bool found{dbw.Read(key, value)};
                const auto it{oracle.find(key)};
                assert(found == (it != oracle.end()));
                if (found) assert(value == MakeValue(key, it->second));
            },
            [&] {
                const uint16_t key{provider.ConsumeIntegral<uint16_t>()};
                assert(dbw.Exists(key) == oracle.contains(key));
            },
            [&] {
                const std::unique_ptr<CDBIterator> it{dbw.NewIterator()};
                auto oracle_it{oracle.begin()};
                if (provider.ConsumeBool()) {
                    const uint16_t seek_key{provider.ConsumeIntegral<uint16_t>()};
                    it->Seek(seek_key);
                    oracle_it = oracle.lower_bound(seek_key);
                } else {
                    it->SeekToFirst();
                }
                LIMITED_WHILE(it->Valid() && oracle_it != oracle.end(), 256)
                {
                    uint16_t db_key;
                    if (!it->GetKey(db_key)) break;

                    if (db_key == oracle_it->first) {
                        std::vector<uint8_t> db_value;
                        assert(it->GetValue(db_value));
                        assert(db_value == MakeValue(db_key, oracle_it->second));
                        ++oracle_it;
                    } else {
                        assert(obfuscate);
                        std::string key_str;
                        assert(it->GetKey(key_str));
                        assert(key_str == std::string("\000obfuscate_key", 14));
                    }
                    it->Next();
                }
            },
            // --- Stats ---
            [&] {
                if (!obfuscate) {
                    assert(dbw.IsEmpty() == oracle.empty());
                } else {
                    assert(!dbw.IsEmpty());
                }
            },
            [&] {
                uint16_t k1{provider.ConsumeIntegral<uint16_t>()};
                uint16_t k2{provider.ConsumeIntegral<uint16_t>()};
                if (k1 > k2) std::swap(k1, k2);
                (void)dbw.EstimateSize(k1, k2);
            },
            [&] {
                (void)dbw.DynamicMemoryUsage();
            },
            // --- Compaction control ---
            [&] {
                det_env.RunOne();
            });
    }

    det_env.DrainWork();
}

FUZZ_TARGET(dbwrapper_concurrent_reads, .init = initialize_dbwrapper_concurrent_reads)
{
    SeedRandomStateForTest(SeedRand::ZEROS);

    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const size_t num_threads{provider.ConsumeIntegralInRange<uint8_t>(2, NUM_WORKERS)};

    const auto memenv{std::unique_ptr<leveldb::Env>{leveldb::NewMemEnv(leveldb::Env::Default())}};
    DeterministicEnv det_env{memenv.get()};

    CDBWrapper db{DBParams{
        .path = "",
        .cache_bytes = 256 * 1024,
        .obfuscate = true,
        .testing_env = &det_env,
        .max_file_size = 256 * 1024,
    }};

    // Seed the DB. Drain work between each to prevent deadlock.
    // With write_buffer_size=64KB, ~860 entries fill a memtable,
    // 4 flushes trigger L0→L1 compaction.
    FastRandomContext rng{/*fDeterministic=*/true};
    const size_t num_entries{provider.ConsumeIntegralInRange<size_t>(100, 5'000)};
    std::vector<std::string> keys;
    keys.reserve(num_entries);
    constexpr size_t SEED_BATCH_SIZE{400};
    for (size_t start{0}; start < num_entries; start += SEED_BATCH_SIZE) {
        CDBBatch batch{db};
        const size_t end{std::min(start + SEED_BATCH_SIZE, num_entries)};
        for (size_t i{start}; i < end; ++i) {
            auto k{rng.rand256().ToString()};
            const auto v{rng.rand256().ToString()};
            batch.Write(k, v);
            keys.push_back(std::move(k));
        }
        det_env.DrainWork();
        db.WriteBatch(batch, /*fSync=*/true);
    }

    while (det_env.HasPending() && provider.ConsumeBool()) {
        det_env.RunOne();
    }

    // Build query list from seeded keys and random strings.
    std::vector<std::string> queries;
    LIMITED_WHILE(provider.ConsumeBool(), 2000) {
        if (provider.ConsumeBool()) {
            queries.push_back(keys[provider.ConsumeIntegralInRange<size_t>(0, keys.size() - 1)]);
        } else {
            queries.push_back(provider.ConsumeRandomLengthString(64));
        }
    }

    // Baseline read on a single thread
    using Results = std::vector<std::optional<std::string>>;
    const auto num_queries{queries.size()};
    Results baseline(num_queries);
    for (const auto i : std::views::iota(size_t{0}, num_queries)) {
        std::string v;
        if (db.Read(queries[i], v)) baseline[i].emplace(std::move(v));
    }

    // Workers + main thread synchronize on the latch.
    std::latch start_latch{static_cast<ptrdiff_t>(num_threads + 1)};

    std::vector<std::function<Results()>> tasks(num_threads, [&]() -> Results {
        Results results(num_queries);
        start_latch.arrive_and_wait();
        for (const auto i : std::views::iota(size_t{0}, num_queries)) {
            if (std::string v; db.Read(queries[i], v)) results[i].emplace(std::move(v));
        }
        return results;
    });

    auto futures{*Assert(g_pool.Submit(std::move(tasks)))};

    // Release the latch and immediately run compaction on the main
    // thread, racing against the worker reads.
    start_latch.arrive_and_wait();
    det_env.DrainWork();

    for (auto& fut : futures) assert(fut.get() == baseline);

    det_env.DrainWork();
}
