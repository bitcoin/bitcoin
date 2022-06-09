// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <bench/bench.h>
#include <crypto/rfc8439.h>

#include <array>
#include <cstddef>
#include <vector>

static constexpr uint64_t AAD_SIZE = 32;
static constexpr uint64_t PLAINTEXT_SIZE_TINY = 64;
static constexpr uint64_t PLAINTEXT_SIZE_SMALL = 256;
static constexpr uint64_t PLAINTEXT_SIZE_LARGE = 1024 * 1024;

static std::vector<std::byte> zero_key(32, std::byte{0x00});
static std::vector<std::byte> aad(AAD_SIZE, std::byte{0x00});
std::array<std::byte, 12> nonce = {std::byte{0x00}, std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
                                   std::byte{0x04}, std::byte{0x05}, std::byte{0x06}, std::byte{0x07},
                                   std::byte{0x08}, std::byte{0x09}, std::byte{0x0a}, std::byte{0x0b}};

static void RFC8439_AEAD(benchmark::Bench& bench, size_t plaintext_size, bool include_decryption)
{
    std::vector<std::byte> plaintext_in(plaintext_size, std::byte{0x00});
    std::vector<std::byte> output(plaintext_size + POLY1305_TAGLEN, std::byte{0x00});

    bench.batch(plaintext_size).unit("byte").run([&] {
        RFC8439Encrypt(aad, zero_key, nonce, plaintext_in, output);

        if (include_decryption) {
            std::vector<std::byte> decrypted_plaintext(plaintext_size);
            auto authenticated = RFC8439Decrypt(aad, zero_key, nonce, output, decrypted_plaintext);
            assert(authenticated);
            assert(memcmp(decrypted_plaintext.data(), plaintext_in.data(), plaintext_in.size()) == 0);
        }
    });
}

static void RFC8439_AEAD_64BYTES_ONLY_ENCRYPT(benchmark::Bench& bench)
{
    RFC8439_AEAD(bench, PLAINTEXT_SIZE_TINY, false);
}

static void RFC8439_AEAD_256BYTES_ONLY_ENCRYPT(benchmark::Bench& bench)
{
    RFC8439_AEAD(bench, PLAINTEXT_SIZE_SMALL, false);
}

static void RFC8439_AEAD_1MB_ONLY_ENCRYPT(benchmark::Bench& bench)
{
    RFC8439_AEAD(bench, PLAINTEXT_SIZE_LARGE, false);
}

static void RFC8439_AEAD_64BYTES_ENCRYPT_DECRYPT(benchmark::Bench& bench)
{
    RFC8439_AEAD(bench, PLAINTEXT_SIZE_TINY, true);
}

static void RFC8439_AEAD_256BYTES_ENCRYPT_DECRYPT(benchmark::Bench& bench)
{
    RFC8439_AEAD(bench, PLAINTEXT_SIZE_SMALL, true);
}

static void RFC8439_AEAD_1MB_ENCRYPT_DECRYPT(benchmark::Bench& bench)
{
    RFC8439_AEAD(bench, PLAINTEXT_SIZE_LARGE, true);
}

BENCHMARK(RFC8439_AEAD_64BYTES_ONLY_ENCRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(RFC8439_AEAD_256BYTES_ONLY_ENCRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(RFC8439_AEAD_1MB_ONLY_ENCRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(RFC8439_AEAD_64BYTES_ENCRYPT_DECRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(RFC8439_AEAD_256BYTES_ENCRYPT_DECRYPT, benchmark::PriorityLevel::HIGH);
BENCHMARK(RFC8439_AEAD_1MB_ENCRYPT_DECRYPT, benchmark::PriorityLevel::HIGH);
