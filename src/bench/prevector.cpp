// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat.h>
#include <prevector.h>
#include <serialize.h>
#include <streams.h>

#include <bench/bench.h>

struct nontrivial_t {
    int x;
    nontrivial_t() :x(-1) {}
    ADD_SERIALIZE_METHODS
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {READWRITE(x);}
};
static_assert(!IS_TRIVIALLY_CONSTRUCTIBLE<nontrivial_t>::value,
              "expected nontrivial_t to not be trivially constructible");

typedef unsigned char trivial_t;
static_assert(IS_TRIVIALLY_CONSTRUCTIBLE<trivial_t>::value,
              "expected trivial_t to be trivially constructible");

template <typename T>
static void PrevectorDestructor(benchmark::State& state)
{
    while (state.KeepRunning()) {
        prevector<28, T> t0;
        prevector<28, T> t1;
        t0.resize(28);
        t1.resize(29);
    }
}

template <typename T>
static void PrevectorClear(benchmark::State& state)
{
    prevector<28, T> t0;
    prevector<28, T> t1;
    while (state.KeepRunning()) {
        t0.resize(28);
        t0.clear();
        t1.resize(29);
        t1.clear();
    }
}

template <typename T>
void PrevectorResize(benchmark::State& state)
{
    prevector<28, T> t0;
    prevector<28, T> t1;
    while (state.KeepRunning()) {
        t0.resize(28);
        t0.resize(0);
        t1.resize(29);
        t1.resize(0);
    }
}

template <typename T>
static void PrevectorDeserialize(benchmark::State& state)
{
    CDataStream s0(SER_NETWORK, 0);
    prevector<28, T> t0;
    t0.resize(28);
    for (auto x = 0; x < 900; ++x) {
        s0 << t0;
    }
    t0.resize(100);
    for (auto x = 0; x < 101; ++x) {
        s0 << t0;
    }
    while (state.KeepRunning()) {
        prevector<28, T> t1;
        for (auto x = 0; x < 1000; ++x) {
            s0 >> t1;
        }
        s0.Init(SER_NETWORK, 0);
    }
}

#define PREVECTOR_TEST(name, nontrivops, trivops)                       \
    static void Prevector ## name ## Nontrivial(benchmark::State& state) { \
        Prevector ## name<nontrivial_t>(state);                         \
    }                                                                   \
    BENCHMARK(Prevector ## name ## Nontrivial, nontrivops);             \
    static void Prevector ## name ## Trivial(benchmark::State& state) { \
        Prevector ## name<trivial_t>(state);                            \
    }                                                                   \
    BENCHMARK(Prevector ## name ## Trivial, trivops);

PREVECTOR_TEST(Clear, 80 * 1000 * 1000, 70 * 1000 * 1000)
PREVECTOR_TEST(Destructor, 800 * 1000 * 1000, 800 * 1000 * 1000)
PREVECTOR_TEST(Resize, 80 * 1000 * 1000, 70 * 1000 * 1000)
PREVECTOR_TEST(Deserialize, 6800, 52000)

#include <vector>

typedef prevector<28, unsigned char> prevec;

static void PrevectorAssign(benchmark::State& state)
{
    prevec t;
    t.resize(28);
    std::vector<unsigned char> v;
    while (state.KeepRunning()) {
        prevec::const_iterator b = t.begin() + 5;
        prevec::const_iterator e = b + 20;
        v.assign(b, e);
    }
}

static void PrevectorAssignTo(benchmark::State& state)
{
    prevec t;
    t.resize(28);
    std::vector<unsigned char> v;
    while (state.KeepRunning()) {
        prevec::const_iterator b = t.begin() + 5;
        prevec::const_iterator e = b + 20;
        t.assign_to(b, e, v);
    }
}

BENCHMARK(PrevectorAssign, 90 * 1000 * 1000)
BENCHMARK(PrevectorAssignTo, 700 * 1000 * 1000)
