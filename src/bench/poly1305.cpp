// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include <vector>

#include <bench/bench.h>
#include <crypto/poly1305.h>

/* Number of bytes to process per iteration */
static constexpr uint64_t BUFFER_SIZE_TINY  = 64;
static constexpr uint64_t BUFFER_SIZE_SMALL = 256;
static constexpr uint64_t BUFFER_SIZE_LARGE = 1024*1024;

static void POLY1305(benchmark::State& state, size_t buffersize)
{
    std::vector<unsigned char> tag(POLY1305_TAGLEN, 0);
    std::vector<unsigned char> key(POLY1305_KEYLEN, 0);
    std::vector<unsigned char> in(buffersize, 0);
    while (state.KeepRunning())
        poly1305_auth(tag.data(), in.data(), in.size(), key.data());
}

static void POLY1305_64BYTES(benchmark::State& state)
{
    POLY1305(state, BUFFER_SIZE_TINY);
}

static void POLY1305_256BYTES(benchmark::State& state)
{
    POLY1305(state, BUFFER_SIZE_SMALL);
}

static void POLY1305_1MB(benchmark::State& state)
{
    POLY1305(state, BUFFER_SIZE_LARGE);
}

BENCHMARK(POLY1305_64BYTES, 500000);
BENCHMARK(POLY1305_256BYTES, 250000);
BENCHMARK(POLY1305_1MB, 340);
