// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/xoroshiro128plusplus.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto_chacha20)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const auto key = ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, ChaCha20::KEYLEN);
    ChaCha20 chacha20{key};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                auto key = ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, ChaCha20::KEYLEN);
                chacha20.SetKey(key);
            },
            [&] {
                ChaCha20::Nonce96 nonce{
                    fuzzed_data_provider.ConsumeIntegral<uint32_t>(),
                    fuzzed_data_provider.ConsumeIntegral<uint64_t>()};
                chacha20.Seek(nonce, fuzzed_data_provider.ConsumeIntegral<uint32_t>());
            },
            [&] {
                std::vector<uint8_t> output(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                chacha20.Keystream(MakeWritableByteSpan(output));
            },
            [&] {
                std::vector<std::byte> output(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                const auto input = ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, output.size());
                chacha20.Crypt(input, output);
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
    auto key_bytes = ConsumeFixedLengthByteVector<std::byte>(provider, ChaCha20::KEYLEN);
    uint64_t iv = provider.ConsumeIntegral<uint64_t>();
    uint32_t iv_prefix = provider.ConsumeIntegral<uint32_t>();
    uint64_t total_bytes = provider.ConsumeIntegralInRange<uint64_t>(0, 1000000);
    /* ~x = 2^BITS - 1 - x, so ~(total_bytes >> 6) is the maximal seek position. */
    uint32_t seek = provider.ConsumeIntegralInRange<uint32_t>(0, ~(uint32_t)(total_bytes >> 6));

    // Initialize two ChaCha20 ciphers, with the same key/iv/position.
    ChaCha20 crypt1(key_bytes);
    ChaCha20 crypt2(key_bytes);
    crypt1.Seek({iv_prefix, iv}, seek);
    crypt2.Seek({iv_prefix, iv}, seek);

    // Construct vectors with data.
    std::vector<std::byte> data1, data2;
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
            WriteLE64(UCharCast(data1.data() + bytes), val);
            WriteLE64(UCharCast(data2.data() + bytes), val);
            bytes += 8;
        }
        if (bytes < total_bytes) {
            std::byte valbytes[8];
            uint64_t val = rng();
            WriteLE64(UCharCast(valbytes), val);
            std::copy(valbytes, valbytes + (total_bytes - bytes), data1.data() + bytes);
            std::copy(valbytes, valbytes + (total_bytes - bytes), data2.data() + bytes);
        }
    }

    // Whether UseCrypt is used or not, the two byte arrays must match.
    assert(data1 == data2);

    // Encrypt data1, the whole array at once.
    if constexpr (UseCrypt) {
        crypt1.Crypt(data1, data1);
    } else {
        crypt1.Keystream(data1);
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
            crypt2.Crypt(Span{data2}.subspan(bytes2, now), Span{data2}.subspan(bytes2, now));
        } else {
            crypt2.Keystream(Span{data2}.subspan(bytes2, now));
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

FUZZ_TARGET(crypto_fschacha20)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    auto key = fuzzed_data_provider.ConsumeBytes<std::byte>(FSChaCha20::KEYLEN);
    key.resize(FSChaCha20::KEYLEN);

    auto fsc20 = FSChaCha20{key, fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, 1024)};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        auto input = fuzzed_data_provider.ConsumeBytes<std::byte>(fuzzed_data_provider.ConsumeIntegralInRange(0, 4096));
        std::vector<std::byte> output;
        output.resize(input.size());
        fsc20.Crypt(input, output);
    }
}
