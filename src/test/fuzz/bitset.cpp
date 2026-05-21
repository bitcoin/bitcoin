// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <span.h>
#include <test/fuzz/util.h>
#include <util/bitset.h>

#include <bitset>
#include <vector>

namespace {

/** Pop the first byte from a byte-span, and return it. */
uint8_t ReadByte(FuzzBufferType& buffer)
{
    if (buffer.empty()) return 0;
    uint8_t ret = buffer.front();
    buffer = buffer.subspan(1);
    return ret;
}

/** Perform a simulation fuzz test on BitSet type S. */
template<typename S>
void TestType(FuzzBufferType buffer)
{
    /** This fuzz test's design is based on the assumption that the actual bits stored in the
     *  bitsets and their simulations do not matter for the purpose of detecting edge cases, thus
     *  these are taken from a deterministically-seeded RNG instead. To provide some level of
     *  variation however, pick the seed based on the buffer size and size of the chosen bitset. */
    InsecureRandomContext rng(buffer.size() + 0x10000 * S::Size());

    using Sim = std::bitset<S::Size()>;
    // Up to 4 real BitSets (initially 2).
    std::vector<S> real(2);
    // Up to 4 std::bitsets with the same corresponding contents.
    std::vector<Sim> sim(2);

    /* Compare sim[idx] with real[idx], using all inspector operations. */
    auto compare_fn = [&](unsigned idx) {
        /* iterators and operator[] */
        auto it = real[idx].begin();
        unsigned first = S::Size();
        unsigned last = S::Size();
        for (unsigned i = 0; i < S::Size(); ++i) {
            bool match = (it != real[idx].end()) && *it == i;
            assert(sim[idx][i] == real[idx][i]);
            assert(match == real[idx][i]);
            assert((it == real[idx].end()) != (it != real[idx].end()));
            if (match) {
                ++it;
                if (first == S::Size()) first = i;
                last = i;
            }
        }
        assert(it == real[idx].end());
        assert(!(it != real[idx].end()));
        /* Any / None */
        assert(sim[idx].any() == real[idx].Any());
        assert(sim[idx].none() == real[idx].None());
        /* First / Last */
        if (sim[idx].any()) {
            assert(first == real[idx].First());
            assert(last == real[idx].Last());
        }
        /* Count */
        assert(sim[idx].count() == real[idx].Count());
    };

    LIMITED_WHILE(buffer.size() > 0, 1000) {
        // Read one byte to determine which operation to execute on the BitSets.
        int command = ReadByte(buffer) % 64;
        // Read another byte that determines which bitsets will be involved.
        unsigned args = ReadByte(buffer);
        unsigned dest = ((args & 7) * sim.size()) >> 3;
        unsigned src = (((args >> 3) & 7) * sim.size()) >> 3;
        unsigned aux = (((args >> 6) & 3) * sim.size()) >> 2;
        // Args are in range for non-empty sim, or sim is completely empty and will be grown
        assert((sim.empty() && dest == 0 && src == 0 && aux == 0) ||
            (!sim.empty() &&  dest < sim.size() && src < sim.size() && aux < sim.size()));

        // Pick one operation based on value of command. Not all operations are always applicable.
        // Loop through the applicable ones until command reaches 0 (which avoids the need to
        // compute the number of applicable commands ahead of time).
        while (true) {
            if (dest < sim.size() && command-- == 0) {
                /* Set() (true) */
                unsigned val = ReadByte(buffer) % S::Size();
                assert(sim[dest][val] == real[dest][val]);
                sim[dest].set(val);
                real[dest].Set(val);
                break;
            } else if (dest < sim.size() && command-- == 0) {
                /* Reset() */
                unsigned val = ReadByte(buffer) % S::Size();
                assert(sim[dest][val] == real[dest][val]);
                sim[dest].reset(val);
                real[dest].Reset(val);
                break;
            } else if (dest < sim.size() && command-- == 0) {
                /* Set() (conditional) */
                unsigned val = ReadByte(buffer) % S::Size();
                assert(sim[dest][val] == real[dest][val]);
                sim[dest].set(val, args >> 7);
                real[dest].Set(val, args >> 7);
                break;
            } else if (sim.size() < 4 && command-- == 0) {
                /* Construct empty. */
                sim.resize(sim.size() + 1);
                real.resize(real.size() + 1);
                break;
            } else if (sim.size() < 4 && command-- == 0) {
                /* Construct singleton. */
                unsigned val = ReadByte(buffer) % S::Size();
                std::bitset<S::Size()> newset;
                newset[val] = true;
                sim.push_back(newset);
                real.push_back(S::Singleton(val));
                break;
            } else if (dest < sim.size() && command-- == 0) {
                /* Make random. */
                compare_fn(dest);
                sim[dest].reset();
                real[dest] = S{};
                for (unsigned i = 0; i < S::Size(); ++i) {
                    if (rng.randbool()) {
                        sim[dest][i] = true;
                        real[dest].Set(i);
                    }
                }
                break;
            } else if (dest < sim.size() && command-- == 0) {
                /* Assign initializer list. */
                unsigned r1 = rng.randrange(S::Size());
                unsigned r2 = rng.randrange(S::Size());
                unsigned r3 = rng.randrange(S::Size());
                compare_fn(dest);
                sim[dest].reset();
                real[dest] = {r1, r2, r3};
                sim[dest].set(r1);
                sim[dest].set(r2);
                sim[dest].set(r3);
                break;
            } else if (!sim.empty() && command-- == 0) {
                /* Destruct. */
                compare_fn(sim.size() - 1);
                sim.pop_back();
                real.pop_back();
                break;
            } else if (sim.size() < 4 && src < sim.size() && command-- == 0) {
                /* Copy construct. */
                sim.emplace_back(sim[src]);
                real.emplace_back(real[src]);
                break;
            } else if (src < sim.size() && dest < sim.size() && command-- == 0) {
                /* Copy assign. */
                compare_fn(dest);
                sim[dest] = sim[src];
                real[dest] = real[src];
                break;
            } else if (src < sim.size() && dest < sim.size() && command-- == 0) {
                /* swap() function. */
                swap(sim[dest], sim[src]);
                swap(real[dest], real[src]);
                break;
            } else if (sim.size() < 4 && command-- == 0) {
                /* Construct with initializer list. */
                unsigned r1 = rng.randrange(S::Size());
                unsigned r2 = rng.randrange(S::Size());
                sim.emplace_back();
                sim.back().set(r1);
                sim.back().set(r2);
                real.push_back(S{r1, r2});
                break;
            } else if (dest < sim.size() && command-- == 0) {
                /* Fill() + copy assign. */
                unsigned len = ReadByte(buffer) % S::Size();
                compare_fn(dest);
                sim[dest].reset();
                for (unsigned i = 0; i < len; ++i) sim[dest][i] = true;
                real[dest] = S::Fill(len);
                break;
            } else if (src < sim.size() && command-- == 0) {
                /* Iterator copy based compare. */
                unsigned val = ReadByte(buffer) % S::Size();
                /* In a first loop, compare begin..end, and copy to it_copy at some point. */
                auto it = real[src].begin(), it_copy = it;
                for (unsigned i = 0; i < S::Size(); ++i) {
                    if (i == val) it_copy = it;
                    bool match = (it != real[src].end()) && *it == i;
                    assert(match == sim[src][i]);
                    if (match) ++it;
                }
                assert(it == real[src].end());
                /* Then compare from the copied point again to end. */
                for (unsigned i = val; i < S::Size(); ++i) {
                    bool match = (it_copy != real[src].end()) && *it_copy == i;
                    assert(match == sim[src][i]);
                    if (match) ++it_copy;
                }
                assert(it_copy == real[src].end());
                break;
            } else if (src < sim.size() && dest < sim.size() && command-- == 0) {
                /* operator|= */
                compare_fn(dest);
                sim[dest] |= sim[src];
                real[dest] |= real[src];
                break;
            } else if (src < sim.size() && dest < sim.size() && command-- == 0) {
                /* operator&= */
                compare_fn(dest);
                sim[dest] &= sim[src];
                real[dest] &= real[src];
                break;
            } else if (src < sim.size() && dest < sim.size() && command-- == 0) {
                /* operator-= */
                compare_fn(dest);
                sim[dest] &= ~sim[src];
                real[dest] -= real[src];
                break;
            } else if (src < sim.size() && dest < sim.size() && command-- == 0) {
                /* operator^= */
                compare_fn(dest);
                sim[dest] ^= sim[src];
                real[dest] ^= real[src];
                break;
            } else if (src < sim.size() && dest < sim.size() && aux < sim.size() && command-- == 0) {
                /* operator| */
                compare_fn(dest);
                sim[dest] = sim[src] | sim[aux];
                real[dest] = real[src] | real[aux];
                break;
            } else if (src < sim.size() && dest < sim.size() && aux < sim.size() && command-- == 0) {
                /* operator& */
                compare_fn(dest);
                sim[dest] = sim[src] & sim[aux];
                real[dest] = real[src] & real[aux];
                break;
            } else if (src < sim.size() && dest < sim.size() && aux < sim.size() && command-- == 0) {
                /* operator- */
                compare_fn(dest);
                sim[dest] = sim[src] & ~sim[aux];
                real[dest] = real[src] - real[aux];
                break;
            } else if (src < sim.size() && dest < sim.size() && aux < sim.size() && command-- == 0) {
                /* operator^ */
                compare_fn(dest);
                sim[dest] = sim[src] ^ sim[aux];
                real[dest] = real[src] ^ real[aux];
                break;
            } else if (src < sim.size() && aux < sim.size() && command-- == 0) {
                /* IsSupersetOf() and IsSubsetOf() */
                bool is_superset = (sim[aux] & ~sim[src]).none();
                bool is_subset = (sim[src] & ~sim[aux]).none();
                assert(real[src].IsSupersetOf(real[aux]) == is_superset);
                assert(real[src].IsSubsetOf(real[aux]) == is_subset);
                assert(real[aux].IsSupersetOf(real[src]) == is_subset);
                assert(real[aux].IsSubsetOf(real[src]) == is_superset);
                break;
            } else if (src < sim.size() && aux < sim.size() && command-- == 0) {
                /* operator== and operator!= */
                assert((sim[src] == sim[aux]) == (real[src] == real[aux]));
                assert((sim[src] != sim[aux]) == (real[src] != real[aux]));
                break;
            } else if (src < sim.size() && aux < sim.size() && command-- == 0) {
                /* Overlaps() */
                assert((sim[src] & sim[aux]).any() == real[src].Overlaps(real[aux]));
                assert((sim[src] & sim[aux]).any() == real[aux].Overlaps(real[src]));
                break;
            }
        }
    }
    /* Fully compare the final state. */
    for (unsigned i = 0; i < sim.size(); ++i) {
        compare_fn(i);
    }
}

} // namespace

FUZZ_TARGET(bitset)
{
    unsigned typdat = ReadByte(buffer) % 8;
    if (typdat == 0) {
        /* 16 bits */
        TestType<bitset_detail::IntBitSet<uint16_t>>(buffer);
        TestType<bitset_detail::MultiIntBitSet<uint16_t, 1>>(buffer);
    } else if (typdat == 1) {
        /* 32 bits */
        TestType<bitset_detail::MultiIntBitSet<uint16_t, 2>>(buffer);
        TestType<bitset_detail::IntBitSet<uint32_t>>(buffer);
    } else if (typdat == 2) {
        /* 48 bits */
        TestType<bitset_detail::MultiIntBitSet<uint16_t, 3>>(buffer);
    } else if (typdat == 3) {
        /* 64 bits */
        TestType<bitset_detail::IntBitSet<uint64_t>>(buffer);
        TestType<bitset_detail::MultiIntBitSet<uint64_t, 1>>(buffer);
        TestType<bitset_detail::MultiIntBitSet<uint32_t, 2>>(buffer);
        TestType<bitset_detail::MultiIntBitSet<uint16_t, 4>>(buffer);
    } else if (typdat == 4) {
        /* 96 bits */
        TestType<bitset_detail::MultiIntBitSet<uint32_t, 3>>(buffer);
    } else if (typdat == 5) {
        /* 128 bits */
        TestType<bitset_detail::MultiIntBitSet<uint64_t, 2>>(buffer);
        TestType<bitset_detail::MultiIntBitSet<uint32_t, 4>>(buffer);
    } else if (typdat == 6) {
        /* 192 bits */
        TestType<bitset_detail::MultiIntBitSet<uint64_t, 3>>(buffer);
    } else if (typdat == 7) {
        /* 256 bits */
        TestType<bitset_detail::MultiIntBitSet<uint64_t, 4>>(buffer);
    }
}
