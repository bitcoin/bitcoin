// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha_poly_aead.h>
#include <crypto/poly1305.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto_chacha20_poly1305_aead)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> k1 = ConsumeFixedLengthByteVector(fuzzed_data_provider, CHACHA20_POLY1305_AEAD_KEY_LEN);
    const std::vector<uint8_t> k2 = ConsumeFixedLengthByteVector(fuzzed_data_provider, CHACHA20_POLY1305_AEAD_KEY_LEN);

    ChaCha20Poly1305AEAD aead(k1.data(), k1.size(), k2.data(), k2.size());
    size_t buffer_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096);
    std::vector<uint8_t> in(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    std::vector<uint8_t> out(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    bool is_encrypt = fuzzed_data_provider.ConsumeBool();
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                buffer_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(64, 4096);
                in = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
                out = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
            },
            [&] {
                (void)aead.Crypt(out.data(), out.size(), in.data(), buffer_size, is_encrypt);
            },
            [&] {
                (void)aead.DecryptLength(in.data());
            },
            [&] {
                is_encrypt = fuzzed_data_provider.ConsumeBool();
            });
    }
}
