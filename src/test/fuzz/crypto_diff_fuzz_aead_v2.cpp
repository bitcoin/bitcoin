// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/endian.h>
#include <crypto/chacha_poly_aead.h>
#include <crypto/poly1305.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

/**
 * Alternate ChaCha20Forward4064-Poly1305@Bitcoin cipher suite construction
 */

struct AltChaCha20Forward4064 {
    ChaCha20 chacha;
    int iv = 0;
    int keystream_pos = 0;
    unsigned char keystream[4096] = {0};
};

struct AltChaCha20Forward4064Poly1305 {
    struct AltChaCha20Forward4064 F;
    struct AltChaCha20Forward4064 V;
};

void initialise(AltChaCha20Forward4064& instance)
{
    instance.chacha.SetIV(instance.iv);
    instance.chacha.Keystream(instance.keystream, 4096);
}

/**
 * Rekey when keystream_pos=4064
 */
void rekey(AltChaCha20Forward4064& instance)
{
    if (instance.keystream_pos == 4064) {
        instance.chacha.SetKey(instance.keystream + 4064, 32);
        instance.chacha.SetIV(++instance.iv);
        instance.chacha.Keystream(instance.keystream, 4096);
        instance.keystream_pos = 0;
    }
}

/**
 * Encrypting a message
 * Input:  Message m and Payload length bytes
 * Output: Ciphertext c consists of: (1)+(2)+(3)
 */
bool Encryption(const unsigned char* m, unsigned char* c, int bytes, AltChaCha20Forward4064& F, AltChaCha20Forward4064& V)
{
    if (bytes < 0)
        return false;

    int ptr = 0;
    // (1) 3 bytes LE of message m
    for (int i = 0; i < 3; ++i) {
        c[ptr] = F.keystream[F.keystream_pos] ^ m[i];
        ++F.keystream_pos;
        if (F.keystream_pos == 4064) {
            rekey(F);
        }
        ++ptr;
    }

    // (2) encrypted message m
    for (int i = 0; i < bytes; ++i) {
        c[ptr] = V.keystream[V.keystream_pos] ^ m[i + 3];
        ++V.keystream_pos;
        if (V.keystream_pos == 4064) {
            rekey(V);
        }
        ++ptr;
    }

    // (3) 16 byte MAC tag
    unsigned char key[POLY1305_KEYLEN];
    for (int i = 0; i < POLY1305_KEYLEN; ++i) {
        key[i] = F.keystream[F.keystream_pos];
        ++F.keystream_pos;
        if (F.keystream_pos == 4064) {
            rekey(F);
        }
    }
    poly1305_auth(c + bytes + 3, c, bytes + 3, key);
    memory_cleanse(key, POLY1305_KEYLEN);

    return true;
}

/**
 * Decrypting the 3 bytes Payload length
 */
int DecryptionAAD(const unsigned char* c, AltChaCha20Forward4064& F)
{
    uint32_t x;
    unsigned char length[3];
    for (int i = 0; i < 3; ++i) {
        length[i] = F.keystream[F.keystream_pos] ^ c[i];
        ++F.keystream_pos;
        if (F.keystream_pos == 4064) {
            rekey(F);
        }
    }
    memcpy((char*)&x, length, 3);
    return le32toh(x);
}

/**
 * Decrypting a message
 * Input:  Ciphertext c consists of: (1)+(2)+(3) and Payload length bytes
 * Output: Message m
 */
bool Decryption(unsigned char* m, const unsigned char* c, int bytes, AltChaCha20Forward4064& F, AltChaCha20Forward4064& V)
{
    if (bytes < 0)
        return false;

    /// (1) Decrypt first 3 bytes from c as LE of message m. This is done before calling Decryption().
    /// It's necessary since F's keystream is required for decryption of length
    /// and can get lost if rekeying of F happens during poly1305 key generation.
    for (int i = 0; i < 3; ++i) {
        m[i] = c[i];
    }

    // (3) 16 byte MAC tag
    unsigned char out[POLY1305_TAGLEN];
    unsigned char key[POLY1305_KEYLEN];
    for (int i = 0; i < POLY1305_KEYLEN; ++i) {
        key[i] = F.keystream[F.keystream_pos];
        ++F.keystream_pos;
        if (F.keystream_pos == 4064) {
            rekey(F);
        }
    }
    poly1305_auth(out, c, bytes + 3, key);
    if (memcmp(out, c + 3 + bytes, POLY1305_TAGLEN) != 0) return false;
    memory_cleanse(key, POLY1305_KEYLEN);
    memory_cleanse(out, POLY1305_TAGLEN);

    // (2) decrypted message m
    for (int i = 0; i < bytes; ++i) {
        m[i + 3] = V.keystream[V.keystream_pos] ^ c[i + 3];
        ++V.keystream_pos;
        if (V.keystream_pos == 4064) {
            rekey(V);
        }
    }
    return true;
}

FUZZ_TARGET(crypto_diff_fuzz_aead_v2)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> k1 = ConsumeFixedLengthByteVector(fuzzed_data_provider, CHACHA20_POLY1305_AEAD_KEY_LEN);
    const std::vector<uint8_t> k2 = ConsumeFixedLengthByteVector(fuzzed_data_provider, CHACHA20_POLY1305_AEAD_KEY_LEN);

    ChaCha20Poly1305AEAD aead(k1.data(), k1.size(), k2.data(), k2.size());
    ChaCha20 instance1(k1.data(), k1.size()), instance2(k2.data(), k2.size());
    struct AltChaCha20Forward4064 F = {instance1};
    initialise(F);
    struct AltChaCha20Forward4064 V = {instance2};
    initialise(V);
    struct AltChaCha20Forward4064Poly1305 aead2 = {F, V};

    size_t buffer_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096);
    std::vector<uint8_t> in(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN, 0);
    std::vector<uint8_t> out(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    std::vector<uint8_t> in2(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN, 0);
    std::vector<uint8_t> out2(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
    bool is_encrypt = fuzzed_data_provider.ConsumeBool();

    while (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                buffer_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(64, 4096);
                in = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN, 0);
                out = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
                in2 = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN, 0);
                out2 = std::vector<uint8_t>(buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN, 0);
            },
            [&] {
                if (is_encrypt) {
                    bool res = aead.Crypt(out.data(), out.size(), in.data(), buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN, is_encrypt);
                    bool res1 = Encryption(in2.data(), out2.data(), buffer_size, aead2.F, aead2.V);
                    assert(res == res1);
                    if (res) assert(memcmp(out.data(), out2.data(), buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN) == 0);
                } else {
                    uint32_t len = aead.DecryptLength(out.data());
                    uint32_t len1 = DecryptionAAD(out2.data(), aead2.F);
                    assert(len == len1);
                    bool res = aead.Crypt(in.data(), buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN, out.data(), out.size(), is_encrypt);
                    bool res1 = Decryption(in2.data(), out2.data(), buffer_size, aead2.F, aead2.V);
                    assert(res == res1);
                    if (res) assert(memcmp(in.data(), in2.data(), buffer_size + CHACHA20_POLY1305_AEAD_AAD_LEN) == 0);
                }
            },
            [&] {
                is_encrypt = fuzzed_data_provider.ConsumeBool();
            });
    }
}