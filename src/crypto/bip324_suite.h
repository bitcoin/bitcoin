// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_BIP324_SUITE_H
#define BITCOIN_CRYPTO_BIP324_SUITE_H

#include <crypto/chacha20.h>
#include <crypto/rfc8439.h>
#include <span.h>

#include <array>
#include <cstddef>

static constexpr size_t BIP324_KEY_LEN = 32;         // bytes
static constexpr size_t BIP324_HEADER_LEN = 1;       // bytes
static constexpr size_t BIP324_LENGTH_FIELD_LEN = 3; // bytes
static constexpr size_t REKEY_INTERVAL = 224;        // packets
static constexpr size_t NONCE_LENGTH = 12;           // bytes

enum BIP324HeaderFlags : uint8_t {
    BIP324_NONE = 0,
    BIP324_IGNORE = (1 << 7),
};

using BIP324Key = std::array<std::byte, BIP324_KEY_LEN>;

class BIP324CipherSuite
{
private:
    FSChaCha20 fsc20;
    uint32_t packet_counter{0};
    BIP324Key key_P;
    std::array<std::byte, NONCE_LENGTH> nonce;

    void set_nonce()
    {
        WriteLE32(reinterpret_cast<unsigned char*>(nonce.data()), packet_counter % REKEY_INTERVAL);
        WriteLE64(reinterpret_cast<unsigned char*>(nonce.data()) + 4, packet_counter / REKEY_INTERVAL);
    }

    void Rekey();

public:
    BIP324CipherSuite(const BIP324Key& K_L, const BIP324Key& K_P)
        : fsc20{K_L, REKEY_INTERVAL},
          key_P{K_P}
    {
        set_nonce();
    };

    explicit BIP324CipherSuite(const BIP324CipherSuite&) = delete;
    ~BIP324CipherSuite();

    /** Encrypts/decrypts a packet
        input, contents to encrypt or the encrypted header + encrypted contents + MAC to decrypt (encrypted length is decrypted using DecryptLength() prior to calling Crypt()
        encrypt, set to true if we encrypt, false to decrypt.

        Returns true upon success. Upon failure, the output should not be used.
        */
    [[nodiscard]] bool Crypt(const Span<const std::byte> aad,
                             const Span<const std::byte> input,
                             Span<std::byte> output,
                             BIP324HeaderFlags& flags, bool encrypt);

    /** Decrypts the 3 byte encrypted length field (the packet header and contents length) and decodes it into a uint32_t field
        The FSChaCha20 keystream will advance. As a result, DecryptLength() cannot be called multiple times to get the same result. The caller must cache the result for re-use.
        */
    [[nodiscard]] uint32_t DecryptLength(const std::array<std::byte, BIP324_LENGTH_FIELD_LEN>& encrypted_length);
};

#endif // BITCOIN_CRYPTO_BIP324_SUITE_H
