// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA_POLY_AEAD_H
#define BITCOIN_CRYPTO_CHACHA_POLY_AEAD_H

#include <crypto/chacha20.h>
#include <crypto/poly1305.h>

#include <cmath>

static constexpr int CHACHA20_POLY1305_AEAD_KEY_LEN = 32;
static constexpr int CHACHA20_POLY1305_AEAD_AAD_LEN = 3; /* 3 bytes length */
static constexpr int CHACHA20_POLY1305_AEAD_TAG_LEN = 16; /* 16 bytes poly1305 tag */
static constexpr int CHACHA20_ROUND_OUTPUT = 64;         /* 64 bytes per round */
static constexpr int AAD_PACKAGES_PER_ROUND = 21;        /* 64 / 3 round down*/

/* A AEAD class for ChaCha20-Poly1305@bitcoin.
 *
 * ChaCha20 is a stream cipher designed by Daniel Bernstein and described in
 * <ref>[https://cr.yp.to/chacha/chacha-20080128.pdf ChaCha20]</ref>. It operates
 * by permuting 128 fixed bits, 128 or 256 bits of key, a 64 bit nonce and a 64
 * bit counter into 64 bytes of output. This output is used as a keystream, with
 * any unused bytes simply discarded.
 *
 * Poly1305 <ref>[https://cr.yp.to/mac/poly1305-20050329.pdf Poly1305]</ref>, also
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
The chacha20-poly1305@bitcoin cipher requires two 256 bits of key material as
* output from the key exchange. Each key (K_1 and K_2) are used by two separate
* instances of chacha20.
*
* The instance keyed by K_1 is a stream cipher that is used for the per-message
* metadata, specifically for the poly1305 authentication key as well as for the
* length encryption. The second instance, keyed by K_2, is used to encrypt the
* entire payload.
*
* Two separate cipher instances are used here so as to keep the packet lengths
* confidential (best effort; for passive observing) but not create an oracle for
* the packet payload cipher by decrypting and using the packet length prior to
* checking the MAC. By using an independently-keyed cipher instance to encrypt
* the length, an active attacker seeking to exploit the packet input handling as
* a decryption oracle can learn nothing about the payload contents or its MAC
* (assuming key derivation, ChaCha20 and Poly1305 are secure). Active observers
* can still obtain the message length (ex. active ciphertext bit flipping or
* traffic shemantics analysis)
*
* The AEAD is constructed as follows: generate two ChaCha20 streams, initially
* keyed with K_1 and K_2 and IV of 0 and a block counter of 0 and a sequence
* number 0 as IV. After encrypting 4064 bytes, the following 32 bytes are used to
* re-key the ChaCha20 context.
*
* Byte-level forward security is possible by precomputing 4096 bytes of stream
* output, caching it, resetting the key to the final 32 bytes of the output, and
* then wiping the remaining 4064 bytes of cached data as it gets used.
*
* For each packet, use 3 bytes from the remaining ChaCha20 stream generated using
* K_1 to encrypt the length. Use additional 32 bytes of the same stream to
* generate a Poly1305 key.
*
* If we reach bytes 4064 on the ChaCha20 stream, use the next 32 bytes (byte
* 4065-4096) and set is as the new ChaCha20 key, reset the counter to 0 while
* incrementing the sequence number + 1 and set is as IV (little endian encoding).
*
* For the payload, use the ChaCha20 stream keyed with K_1 and apply the same
* re-key rules.
*
*
* ==== Packet Handling ====
*
* When receiving a packet, the length must be decrypted first. When 3 bytes of
* ciphertext length have been received, they MUST be decrypted.
*
* Once the entire packet has been received, the MAC MUST be checked before
* decryption. A per-packet Poly1305 key is generated as described above and the
* MAC tag is calculated using Poly1305 with this key over the ciphertext of the
* packet length and the payload together. The calculated MAC is then compared in
* constant time with the one appended to the packet and the packet decrypted
* using ChaCha20 as described above (with K_2, the packet sequence number as
* nonce and a starting block counter of 1).
*
* Detection of an invalid MAC MUST lead to immediate connection termination.
*
* To send a packet, first encode the 3 byte length and encrypt it using the
* ChaCha20 stream keyed with K_1 as described above. Encrypt the packet payload
* (using the ChaCha20 stream keyed with K_2) and append it to the encrypted
* length. Finally, calculate a MAC tag and append it.
*/

const int KEYSTREAM_SIZE = 4096;

class ChaCha20ReKey4096 {
private:
    ChaCha20 m_ctx;
    uint64_t m_seqnr{0};
    size_t m_keystream_pos{0};
    unsigned char m_keystream[KEYSTREAM_SIZE] = {0};
public:
    ~ChaCha20ReKey4096();
    void SetKey(const unsigned char* key, size_t keylen);
    void Crypt(const unsigned char* input, unsigned char* output, size_t bytes);
};

class ChaCha20Poly1305AEAD
{
private:
    ChaCha20ReKey4096 m_chacha_main;                                      // payload and poly1305 key-derivation cipher instance
    ChaCha20ReKey4096 m_chacha_header;                                    // AAD cipher instance (encrypted length)

public:
    ChaCha20Poly1305AEAD(const unsigned char* K_1, size_t K_1_len, const unsigned char* K_2, size_t K_2_len);

    explicit ChaCha20Poly1305AEAD(const ChaCha20Poly1305AEAD&) = delete;

    /** Encrypts/decrypts a packet
        dest, output buffer, must be of a size equal or larger then CHACHA20_POLY1305_AEAD_AAD_LEN + payload (+ POLY1305_TAG_LEN in encryption) bytes
        destlen, length of the destination buffer
        src, the AAD+payload to encrypt or the AAD+payload+MAC to decrypt
        src_len, the length of the source buffer
        is_encrypt, set to true if we encrypt (creates and appends the MAC instead of verifying it)
        */
    bool Crypt(unsigned char* dest, size_t dest_len, const unsigned char* src, size_t src_len, bool is_encrypt);

    /** decrypts the 3 bytes AAD data (the packet length) and decodes it into a uint32_t field
        the ciphertext will not be manipulated but the AEAD state changes (can't be called multiple times)
        Ciphertext needs to stay encrypted due to the MAC check that will follow (requires encrypted length)
        */
    bool DecryptLength(uint32_t* len24_out, const uint8_t* ciphertext);
};

#endif // BITCOIN_CRYPTO_CHACHA_POLY_AEAD_H
