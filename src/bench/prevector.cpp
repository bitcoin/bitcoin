// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <prevector.h>
#include <serialize.h>
#include <streams.h>
#include <type_traits>

#include <bench/bench.h>

struct nontrivial_t {
    int x{-1};
    nontrivial_t() = default;
    SERIALIZE_METHODS(nontrivial_t, obj) { READWRITE(obj.x); }
};
static_assert(!std::is_trivially_default_constructible<nontrivial_t>::value,
              "expected nontrivial_t to not be trivially constructible");

typedef unsigned char trivial_t;
static_assert(std::is_trivially_default_constructible<trivial_t>::value,
              "expected trivial_t to be trivially constructible");

template <typename T>
static void PrevectorDestructor(benchmark::Bench& bench)
{
    bench.batch(2).run([&] {
        prevector<28, T> t0;
        prevector<28, T> t1;
        t0.resize(28);
        t1.resize(29);
    });
}

template <typename T>
static void PrevectorClear(benchmark::Bench& bench)
{
    prevector<28, T> t0;
    prevector<28, T> t1;
    bench.batch(2).run([&] {
        t0.resize(28);
        t0.clear();
        t1.resize(29);
        t1.clear();
    });
}

template <typename T>
static void PrevectorResize(benchmark::Bench& bench)
{
    prevector<28, T> t0;
    prevector<28, T> t1;
    bench.batch(4).run([&] {
        t0.resize(28);
        t0.resize(0);
        t1.resize(29);
        t1.resize(0);
    });
}

template <typename T>
static void PrevectorDeserialize(benchmark::Bench& bench)
{
    DataStream s0{};
    prevector<28, T> t0;
    t0.resize(28);
    for (auto x = 0; x < 900; ++x) {
        s0 << t0;
    }
    t0.resize(100);
    for (auto x = 0; x < 101; ++x) {
        s0 << t0;
    }
    bench.batch(1000).run([&] {
        prevector<28, T> t1;
        for (auto x = 0; x < 1000; ++x) {
            s0 >> t1;
        }
        s0.Rewind();
    });
}

#define PREVECTOR_TEST(name)                                         \
    static void Prevector##name##Nontrivial(benchmark::Bench& bench) \
    {                                                                \
        Prevector##name<nontrivial_t>(bench);                        \
    }                                                                \
    BENCHMARK(Prevector##name##Nontrivial, benchmark::PriorityLevel::HIGH);         \
    static void Prevector##name##Trivial(benchmark::Bench& bench)    \
    {                                                                \
        Prevector##name<trivial_t>(bench);                           \
    }                                                                \
    BENCHMARK(Prevector##name##Trivial, benchmark::PriorityLevel::HIGH);

PREVECTOR_TEST(Clear)
PREVECTOR_TEST(Destructor)
PREVECTOR_TEST(Resize)
PREVECTOR_TEST(Deserialize)
