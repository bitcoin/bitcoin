// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20poly1305.h>

#include <crypto/common.h>
#include <crypto/chacha20.h>
#include <crypto/poly1305.h>
#include <span.h>
#include <support/cleanse.h>

#include <assert.h>
#include <cstddef>

AEADChaCha20Poly1305::AEADChaCha20Poly1305(std::span<const std::byte> key) noexcept : m_chacha20(key)
{
    assert(key.size() == KEYLEN);
}

void AEADChaCha20Poly1305::SetKey(std::span<const std::byte> key) noexcept
{
    assert(key.size() == KEYLEN);
    m_chacha20.SetKey(key);
}

namespace {

int timingsafe_bcmp_internal(const unsigned char* b1, const unsigned char* b2, size_t n) noexcept
{
    const unsigned char *p1 = b1, *p2 = b2;
    int ret = 0;
    for (; n > 0; n--)
        ret |= *p1++ ^ *p2++;
    return (ret != 0);
}

/** Compute poly1305 tag. chacha20 must be set to the right nonce, block 0. Will be at block 1 after. */
void ComputeTag(ChaCha20& chacha20, std::span<const std::byte> aad, std::span<const std::byte> cipher, std::span<std::byte> tag) noexcept
{
    static const std::byte PADDING[16] = {{}};

    // Get block of keystream (use a full 64 byte buffer to avoid the need for chacha20's own buffering).
    std::byte first_block[ChaCha20Aligned::BLOCKLEN];
    chacha20.Keystream(first_block);

    // Use the first 32 bytes of the first keystream block as poly1305 key.
    Poly1305 poly1305{std::span{first_block}.first(Poly1305::KEYLEN)};

    // Compute tag:
    // - Process the padded AAD with Poly1305.
    const unsigned aad_padding_length = (16 - (aad.size() % 16)) % 16;
    poly1305.Update(aad).Update(std::span{PADDING}.first(aad_padding_length));
    // - Process the padded ciphertext with Poly1305.
    const unsigned cipher_padding_length = (16 - (cipher.size() % 16)) % 16;
    poly1305.Update(cipher).Update(std::span{PADDING}.first(cipher_padding_length));
    // - Process the AAD and plaintext length with Poly1305.
    std::byte length_desc[Poly1305::TAGLEN];
    WriteLE64(length_desc, aad.size());
    WriteLE64(length_desc + 8, cipher.size());
    poly1305.Update(length_desc);

    // Output tag.
    poly1305.Finalize(tag);
}

} // namespace

void AEADChaCha20Poly1305::Encrypt(std::span<const std::byte> plain1, std::span<const std::byte> plain2, std::span<const std::byte> aad, Nonce96 nonce, std::span<std::byte> cipher) noexcept
{
    assert(cipher.size() == plain1.size() + plain2.size() + EXPANSION);

    // Encrypt using ChaCha20 (starting at block 1).
    m_chacha20.Seek(nonce, 1);
    m_chacha20.Crypt(plain1, cipher.first(plain1.size()));
    m_chacha20.Crypt(plain2, cipher.subspan(plain1.size()).first(plain2.size()));

    // Seek to block 0, and compute tag using key drawn from there.
    m_chacha20.Seek(nonce, 0);
    ComputeTag(m_chacha20, aad, cipher.first(cipher.size() - EXPANSION), cipher.last(EXPANSION));
}

bool AEADChaCha20Poly1305::Decrypt(std::span<const std::byte> cipher, std::span<const std::byte> aad, Nonce96 nonce, std::span<std::byte> plain1, std::span<std::byte> plain2) noexcept
{
    assert(cipher.size() == plain1.size() + plain2.size() + EXPANSION);

    // Verify tag (using key drawn from block 0).
    m_chacha20.Seek(nonce, 0);
    std::byte expected_tag[EXPANSION];
    ComputeTag(m_chacha20, aad, cipher.first(cipher.size() - EXPANSION), expected_tag);
    if (timingsafe_bcmp_internal(UCharCast(expected_tag), UCharCast(cipher.last(EXPANSION).data()), EXPANSION)) return false;

    // Decrypt (starting at block 1).
    m_chacha20.Crypt(cipher.first(plain1.size()), plain1);
    m_chacha20.Crypt(cipher.subspan(plain1.size()).first(plain2.size()), plain2);
    return true;
}

void AEADChaCha20Poly1305::Keystream(Nonce96 nonce, std::span<std::byte> keystream) noexcept
{
    // Skip the first output block, as it's used for generating the poly1305 key.
    m_chacha20.Seek(nonce, 1);
    m_chacha20.Keystream(keystream);
}

void FSChaCha20Poly1305::NextPacket() noexcept
{
    if (++m_packet_counter == m_rekey_interval) {
        // Generate a full block of keystream, to avoid needing the ChaCha20 buffer, even though
        // we only need KEYLEN (32) bytes.
        std::byte one_block[ChaCha20Aligned::BLOCKLEN];
        m_aead.Keystream({0xFFFFFFFF, m_rekey_counter}, one_block);
        // Switch keys.
        m_aead.SetKey(std::span{one_block}.first(KEYLEN));
        // Wipe the generated keystream (a copy remains inside m_aead, which will be cleaned up
        // once it cycles again, or is destroyed).
        memory_cleanse(one_block, sizeof(one_block));
        // Update counters.
        m_packet_counter = 0;
        ++m_rekey_counter;
    }
}

void FSChaCha20Poly1305::Encrypt(std::span<const std::byte> plain1, std::span<const std::byte> plain2, std::span<const std::byte> aad, std::span<std::byte> cipher) noexcept
{
    m_aead.Encrypt(plain1, plain2, aad, {m_packet_counter, m_rekey_counter}, cipher);
    NextPacket();
}

bool FSChaCha20Poly1305::Decrypt(std::span<const std::byte> cipher, std::span<const std::byte> aad, std::span<std::byte> plain1, std::span<std::byte> plain2) noexcept
{
    bool ret = m_aead.Decrypt(cipher, aad, {m_packet_counter, m_rekey_counter}, plain1, plain2);
    NextPacket();
    return ret;
}
