// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20poly1305.h>

#include <crypto/common.h>
#include <crypto/chacha20.h>
#include <crypto/poly1305.h>
#include <span.h>

#include <assert.h>
#include <cstdint>
#include <cstddef>
#include <iterator>

AEADChaCha20Poly1305::AEADChaCha20Poly1305(Span<const std::byte> key) noexcept : m_chacha20(UCharCast(key.data()))
{
    assert(key.size() == KEYLEN);
}

void AEADChaCha20Poly1305::SetKey(Span<const std::byte> key) noexcept
{
    assert(key.size() == KEYLEN);
    m_chacha20.SetKey32(UCharCast(key.data()));
}

namespace {

#ifndef HAVE_TIMINGSAFE_BCMP
#define HAVE_TIMINGSAFE_BCMP

int timingsafe_bcmp(const unsigned char* b1, const unsigned char* b2, size_t n) noexcept
{
    const unsigned char *p1 = b1, *p2 = b2;
    int ret = 0;
    for (; n > 0; n--)
        ret |= *p1++ ^ *p2++;
    return (ret != 0);
}

#endif

/** Compute poly1305 tag. chacha20 must be set to the right nonce, block 0. Will be at block 1 after. */
void ComputeTag(ChaCha20& chacha20, Span<const std::byte> aad, Span<const std::byte> cipher, Span<std::byte> tag) noexcept
{
    static const std::byte PADDING[16] = {{}};

    // Get block of keystream (use a full 64 byte buffer to avoid the need for chacha20's own buffering).
    std::byte first_block[64];
    chacha20.Keystream(UCharCast(first_block), sizeof(first_block));

    // Use the first 32 bytes of the first keystream block as poly1305 key.
    Poly1305 poly1305{Span{first_block}.first(Poly1305::KEYLEN)};

    // Compute tag:
    // - Process the padded AAD with Poly1305.
    const unsigned aad_padding_length = (16 - (aad.size() % 16)) % 16;
    poly1305.Update(aad).Update(Span{PADDING}.first(aad_padding_length));
    // - Process the padded ciphertext with Poly1305.
    const unsigned cipher_padding_length = (16 - (cipher.size() % 16)) % 16;
    poly1305.Update(cipher).Update(Span{PADDING}.first(cipher_padding_length));
    // - Process the AAD and plaintext length with Poly1305.
    std::byte length_desc[Poly1305::TAGLEN];
    WriteLE64(UCharCast(length_desc), aad.size());
    WriteLE64(UCharCast(length_desc + 8), cipher.size());
    poly1305.Update(length_desc);

    // Output tag.
    poly1305.Finalize(tag);
}

} // namespace

void AEADChaCha20Poly1305::Encrypt(Span<const std::byte> plain, Span<const std::byte> aad, Nonce96 nonce, Span<std::byte> cipher) noexcept
{
    assert(cipher.size() == plain.size() + EXPANSION);

    // Encrypt using ChaCha20 (starting at block 1).
    m_chacha20.Seek64(nonce, 1);
    m_chacha20.Crypt(UCharCast(plain.data()), UCharCast(cipher.data()), plain.size());

    // Seek to block 0, and compute tag using key drawn from there.
    m_chacha20.Seek64(nonce, 0);
    ComputeTag(m_chacha20, aad, cipher.first(plain.size()), cipher.last(EXPANSION));
}

bool AEADChaCha20Poly1305::Decrypt(Span<const std::byte> cipher, Span<const std::byte> aad, Nonce96 nonce, Span<std::byte> plain) noexcept
{
    assert(cipher.size() == plain.size() + EXPANSION);

    // Verify tag (using key drawn from block 0).
    m_chacha20.Seek64(nonce, 0);
    std::byte expected_tag[EXPANSION];
    ComputeTag(m_chacha20, aad, cipher.first(plain.size()), expected_tag);
    if (timingsafe_bcmp(UCharCast(expected_tag), UCharCast(cipher.data() + plain.size()), EXPANSION)) return false;

    // Decrypt (starting at block 1).
    m_chacha20.Crypt(UCharCast(cipher.data()), UCharCast(plain.data()), plain.size());
    return true;
}
