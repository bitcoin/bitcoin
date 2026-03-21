// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dbwrapper.h>
#include <compat/byteswap.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>

#include <leveldb/env.h>
#include <leveldb/helpers/memenv/memenv.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
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

    std::deque<Work> m_queue;

public:
    explicit DeterministicEnv(leveldb::Env* base) : EnvWrapper(base) {}

    void Schedule(WorkFunction function, void* arg) override
    {
        m_queue.push_back({function, arg});
    }

    /** Execute one pending background task. The task may schedule a
     *  successor which is left pending for a later call. */
    bool RunOne()
    {
        if (m_queue.empty()) return false;
        const Work work{m_queue.front()};
        m_queue.pop_front();
        work.function(work.arg);
        return true;
    }

    /** Execute pending background tasks until none remain. */
    void DrainWork() { while (RunOne()) {} }
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
        return std::make_unique<CDBWrapper>(DBParams{
            .path = "dbwrapper_fuzz",
            .cache_bytes = provider.ConsumeIntegralInRange<size_t>(64 << 10, 1_MiB),
            .obfuscate = obfuscate,
            .options = options,
            .testing_env = testing_env,
            .max_file_size = provider.ConsumeBool()
                ? DBWRAPPER_MAX_FILE_SIZE
                : provider.ConsumeIntegralInRange<size_t>(1_MiB, 4_MiB),
        });
    }};
    std::unique_ptr<CDBWrapper> dbw{make_db()};

    // Oracle: key → value size. Content is reconstructed via MakeValue().
    Oracle oracle;

    LIMITED_WHILE(provider.ConsumeBool(), 1'000)
    {
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
                    LIMITED_WHILE(provider.ConsumeBool(), 20)
                    {
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
