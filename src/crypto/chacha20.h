// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_H
#define BITCOIN_CRYPTO_CHACHA20_H

#include <crypto/common.h>
#include <span.h>
#include <support/cleanse.h>

#include <array>
#include <cstddef>
#include <cstdlib>
#include <stdint.h>

static constexpr size_t FSCHACHA20_KEYLEN = 32;         // bytes

// classes for ChaCha20 256-bit stream cipher developed by Daniel J. Bernstein
// https://cr.yp.to/chacha/chacha-20080128.pdf */

/** ChaCha20 cipher that only operates on multiples of 64 bytes. */
class ChaCha20Aligned
{
private:
    uint32_t input[12];
    bool is_rfc8439{false};

public:
    ChaCha20Aligned();

    /** Initialize a cipher with specified 32-byte key. */
    ChaCha20Aligned(const unsigned char* key32);

    /** set 32-byte key. */
    void SetKey32(const unsigned char* key32);

    /** set the 64-bit nonce. */
    void SetIV(uint64_t iv);

    /** set the 64bit block counter (pos seeks to byte position 64*pos). */
    void Seek64(uint64_t pos);

    void SetRFC8439Nonce(const std::array<std::byte, 12>& nonce);
    void SeekRFC8439(uint32_t pos);

    /** outputs the keystream of size <64*blocks> into <c> */
    void Keystream64(unsigned char* c, size_t blocks);

    /** enciphers the message <input> of length <64*blocks> and write the enciphered representation into <output>
     *  Used for encryption and decryption (XOR)
     */
    void Crypt64(const unsigned char* input, unsigned char* output, size_t blocks);
};

/** Unrestricted ChaCha20 cipher. */
class ChaCha20
{
private:
    ChaCha20Aligned m_aligned;
    unsigned char m_buffer[64] = {0};
    unsigned m_bufleft{0};

public:
    ChaCha20() = default;

    /** Initialize a cipher with specified 32-byte key. */
    ChaCha20(const unsigned char* key32) : m_aligned(key32) {}

    /** set 32-byte key. */
    void SetKey32(const unsigned char* key32)
    {
        m_aligned.SetKey32(key32);
        m_bufleft = 0;
    }

    /** set the 64-bit nonce. */
    void SetIV(uint64_t iv) { m_aligned.SetIV(iv); }

    /** set the 64bit block counter (pos seeks to byte position 64*pos). */
    void Seek64(uint64_t pos)
    {
        m_aligned.Seek64(pos);
        m_bufleft = 0;
    }

    void SetRFC8439Nonce(const std::array<std::byte, 12>& nonce) {
        m_aligned.SetRFC8439Nonce(nonce);
        m_bufleft = 0;
    }

    void SeekRFC8439(uint32_t pos) {
        m_aligned.SeekRFC8439(pos);
        m_bufleft = 0;
    }

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
    uint32_t chunk_counter{0};
    std::array<std::byte, FSCHACHA20_KEYLEN> key;

    void set_nonce()
    {
        std::array<std::byte, 12> nonce;
        WriteLE32(reinterpret_cast<unsigned char*>(nonce.data()), uint32_t{0});
        WriteLE64(reinterpret_cast<unsigned char*>(nonce.data()) + 4, chunk_counter / rekey_interval);
        c20.SetRFC8439Nonce(nonce);
    }

public:
    FSChaCha20() = delete;
    FSChaCha20(const std::array<std::byte, FSCHACHA20_KEYLEN>& key,
               size_t rekey_interval)
        : c20{reinterpret_cast<const unsigned char*>(key.data())},
          rekey_interval{rekey_interval},
          key{key}
    {
        assert(rekey_interval > 0);
        set_nonce();
    }

    ~FSChaCha20()
    {
        memory_cleanse(key.data(), FSCHACHA20_KEYLEN);
    }

    void Crypt(Span<const std::byte> input, Span<std::byte> output);
};
#endif // BITCOIN_CRYPTO_CHACHA20_H
