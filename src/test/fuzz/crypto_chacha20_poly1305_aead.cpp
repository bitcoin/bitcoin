// Copyright (c) 2020 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha_poly_aead.h>
#include <crypto/poly1305.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
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
        switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 6)) {
        case 0: {
            buffer_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(64, 4096);
            in = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
            out = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
            break;
        }
        case 1: {
            (void)aead.Crypt(seqnr_payload, seqnr_aad, aad_pos, out.data(), out.size(), in.data(), buffer_size, is_encrypt);
            break;
        }
        case 2: {
            uint32_t len = 0;
            const bool ok = aead.GetLength(&len, seqnr_aad, aad_pos, in.data());
            assert(ok);
            break;
        }
        case 3: {
            seqnr_payload += 1;
            aad_pos += CHACHA20_POLY1305_AEAD_AAD_LEN;
            if (aad_pos + CHACHA20_POLY1305_AEAD_AAD_LEN > CHACHA20_ROUND_OUTPUT) {
                aad_pos = 0;
                seqnr_aad += 1;
            }
            break;
        }
        case 4: {
            seqnr_payload = fuzzed_data_provider.ConsumeIntegral<int>();
            break;
        }
        case 5: {
            seqnr_aad = fuzzed_data_provider.ConsumeIntegral<int>();
            break;
        }
        case 6: {
            is_encrypt = fuzzed_data_provider.ConsumeBool();
            break;
        }
        }
    }
}
