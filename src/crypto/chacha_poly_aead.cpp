// Copyright (c) 2019 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha_poly_aead.h>

#include <crypto/poly1305.h>
#include <support/cleanse.h>

#include <assert.h>
#include <string.h>

#include <cstdio>
#include <limits>

#ifndef HAVE_TIMINGSAFE_BCMP

int timingsafe_bcmp(const unsigned char* b1, const unsigned char* b2, size_t n)
{
    const unsigned char *p1 = b1, *p2 = b2;
    int ret = 0;

    for (; n > 0; n--)
        ret |= *p1++ ^ *p2++;
    return (ret != 0);
}

#endif // TIMINGSAFE_BCMP

ChaCha20Poly1305AEAD::ChaCha20Poly1305AEAD(const unsigned char* K_1, size_t K_1_len, const unsigned char* K_2, size_t K_2_len)
{
    assert(K_1_len == CHACHA20_POLY1305_AEAD_KEY_LEN);
    assert(K_2_len == CHACHA20_POLY1305_AEAD_KEY_LEN);
    m_chacha_main.SetKey(K_1, CHACHA20_POLY1305_AEAD_KEY_LEN);
    m_chacha_header.SetKey(K_2, CHACHA20_POLY1305_AEAD_KEY_LEN);

    // set the cached sequence number to uint64 max which hints for an unset cache.
    // we can't hit uint64 max since the rekey rule (which resets the sequence number) is 1GB
    m_cached_aad_seqnr = std::numeric_limits<uint64_t>::max();
}

bool ChaCha20Poly1305AEAD::Crypt(uint64_t seqnr_payload, uint64_t seqnr_aad, int aad_pos, unsigned char* dest, size_t dest_len /* length of the output buffer for sanity checks */, const unsigned char* src, size_t src_len, bool is_encrypt)
{
    // check buffer boundaries
    if (
        // if we encrypt, make sure the source contains at least the expected AAD and the destination has at least space for the source + MAC
        (is_encrypt && (src_len < CHACHA20_POLY1305_AEAD_AAD_LEN || dest_len < src_len + POLY1305_TAGLEN)) ||
        // if we decrypt, make sure the source contains at least the expected AAD+MAC and the destination has at least space for the source - MAC
        (!is_encrypt && (src_len < CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN || dest_len < src_len - POLY1305_TAGLEN))) {
        return false;
    }

    unsigned char expected_tag[POLY1305_TAGLEN], poly_key[POLY1305_KEYLEN];
    memset(poly_key, 0, sizeof(poly_key));
    m_chacha_main.SetIV(seqnr_payload);

    // block counter 0 for the poly1305 key
    // use lower 32bytes for the poly1305 key
    // (throws away 32 unused bytes (upper 32) from this ChaCha20 round)
    m_chacha_main.Seek(0);
    m_chacha_main.Crypt(poly_key, poly_key, sizeof(poly_key));

    // if decrypting, verify the tag prior to decryption
    if (!is_encrypt) {
        const unsigned char* tag = src + src_len - POLY1305_TAGLEN;
        poly1305_auth(expected_tag, src, src_len - POLY1305_TAGLEN, poly_key);

        // constant time compare the calculated MAC with the provided MAC
        if (timingsafe_bcmp(expected_tag, tag, POLY1305_TAGLEN) != 0) {
            memory_cleanse(expected_tag, sizeof(expected_tag));
            memory_cleanse(poly_key, sizeof(poly_key));
            return false;
        }
        memory_cleanse(expected_tag, sizeof(expected_tag));
        // MAC has been successfully verified, make sure we don't covert it in decryption
        src_len -= POLY1305_TAGLEN;
    }

    // calculate and cache the next 64byte keystream block if requested sequence number is not yet the cache
    if (m_cached_aad_seqnr != seqnr_aad) {
        m_cached_aad_seqnr = seqnr_aad;
        m_chacha_header.SetIV(seqnr_aad);
        m_chacha_header.Seek(0);
        m_chacha_header.Keystream(m_aad_keystream_buffer, CHACHA20_ROUND_OUTPUT);
    }
    // crypt the AAD (3 bytes message length) with given position in AAD cipher instance keystream
    dest[0] = src[0] ^ m_aad_keystream_buffer[aad_pos];
    dest[1] = src[1] ^ m_aad_keystream_buffer[aad_pos + 1];
    dest[2] = src[2] ^ m_aad_keystream_buffer[aad_pos + 2];

    // Set the playload ChaCha instance block counter to 1 and crypt the payload
    m_chacha_main.Seek(1);
    m_chacha_main.Crypt(src + CHACHA20_POLY1305_AEAD_AAD_LEN, dest + CHACHA20_POLY1305_AEAD_AAD_LEN, src_len - CHACHA20_POLY1305_AEAD_AAD_LEN);

    // If encrypting, calculate and append tag
    if (is_encrypt) {
        // the poly1305 tag expands over the AAD (3 bytes length) & encrypted payload
        poly1305_auth(dest + src_len, dest, src_len, poly_key);
    }

    // cleanse no longer required MAC and polykey
    memory_cleanse(poly_key, sizeof(poly_key));
    return true;
}

bool ChaCha20Poly1305AEAD::GetLength(uint32_t* len24_out, uint64_t seqnr_aad, int aad_pos, const uint8_t* ciphertext)
{
    // enforce valid aad position to avoid accessing outside of the 64byte keystream cache
    // (there is space for 21 times 3 bytes)
    assert(aad_pos >= 0 && aad_pos < CHACHA20_ROUND_OUTPUT - CHACHA20_POLY1305_AEAD_AAD_LEN);
    if (m_cached_aad_seqnr != seqnr_aad) {
        // we need to calculate the 64 keystream bytes since we reached a new aad sequence number
        m_cached_aad_seqnr = seqnr_aad;
        m_chacha_header.SetIV(seqnr_aad);                                         // use LE for the nonce
        m_chacha_header.Seek(0);                                                  // block counter 0
        m_chacha_header.Keystream(m_aad_keystream_buffer, CHACHA20_ROUND_OUTPUT); // write keystream to the cache
    }

    // decrypt the ciphertext length by XORing the right position of the 64byte keystream cache with the ciphertext
    *len24_out = (ciphertext[0] ^ m_aad_keystream_buffer[aad_pos + 0]) |
                 (ciphertext[1] ^ m_aad_keystream_buffer[aad_pos + 1]) << 8 |
                 (ciphertext[2] ^ m_aad_keystream_buffer[aad_pos + 2]) << 16;

    return true;
}
