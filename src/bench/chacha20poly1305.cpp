// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

#include <bench/bench.h>
#include <hash.h>
#include <utiltime.h>
#include <crypto/chachapoly_aead.h>

/* Number of bytes to crypt/hash per iteration */
static const uint64_t BUFFER_SIZE_A = 1000*1000;
static const uint64_t BUFFER_SIZE_B = 256;

static void CHACHA20POLY1305AEAD_(benchmark::State& state, uint64_t bufsize)
{
    struct chachapolyaead_ctx aead_ctx;
    static const uint32_t seqnr = 100;
    static const uint8_t aead_keys[64] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    chacha20poly1305_init(&aead_ctx, aead_keys, 64);
    std::vector<uint8_t> in(bufsize, 0);
    std::vector<uint8_t> out(bufsize, 0);

    while (state.KeepRunning()) {
        chacha20poly1305_crypt(&aead_ctx, seqnr, in.data(), out.data(), in.size()-16,
            4, 1);
    }
}

static void CHACHA20POLY1305AEAD_BIG(benchmark::State& state)
{
    CHACHA20POLY1305AEAD_(state, BUFFER_SIZE_A);
}

static void CHACHA20POLY1305AEAD_SMALL(benchmark::State& state)
{
    CHACHA20POLY1305AEAD_(state, BUFFER_SIZE_B);
}

static void HASH256_(benchmark::State& state, uint64_t bufsize)
{
    uint8_t hash[CHash256::OUTPUT_SIZE];
    std::vector<uint8_t> in(bufsize, 0);
    while (state.KeepRunning()) {
        CHash256().Write(in.data(), in.size()).Finalize(hash);
    }
}
static void HASH256_BIG(benchmark::State& state) {
    HASH256_(state, BUFFER_SIZE_A);
}
static void HASH256_SMALL(benchmark::State& state) {
    HASH256_(state, BUFFER_SIZE_B);
}

BENCHMARK(HASH256_SMALL, 250000);
BENCHMARK(HASH256_BIG, 340);
BENCHMARK(CHACHA20POLY1305AEAD_SMALL, 250000);
BENCHMARK(CHACHA20POLY1305AEAD_BIG, 340);
