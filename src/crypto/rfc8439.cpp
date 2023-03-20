// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/rfc8439.h>

#include <crypto/chacha20.h>
#include <crypto/common.h>

#include <cstring>

#ifndef HAVE_TIMINGSAFE_BCMP
#define HAVE_TIMINGSAFE_BCMP

int timingsafe_bcmp(const unsigned char* b1, const unsigned char* b2, size_t n)
{
    const unsigned char *p1 = b1, *p2 = b2;
    int ret = 0;

    for (; n > 0; n--)
        ret |= *p1++ ^ *p2++;
    return (ret != 0);
}

#endif // TIMINGSAFE_BCMP

inline size_t padded16_size(size_t len)
{
    return (len % 16 == 0) ? len : (len / 16 + 1) * 16;
}

void ComputeRFC8439Tag(const std::array<std::byte, POLY1305_KEYLEN>& polykey,
                       Span<const std::byte> aad, Span<const std::byte> ciphertext,
                       Span<std::byte> tag_out)
{
    assert(tag_out.size() == POLY1305_TAGLEN);
    std::vector<std::byte> bytes_to_authenticate;
    auto padded_aad_size = padded16_size(aad.size());
    auto padded_ciphertext_size = padded16_size(ciphertext.size());
    bytes_to_authenticate.resize(padded_aad_size + padded_ciphertext_size + 16, std::byte{0x00});
    std::copy(aad.begin(), aad.end(), bytes_to_authenticate.begin());
    std::copy(ciphertext.begin(), ciphertext.end(), bytes_to_authenticate.begin() + padded_aad_size);
    WriteLE64(reinterpret_cast<unsigned char*>(bytes_to_authenticate.data()) + padded_aad_size + padded_ciphertext_size, aad.size());
    WriteLE64(reinterpret_cast<unsigned char*>(bytes_to_authenticate.data()) + padded_aad_size + padded_ciphertext_size + 8, ciphertext.size());

    poly1305_auth(reinterpret_cast<unsigned char*>(tag_out.data()),
                  reinterpret_cast<const unsigned char*>(bytes_to_authenticate.data()),
                  bytes_to_authenticate.size(),
                  reinterpret_cast<const unsigned char*>(polykey.data()));
}

std::array<std::byte, POLY1305_KEYLEN> GetPoly1305Key(ChaCha20& c20)
{
    c20.SeekRFC8439(0);
    std::array<std::byte, POLY1305_KEYLEN> polykey;
    c20.Keystream(reinterpret_cast<unsigned char*>(polykey.data()), POLY1305_KEYLEN);
    return polykey;
}

void RFC8439Crypt(ChaCha20& c20, const Span<const std::byte> in_bytes, Span<std::byte> out_bytes)
{
    assert(in_bytes.size() <= out_bytes.size());
    c20.SeekRFC8439(1);
    c20.Crypt(reinterpret_cast<const unsigned char*>(in_bytes.data()), reinterpret_cast<unsigned char*>(out_bytes.data()), in_bytes.size());
}

void RFC8439Encrypt(const Span<const std::byte> aad, const Span<const std::byte> key, const std::array<std::byte, 12>& nonce, const Span<const std::byte> plaintext, Span<std::byte> output)
{
    assert(key.size() == RFC8439_KEYLEN);
    assert(output.size() >= plaintext.size() + POLY1305_TAGLEN);

    ChaCha20 c20{reinterpret_cast<const unsigned char*>(key.data())};
    c20.SetRFC8439Nonce(nonce);

    std::array<std::byte, POLY1305_KEYLEN> polykey{GetPoly1305Key(c20)};

    RFC8439Crypt(c20, plaintext, output);
    ComputeRFC8439Tag(polykey, aad,
                      {output.data(), plaintext.size()},
                      {output.data() + plaintext.size(), POLY1305_TAGLEN});
}

bool RFC8439Decrypt(const Span<const std::byte> aad, const Span<const std::byte> key, const std::array<std::byte, 12>& nonce, const Span<const std::byte> input, Span<std::byte> plaintext)
{
    assert(key.size() == RFC8439_KEYLEN);
    assert(plaintext.size() >= input.size() - POLY1305_TAGLEN);

    ChaCha20 c20{reinterpret_cast<const unsigned char*>(key.data())};
    c20.SetRFC8439Nonce(nonce);

    std::array<std::byte, POLY1305_KEYLEN> polykey{GetPoly1305Key(c20)};
    std::array<std::byte, POLY1305_TAGLEN> tag;

    ComputeRFC8439Tag(polykey, aad, {input.data(), input.size() - POLY1305_TAGLEN}, tag);

    if (timingsafe_bcmp(reinterpret_cast<const unsigned char*>(input.data() + input.size() - POLY1305_TAGLEN),
                                reinterpret_cast<const unsigned char*>(tag.data()), POLY1305_TAGLEN) != 0) {
        return false;
    }

    RFC8439Crypt(c20, {input.data(), input.size() - POLY1305_TAGLEN}, plaintext);
    return true;
}
