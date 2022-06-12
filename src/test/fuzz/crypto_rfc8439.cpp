// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/rfc8439.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstddef>

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

    std::vector<Span<const std::byte>> ins{plaintext};

    auto encrypted = RFC8439Encrypt(aad, key, nonce, ins);
    assert(encrypted.ciphertext.size() == plaintext.size());

    auto bit_flip_attack = !plaintext.empty() && fdp.ConsumeBool();
    if (bit_flip_attack) {
        auto byte_to_flip = fdp.ConsumeIntegralInRange(0, static_cast<int>(encrypted.ciphertext.size() - 1));
        encrypted.ciphertext[byte_to_flip] = encrypted.ciphertext[byte_to_flip] ^ std::byte{0xFF};
    }

    auto decrypted = RFC8439Decrypt(aad, key, nonce, encrypted);
    if (bit_flip_attack) {
        assert(!decrypted.success);
    } else {
        assert(decrypted.success);
        assert(plaintext.size() == decrypted.plaintext.size());
        if (!plaintext.empty()) {
            assert(memcmp(decrypted.plaintext.data(), plaintext.data(), plaintext.size()) == 0);
        }
    }
}
