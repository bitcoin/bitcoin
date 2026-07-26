// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dbwrapper.h>
#include <compat/byteswap.h>
#include <random.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
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
#include <set>
#include <span>
#include <string>
#include <tuple>
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
 * Callers must also DrainWork() before destroying the CDBWrapper, since the
 * leveldb destructor waits for any pending background work to complete.
 *
 * The same reasoning rules out exercising DBOptions::force_compact under
 * this env, because CompactRange(nullptr, nullptr) blocks waiting for
 * background work that is queued on the (blocked) foreground thread. The
 * sibling dbwrapper_threaded target covers that path.
 */
class DeterministicEnv final : public leveldb::EnvWrapper
{
    using WorkFunction = void (*)(void*);

    struct Work {
        WorkFunction function;
        void* arg;
    };

    Mutex m_mutex;
    std::deque<Work> m_queue GUARDED_BY(m_mutex);

public:
    explicit DeterministicEnv(leveldb::Env* base) : EnvWrapper(base) {}

    void Schedule(WorkFunction function, void* arg) override EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        m_queue.push_back({function, arg});
    }

    /** Execute one pending background task. The task may schedule a
     *  successor which is left pending for a later call. */
    bool RunOne() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        Work work;
        {
            LOCK(m_mutex);
            if (m_queue.empty()) return false;
            work = m_queue.front();
            m_queue.pop_front();
        }
        work.function(work.arg);
        return true;
    }

    /** Execute pending background tasks until none remain. */
    void DrainWork() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex) { while (RunOne()) {} }
};

constexpr size_t MAX_VALUE_LEN{4096};
constexpr uint8_t MAX_VALUE_MULTIPLIER{8};
constexpr size_t WRITE_BATCH_HEADER{12}; // See kHeader in db/write_batch.cc

/** Mirror of CDBWrapper::OBFUSCATION_KEY, the fixed key under which leveldb
 *  stores the obfuscation metadata entry when obfuscation is enabled. */
const std::string OBFUSCATION_KEY{"\000obfuscate_key", 14};

/** Generate a deterministic value from key and size. The fuzz input picks
 *  a 16-bit length (up to MAX_VALUE_LEN) and an 8-bit multiplier so that a
 *  small amount of fuzz input can produce a wide range of value sizes. */
std::vector<uint8_t> MakeValue(uint16_t key, uint32_t size)
{
    std::vector<uint8_t> v(size);
    std::iota(v.begin(), v.end(), static_cast<uint8_t>(key ^ (key >> 8)));
    return v;
}

/** Equivalent to leveldb::BytewiseComparator() on 2-byte little-endian
 *  serialized uint16_t keys, while keeping the oracle keyed by uint16_t. */
struct LevelDBBytewiseU16Cmp {
    bool operator()(uint16_t a, uint16_t b) const { return internal_bswap_16(a) < internal_bswap_16(b); }
};

/** key → value-size map ordered by LevelDB's bytewise comparator. */
using Oracle = std::map<uint16_t, uint32_t, LevelDBBytewiseU16Cmp>;

struct FailUnserialize {
    template <typename Stream>
    void Unserialize(Stream&) { throw std::ios_base::failure{"always fail"}; }
};

uint16_t ConsumeKey(FuzzedDataProvider& provider) { return provider.ConsumeIntegral<uint16_t>(); }
uint32_t ConsumeValueSize(FuzzedDataProvider& provider)
{
    const uint16_t len{provider.ConsumeIntegralInRange<uint16_t>(0, MAX_VALUE_LEN)};
    const uint8_t multiplier{provider.ConsumeIntegralInRange<uint8_t>(1, MAX_VALUE_MULTIPLIER)};
    return static_cast<uint32_t>(len) * multiplier;
}

/** Verify that the DB iterator matches the oracle, handling the obfuscation
 *  metadata entry (stored under a non-uint16_t key) when obfuscation is on. */
void VerifyIterator(CDBWrapper& dbw, const Oracle& oracle,
                    bool obfuscate, std::optional<uint16_t> seek_key = std::nullopt)
{
    const std::unique_ptr<CDBIterator> it{dbw.NewIterator()};
    auto oracle_it{seek_key ? oracle.lower_bound(*seek_key) : oracle.begin()};
    if (seek_key) {
        it->Seek(*seek_key);
    } else {
        it->SeekToFirst();
    }
    for (; it->Valid(); it->Next()) {
        uint16_t db_key;
        assert(it->GetKey(db_key));
        if (oracle_it != oracle.end() && db_key == oracle_it->first) {
            std::vector<uint8_t> db_value;
            assert(it->GetValue(db_value));
            assert(db_value == MakeValue(db_key, oracle_it->second));
            ++oracle_it;
        } else {
            assert(obfuscate);
            std::string key_str;
            assert(it->GetKey(key_str));
            assert(key_str == OBFUSCATION_KEY);
        }
    }
    assert(oracle_it == oracle.end());
}

/** Maximum number of concurrent reader threads in dbwrapper_concurrent_reads. */
constexpr size_t MAX_READ_WORKERS{8};

/** Maximum number of queries each worker executes in dbwrapper_concurrent_reads. */
constexpr size_t MAX_READ_QUERIES_PER_WORKER{128};

ThreadPool g_read_pool{"dbfuzz"};

void StartReadPoolIfNeeded()
{
    if (!g_read_pool.WorkersCount()) g_read_pool.Start(MAX_READ_WORKERS);
}

/** Build randomized DBParams from the fuzz input, shared by all targets. */
DBParams ConsumeDBParams(FuzzedDataProvider& provider, leveldb::Env* testing_env,
                         bool obfuscate, DBOptions options = {})
{
    return DBParams{
        .path = "dbwrapper_fuzz",
        .cache_bytes = provider.ConsumeIntegralInRange<size_t>(64 << 10, 1_MiB),
        .obfuscate = obfuscate,
        .bloom_filter = provider.ConsumeBool(),
        .options = options,
        .testing_env = testing_env,
        .max_file_size = provider.ConsumeBool()
            ? DBWRAPPER_MAX_FILE_SIZE
            : provider.ConsumeIntegralInRange<size_t>(1_MiB, 4_MiB),
    };
}

template <typename DrainWorkFn, typename RunOneFn>
void TestDbWrapper(FuzzedDataProvider& provider,
                      leveldb::Env* testing_env,
                      DrainWorkFn drain_work,
                      RunOneFn run_one,
                      bool allow_force_compact)
{
    SeedRandomStateForTest(SeedRand::ZEROS);

    const bool obfuscate{provider.ConsumeBool()};

    const auto make_db{[&](DBOptions options = {}) {
        return std::make_unique<CDBWrapper>(ConsumeDBParams(provider, testing_env, obfuscate, options));
    }};
    std::unique_ptr<CDBWrapper> dbw{make_db()};

    // Oracle: key → value size. Content is reconstructed via MakeValue().
    Oracle oracle;

    LIMITED_WHILE (provider.ConsumeBool(), 1'000) {
        CallOneOf(
            provider,
            // --- Mutations ---
            [&] {
                const auto key{ConsumeKey(provider)};
                const auto size{ConsumeValueSize(provider)};
                drain_work();
                dbw->Write(key, MakeValue(key, size), /*fSync=*/provider.ConsumeBool());
                oracle[key] = size;
            },
            [&] {
                const auto key{ConsumeKey(provider)};
                drain_work();
                dbw->Erase(key, /*fSync=*/provider.ConsumeBool());
                oracle.erase(key);
            },
            [&] {
                CDBBatch batch{*dbw};
                std::map<uint16_t, uint32_t> batch_writes;
                std::set<uint16_t> batch_erases;
                const auto fill{[&] {
                    LIMITED_WHILE (provider.ConsumeBool(), 20) {
                        const auto key{ConsumeKey(provider)};
                        if (provider.ConsumeBool()) {
                            const auto size{ConsumeValueSize(provider)};
                            batch.Write(key, MakeValue(key, size));
                            batch_writes[key] = size;
                            batch_erases.erase(key);
                        } else {
                            batch.Erase(key);
                            batch_erases.insert(key);
                            batch_writes.erase(key);
                        }
                    }
                }};
                fill();
                if (provider.ConsumeBool()) {
                    assert(batch.ApproximateSize() >= WRITE_BATCH_HEADER);
                    batch.Clear();
                    assert(batch.ApproximateSize() == WRITE_BATCH_HEADER);
                    batch_writes.clear();
                    batch_erases.clear();
                    fill();
                }
                drain_work();
                dbw->WriteBatch(batch, /*fSync=*/provider.ConsumeBool());
                for (const auto& [k, v] : batch_writes) oracle[k] = v;
                for (const auto& k : batch_erases) oracle.erase(k);
            },
            [&] {
                drain_work();
                dbw.reset();
                DBOptions options{};
                if (allow_force_compact && provider.ConsumeBool()) {
                    options.force_compact = true;
                }
                dbw = make_db(options);
                VerifyIterator(*dbw, oracle, obfuscate);
            },
            // --- Reads ---
            [&] {
                const auto key{ConsumeKey(provider)};
                std::vector<uint8_t> value;
                const bool found{dbw->Read(key, value)};
                if (const auto it{oracle.find(key)}; it != oracle.end()) {
                    assert(found && value == MakeValue(key, it->second));
                } else {
                    assert(!found);
                }
            },
            [&] {
                const auto key{ConsumeKey(provider)};
                assert(dbw->Exists(key) == oracle.contains(key));
            },
            [&] {
                uint16_t key{};
                if (!oracle.empty() && provider.ConsumeBool()) {
                    auto it{oracle.begin()};
                    std::advance(it, provider.ConsumeIntegralInRange<size_t>(0, oracle.size() - 1));
                    key = it->first;
                } else {
                    key = ConsumeKey(provider);
                }
                FailUnserialize wrong_type;
                assert(!dbw->Read(key, wrong_type));
            },
            [&] {
                const auto seek_key{provider.ConsumeBool()
                                        ? std::optional<uint16_t>{ConsumeKey(provider)}
                                        : std::nullopt};
                VerifyIterator(*dbw, oracle, obfuscate, seek_key);
            },
            // --- Stats ---
            [&] {
                assert(dbw->IsEmpty() == (oracle.empty() && !obfuscate));
            },
            [&] {
                const auto [k1, k2]{std::minmax({ConsumeKey(provider), ConsumeKey(provider)}, LevelDBBytewiseU16Cmp{})};
                const size_t estimate_size{dbw->EstimateSize(k1, k2)};
                if (k1 == k2) assert(estimate_size == 0);
            },
            [&] {
                (void)dbw->DynamicMemoryUsage();
            },
            // --- Compaction control (no-op when run_one is no-op) ---
            [&] {
                run_one();
            });
    }

    VerifyIterator(*dbw, oracle, obfuscate);
    drain_work();
}

} // namespace

FUZZ_TARGET(dbwrapper, .init = [] { static auto setup{MakeNoLogFileContext<>()}; })
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    const auto memenv{std::unique_ptr<leveldb::Env>{leveldb::NewMemEnv(leveldb::Env::Default())}};
    DeterministicEnv det_env{memenv.get()};
    TestDbWrapper(
        provider, &det_env,
        [&] { det_env.DrainWork(); },
        [&] { return det_env.RunOne(); },
        /*allow_force_compact=*/false);
}

FUZZ_TARGET(dbwrapper_threaded, .init = [] { static auto setup{MakeNoLogFileContext<>()}; })
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    const auto memenv{std::unique_ptr<leveldb::Env>{leveldb::NewMemEnv(leveldb::Env::Default())}};
    TestDbWrapper(
        provider, memenv.get(),
        /*drain_work=*/[] {},
        /*run_one=*/[] { return false; },
        /*allow_force_compact=*/true);
}

FUZZ_TARGET(dbwrapper_concurrent_reads, .init = [] { static auto setup{MakeNoLogFileContext<>()}; })
{
    StartReadPoolIfNeeded();
    SeedRandomStateForTest(SeedRand::ZEROS);

    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    const auto memenv{std::unique_ptr<leveldb::Env>{leveldb::NewMemEnv(leveldb::Env::Default())}};
    DeterministicEnv det_env{memenv.get()};

    CDBWrapper db{ConsumeDBParams(provider, &det_env, /*obfuscate=*/provider.ConsumeBool())};

    // Seed the DB. Drain work after small batches so we don't deadlock on a
    // scheduled compaction.
    const size_t num_entries{provider.ConsumeIntegralInRange<size_t>(100, 5'000)};
    std::vector<uint16_t> keys;
    keys.reserve(num_entries);
    Oracle oracle;
    constexpr size_t SEED_BATCH_SIZE{400};
    for (size_t start{0}; start < num_entries; start += SEED_BATCH_SIZE) {
        CDBBatch batch{db};
        const size_t end{std::min(start + SEED_BATCH_SIZE, num_entries)};
        for (size_t i{start}; i < end; ++i) {
            const auto k{ConsumeKey(provider)};
            const auto size{ConsumeValueSize(provider)};
            batch.Write(k, MakeValue(k, size));
            keys.push_back(k);
            oracle[k] = size;
        }
        det_env.DrainWork();
        db.WriteBatch(batch, /*fSync=*/true);
    }

    while (provider.ConsumeBool() && det_env.RunOne()) {}

    // Build query list from seeded and random keys.
    const size_t num_queries{provider.ConsumeIntegralInRange<size_t>(1, 2'000)};
    enum class ReadOp { Read, Exists, IteratorSeek };
    std::vector<std::tuple<ReadOp, uint16_t>> queries;
    queries.reserve(num_queries);
    for (size_t i{0}; i < num_queries; ++i) {
        const auto op{provider.PickValueInArray({ReadOp::Read, ReadOp::Exists, ReadOp::IteratorSeek})};
        const uint16_t key{provider.ConsumeBool()
                               ? keys[provider.ConsumeIntegralInRange<size_t>(0, keys.size() - 1)]
                               : ConsumeKey(provider)};
        queries.emplace_back(op, key);
    }


    // Workers + main thread synchronize on the latch so all reads start together.
    std::latch start_latch{static_cast<ptrdiff_t>(MAX_READ_WORKERS + 1)};
    std::vector<std::function<void()>> tasks(MAX_READ_WORKERS);
    FastRandomContext rng{ConsumeUInt256(provider)};
    std::ranges::generate(tasks, [&] {
        return [&, seed = rng.rand256()] {
            FastRandomContext thread_rng{seed};
            std::vector<size_t> order(queries.size());
            std::iota(order.begin(), order.end(), size_t{0});
            std::ranges::shuffle(order, thread_rng);
            const size_t queries_to_run{std::min(queries.size(), MAX_READ_QUERIES_PER_WORKER)};
            std::vector<uint8_t> v;
            std::string key_str;
            start_latch.arrive_and_wait();
            const std::unique_ptr<CDBIterator> it{db.NewIterator()};
            // Every read must agree with the oracle, the source of truth.
            for (const auto i : std::span{order}.first(queries_to_run)) {
                const auto& [op, key] = queries[i];
                switch (op) {
                case ReadOp::Read:
                    if (const auto oit{oracle.find(key)}; oit != oracle.end()) {
                        assert(db.Read(key, v) && v == MakeValue(key, oit->second));
                    } else {
                        assert(!db.Read(key, v));
                    }
                    break;
                case ReadOp::Exists:
                    assert(db.Exists(key) == oracle.contains(key));
                    break;
                case ReadOp::IteratorSeek:
                    it->Seek(key);
                    // Skip the obfuscation metadata entry (a non-uint16_t key) if we land
                    // on it, so the result matches the oracle, which only tracks user keys.
                    if (it->Valid() && it->GetKey(key_str) && key_str == OBFUSCATION_KEY) it->Next();
                    if (const auto oit{oracle.lower_bound(key)}; oit != oracle.end()) {
                        assert(it->Valid());
                        uint16_t actual_key;
                        assert(it->GetKey(actual_key) && actual_key == oit->first);
                        assert(it->GetValue(v) && v == MakeValue(actual_key, oit->second));
                    } else {
                        assert(!it->Valid());
                    }
                    break;
                }
            }
        };
    });
    auto futures{*Assert(g_read_pool.Submit(std::move(tasks)))};

    // Release the workers and immediately run the queued compaction on this
    // thread, so compaction races against the concurrent reads.
    start_latch.arrive_and_wait();
    det_env.DrainWork();

    for (auto& fut : futures) fut.get();
    det_env.DrainWork();
}
