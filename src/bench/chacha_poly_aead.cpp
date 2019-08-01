// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

#include <bench/bench.h>
#include <crypto/chacha_poly_aead.h>
#include <crypto/poly1305.h> // for the POLY1305_TAGLEN constant
#include <hash.h>

#include <limits>
#include <assert.h>

/* Number of bytes to process per iteration */
static constexpr uint64_t BUFFER_SIZE_TINY = 64;
static constexpr uint64_t BUFFER_SIZE_SMALL = 256;
static constexpr uint64_t BUFFER_SIZE_LARGE = 1024 * 1024;

static const unsigned char k1[32] = {0};
static const unsigned char k2[32] = {0};

static ChaCha20Poly1305AEAD aead(k1, 32, k2, 32);

static void CHACHA20_POLY1305_AEAD(benchmark::State& state, size_t buffersize, bool include_decryption)
{
    std::vector<unsigned char> in(buffersize + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    std::vector<unsigned char> out(buffersize + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    uint64_t seqnr_payload = 0;
    uint64_t seqnr_aad = 0;
    int aad_pos = 0;
    uint32_t len = 0;
    while (state.KeepRunning()) {
        // encrypt or decrypt the buffer with a static key
        assert(aead.Crypt(seqnr_payload, seqnr_aad, aad_pos, out.data(), out.size(), in.data(), buffersize, true));

        if (include_decryption) {
            // if we decrypt, include the GetLength
            assert(aead.GetLength(&len, seqnr_aad, aad_pos, in.data()));
            assert(aead.Crypt(seqnr_payload, seqnr_aad, aad_pos, out.data(), out.size(), in.data(), buffersize, true));
        }

        // increase main sequence number
        seqnr_payload++;
        // increase aad position (position in AAD keystream)
        aad_pos += CHACHA20_POLY1305_AEAD_AAD_LEN;
        if (aad_pos + CHACHA20_POLY1305_AEAD_AAD_LEN > CHACHA20_ROUND_OUTPUT) {
            aad_pos = 0;
            seqnr_aad++;
        }
        if (seqnr_payload + 1 == std::numeric_limits<uint64_t>::max()) {
            // reuse of nonce+key is okay while benchmarking.
            seqnr_payload = 0;
            seqnr_aad = 0;
            aad_pos = 0;
        }
    }
}

static void CHACHA20_POLY1305_AEAD_64BYTES_ONLY_ENCRYPT(benchmark::State& state)
{
    CHACHA20_POLY1305_AEAD(state, BUFFER_SIZE_TINY, false);
}

static void CHACHA20_POLY1305_AEAD_256BYTES_ONLY_ENCRYPT(benchmark::State& state)
{
    CHACHA20_POLY1305_AEAD(state, BUFFER_SIZE_SMALL, false);
}

static void CHACHA20_POLY1305_AEAD_1MB_ONLY_ENCRYPT(benchmark::State& state)
{
    CHACHA20_POLY1305_AEAD(state, BUFFER_SIZE_LARGE, false);
}

static void CHACHA20_POLY1305_AEAD_64BYTES_ENCRYPT_DECRYPT(benchmark::State& state)
{
    CHACHA20_POLY1305_AEAD(state, BUFFER_SIZE_TINY, true);
}

static void CHACHA20_POLY1305_AEAD_256BYTES_ENCRYPT_DECRYPT(benchmark::State& state)
{
    CHACHA20_POLY1305_AEAD(state, BUFFER_SIZE_SMALL, true);
}

static void CHACHA20_POLY1305_AEAD_1MB_ENCRYPT_DECRYPT(benchmark::State& state)
{
    CHACHA20_POLY1305_AEAD(state, BUFFER_SIZE_LARGE, true);
}

// Add Hash() (dbl-sha256) bench for comparison

static void HASH(benchmark::State& state, size_t buffersize)
{
    uint8_t hash[CHash256::OUTPUT_SIZE];
    std::vector<uint8_t> in(buffersize,0);
    while (state.KeepRunning())
        CHash256().Write(in.data(), in.size()).Finalize(hash);
}

static void HASH_64BYTES(benchmark::State& state)
{
    HASH(state, BUFFER_SIZE_TINY);
}

static void HASH_256BYTES(benchmark::State& state)
{
    HASH(state, BUFFER_SIZE_SMALL);
}

static void HASH_1MB(benchmark::State& state)
{
    HASH(state, BUFFER_SIZE_LARGE);
}

//TODO add back below once benchmark backports are done
BENCHMARK(CHACHA20_POLY1305_AEAD_64BYTES_ONLY_ENCRYPT/*, 500000*/);
BENCHMARK(CHACHA20_POLY1305_AEAD_256BYTES_ONLY_ENCRYPT/*, 250000*/);
BENCHMARK(CHACHA20_POLY1305_AEAD_1MB_ONLY_ENCRYPT/*, 340*/);
BENCHMARK(CHACHA20_POLY1305_AEAD_64BYTES_ENCRYPT_DECRYPT/*, 500000*/);
BENCHMARK(CHACHA20_POLY1305_AEAD_256BYTES_ENCRYPT_DECRYPT/*, 250000*/);
BENCHMARK(CHACHA20_POLY1305_AEAD_1MB_ENCRYPT_DECRYPT/*, 340*/);
BENCHMARK(HASH_64BYTES/*, 500000*/);
BENCHMARK(HASH_256BYTES/*, 250000*/);
BENCHMARK(HASH_1MB/*, 340*/);
