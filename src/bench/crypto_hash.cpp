// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

#include "bench.h"
#include "bloom.h"
#include "utiltime.h"
#include "crypto/ripemd160.h"
#include "crypto/sha1.h"
#include "crypto/sha256.h"
#include "crypto/sha512.h"

/* Number of bytes to hash per iteration */
static const uint64_t BUFFER_SIZE = 1000*1000;

static void RIPEMD160(benchmark::State& state)
{
    uint8_t hash[CRIPEMD160::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CRIPEMD160().Write(begin_ptr(in), in.size()).Finalize(hash);
}

static void SHA1(benchmark::State& state)
{
    uint8_t hash[CSHA1::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA1().Write(begin_ptr(in), in.size()).Finalize(hash);
}

static void SHA256(benchmark::State& state)
{
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA256().Write(begin_ptr(in), in.size()).Finalize(hash);
}

static void SHA512(benchmark::State& state)
{
    uint8_t hash[CSHA512::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    while (state.KeepRunning())
        CSHA512().Write(begin_ptr(in), in.size()).Finalize(hash);
}

BENCHMARK(RIPEMD160);
BENCHMARK(SHA1);
BENCHMARK(SHA256);
BENCHMARK(SHA512);
