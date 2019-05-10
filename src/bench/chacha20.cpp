// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

#include <bench/bench.h>
#include <hash.h>
#include <crypto/chacha20.h>

/* Number of bytes to process per iteration */
static const uint64_t BUFFER_SIZE_TINY  = 64;
static const uint64_t BUFFER_SIZE_SMALL = 256;
static const uint64_t BUFFER_SIZE_LARGE = 1024*1024;

static void CHACHA20(benchmark::State& state, size_t buffersize)
{
    std::vector<uint8_t> key(32,0);
    ChaCha20 ctx(key.data(), key.size());
    ctx.SetIV(0);
    ctx.Seek(0);
    std::vector<uint8_t> in(buffersize,0);
    std::vector<uint8_t> out(buffersize,0);
    while (state.KeepRunning()) {
        ctx.Crypt(in.data(), out.data(), in.size());
    }
}

static void CHACHA20_64BYTES(benchmark::State& state)
{
    CHACHA20(state, BUFFER_SIZE_TINY);
}

static void CHACHA20_256BYTES(benchmark::State& state)
{
    CHACHA20(state, BUFFER_SIZE_SMALL);
}

static void CHACHA20_1MB(benchmark::State& state)
{
    CHACHA20(state, BUFFER_SIZE_LARGE);
}

BENCHMARK(CHACHA20_64BYTES, 500000);
BENCHMARK(CHACHA20_256BYTES, 250000);
BENCHMARK(CHACHA20_1MB, 340);
