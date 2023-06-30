// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20POLY1305_H
#define BITCOIN_CRYPTO_CHACHA20POLY1305_H

#include <cstddef>
#include <cstdlib>
#include <stdint.h>

#include <crypto/chacha20.h>
#include <crypto/poly1305.h>
#include <span.h>

/** The AEAD_CHACHA20_POLY1305 authenticated encryption algorithm from RFC8439 section 2.8. */
class AEADChaCha20Poly1305
{
    /** Internal stream cipher. */
    ChaCha20 m_chacha20;

public:
    /** Expected size of key argument in constructor. */
    static constexpr unsigned KEYLEN = 32;

    /** Expansion when encrypting. */
    static constexpr unsigned EXPANSION = Poly1305::TAGLEN;

    /** Initialize an AEAD instance with a specified 32-byte key. */
    AEADChaCha20Poly1305(Span<const std::byte> key) noexcept;

    /** Switch to another 32-byte key. */
    void SetKey(Span<const std::byte> key) noexcept;

    /** 96-bit nonce type. */
    using Nonce96 = ChaCha20::Nonce96;

    /** Encrypt a message with a specified 96-bit nonce and aad.
     *
     * Requires cipher.size() = plain.size() + EXPANSION.
     */
    void Encrypt(Span<const std::byte> plain, Span<const std::byte> aad, Nonce96 nonce, Span<std::byte> cipher) noexcept;

    /** Decrypt a message with a specified 96-bit nonce and aad. Returns true if valid.
     *
     * Requires cipher.size() = plain.size() + EXPANSION.
     */
    bool Decrypt(Span<const std::byte> cipher, Span<const std::byte> aad, Nonce96 nonce, Span<std::byte> plain) noexcept;
};

#endif // BITCOIN_CRYPTO_CHACHA20POLY1305_H
