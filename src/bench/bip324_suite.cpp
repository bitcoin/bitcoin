// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <assert.h>
#include <bench/bench.h>
#include <crypto/bip324_suite.h>
#include <crypto/rfc8439.h> // for the RFC8439_EXPANSION constant
#include <hash.h>

#include <array>
#include <cstddef>
#include <vector>

/* Number of bytes to process per iteration */
static constexpr uint64_t BUFFER_SIZE_TINY = 64;
static constexpr uint64_t BUFFER_SIZE_SMALL = 256;
static constexpr uint64_t BUFFER_SIZE_LARGE = 1024 * 1024;

static const std::vector<std::byte> zero_vec(BIP324_KEY_LEN, std::byte{0x00});

static void BIP324_CIPHER_SUITE(benchmark::Bench& bench, size_t contents_len, bool include_decryption)
{
    BIP324Key zero_arr;
    memcpy(zero_arr.data(), zero_vec.data(), BIP324_KEY_LEN);
    BIP324CipherSuite enc{zero_arr, zero_arr};
    BIP324CipherSuite dec{zero_arr, zero_arr};

    auto packet_len = BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + contents_len + RFC8439_EXPANSION;

    std::vector<std::byte> in(contents_len, std::byte{0x00});
    std::vector<std::byte> out(packet_len, std::byte{0x00});

    BIP324HeaderFlags flags{BIP324_NONE};

    bench.batch(contents_len).unit("byte").run([&] {
        // encrypt or decrypt the buffer with a static key
        const bool crypt_ok_1 = enc.Crypt({}, in, out, flags, true);
        assert(crypt_ok_1);

        if (include_decryption) {
            // if we decrypt, we need to decrypt the length first
            std::array<std::byte, BIP324_LENGTH_FIELD_LEN> encrypted_pkt_len;
            memcpy(encrypted_pkt_len.data(), out.data(), BIP324_LENGTH_FIELD_LEN);
            (void)dec.DecryptLength(encrypted_pkt_len);
            const bool crypt_ok_2 = dec.Crypt({}, {out.data() + BIP324_LENGTH_FIELD_LEN, out.size() - BIP324_LENGTH_FIELD_LEN}, in, flags, false);
            assert(crypt_ok_2);
        }
    });
}

static void BIP324_CIPHER_SUITE_64BYTES_ONLY_ENCRYPT(benchmark::Bench& bench)
{
    BIP324_CIPHER_SUITE(bench, BUFFER_SIZE_TINY, false);
}

static void BIP324_CIPHER_SUITE_256BYTES_ONLY_ENCRYPT(benchmark::Bench& bench)
{
    BIP324_CIPHER_SUITE(bench, BUFFER_SIZE_SMALL, false);
}

static void BIP324_CIPHER_SUITE_1MB_ONLY_ENCRYPT(benchmark::Bench& bench)
{
    BIP324_CIPHER_SUITE(bench, BUFFER_SIZE_LARGE, false);
}

static void BIP324_CIPHER_SUITE_64BYTES_ENCRYPT_DECRYPT(benchmark::Bench& bench)
{
    BIP324_CIPHER_SUITE(bench, BUFFER_SIZE_TINY, true);
}

static void BIP324_CIPHER_SUITE_256BYTES_ENCRYPT_DECRYPT(benchmark::Bench& bench)
{
    BIP324_CIPHER_SUITE(bench, BUFFER_SIZE_SMALL, true);
}

static void BIP324_CIPHER_SUITE_1MB_ENCRYPT_DECRYPT(benchmark::Bench& bench)
{
    BIP324_CIPHER_SUITE(bench, BUFFER_SIZE_LARGE, true);
}

// Add Hash() (dbl-sha256) bench for comparison

static void HASH(benchmark::Bench& bench, size_t buffersize)
{
    uint8_t hash[CHash256::OUTPUT_SIZE];
    std::vector<uint8_t> in(buffersize, 0);
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

BENCHMARK(BIP324_CIPHER_SUITE_64BYTES_ONLY_ENCRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(BIP324_CIPHER_SUITE_256BYTES_ONLY_ENCRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(BIP324_CIPHER_SUITE_1MB_ONLY_ENCRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(BIP324_CIPHER_SUITE_64BYTES_ENCRYPT_DECRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(BIP324_CIPHER_SUITE_256BYTES_ENCRYPT_DECRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(BIP324_CIPHER_SUITE_1MB_ENCRYPT_DECRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(HASH_64BYTES, benchmark::PriorityLevel::HIGH);
BENCHMARK(HASH_256BYTES, benchmark::PriorityLevel::HIGH);
BENCHMARK(HASH_1MB, benchmark::PriorityLevel::HIGH);
