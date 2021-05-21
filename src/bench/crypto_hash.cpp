// Copyright (c) 2016 The Bitcoin Core developers
// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

#include <bench/bench.h>
#include <bloom.h>
#include <hash.h>
#include <random.h>
#include <uint256.h>
#include <utiltime.h>
#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <crypto/sha3.h>
#include <crypto/sha512.h>

/* Number of bytes to hash per iteration */
static const uint64_t BUFFER_SIZE = 1000*1000;

static void HASH_RIPEMD160(benchmark::State& state)
{
    uint8_t hash[CRIPEMD160::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CRIPEMD160().Write(in.data(), in.size()).Finalize(hash);
}

static void HASH_SHA1(benchmark::State& state)
{
    uint8_t hash[CSHA1::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA1().Write(in.data(), in.size()).Finalize(hash);
}

static void HASH_SHA256(benchmark::State& state)
{
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA256().Write(in.data(), in.size()).Finalize(hash);
}

static void HASH_SHA3_256_1M(benchmark::State& state)
{
    uint8_t hash[SHA3_256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        SHA3_256().Write(in).Finalize(hash);
}

static void HASH_SHA256_0032b(benchmark::State& state)
{
    std::vector<uint8_t> in(32,0);
    while (state.KeepRunning()) {
        CSHA256()
            .Write(in.data(), in.size())
            .Finalize(in.data());
    }
}

static void HASH_DSHA256(benchmark::State& state)
{
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CHash256().Write(in).Finalize(hash);
}

static void HASH_DSHA256_0032b(benchmark::State& state)
{
    std::vector<uint8_t> in(32,0);
    while (state.KeepRunning()) {
        CHash256().Write(in).Finalize(in);
    }
}

static void HASH_SHA256D64_1024(benchmark::State& state)
{
    std::vector<uint8_t> in(64 * 1024, 0);
    while (state.KeepRunning()) {
        SHA256D64(in.data(), in.data(), 1024);
    }
}

static void HASH_SHA512(benchmark::State& state)
{
    uint8_t hash[CSHA512::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA512().Write(in.data(), in.size()).Finalize(hash);
}

static void HASH_SipHash_0032b(benchmark::State& state)
{
    uint256 x;
    uint64_t k1 = 0;
    while (state.KeepRunning()) {
        *((uint64_t*)x.begin()) = SipHashUint256(0, ++k1, x);
    }
}

static void FastRandom_32bit(benchmark::State& state)
{
    FastRandomContext rng(true);
    uint32_t x = 0;
    while (state.KeepRunning()) {
        x += rng.rand32();
    }
}

static void FastRandom_1bit(benchmark::State& state)
{
    FastRandomContext rng(true);
    uint32_t x = 0;
    while (state.KeepRunning()) {
        x += rng.randbool();
    }
}

static void HASH_DSHA256_0032b_single(benchmark::State& state)
{
    std::vector<uint8_t> in(32,0);
    while (state.KeepRunning())
        CHash256().Write(in).Finalize(in);
}

static void HASH_DSHA256_0080b_single(benchmark::State& state)
{
    std::vector<uint8_t> in(80,0);
    while (state.KeepRunning())
        CHash256().Write(in).Finalize(in);
}

static void HASH_DSHA256_0128b_single(benchmark::State& state)
{
    std::vector<uint8_t> in(128,0);
    while (state.KeepRunning())
        CHash256().Write(in).Finalize(in);
}

static void HASH_DSHA256_0512b_single(benchmark::State& state)
{
    std::vector<uint8_t> in(512,0);
    while (state.KeepRunning())
        CHash256().Write(in).Finalize(in);
}

static void HASH_DSHA256_1024b_single(benchmark::State& state)
{
    std::vector<uint8_t> in(1024,0);
    while (state.KeepRunning())
        CHash256().Write(in).Finalize(in);
}

static void HASH_DSHA256_2048b_single(benchmark::State& state)
{
    std::vector<uint8_t> in(2048,0);
    while (state.KeepRunning())
        CHash256().Write(in).Finalize(in);
}

static void HASH_X11(benchmark::State& state)
{
    uint256 hash;
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        hash = HashX11(in.begin(), in.end());
}

static void HASH_X11_0032b_single(benchmark::State& state)
{
    uint256 hash;
    std::vector<uint8_t> in(32,0);
    while (state.KeepRunning())
        hash = HashX11(in.begin(), in.end());
}

static void HASH_X11_0080b_single(benchmark::State& state)
{
    uint256 hash;
    std::vector<uint8_t> in(80,0);
    while (state.KeepRunning())
        hash = HashX11(in.begin(), in.end());
}

static void HASH_X11_0128b_single(benchmark::State& state)
{
    uint256 hash;
    std::vector<uint8_t> in(128,0);
    while (state.KeepRunning())
        hash = HashX11(in.begin(), in.end());
}

static void HASH_X11_0512b_single(benchmark::State& state)
{
    uint256 hash;
    std::vector<uint8_t> in(512,0);
    while (state.KeepRunning())
        hash = HashX11(in.begin(), in.end());
}

static void HASH_X11_1024b_single(benchmark::State& state)
{
    uint256 hash;
    std::vector<uint8_t> in(1024,0);
    while (state.KeepRunning())
        hash = HashX11(in.begin(), in.end());
}

static void HASH_X11_2048b_single(benchmark::State& state)
{
    uint256 hash;
    std::vector<uint8_t> in(2048,0);
    while (state.KeepRunning())
        hash = HashX11(in.begin(), in.end());
}

BENCHMARK(HASH_RIPEMD160, 440);
BENCHMARK(HASH_SHA1, 570);
BENCHMARK(HASH_SHA256, 340);
BENCHMARK(HASH_DSHA256, 340);
BENCHMARK(HASH_SHA512, 330);
BENCHMARK(HASH_X11, 500);
BENCHMARK(HASH_SHA3_256_1M, 250);

BENCHMARK(HASH_SHA256_0032b, 4 * 1000 * 1000);
BENCHMARK(HASH_DSHA256_0032b, 2 * 1000 * 1000);
BENCHMARK(HASH_SipHash_0032b, 35 * 1000 * 1000);
BENCHMARK(HASH_SHA256D64_1024, 7400);

BENCHMARK(HASH_DSHA256_0032b_single, 2000 * 1000);
BENCHMARK(HASH_DSHA256_0080b_single, 1500 * 1000);
BENCHMARK(HASH_DSHA256_0128b_single, 1200 * 1000);
BENCHMARK(HASH_DSHA256_0512b_single, 500 * 1000);
BENCHMARK(HASH_DSHA256_1024b_single, 300 * 1000);
BENCHMARK(HASH_DSHA256_2048b_single, 150 * 1000);
BENCHMARK(HASH_X11_0032b_single, 70 * 1000);
BENCHMARK(HASH_X11_0080b_single, 65 * 1000);
BENCHMARK(HASH_X11_0128b_single, 60 * 1000);
BENCHMARK(HASH_X11_0512b_single, 50 * 1000);
BENCHMARK(HASH_X11_1024b_single, 50 * 1000);
BENCHMARK(HASH_X11_2048b_single, 50 * 1000);
BENCHMARK(FastRandom_32bit, 110 * 1000 * 1000);
BENCHMARK(FastRandom_1bit, 440 * 1000 * 1000);
