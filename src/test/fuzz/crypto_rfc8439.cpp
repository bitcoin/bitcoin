// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/rfc8439.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstddef>
#include <vector>

FUZZ_TARGET(crypto_rfc8439)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    auto aad_len = fdp.ConsumeIntegralInRange(0, 1023);
    auto aad = fdp.ConsumeBytes<std::byte>(aad_len);
    aad.resize(aad_len);

    auto key = fdp.ConsumeBytes<std::byte>(RFC8439_KEYLEN);
    key.resize(RFC8439_KEYLEN);

    auto nonce_vec = fdp.ConsumeBytes<std::byte>(12);
    nonce_vec.resize(12);
    std::array<std::byte, 12> nonce;
    memcpy(nonce.data(), nonce_vec.data(), 12);

    auto plaintext_len = fdp.ConsumeIntegralInRange(0, 4095);
    auto plaintext = fdp.ConsumeBytes<std::byte>(plaintext_len);
    plaintext.resize(plaintext_len);

    std::vector<std::byte> output(plaintext.size() + POLY1305_TAGLEN, std::byte{0x00});
    RFC8439Encrypt(aad, key, nonce, plaintext, output);

    auto bit_flip_attack = !plaintext.empty() && fdp.ConsumeBool();
    if (bit_flip_attack) {
        auto byte_to_flip = fdp.ConsumeIntegralInRange(0, static_cast<int>(output.size() - POLY1305_TAGLEN - 1));
        output[byte_to_flip] = output[byte_to_flip] ^ std::byte{0xFF};
    }

    std::vector<std::byte> decrypted_plaintext(plaintext.size(), std::byte{0x00});
    auto authenticated = RFC8439Decrypt(aad, key, nonce, output, decrypted_plaintext);
    if (bit_flip_attack) {
        assert(!authenticated);
    } else {
        assert(authenticated);
        if (!plaintext.empty()) {
            assert(memcmp(decrypted_plaintext.data(), plaintext.data(), plaintext.size()) == 0);
        }
    }
}
