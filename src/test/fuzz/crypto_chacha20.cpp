// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/crypto.h>

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
                chacha20.Seek(
                    {
                        fuzzed_data_provider.ConsumeIntegral<uint32_t>(),
                        fuzzed_data_provider.ConsumeIntegral<uint64_t>()
                    }, fuzzed_data_provider.ConsumeIntegral<uint32_t>());
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
