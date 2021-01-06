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
static constexpr size_t REKEY_INTERVAL = 256;        // messages
static constexpr size_t NONCE_LENGTH = 12;           // bytes

enum BIP324HeaderFlags : uint8_t {
    BIP324_IGNORE = (1 << 7),
};

class BIP324CipherSuite
{
private:
    FSChaCha20 fsc20;
    uint64_t rekey_ctr;
    uint32_t msg_ctr;
    std::array<std::byte, BIP324_KEY_LEN> payload_key;
    std::array<std::byte, BIP324_KEY_LEN> rekey_salt;
    std::array<std::byte, 12> nonce;

    void set_nonce()
    {
        WriteLE32(reinterpret_cast<unsigned char*>(nonce.data()), msg_ctr);
        WriteLE64(reinterpret_cast<unsigned char*>(nonce.data()) + 4, rekey_ctr);
    }

public:
    BIP324CipherSuite(const std::array<std::byte, BIP324_KEY_LEN>& K_L,
                      const std::array<std::byte, BIP324_KEY_LEN>& K_P,
                      const std::array<std::byte, BIP324_KEY_LEN>& rekey_salt)
        : fsc20{K_L, rekey_salt, REKEY_INTERVAL},
          rekey_ctr{0},
          msg_ctr{0},
          payload_key{K_P},
          rekey_salt{rekey_salt}
    {
        set_nonce();
    };

    explicit BIP324CipherSuite(const BIP324CipherSuite&) = delete;
    ~BIP324CipherSuite();

    /** Encrypts/decrypts a packet
        input, payload to encrypt or the encrypted header + encrypted payload + MAC to decrypt (encrypted length is decrypted using DecryptLength() prior to calling Crypt()
        encrypt, set to true if we encrypt, false to decrypt.

        Returns true upon success. Upon failure, the output should not be used.
        */
    [[nodiscard]] bool Crypt(Span<const std::byte> input, Span<std::byte> output, BIP324HeaderFlags& flags, bool encrypt);

    /** Decrypts the 3 byte length field ciphertext (the packet length) and decodes it into a uint32_t field
        The FSChaCha20 keystream will advance. As a result, DecryptLength() cannot be called multiple times to get the same result. The caller must cache the result for re-use.
        */
    [[nodiscard]] uint32_t DecryptLength(const std::array<std::byte, BIP324_LENGTH_FIELD_LEN>& ciphertext);
};

#endif // BITCOIN_CRYPTO_BIP324_SUITE_H
