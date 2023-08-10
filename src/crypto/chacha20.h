// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_H
#define BITCOIN_CRYPTO_CHACHA20_H

#include <span.h>

#include <array>
#include <cstddef>
#include <cstdlib>
#include <stdint.h>
#include <utility>

// classes for ChaCha20 256-bit stream cipher developed by Daniel J. Bernstein
// https://cr.yp.to/chacha/chacha-20080128.pdf.
//
// The 128-bit input is here implemented as a 96-bit nonce and a 32-bit block
// counter, as in RFC8439 Section 2.3. When the 32-bit block counter overflows
// the first 32-bit part of the nonce is automatically incremented, making it
// conceptually compatible with variants that use a 64/64 split instead.

/** ChaCha20 cipher that only operates on multiples of 64 bytes. */
class ChaCha20Aligned
{
private:
    uint32_t input[12];

public:
    ChaCha20Aligned();

    /** Initialize a cipher with specified 32-byte key. */
    ChaCha20Aligned(const unsigned char* key32);

    /** Destructor to clean up private memory. */
    ~ChaCha20Aligned();

    /** set 32-byte key. */
    void SetKey32(const unsigned char* key32);

    /** Type for 96-bit nonces used by the Set function below.
     *
     * The first field corresponds to the LE32-encoded first 4 bytes of the nonce, also referred
     * to as the '32-bit fixed-common part' in Example 2.8.2 of RFC8439.
     *
     * The second field corresponds to the LE64-encoded last 8 bytes of the nonce.
     *
     */
    using Nonce96 = std::pair<uint32_t, uint64_t>;

    /** Set the 96-bit nonce and 32-bit block counter.
     *
     * Block_counter selects a position to seek to (to byte 64*block_counter). After 256 GiB, the
     * block counter overflows, and nonce.first is incremented.
     */
    void Seek64(Nonce96 nonce, uint32_t block_counter);

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

    /** Destructor to clean up private memory. */
    ~ChaCha20();

    /** set 32-byte key. */
    void SetKey32(const unsigned char* key32)
    {
        m_aligned.SetKey32(key32);
        m_bufleft = 0;
    }

    /** 96-bit nonce type. */
    using Nonce96 = ChaCha20Aligned::Nonce96;

    /** Set the 96-bit nonce and 32-bit block counter. */
    void Seek64(Nonce96 nonce, uint32_t block_counter)
    {
        m_aligned.Seek64(nonce, block_counter);
        m_bufleft = 0;
    }

    /** outputs the keystream of size <bytes> into <c> */
    void Keystream(unsigned char* c, size_t bytes);

    /** enciphers the message <input> of length <bytes> and write the enciphered representation into <output>
     *  Used for encryption and decryption (XOR)
     */
    void Crypt(const unsigned char* input, unsigned char* output, size_t bytes);
};

/** Forward-secure ChaCha20
 *
 * This implements a stream cipher that automatically transitions to a new stream with a new key
 * and new nonce after a predefined number of encryptions or decryptions.
 *
 * See BIP324 for details.
 */
class FSChaCha20
{
private:
    /** Internal stream cipher. */
    ChaCha20 m_chacha20;

    /** The number of encryptions/decryptions before a rekey happens. */
    const uint32_t m_rekey_interval;

    /** The number of encryptions/decryptions since the last rekey. */
    uint32_t m_chunk_counter{0};

    /** The number of rekey operations that have happened. */
    uint64_t m_rekey_counter{0};

public:
    /** Length of keys expected by the constructor. */
    static constexpr unsigned KEYLEN = 32;

    // No copy or move to protect the secret.
    FSChaCha20(const FSChaCha20&) = delete;
    FSChaCha20(FSChaCha20&&) = delete;
    FSChaCha20& operator=(const FSChaCha20&) = delete;
    FSChaCha20& operator=(FSChaCha20&&) = delete;

    /** Construct an FSChaCha20 cipher that rekeys every rekey_interval Crypt() calls. */
    FSChaCha20(Span<const std::byte> key, uint32_t rekey_interval) noexcept;

    /** Encrypt or decrypt a chunk. */
    void Crypt(Span<const std::byte> input, Span<std::byte> output) noexcept;
};

#endif // BITCOIN_CRYPTO_CHACHA20_H
