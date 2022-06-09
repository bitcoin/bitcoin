// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto_chacha20)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    ChaCha20 chacha20;
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(16, 32));
        chacha20 = ChaCha20{key.data(), key.size()};
    }
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(16, 32));
                chacha20.SetKey(key.data(), key.size());
            },
            [&] {
                chacha20.SetIV(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                chacha20.Seek(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
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

FUZZ_TARGET(crypto_fschacha20)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    auto key_vec = ConsumeFixedLengthByteVector(fuzzed_data_provider, FSCHACHA20_KEYLEN);
    key_vec.resize(FSCHACHA20_KEYLEN);
    auto salt_vec = ConsumeFixedLengthByteVector(fuzzed_data_provider, FSCHACHA20_KEYLEN);
    salt_vec.resize(FSCHACHA20_KEYLEN);

    std::array<std::byte, FSCHACHA20_KEYLEN> key;
    memcpy(key.data(), key_vec.data(), FSCHACHA20_KEYLEN);

    std::array<std::byte, FSCHACHA20_KEYLEN> salt;
    memcpy(salt.data(), salt_vec.data(), FSCHACHA20_KEYLEN);

    auto fsc20 = FSChaCha20{key, salt, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 1024)};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        auto input = fuzzed_data_provider.ConsumeBytes<std::byte>(fuzzed_data_provider.ConsumeIntegralInRange(0, 4096));
        std::vector<std::byte> output;
        output.resize(input.size());
        fsc20.Crypt(input, output);
    }
}
