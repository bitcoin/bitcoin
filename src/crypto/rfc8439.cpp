// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20.h>
#include <crypto/common.h>
#include <crypto/rfc8439.h>

#include <cstring>

#ifndef RFC8439_TIMINGSAFE_BCMP
#define RFC8439_TIMINGSAFE_BCMP

int rfc8439_timingsafe_bcmp(const unsigned char* b1, const unsigned char* b2, size_t n)
{
    const unsigned char *p1 = b1, *p2 = b2;
    int ret = 0;

    for (; n > 0; n--)
        ret |= *p1++ ^ *p2++;
    return (ret != 0);
}

#endif // RFC8439_TIMINGSAFE_BCMP

inline size_t padded16_size(size_t len)
{
    return (len % 16 == 0) ? len : (len / 16 + 1) * 16;
}

std::array<std::byte, POLY1305_TAGLEN> ComputeRFC8439Tag(const std::array<std::byte, POLY1305_KEYLEN>& polykey,
                                                         Span<const std::byte> aad, Span<const std::byte> ciphertext)
{
    std::vector<std::byte> bytes_to_authenticate;
    auto padded_aad_size = padded16_size(aad.size());
    auto padded_ciphertext_size = padded16_size(ciphertext.size());
    bytes_to_authenticate.resize(padded_aad_size + padded_ciphertext_size + 16, std::byte{0x00});
    std::copy(aad.begin(), aad.end(), bytes_to_authenticate.begin());
    std::copy(ciphertext.begin(), ciphertext.end(), bytes_to_authenticate.begin() + padded_aad_size);
    WriteLE64(reinterpret_cast<unsigned char*>(bytes_to_authenticate.data()) + padded_aad_size + padded_ciphertext_size, aad.size());
    WriteLE64(reinterpret_cast<unsigned char*>(bytes_to_authenticate.data()) + padded_aad_size + padded_ciphertext_size + 8, ciphertext.size());

    std::array<std::byte, POLY1305_TAGLEN> ret_tag;
    poly1305_auth(reinterpret_cast<unsigned char*>(ret_tag.data()),
                  reinterpret_cast<const unsigned char*>(bytes_to_authenticate.data()),
                  bytes_to_authenticate.size(),
                  reinterpret_cast<const unsigned char*>(polykey.data()));

    return ret_tag;
}

std::array<std::byte, POLY1305_KEYLEN> GetPoly1305Key(ChaCha20& c20)
{
    c20.SeekRFC8439(0);
    std::array<std::byte, POLY1305_KEYLEN> polykey;
    c20.Keystream(reinterpret_cast<unsigned char*>(polykey.data()), POLY1305_KEYLEN);
    return polykey;
}

void RFC8439Crypt(ChaCha20& c20, Span<const std::byte> in_bytes, Span<std::byte> out_bytes)
{
    assert(in_bytes.size() == out_bytes.size());
    c20.SeekRFC8439(1);
    c20.Crypt(reinterpret_cast<const unsigned char*>(in_bytes.data()), reinterpret_cast<unsigned char*>(out_bytes.data()), in_bytes.size());
}

RFC8439Encrypted RFC8439Encrypt(Span<const std::byte> aad, Span<const std::byte> key, const std::array<std::byte, 12>& nonce, Span<const std::byte> plaintext)
{
    assert(key.size() == RFC8439_KEYLEN);
    RFC8439Encrypted ret;

    ChaCha20 c20{reinterpret_cast<const unsigned char*>(key.data()), key.size()};
    c20.SetRFC8439Nonce(nonce);

    std::array<std::byte, POLY1305_KEYLEN> polykey{GetPoly1305Key(c20)};

    ret.ciphertext.resize(plaintext.size());
    RFC8439Crypt(c20, plaintext, ret.ciphertext);
    ret.tag = ComputeRFC8439Tag(polykey, aad, ret.ciphertext);
    return ret;
}

RFC8439Decrypted RFC8439Decrypt(Span<const std::byte> aad, Span<const std::byte> key, const std::array<std::byte, 12>& nonce, const RFC8439Encrypted& encrypted)
{
    assert(key.size() == RFC8439_KEYLEN);
    assert(encrypted.tag.size() == POLY1305_TAGLEN);

    RFC8439Decrypted ret;

    ChaCha20 c20{reinterpret_cast<const unsigned char*>(key.data()), key.size()};
    c20.SetRFC8439Nonce(nonce);

    std::array<std::byte, POLY1305_KEYLEN> polykey{GetPoly1305Key(c20)};
    auto tag = ComputeRFC8439Tag(polykey, aad, encrypted.ciphertext);

    if (rfc8439_timingsafe_bcmp(reinterpret_cast<const unsigned char*>(encrypted.tag.data()),
                                reinterpret_cast<const unsigned char*>(tag.data()), encrypted.tag.size()) != 0) {
        ret.success = false;
        return ret;
    }

    ret.success = true;
    ret.plaintext.resize(encrypted.ciphertext.size());
    RFC8439Crypt(c20, encrypted.ciphertext, ret.plaintext);
    return ret;
}
