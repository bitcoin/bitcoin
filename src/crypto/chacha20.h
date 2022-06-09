// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_H
#define BITCOIN_CRYPTO_CHACHA20_H

#include <crypto/common.h>
#include <span.h>

#include <array>
#include <cstddef>
#include <stdint.h>
#include <stdlib.h>

static constexpr size_t FSCHACHA20_KEYLEN = 32;

/** A class for ChaCha20 256-bit stream cipher developed by Daniel J. Bernstein
    https://cr.yp.to/chacha/chacha-20080128.pdf */
class ChaCha20
{
private:
    uint32_t input[16];
    uint8_t prev_block_bytes[64];
    uint8_t prev_block_start_pos{0};
    bool is_rfc8439{false};

public:
    ChaCha20();
    ChaCha20(const unsigned char* key, size_t keylen);
    void SetKey(const unsigned char* key, size_t keylen); //!< set key with flexible keylength; 256bit recommended */
    void SetIV(uint64_t iv); // set the 64bit nonce
    void Seek(uint64_t pos); // set the 64bit block counter

    void SetRFC8439Nonce(const std::array<std::byte, 12>& nonce);
    void SeekRFC8439(uint32_t pos);

    /** outputs the keystream of size <bytes> into <c> */
    void Keystream(unsigned char* c, size_t bytes);

    /** enciphers the message <input> of length <bytes> and write the enciphered representation into <output>
     *  Used for encryption and decryption (XOR)
     */
    void Crypt(const unsigned char* input, unsigned char* output, size_t bytes);
};

class FSChaCha20
{
private:
    ChaCha20 c20;
    size_t rekey_interval;
    uint32_t messages_with_key;
    uint64_t rekey_counter;
    std::array<std::byte, FSCHACHA20_KEYLEN> key;
    std::array<std::byte, FSCHACHA20_KEYLEN> rekey_salt;

    void set_nonce()
    {
        std::array<std::byte, 12> nonce;
        WriteLE32(reinterpret_cast<unsigned char*>(nonce.data()), uint32_t{0});
        WriteLE64(reinterpret_cast<unsigned char*>(nonce.data()) + 4, rekey_counter);
        c20.SetRFC8439Nonce(nonce);
    }

public:
    FSChaCha20() = delete;
    FSChaCha20(const std::array<std::byte, FSCHACHA20_KEYLEN>& key,
               const std::array<std::byte, FSCHACHA20_KEYLEN>& rekey_salt,
               size_t rekey_interval)
        : c20{reinterpret_cast<const unsigned char*>(key.data()), FSCHACHA20_KEYLEN},
          rekey_interval{rekey_interval},
          messages_with_key{0},
          rekey_counter{0},
          key{key},
          rekey_salt{rekey_salt}
    {
        assert(rekey_interval > 0);
        set_nonce();
    }

    void Crypt(Span<const std::byte> input, Span<std::byte> output);
};
#endif // BITCOIN_CRYPTO_CHACHA20_H
