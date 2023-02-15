// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/xoroshiro128plusplus.h>

#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto_chacha20)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    ChaCha20 chacha20;
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, 32);
        chacha20 = ChaCha20{key.data()};
    }
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, 32);
                chacha20.SetKey32(key.data());
            },
            [&] {
                chacha20.SetIV(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                chacha20.Seek64(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                std::vector<uint8_t> output(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                chacha20.Keystream(output.data(), output.size());
            },
            [&] {
                std::vector<uint8_t> output(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                const std::vector<uint8_t> input = ConsumeFixedLengthByteVector(fuzzed_data_provider, output.size());
                chacha20.Crypt(input.data(), output.data(), input.size());
            });
    }
}

namespace
{

/** Fuzzer that invokes ChaCha20::Crypt() or ChaCha20::Keystream multiple times:
    once for a large block at once, and then the same data in chunks, comparing
    the outcome.

    If UseCrypt, seeded Xoroshiro128++ output is used as input to Crypt().
    If not, Keystream() is used directly, or sequences of 0x00 are encrypted.
*/
template<bool UseCrypt>
void ChaCha20SplitFuzz(FuzzedDataProvider& provider)
{
    // Determine key, iv, start position, length.
    unsigned char key[32] = {0};
    auto key_bytes = provider.ConsumeBytes<unsigned char>(32);
    std::copy(key_bytes.begin(), key_bytes.end(), key);
    uint64_t iv = provider.ConsumeIntegral<uint64_t>();
    uint64_t total_bytes = provider.ConsumeIntegralInRange<uint64_t>(0, 1000000);
    /* ~x = 2^64 - 1 - x, so ~(total_bytes >> 6) is the maximal seek position. */
    uint64_t seek = provider.ConsumeIntegralInRange<uint64_t>(0, ~(total_bytes >> 6));

    // Initialize two ChaCha20 ciphers, with the same key/iv/position.
    ChaCha20 crypt1(key);
    ChaCha20 crypt2(key);
    crypt1.SetIV(iv);
    crypt1.Seek64(seek);
    crypt2.SetIV(iv);
    crypt2.Seek64(seek);

    // Construct vectors with data.
    std::vector<unsigned char> data1, data2;
    data1.resize(total_bytes);
    data2.resize(total_bytes);

    // If using Crypt(), initialize data1 and data2 with the same Xoroshiro128++ based
    // stream.
    if constexpr (UseCrypt) {
        uint64_t seed = provider.ConsumeIntegral<uint64_t>();
        XoRoShiRo128PlusPlus rng(seed);
        uint64_t bytes = 0;
        while (bytes < (total_bytes & ~uint64_t{7})) {
            uint64_t val = rng();
            WriteLE64(data1.data() + bytes, val);
            WriteLE64(data2.data() + bytes, val);
            bytes += 8;
        }
        if (bytes < total_bytes) {
            unsigned char valbytes[8];
            uint64_t val = rng();
            WriteLE64(valbytes, val);
            std::copy(valbytes, valbytes + (total_bytes - bytes), data1.data() + bytes);
            std::copy(valbytes, valbytes + (total_bytes - bytes), data2.data() + bytes);
        }
    }

    // Whether UseCrypt is used or not, the two byte arrays must match.
    assert(data1 == data2);

    // Encrypt data1, the whole array at once.
    if constexpr (UseCrypt) {
        crypt1.Crypt(data1.data(), data1.data(), total_bytes);
    } else {
        crypt1.Keystream(data1.data(), total_bytes);
    }

    // Encrypt data2, in at most 256 chunks.
    uint64_t bytes2 = 0;
    int iter = 0;
    while (true) {
        bool is_last = (iter == 255) || (bytes2 == total_bytes) || provider.ConsumeBool();
        ++iter;
        // Determine how many bytes to encrypt in this chunk: a fuzzer-determined
        // amount for all but the last chunk (which processes all remaining bytes).
        uint64_t now = is_last ? total_bytes - bytes2 :
            provider.ConsumeIntegralInRange<uint64_t>(0, total_bytes - bytes2);
        // For each chunk, consider using Crypt() even when UseCrypt is false.
        // This tests that Keystream() has the same behavior as Crypt() applied
        // to 0x00 input bytes.
        if (UseCrypt || provider.ConsumeBool()) {
            crypt2.Crypt(data2.data() + bytes2, data2.data() + bytes2, now);
        } else {
            crypt2.Keystream(data2.data() + bytes2, now);
        }
        bytes2 += now;
        if (is_last) break;
    }
    // We should have processed everything now.
    assert(bytes2 == total_bytes);
    // And the result should match.
    assert(data1 == data2);
}

} // namespace

FUZZ_TARGET(chacha20_split_crypt)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    ChaCha20SplitFuzz<true>(provider);
}

FUZZ_TARGET(chacha20_split_keystream)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    ChaCha20SplitFuzz<false>(provider);
}
