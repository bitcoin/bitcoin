// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <span.h>
#include <test/fuzz/util.h>
#include <util/vecdeque.h>

#include <cstdint>
#include <deque>

namespace {

/** The maximum number of simultaneous buffers kept by the test. */
static constexpr size_t MAX_BUFFERS{3};
/** How many elements are kept in a buffer at most. */
static constexpr size_t MAX_BUFFER_SIZE{48};
/** How many operations are performed at most on the buffers in one test. */
static constexpr size_t MAX_OPERATIONS{1024};

/** Perform a simulation fuzz test on VecDeque type T.
 *
 * T must be constructible from a uint64_t seed, comparable to other T, copyable, and movable.
 */
template<typename T, bool CheckNoneLeft>
void TestType(std::span<const uint8_t> buffer, uint64_t rng_tweak)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    // Local RNG, only used for the seeds to initialize T objects with.
    InsecureRandomContext rng(provider.ConsumeIntegral<uint64_t>() ^ rng_tweak);

    // Real circular buffers.
    std::vector<VecDeque<T>> real;
    real.reserve(MAX_BUFFERS);
    // Simulated circular buffers.
    std::vector<std::deque<T>> sim;
    sim.reserve(MAX_BUFFERS);
    // Temporary object of type T.
    std::optional<T> tmp;

    // Compare a real and a simulated buffer.
    auto compare_fn = [](const VecDeque<T>& r, const std::deque<T>& s) {
        assert(r.size() == s.size());
        assert(r.empty() == s.empty());
        assert(r.capacity() >= r.size());
        if (s.size() == 0) return;
        assert(r.front() == s.front());
        assert(r.back() == s.back());
        for (size_t i = 0; i < s.size(); ++i) {
            assert(r[i] == s[i]);
        }
    };

    LIMITED_WHILE(provider.remaining_bytes(), MAX_OPERATIONS) {
        int command = provider.ConsumeIntegral<uint8_t>() % 64;
        unsigned idx = real.empty() ? 0 : provider.ConsumeIntegralInRange<unsigned>(0, real.size() - 1);
        const size_t num_buffers = sim.size();
        // Pick one operation based on value of command. Not all operations are always applicable.
        // Loop through the applicable ones until command reaches 0 (which avoids the need to
        // compute the number of applicable commands ahead of time).
        const bool non_empty{num_buffers != 0};
        const bool non_full{num_buffers < MAX_BUFFERS};
        const bool partially_full{non_empty && non_full};
        const bool multiple_exist{num_buffers > 1};
        const bool existing_buffer_non_full{non_empty && sim[idx].size() < MAX_BUFFER_SIZE};
        const bool existing_buffer_non_empty{non_empty && !sim[idx].empty()};
        assert(non_full || non_empty);
        while (true) {
            if (non_full && command-- == 0) {
                /* Default construct. */
                real.emplace_back();
                sim.emplace_back();
                break;
            }
            if (non_empty && command-- == 0) {
                /* resize() */
                compare_fn(real[idx], sim[idx]);
                size_t new_size = provider.ConsumeIntegralInRange<size_t>(0, MAX_BUFFER_SIZE);
                real[idx].resize(new_size);
                sim[idx].resize(new_size);
                assert(real[idx].size() == new_size);
                break;
            }
            if (non_empty && command-- == 0) {
                /* clear() */
                compare_fn(real[idx], sim[idx]);
                real[idx].clear();
                sim[idx].clear();
                assert(real[idx].empty());
                break;
            }
            if (non_empty && command-- == 0) {
                /* Copy construct default. */
                compare_fn(real[idx], sim[idx]);
                real[idx] = VecDeque<T>();
                sim[idx].clear();
                assert(real[idx].size() == 0);
                break;
            }
            if (non_empty && command-- == 0) {
                /* Destruct. */
                compare_fn(real.back(), sim.back());
                real.pop_back();
                sim.pop_back();
                break;
            }
            if (partially_full && command-- == 0) {
                /* Copy construct. */
                real.emplace_back(real[idx]);
                sim.emplace_back(sim[idx]);
                break;
            }
            if (partially_full && command-- == 0) {
                /* Move construct. */
                VecDeque<T> copy(real[idx]);
                real.emplace_back(std::move(copy));
                sim.emplace_back(sim[idx]);
                break;
            }
            if (multiple_exist && command-- == 0) {
                /* swap() */
                swap(real[idx], real[(idx + 1) % num_buffers]);
                swap(sim[idx], sim[(idx + 1) % num_buffers]);
                break;
            }
            if (multiple_exist && command-- == 0) {
                /* Copy assign. */
                compare_fn(real[idx], sim[idx]);
                real[idx] = real[(idx + 1) % num_buffers];
                sim[idx] = sim[(idx + 1) % num_buffers];
                break;
            }
            if (multiple_exist && command-- == 0) {
                /* Move assign. */
                VecDeque<T> copy(real[(idx + 1) % num_buffers]);
                compare_fn(real[idx], sim[idx]);
                real[idx] = std::move(copy);
                sim[idx] = sim[(idx + 1) % num_buffers];
                break;
            }
            if (non_empty && command-- == 0) {
                /* Self swap() */
                swap(real[idx], real[idx]);
                break;
            }
            if (non_empty && command-- == 0) {
                /* Self-copy assign. */
                real[idx] = real[idx];
                break;
            }
            if (non_empty && command-- == 0) {
                /* Self-move assign. */
                // Do not use std::move(real[idx]) here: -Wself-move correctly warns about that.
                real[idx] = static_cast<VecDeque<T>&&>(real[idx]);
                break;
            }
            if (non_empty && command-- == 0) {
                /* reserve() */
                size_t res_size = provider.ConsumeIntegralInRange<size_t>(0, MAX_BUFFER_SIZE);
                size_t old_cap = real[idx].capacity();
                size_t old_size = real[idx].size();
                real[idx].reserve(res_size);
                assert(real[idx].size() == old_size);
                assert(real[idx].capacity() == std::max(old_cap, res_size));
                break;
            }
            if (non_empty && command-- == 0) {
                /* shrink_to_fit() */
                size_t old_size = real[idx].size();
                real[idx].shrink_to_fit();
                assert(real[idx].size() == old_size);
                assert(real[idx].capacity() == old_size);
                break;
            }
            if (existing_buffer_non_full && command-- == 0) {
                /* push_back() (copying) */
                tmp = T(rng.rand64());
                size_t old_size = real[idx].size();
                size_t old_cap = real[idx].capacity();
                real[idx].push_back(*tmp);
                sim[idx].push_back(*tmp);
                assert(real[idx].size() == old_size + 1);
                if (old_cap > old_size) {
                    assert(real[idx].capacity() == old_cap);
                } else {
                    assert(real[idx].capacity() > old_cap);
                    assert(real[idx].capacity() <= 2 * (old_cap + 1));
                }
                break;
            }
            if (existing_buffer_non_full && command-- == 0) {
                /* push_back() (moving) */
                tmp = T(rng.rand64());
                size_t old_size = real[idx].size();
                size_t old_cap = real[idx].capacity();
                sim[idx].push_back(*tmp);
                real[idx].push_back(std::move(*tmp));
                assert(real[idx].size() == old_size + 1);
                if (old_cap > old_size) {
                    assert(real[idx].capacity() == old_cap);
                } else {
                    assert(real[idx].capacity() > old_cap);
                    assert(real[idx].capacity() <= 2 * (old_cap + 1));
                }
                break;
            }
            if (existing_buffer_non_full && command-- == 0) {
                /* emplace_back() */
                uint64_t seed{rng.rand64()};
                size_t old_size = real[idx].size();
                size_t old_cap = real[idx].capacity();
                sim[idx].emplace_back(seed);
                real[idx].emplace_back(seed);
                assert(real[idx].size() == old_size + 1);
                if (old_cap > old_size) {
                    assert(real[idx].capacity() == old_cap);
                } else {
                    assert(real[idx].capacity() > old_cap);
                    assert(real[idx].capacity() <= 2 * (old_cap + 1));
                }
                break;
            }
            if (existing_buffer_non_full && command-- == 0) {
                /* push_front() (copying) */
                tmp = T(rng.rand64());
                size_t old_size = real[idx].size();
                size_t old_cap = real[idx].capacity();
                real[idx].push_front(*tmp);
                sim[idx].push_front(*tmp);
                assert(real[idx].size() == old_size + 1);
                if (old_cap > old_size) {
                    assert(real[idx].capacity() == old_cap);
                } else {
                    assert(real[idx].capacity() > old_cap);
                    assert(real[idx].capacity() <= 2 * (old_cap + 1));
                }
                break;
            }
            if (existing_buffer_non_full && command-- == 0) {
                /* push_front() (moving) */
                tmp = T(rng.rand64());
                size_t old_size = real[idx].size();
                size_t old_cap = real[idx].capacity();
                sim[idx].push_front(*tmp);
                real[idx].push_front(std::move(*tmp));
                assert(real[idx].size() == old_size + 1);
                if (old_cap > old_size) {
                    assert(real[idx].capacity() == old_cap);
                } else {
                    assert(real[idx].capacity() > old_cap);
                    assert(real[idx].capacity() <= 2 * (old_cap + 1));
                }
                break;
            }
            if (existing_buffer_non_full && command-- == 0) {
                /* emplace_front() */
                uint64_t seed{rng.rand64()};
                size_t old_size = real[idx].size();
                size_t old_cap = real[idx].capacity();
                sim[idx].emplace_front(seed);
                real[idx].emplace_front(seed);
                assert(real[idx].size() == old_size + 1);
                if (old_cap > old_size) {
                    assert(real[idx].capacity() == old_cap);
                } else {
                    assert(real[idx].capacity() > old_cap);
                    assert(real[idx].capacity() <= 2 * (old_cap + 1));
                }
                break;
            }
            if (existing_buffer_non_empty && command-- == 0) {
                /* front() [modifying] */
                tmp = T(rng.rand64());
                size_t old_size = real[idx].size();
                assert(sim[idx].front() == real[idx].front());
                sim[idx].front() = *tmp;
                real[idx].front() = std::move(*tmp);
                assert(real[idx].size() == old_size);
                break;
            }
            if (existing_buffer_non_empty && command-- == 0) {
                /* back() [modifying] */
                tmp = T(rng.rand64());
                size_t old_size = real[idx].size();
                assert(sim[idx].back() == real[idx].back());
                sim[idx].back() = *tmp;
                real[idx].back() = *tmp;
                assert(real[idx].size() == old_size);
                break;
            }
            if (existing_buffer_non_empty && command-- == 0) {
                /* operator[] [modifying] */
                tmp = T(rng.rand64());
                size_t pos = provider.ConsumeIntegralInRange<size_t>(0, sim[idx].size() - 1);
                size_t old_size = real[idx].size();
                assert(sim[idx][pos] == real[idx][pos]);
                sim[idx][pos] = *tmp;
                real[idx][pos] = std::move(*tmp);
                assert(real[idx].size() == old_size);
                break;
            }
            if (existing_buffer_non_empty && command-- == 0) {
                /* pop_front() */
                assert(sim[idx].front() == real[idx].front());
                size_t old_size = real[idx].size();
                sim[idx].pop_front();
                real[idx].pop_front();
                assert(real[idx].size() == old_size - 1);
                break;
            }
            if (existing_buffer_non_empty && command-- == 0) {
                /* pop_back() */
                assert(sim[idx].back() == real[idx].back());
                size_t old_size = real[idx].size();
                sim[idx].pop_back();
                real[idx].pop_back();
                assert(real[idx].size() == old_size - 1);
                break;
            }
        }
    }

    /* Fully compare the final state. */
    for (unsigned i = 0; i < sim.size(); ++i) {
        // Make sure const getters work.
        const VecDeque<T>& realbuf = real[i];
        const std::deque<T>& simbuf = sim[i];
        compare_fn(realbuf, simbuf);
        for (unsigned j = 0; j < sim.size(); ++j) {
            assert((realbuf == real[j]) == (simbuf == sim[j]));
            assert(((realbuf <=> real[j]) >= 0) == (simbuf >= sim[j]));
            assert(((realbuf <=> real[j]) <= 0) == (simbuf <= sim[j]));
        }
        // Clear out the buffers so we can check below that no objects exist anymore.
        sim[i].clear();
        real[i].clear();
    }

    if constexpr (CheckNoneLeft) {
        tmp = std::nullopt;
        T::CheckNoneExist();
    }
}

/** Data structure with built-in tracking of all existing objects. */
template<size_t Size>
class TrackedObj
{
    static_assert(Size > 0);

    /* Data type for map that actually stores the object data.
     *
     * The key is a pointer to the TrackedObj, the value is the uint64_t it was initialized with.
     * Default-constructed and moved-from objects hold an std::nullopt.
     */
    using track_map_type = std::map<const TrackedObj<Size>*, std::optional<uint64_t>>;

private:

    /** Actual map. */
    static inline track_map_type g_tracker;

    /** Iterators into the tracker map for this object.
     *
     * This is an array of size Size, all holding the same value, to give the object configurable
     * size. The value is g_tracker.end() if this object is not fully initialized. */
    typename track_map_type::iterator m_track_entry[Size];

    void Check() const
    {
        auto it = g_tracker.find(this);
        for (size_t i = 0; i < Size; ++i) {
            assert(m_track_entry[i] == it);
        }
    }

    /** Create entry for this object in g_tracker and populate m_track_entry. */
    void Register()
    {
        auto [it, inserted] = g_tracker.emplace(this, std::nullopt);
        assert(inserted);
        for (size_t i = 0; i < Size; ++i) {
            m_track_entry[i] = it;
        }
    }

    void Deregister()
    {
        Check();
        assert(m_track_entry[0] != g_tracker.end());
        g_tracker.erase(m_track_entry[0]);
        for (size_t i = 0; i < Size; ++i) {
            m_track_entry[i] = g_tracker.end();
        }
    }

    /** Get value corresponding to this object in g_tracker. */
    std::optional<uint64_t>& Deref()
    {
        Check();
        assert(m_track_entry[0] != g_tracker.end());
        return m_track_entry[0]->second;
    }

    /** Get value corresponding to this object in g_tracker. */
    const std::optional<uint64_t>& Deref() const
    {
        Check();
        assert(m_track_entry[0] != g_tracker.end());
        return m_track_entry[0]->second;
    }

public:
    ~TrackedObj() { Deregister(); }
    TrackedObj() { Register(); }

    TrackedObj(uint64_t value)
    {
        Register();
        Deref() = value;
    }

    TrackedObj(const TrackedObj& other)
    {
        Register();
        Deref() = other.Deref();
    }

    TrackedObj(TrackedObj&& other)
    {
        Register();
        Deref() = other.Deref();
        other.Deref() = std::nullopt;
    }

    TrackedObj& operator=(const TrackedObj& other)
    {
        if (this == &other) return *this;
        Deref() = other.Deref();
        return *this;
    }

    TrackedObj& operator=(TrackedObj&& other)
    {
        if (this == &other) return *this;
        Deref() = other.Deref();
        other.Deref() = std::nullopt;
        return *this;
    }

    friend bool operator==(const TrackedObj& a, const TrackedObj& b)
    {
        return a.Deref() == b.Deref();
    }

    friend std::strong_ordering operator<=>(const TrackedObj& a, const TrackedObj& b)
    {
        // Libc++ 15 & 16 do not support std::optional<T>::operator<=> yet. See
        // https://reviews.llvm.org/D146392.
        if (!a.Deref().has_value() || !b.Deref().has_value()) {
            return a.Deref().has_value() <=> b.Deref().has_value();
        }
        return *a.Deref() <=> *b.Deref();
    }

    static void CheckNoneExist()
    {
        assert(g_tracker.empty());
    }
};

} // namespace

FUZZ_TARGET(vecdeque)
{
    // Run the test with simple uints (which satisfy all the trivial properties).
    static_assert(std::is_trivially_copyable_v<uint32_t>);
    static_assert(std::is_trivially_destructible_v<uint64_t>);
    TestType<uint8_t, false>(buffer, 1);
    TestType<uint16_t, false>(buffer, 2);
    TestType<uint32_t, false>(buffer, 3);
    TestType<uint64_t, false>(buffer, 4);

    // Run the test with TrackedObjs (which do not).
    static_assert(!std::is_trivially_copyable_v<TrackedObj<3>>);
    static_assert(!std::is_trivially_destructible_v<TrackedObj<17>>);
    TestType<TrackedObj<1>, true>(buffer, 5);
    TestType<TrackedObj<3>, true>(buffer, 6);
    TestType<TrackedObj<17>, true>(buffer, 7);
}
