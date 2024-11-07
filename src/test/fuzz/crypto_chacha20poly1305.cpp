// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20poly1305.h>
#include <random.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstddef>
#include <cstdint>
#include <vector>

constexpr static inline void crypt_till_rekey(FSChaCha20Poly1305& aead, int rekey_interval, bool encrypt)
{
    for (int i = 0; i < rekey_interval; ++i) {
        std::byte dummy_tag[FSChaCha20Poly1305::EXPANSION] = {{}};
        if (encrypt) {
            aead.Encrypt(Span{dummy_tag}.first(0), Span{dummy_tag}.first(0), dummy_tag);
        } else {
            aead.Decrypt(dummy_tag, Span{dummy_tag}.first(0), Span{dummy_tag}.first(0));
        }
    }
}

FUZZ_TARGET(crypto_aeadchacha20poly1305)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    auto key = provider.ConsumeBytes<std::byte>(32);
    key.resize(32);
    AEADChaCha20Poly1305 aead(key);

    // Initialize RNG deterministically, to generate contents and AAD. We assume that there are no
    // (potentially buggy) edge cases triggered by specific values of contents/AAD, so we can avoid
    // reading the actual data for those from the fuzzer input (which would need large amounts of
    // data).
    InsecureRandomContext rng(provider.ConsumeIntegral<uint64_t>());

    LIMITED_WHILE(provider.ConsumeBool(), 100)
    {
        // Mode:
        // - Bit 0: whether to use single-plain Encrypt/Decrypt; otherwise use a split at prefix.
        // - Bit 2: whether this ciphertext will be corrupted (making it the last sent one)
        // - Bit 3-4: controls the maximum aad length (max 511 bytes)
        // - Bit 5-7: controls the maximum content length (max 16383 bytes, for performance reasons)
        unsigned mode = provider.ConsumeIntegral<uint8_t>();
        bool use_splits = mode & 1;
        bool damage = mode & 4;
        unsigned aad_length_bits = 3 * ((mode >> 3) & 3);
        unsigned aad_length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << aad_length_bits) - 1);
        unsigned length_bits = 2 * ((mode >> 5) & 7);
        unsigned length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << length_bits) - 1);
        // Generate aad and content.
        auto aad = rng.randbytes<std::byte>(aad_length);
        auto plain = rng.randbytes<std::byte>(length);
        std::vector<std::byte> cipher(length + AEADChaCha20Poly1305::EXPANSION);
        // Generate nonce
        AEADChaCha20Poly1305::Nonce96 nonce = {(uint32_t)rng(), rng()};

        if (use_splits && length > 0) {
            size_t split_index = provider.ConsumeIntegralInRange<size_t>(1, length);
            aead.Encrypt(Span{plain}.first(split_index), Span{plain}.subspan(split_index), aad, nonce, cipher);
        } else {
            aead.Encrypt(plain, aad, nonce, cipher);
        }

        // Test Keystream output
        std::vector<std::byte> keystream(length);
        aead.Keystream(nonce, keystream);
        for (size_t i = 0; i < length; ++i) {
            assert((plain[i] ^ keystream[i]) == cipher[i]);
        }

        std::vector<std::byte> decrypted_contents(length);
        bool ok{false};

        // damage the key
        unsigned key_position = provider.ConsumeIntegralInRange<unsigned>(0, 31);
        std::byte damage_val{(uint8_t)(1U << (key_position & 7))};
        std::vector<std::byte> bad_key = key;
        bad_key[key_position] ^= damage_val;

        AEADChaCha20Poly1305 bad_aead(bad_key);
        ok = bad_aead.Decrypt(cipher, aad, nonce, decrypted_contents);
        assert(!ok);

        // Optionally damage 1 bit in either the cipher (corresponding to a change in transit)
        // or the aad (to make sure that decryption will fail if the AAD mismatches).
        if (damage) {
            unsigned damage_bit = provider.ConsumeIntegralInRange<unsigned>(0, (cipher.size() + aad.size()) * 8U - 1U);
            unsigned damage_pos = damage_bit >> 3;
            std::byte damage_val{(uint8_t)(1U << (damage_bit & 7))};
            if (damage_pos >= cipher.size()) {
                aad[damage_pos - cipher.size()] ^= damage_val;
            } else {
                cipher[damage_pos] ^= damage_val;
            }
        }

        if (use_splits && length > 0) {
            size_t split_index = provider.ConsumeIntegralInRange<size_t>(1, length);
            ok = aead.Decrypt(cipher, aad, nonce, Span{decrypted_contents}.first(split_index), Span{decrypted_contents}.subspan(split_index));
        } else {
            ok = aead.Decrypt(cipher, aad, nonce, decrypted_contents);
        }

        // Decryption *must* fail if the packet was damaged, and succeed if it wasn't.
        assert(!ok == damage);
        if (!ok) break;
        assert(decrypted_contents == plain);
    }
}

FUZZ_TARGET(crypto_fschacha20poly1305)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    uint32_t rekey_interval = provider.ConsumeIntegralInRange<size_t>(32, 512);
    auto key = provider.ConsumeBytes<std::byte>(32);
    key.resize(32);
    FSChaCha20Poly1305 enc_aead(key, rekey_interval);
    FSChaCha20Poly1305 dec_aead(key, rekey_interval);

    // Initialize RNG deterministically, to generate contents and AAD. We assume that there are no
    // (potentially buggy) edge cases triggered by specific values of contents/AAD, so we can avoid
    // reading the actual data for those from the fuzzer input (which would need large amounts of
    // data).
    InsecureRandomContext rng(provider.ConsumeIntegral<uint64_t>());

    LIMITED_WHILE(provider.ConsumeBool(), 100)
    {
        // Mode:
        // - Bit 0: whether to use single-plain Encrypt/Decrypt; otherwise use a split at prefix.
        // - Bit 2: whether this ciphertext will be corrupted (making it the last sent one)
        // - Bit 3-4: controls the maximum aad length (max 511 bytes)
        // - Bit 5-7: controls the maximum content length (max 16383 bytes, for performance reasons)
        unsigned mode = provider.ConsumeIntegral<uint8_t>();
        bool use_splits = mode & 1;
        bool damage = mode & 4;
        unsigned aad_length_bits = 3 * ((mode >> 3) & 3);
        unsigned aad_length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << aad_length_bits) - 1);
        unsigned length_bits = 2 * ((mode >> 5) & 7);
        unsigned length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << length_bits) - 1);
        // Generate aad and content.
        auto aad = rng.randbytes<std::byte>(aad_length);
        auto plain = rng.randbytes<std::byte>(length);
        std::vector<std::byte> cipher(length + FSChaCha20Poly1305::EXPANSION);

        crypt_till_rekey(enc_aead, rekey_interval, true);
        if (use_splits && length > 0) {
            size_t split_index = provider.ConsumeIntegralInRange<size_t>(1, length);
            enc_aead.Encrypt(Span{plain}.first(split_index), Span{plain}.subspan(split_index), aad, cipher);
        } else {
            enc_aead.Encrypt(plain, aad, cipher);
        }

        std::vector<std::byte> decrypted_contents(length);
        bool ok{false};

        // damage the key
        unsigned key_position = provider.ConsumeIntegralInRange<unsigned>(0, 31);
        std::byte damage_val{(uint8_t)(1U << (key_position & 7))};
        std::vector<std::byte> bad_key = key;
        bad_key[key_position] ^= damage_val;

        FSChaCha20Poly1305 bad_fs_aead(bad_key, rekey_interval);
        crypt_till_rekey(bad_fs_aead, rekey_interval, false);
        ok = bad_fs_aead.Decrypt(cipher, aad, decrypted_contents);
        assert(!ok);

        // Optionally damage 1 bit in either the cipher (corresponding to a change in transit)
        // or the aad (to make sure that decryption will fail if the AAD mismatches).
        if (damage) {
            unsigned damage_bit = provider.ConsumeIntegralInRange<unsigned>(0, (cipher.size() + aad.size()) * 8U - 1U);
            unsigned damage_pos = damage_bit >> 3;
            std::byte damage_val{(uint8_t)(1U << (damage_bit & 7))};
            if (damage_pos >= cipher.size()) {
                aad[damage_pos - cipher.size()] ^= damage_val;
            } else {
                cipher[damage_pos] ^= damage_val;
            }
        }

        crypt_till_rekey(dec_aead, rekey_interval, false);
        if (use_splits && length > 0) {
            size_t split_index = provider.ConsumeIntegralInRange<size_t>(1, length);
            ok = dec_aead.Decrypt(cipher, aad, Span{decrypted_contents}.first(split_index), Span{decrypted_contents}.subspan(split_index));
        } else {
            ok = dec_aead.Decrypt(cipher, aad, decrypted_contents);
        }

        // Decryption *must* fail if the packet was damaged, and succeed if it wasn't.
        assert(!ok == damage);
        if (!ok) break;
        assert(decrypted_contents == plain);
    }
}
