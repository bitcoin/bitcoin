// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA_POLY_AEAD_H
#define BITCOIN_CRYPTO_CHACHA_POLY_AEAD_H

#include <crypto/chacha20.h>

#include <cmath>

static constexpr int CHACHA20_POLY1305_AEAD_KEY_LEN = 32;
static constexpr int CHACHA20_POLY1305_AEAD_AAD_LEN = 3; /* 3 bytes length */
static constexpr int CHACHA20_ROUND_OUTPUT = 64;         /* 64 bytes per round */
static constexpr int AAD_PACKAGES_PER_ROUND = 21;        /* 64 / 3 round down*/

/* A AEAD class for ChaCha20-Poly1305@bitcoin.
 *
 * ChaCha20 is a stream cipher designed by Daniel Bernstein and described in
 * <ref>[http://cr.yp.to/chacha/chacha-20080128.pdf ChaCha20]</ref>. It operates
 * by permuting 128 fixed bits, 128 or 256 bits of key, a 64 bit nonce and a 64
 * bit counter into 64 bytes of output. This output is used as a keystream, with
 * any unused bytes simply discarded.
 *
 * Poly1305 <ref>[http://cr.yp.to/mac/poly1305-20050329.pdf Poly1305]</ref>, also
 * by Daniel Bernstein, is a one-time Carter-Wegman MAC that computes a 128 bit
 * integrity tag given a message and a single-use 256 bit secret key.
 *
 * The chacha20-poly1305@bitcoin combines these two primitives into an
 * authenticated encryption mode. The construction used is based on that proposed
 * for TLS by Adam Langley in
 * <ref>[http://tools.ietf.org/html/draft-agl-tls-chacha20poly1305-03 "ChaCha20
 * and Poly1305 based Cipher Suites for TLS", Adam Langley]</ref>, but differs in
 * the layout of data passed to the MAC and in the addition of encryption of the
 * packet lengths.
 *
 * ==== Detailed Construction ====
 *
 * The chacha20-poly1305@bitcoin cipher requires two 256 bits of key material as
 * output from the key exchange. Each key (K_1 and K_2) are used by two separate
 * instances of chacha20.
 *
 * The instance keyed by K_1 is a stream cipher that is used only to encrypt the 3
 * byte packet length field and has its own sequence number. The second instance,
 * keyed by K_2, is used in conjunction with poly1305 to build an AEAD
 * (Authenticated Encryption with Associated Data) that is used to encrypt and
 * authenticate the entire packet.
 *
 * Two separate cipher instances are used here so as to keep the packet lengths
 * confidential but not create an oracle for the packet payload cipher by
 * decrypting and using the packet length prior to checking the MAC. By using an
 * independently-keyed cipher instance to encrypt the length, an active attacker
 * seeking to exploit the packet input handling as a decryption oracle can learn
 * nothing about the payload contents or its MAC (assuming key derivation,
 * ChaCha20 and Poly1305 are secure).
 *
 * The AEAD is constructed as follows: for each packet, generate a Poly1305 key by
 * taking the first 256 bits of ChaCha20 stream output generated using K_2, an IV
 * consisting of the packet sequence number encoded as an LE uint64 and a ChaCha20
 * block counter of zero. The K_2 ChaCha20 block counter is then set to the
 * little-endian encoding of 1 (i.e. {1, 0, 0, 0, 0, 0, 0, 0}) and this instance
 * is used for encryption of the packet payload.
 *
 * ==== Packet Handling ====
 *
 * When receiving a packet, the length must be decrypted first. When 3 bytes of
 * ciphertext length have been received, they may be decrypted.
 *
 * A ChaCha20 round always calculates 64bytes which is sufficient to crypt 21
 * times a 3 bytes length field (21*3 = 63). The length field sequence number can
 * thus be used 21 times (keystream caching).
 *
 * The length field must be enc-/decrypted with the ChaCha20 keystream keyed with
 * K_1 defined by block counter 0, the length field sequence number in little
 * endian and a keystream position from 0 to 60.
 *
 * Once the entire packet has been received, the MAC MUST be checked before
 * decryption. A per-packet Poly1305 key is generated as described above and the
 * MAC tag calculated using Poly1305 with this key over the ciphertext of the
 * packet length and the payload together. The calculated MAC is then compared in
 * constant time with the one appended to the packet and the packet decrypted
 * using ChaCha20 as described above (with K_2, the packet sequence number as
 * nonce and a starting block counter of 1).
 *
 * Detection of an invalid MAC MUST lead to immediate connection termination.
 *
 * To send a packet, first encode the 3 byte length and encrypt it using K_1 as
 * described above. Encrypt the packet payload (using K_2) and append it to the
 * encrypted length. Finally, calculate a MAC tag and append it.
 *
 * The initiating peer MUST use <code>K_1_A, K_2_A</code> to encrypt messages on
 * the send channel, <code>K_1_B, K_2_B</code> MUST be used to decrypt messages on
 * the receive channel.
 *
 * The responding peer MUST use <code>K_1_A, K_2_A</code> to decrypt messages on
 * the receive channel, <code>K_1_B, K_2_B</code> MUST be used to encrypt messages
 * on the send channel.
 *
 * Optimized implementations of ChaCha20-Poly1305@bitcoin are relatively fast in
 * general, therefore it is very likely that encrypted messages require not more
 * CPU cycles per bytes then the current unencrypted p2p message format
 * (ChaCha20/Poly1305 versus double SHA256).
 *
 * The initial packet sequence numbers are 0.
 *
 * K_2 ChaCha20 cipher instance (payload) must never reuse a {key, nonce} for
 * encryption nor may it be used to encrypt more than 2^70 bytes under the same
 * {key, nonce}.
 *
 * K_1 ChaCha20 cipher instance (length field/AAD) must never reuse a {key, nonce,
 * position-in-keystream} for encryption nor may it be used to encrypt more than
 * 2^70 bytes under the same {key, nonce}.
 *
 * We use message sequence numbers for both communication directions.
 */

class ChaCha20Poly1305AEAD
{
private:
    ChaCha20 m_chacha_main;                                      // payload and poly1305 key-derivation cipher instance
    ChaCha20 m_chacha_header;                                    // AAD cipher instance (encrypted length)
    unsigned char m_aad_keystream_buffer[CHACHA20_ROUND_OUTPUT]; // aad keystream cache
    uint64_t m_cached_aad_seqnr;                                 // aad keystream cache hint

public:
    ChaCha20Poly1305AEAD(const unsigned char* K_1, size_t K_1_len, const unsigned char* K_2, size_t K_2_len);

    explicit ChaCha20Poly1305AEAD(const ChaCha20Poly1305AEAD&) = delete;

    /** Encrypts/decrypts a packet
        seqnr_payload, the message sequence number
        seqnr_aad, the messages AAD sequence number which allows reuse of the AAD keystream
        aad_pos, position to use in the AAD keystream to encrypt the AAD
        dest, output buffer, must be of a size equal or larger then CHACHA20_POLY1305_AEAD_AAD_LEN + payload (+ POLY1305_TAG_LEN in encryption) bytes
        destlen, length of the destination buffer
        src, the AAD+payload to encrypt or the AAD+payload+MAC to decrypt
        src_len, the length of the source buffer
        is_encrypt, set to true if we encrypt (creates and appends the MAC instead of verifying it)
        */
    bool Crypt(uint64_t seqnr_payload, uint64_t seqnr_aad, int aad_pos, unsigned char* dest, size_t dest_len, const unsigned char* src, size_t src_len, bool is_encrypt);

    /** decrypts the 3 bytes AAD data and decodes it into a uint32_t field */
    bool GetLength(uint32_t* len24_out, uint64_t seqnr_aad, int aad_pos, const uint8_t* ciphertext);
};

#endif // BITCOIN_CRYPTO_CHACHA_POLY_AEAD_H
