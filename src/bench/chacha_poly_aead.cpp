// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <bench/bench.h>
#include <crypto/chacha_poly_aead.h>
#include <crypto/poly1305.h> // for the POLY1305_TAGLEN constant
#include <hash.h>

#include <assert.h>
#include <limits>

/* Number of bytes to process per iteration */
static constexpr uint64_t BUFFER_SIZE_TINY = 64;
static constexpr uint64_t BUFFER_SIZE_SMALL = 256;
static constexpr uint64_t BUFFER_SIZE_LARGE = 1024 * 1024;

static const unsigned char k1[32] = {0};
static const unsigned char k2[32] = {0};

static ChaCha20Poly1305AEAD aead(k1, 32, k2, 32);

static void CHACHA20_POLY1305_AEAD(benchmark::Bench& bench, size_t buffersize, bool include_decryption)
{
    std::vector<unsigned char> in(buffersize + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    std::vector<unsigned char> out(buffersize + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    uint32_t len = 0;
    bench.batch(buffersize).unit("byte").run([&] {
        // encrypt or decrypt the buffer with a static key
        const bool crypt_ok_1 = aead.Crypt(out.data(), out.size(), in.data(), buffersize, true);
        assert(crypt_ok_1);

        if (include_decryption) {
            // if we decrypt, include the GetLength
            const bool get_length_ok = aead.DecryptLength(&len, in.data());
            assert(get_length_ok);
            const bool crypt_ok_2 = aead.Crypt(out.data(), out.size(), in.data(), buffersize, true);
            assert(crypt_ok_2);
        }
    });
}

static void CHACHA20_POLY1305_AEAD_64BYTES_ONLY_ENCRYPT(benchmark::Bench& bench)
{
    CHACHA20_POLY1305_AEAD(bench, BUFFER_SIZE_TINY, false);
}

static void CHACHA20_POLY1305_AEAD_256BYTES_ONLY_ENCRYPT(benchmark::Bench& bench)
{
    CHACHA20_POLY1305_AEAD(bench, BUFFER_SIZE_SMALL, false);
}

static void CHACHA20_POLY1305_AEAD_1MB_ONLY_ENCRYPT(benchmark::Bench& bench)
{
    CHACHA20_POLY1305_AEAD(bench, BUFFER_SIZE_LARGE, false);
}

static void CHACHA20_POLY1305_AEAD_64BYTES_ENCRYPT_DECRYPT(benchmark::Bench& bench)
{
    CHACHA20_POLY1305_AEAD(bench, BUFFER_SIZE_TINY, true);
}

static void CHACHA20_POLY1305_AEAD_256BYTES_ENCRYPT_DECRYPT(benchmark::Bench& bench)
{
    CHACHA20_POLY1305_AEAD(bench, BUFFER_SIZE_SMALL, true);
}

static void CHACHA20_POLY1305_AEAD_1MB_ENCRYPT_DECRYPT(benchmark::Bench& bench)
{
    CHACHA20_POLY1305_AEAD(bench, BUFFER_SIZE_LARGE, true);
}

// Add Hash() (dbl-sha256) bench for comparison

static void HASH(benchmark::Bench& bench, size_t buffersize)
{
    uint8_t hash[CHash256::OUTPUT_SIZE];
    std::vector<uint8_t> in(buffersize,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CHash256().Write(in).Finalize(hash);
    });
}

static void HASH_64BYTES(benchmark::Bench& bench)
{
    HASH(bench, BUFFER_SIZE_TINY);
}

static void HASH_256BYTES(benchmark::Bench& bench)
{
    HASH(bench, BUFFER_SIZE_SMALL);
}

static void HASH_1MB(benchmark::Bench& bench)
{
    HASH(bench, BUFFER_SIZE_LARGE);
}

BENCHMARK(CHACHA20_POLY1305_AEAD_64BYTES_ONLY_ENCRYPT);
BENCHMARK(CHACHA20_POLY1305_AEAD_256BYTES_ONLY_ENCRYPT);
BENCHMARK(CHACHA20_POLY1305_AEAD_1MB_ONLY_ENCRYPT);
BENCHMARK(CHACHA20_POLY1305_AEAD_64BYTES_ENCRYPT_DECRYPT);
BENCHMARK(CHACHA20_POLY1305_AEAD_256BYTES_ENCRYPT_DECRYPT);
BENCHMARK(CHACHA20_POLY1305_AEAD_1MB_ENCRYPT_DECRYPT);
BENCHMARK(HASH_64BYTES);
BENCHMARK(HASH_256BYTES);
BENCHMARK(HASH_1MB);
