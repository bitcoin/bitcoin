// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <hash.h>
#include <random.h>
#include <uint256.h>


static void PrePadded(benchmark::State& state)
{

    CSHA256 hasher;

    // Setup the salted hasher
    uint256 nonce = GetRandHash();
    hasher.Write(nonce.begin(), 32);
    hasher.Write(nonce.begin(), 32);
    uint256 data = GetRandHash();
    while (state.KeepRunning()) {
        unsigned char out[32];
        CSHA256 h = hasher;
        h.Write(data.begin(), 32);
        h.Finalize(out);
    }
}

BENCHMARK(PrePadded, 10000);

static void RegularPadded(benchmark::State& state)
{
    CSHA256 hasher;

    // Setup the salted hasher
    uint256 nonce = GetRandHash();
    uint256 data = GetRandHash();
    while (state.KeepRunning()) {
        unsigned char out[32];
        CSHA256 h = hasher;
        h.Write(nonce.begin(), 32);
        h.Write(data.begin(), 32);
        h.Finalize(out);
    }
}

BENCHMARK(RegularPadded, 10000);
