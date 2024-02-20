// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha_poly_aead.h>
#include <crypto/poly1305.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/overflow.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

FUZZ_TARGET(crypto_chacha20_poly1305_aead)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> k1 = ConsumeFixedLengthByteVector(fuzzed_data_provider, CHACHA20_POLY1305_AEAD_KEY_LEN);
    const std::vector<uint8_t> k2 = ConsumeFixedLengthByteVector(fuzzed_data_provider, CHACHA20_POLY1305_AEAD_KEY_LEN);

    ChaCha20Poly1305AEAD aead(k1.data(), k1.size(), k2.data(), k2.size());
    uint64_t seqnr_payload = 0;
    uint64_t seqnr_aad = 0;
    int aad_pos = 0;
    size_t buffer_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096);
    std::vector<uint8_t> in(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    std::vector<uint8_t> out(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    bool is_encrypt = fuzzed_data_provider.ConsumeBool();
    while (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                buffer_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(64, 4096);
                in = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
                out = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
            },
            [&] {
                (void)aead.Crypt(seqnr_payload, seqnr_aad, aad_pos, out.data(), out.size(), in.data(), buffer_size, is_encrypt);
            },
            [&] {
                uint32_t len = 0;
                const bool ok = aead.GetLength(&len, seqnr_aad, aad_pos, in.data());
                assert(ok);
            },
            [&] {
                if (AdditionOverflow(seqnr_payload, static_cast<uint64_t>(1))) {
                    return;
                }
                seqnr_payload += 1;
                aad_pos += CHACHA20_POLY1305_AEAD_AAD_LEN;
                if (aad_pos + CHACHA20_POLY1305_AEAD_AAD_LEN > CHACHA20_ROUND_OUTPUT) {
                    aad_pos = 0;
                    if (AdditionOverflow(seqnr_aad, static_cast<uint64_t>(1))) {
                        return;
                    }
                    seqnr_aad += 1;
                }
            },
            [&] {
                seqnr_payload = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
            },
            [&] {
                seqnr_aad = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
            },
            [&] {
                is_encrypt = fuzzed_data_provider.ConsumeBool();
            });
    }
}
