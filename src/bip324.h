// Copyright (c) 2023 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_BIP324_H
#define TORTOISECOIN_BIP324_H

#include <array>
#include <cstddef>
#include <optional>

#include <crypto/chacha20.h>
#include <crypto/chacha20poly1305.h>
#include <key.h>
#include <pubkey.h>
#include <span.h>

/** The BIP324 packet cipher, encapsulating its key derivation, stream cipher, and AEAD. */
class BIP324Cipher
{
public:
    static constexpr unsigned SESSION_ID_LEN{32};
    static constexpr unsigned GARBAGE_TERMINATOR_LEN{16};
    static constexpr unsigned REKEY_INTERVAL{224};
    static constexpr unsigned LENGTH_LEN{3};
    static constexpr unsigned HEADER_LEN{1};
    static constexpr unsigned EXPANSION = LENGTH_LEN + HEADER_LEN + FSChaCha20Poly1305::EXPANSION;
    static constexpr std::byte IGNORE_BIT{0x80};

private:
    std::optional<FSChaCha20> m_send_l_cipher;
    std::optional<FSChaCha20> m_recv_l_cipher;
    std::optional<FSChaCha20Poly1305> m_send_p_cipher;
    std::optional<FSChaCha20Poly1305> m_recv_p_cipher;

    CKey m_key;
    EllSwiftPubKey m_our_pubkey;

    std::array<std::byte, SESSION_ID_LEN> m_session_id;
    std::array<std::byte, GARBAGE_TERMINATOR_LEN> m_send_garbage_terminator;
    std::array<std::byte, GARBAGE_TERMINATOR_LEN> m_recv_garbage_terminator;

public:
    /** No default constructor; keys must be provided to create a BIP324Cipher. */
    BIP324Cipher() = delete;

    /** Initialize a BIP324 cipher with specified key and encoding entropy (testing only). */
    BIP324Cipher(const CKey& key, Span<const std::byte> ent32) noexcept;

    /** Initialize a BIP324 cipher with specified key (testing only). */
    BIP324Cipher(const CKey& key, const EllSwiftPubKey& pubkey) noexcept;

    /** Retrieve our public key. */
    const EllSwiftPubKey& GetOurPubKey() const noexcept { return m_our_pubkey; }

    /** Initialize when the other side's public key is received. Can only be called once.
     *
     * initiator is set to true if we are the initiator establishing the v2 P2P connection.
     * self_decrypt is only for testing, and swaps encryption/decryption keys, so that encryption
     * and decryption can be tested without knowing the other side's private key.
     */
    void Initialize(const EllSwiftPubKey& their_pubkey, bool initiator, bool self_decrypt = false) noexcept;

    /** Determine whether this cipher is fully initialized. */
    explicit operator bool() const noexcept { return m_send_l_cipher.has_value(); }

    /** Encrypt a packet. Only after Initialize().
     *
     * It must hold that output.size() == contents.size() + EXPANSION.
     */
    void Encrypt(Span<const std::byte> contents, Span<const std::byte> aad, bool ignore, Span<std::byte> output) noexcept;

    /** Decrypt the length of a packet. Only after Initialize().
     *
     * It must hold that input.size() == LENGTH_LEN.
     */
    unsigned DecryptLength(Span<const std::byte> input) noexcept;

    /** Decrypt a packet. Only after Initialize().
     *
     * It must hold that input.size() + LENGTH_LEN == contents.size() + EXPANSION.
     * Contents.size() must equal the length returned by DecryptLength.
     */
    bool Decrypt(Span<const std::byte> input, Span<const std::byte> aad, bool& ignore, Span<std::byte> contents) noexcept;

    /** Get the Session ID. Only after Initialize(). */
    Span<const std::byte> GetSessionID() const noexcept { return m_session_id; }

    /** Get the Garbage Terminator to send. Only after Initialize(). */
    Span<const std::byte> GetSendGarbageTerminator() const noexcept { return m_send_garbage_terminator; }

    /** Get the expected Garbage Terminator to receive. Only after Initialize(). */
    Span<const std::byte> GetReceiveGarbageTerminator() const noexcept { return m_recv_garbage_terminator; }
};

#endif // TORTOISECOIN_BIP324_H
