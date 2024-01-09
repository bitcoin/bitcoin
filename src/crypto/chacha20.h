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
    /** Expected key length in constructor and SetKey. */
    static constexpr unsigned KEYLEN{32};

    /** Block size (inputs/outputs to Keystream / Crypt should be multiples of this). */
    static constexpr unsigned BLOCKLEN{64};

    /** For safety, disallow initialization without key. */
    ChaCha20Aligned() noexcept = delete;

    /** Initialize a cipher with specified 32-byte key. */
    ChaCha20Aligned(Span<const std::byte> key) noexcept;

    /** Destructor to clean up private memory. */
    ~ChaCha20Aligned();

    /** Set 32-byte key, and seek to nonce 0 and block position 0. */
    void SetKey(Span<const std::byte> key) noexcept;

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
     * Block_counter selects a position to seek to (to byte BLOCKLEN*block_counter). After 256 GiB,
     * the block counter overflows, and nonce.first is incremented.
     */
    void Seek(Nonce96 nonce, uint32_t block_counter) noexcept;

    /** outputs the keystream into out, whose length must be a multiple of BLOCKLEN. */
    void Keystream(Span<std::byte> out) noexcept;

    /** en/deciphers the message <input> and write the result into <output>
     *
     * The size of input and output must be equal, and be a multiple of BLOCKLEN.
     */
    void Crypt(Span<const std::byte> input, Span<std::byte> output) noexcept;
};

/** Unrestricted ChaCha20 cipher. */
class ChaCha20
{
private:
    ChaCha20Aligned m_aligned;
    std::array<std::byte, ChaCha20Aligned::BLOCKLEN> m_buffer;
    unsigned m_bufleft{0};

public:
    /** Expected key length in constructor and SetKey. */
    static constexpr unsigned KEYLEN = ChaCha20Aligned::KEYLEN;

    /** For safety, disallow initialization without key. */
    ChaCha20() noexcept = delete;

    /** Initialize a cipher with specified 32-byte key. */
    ChaCha20(Span<const std::byte> key) noexcept : m_aligned(key) {}

    /** Destructor to clean up private memory. */
    ~ChaCha20();

    /** Set 32-byte key, and seek to nonce 0 and block position 0. */
    void SetKey(Span<const std::byte> key) noexcept;

    /** 96-bit nonce type. */
    using Nonce96 = ChaCha20Aligned::Nonce96;

    /** Set the 96-bit nonce and 32-bit block counter. See ChaCha20Aligned::Seek. */
    void Seek(Nonce96 nonce, uint32_t block_counter) noexcept
    {
        m_aligned.Seek(nonce, block_counter);
        m_bufleft = 0;
    }

    /** en/deciphers the message <in_bytes> and write the result into <out_bytes>
     *
     * The size of in_bytes and out_bytes must be equal.
     */
    void Crypt(Span<const std::byte> in_bytes, Span<std::byte> out_bytes) noexcept;

    /** outputs the keystream to out. */
    void Keystream(Span<std::byte> out) noexcept;
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
