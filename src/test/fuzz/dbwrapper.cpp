// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dbwrapper.h>
#include <compat/byteswap.h>
#include <sync.h>
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
        Work work{nullptr, nullptr};
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

constexpr size_t MAX_VALUE_SIZE{4096};
constexpr size_t WRITE_BATCH_HEADER{12}; // See kHeader in db/write_batch.cc

/** Generate a deterministic value from key and length. The fuzz input
 *  controls only the key and a 16-bit length (up to MAX_VALUE_SIZE);
 *  the actual byte content is derived from a fixed pattern and the
 *  size is tripled so that fuzz bytes aren't wasted on bulk data. */
std::vector<uint8_t> MakeValue(uint16_t key, uint16_t len)
{
    std::vector<uint8_t> v(static_cast<size_t>(len) * 3);
    std::iota(v.begin(), v.end(), static_cast<uint8_t>(key));
    return v;
}

/** Equivalent to leveldb::BytewiseComparator() on 2-byte little-endian
 *  serialized uint16_t keys, while keeping the oracle keyed by uint16_t. */
struct LevelDBBytewiseU16Cmp {
    bool operator()(uint16_t a, uint16_t b) const { return internal_bswap_16(a) < internal_bswap_16(b); }
};

struct FailUnserialize {
    template <typename Stream>
    void Unserialize(Stream&) { throw std::ios_base::failure{"always fail"}; }
};

uint16_t ConsumeKey(FuzzedDataProvider& provider) { return provider.ConsumeIntegral<uint16_t>(); }
uint16_t ConsumeValueLength(FuzzedDataProvider& provider) { return provider.ConsumeIntegralInRange<uint16_t>(0, MAX_VALUE_SIZE); }

} // namespace

FUZZ_TARGET(dbwrapper, .init = [] { static auto setup{MakeNoLogFileContext<>()}; })
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    const auto memenv{std::unique_ptr<leveldb::Env>{leveldb::NewMemEnv(leveldb::Env::Default())}};
    DeterministicEnv det_env{memenv.get()};

    const bool obfuscate{provider.ConsumeBool()};

    const auto make_db{[&] {
        return std::make_unique<CDBWrapper>(DBParams{
            .path = "dbwrapper_fuzz",
            .cache_bytes = provider.ConsumeIntegralInRange<size_t>(64 << 10, 1_MiB),
            .obfuscate = obfuscate,
            .testing_env = &det_env,
            .max_file_size = provider.ConsumeIntegralInRange<size_t>(1_MiB, 4_MiB),
        });
    }};
    std::unique_ptr<CDBWrapper> dbw{make_db()};

    // Oracle: key → value length. Content is reconstructed via MakeValue().
    std::map<uint16_t, uint16_t, LevelDBBytewiseU16Cmp> oracle;

    LIMITED_WHILE(provider.ConsumeBool(), 10'000)
    {
        CallOneOf(
            provider,
            // --- Mutations ---
            [&] {
                const auto key{ConsumeKey(provider)};
                const auto len{ConsumeValueLength(provider)};
                det_env.DrainWork();
                dbw->Write(key, MakeValue(key, len), provider.ConsumeBool());
                oracle[key] = len;
            },
            [&] {
                const auto key{ConsumeKey(provider)};
                det_env.DrainWork();
                dbw->Erase(key, provider.ConsumeBool());
                oracle.erase(key);
            },
            [&] {
                CDBBatch batch{*dbw};
                std::map<uint16_t, uint16_t> batch_writes;
                std::set<uint16_t> batch_erases;
                const auto fill{[&] {
                    LIMITED_WHILE(provider.ConsumeBool(), 20)
                    {
                        const auto key{ConsumeKey(provider)};
                        if (provider.ConsumeBool()) {
                            const auto len{ConsumeValueLength(provider)};
                            batch.Write(key, MakeValue(key, len));
                            batch_writes[key] = len;
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
                det_env.DrainWork();
                dbw->WriteBatch(batch, provider.ConsumeBool());
                for (const auto& [k, v] : batch_writes) oracle[k] = v;
                for (const auto& k : batch_erases) oracle.erase(k);
            },
            [&] {
                det_env.DrainWork();
                dbw.reset();
                dbw = make_db();
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
                const auto key{ConsumeKey(provider)};
                FailUnserialize wrong_type;
                assert(!dbw->Read(key, wrong_type));
            },
            [&] {
                const std::unique_ptr<CDBIterator> it{dbw->NewIterator()};
                auto oracle_it{oracle.begin()};
                if (provider.ConsumeBool()) {
                    const uint16_t seek_key{ConsumeKey(provider)};
                    it->Seek(seek_key);
                    oracle_it = oracle.lower_bound(seek_key);
                } else {
                    it->SeekToFirst();
                }
                while (it->Valid()) {
                    uint16_t db_key;
                    if (!it->GetKey(db_key)) break;

                    if (oracle_it != oracle.end() && db_key == oracle_it->first) {
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
                assert(oracle_it == oracle.end());
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
            // --- Compaction control ---
            [&] {
                det_env.RunOne();
            });
    }

    det_env.DrainWork();
}
