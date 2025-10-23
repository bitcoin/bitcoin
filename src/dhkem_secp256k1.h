// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DHKEM_SECP256K1_H
#define BITCOIN_DHKEM_SECP256K1_H

#include <cstdlib>
#include <cstdint>
#include <secp256k1.h>
#include <span>
#include <array>
#include <optional>
#include <vector>
#include <crypto/chacha20poly1305.h>  // for AEADChaCha20Poly1305 and Nonce96

/**
 * secp256k1-based DHKEM for HPKE (Hybrid Public Key Encryption)
 *
 * Provides functions for key pair derivation, serialization, and Diffie-Hellman
 * encapsulation/decapsulation (including authenticated modes) as specified in
 * draft-wahby-cfrg-hpke-kem-secp256k1-01^2.
 */
namespace dhkem_secp256k1 {

static const size_t NSECRET = 32;        //!< Length of KEM shared secret (32 bytes)
static const size_t NENC    = 65;        //!< Length of encapsulated key (ephemeral public key), uncompressed SEC1 (65 bytes)
static const size_t NPK     = 65;        //!< Length of public key serialization, uncompressed (65 bytes)
static const size_t NSK     = 32;        //!< Length of private key serialization (32 bytes)

// Labeled prefix "HPKE-v1" and suite ID for KEM(secp256k1, HKDF-SHA256)
static const unsigned char LABEL_PREFIX[] = {'H','P','K','E','-','v','1'};
static const unsigned char SUITE_ID[]    = {'H','P','K','E', 0x00, 0x16, 0x00, 0x01, 0x00, 0x03}; // e.g., KEM=0x0016, KDF=0x0001, AEAD=0x0003

/**
 * DeriveKeyPair(IKM): Derive a secp256k1 key pair from input keying material.
 *
 * Follows RFC 9180 Section 7.1.3 algorithm with rejection sampling:
 *   - HKDF-Extract with salt="", label="dkp_prk"
 *   - Loop: HKDF-Expand labeled "candidate" until a valid scalar in [1, order-1] is found.
 *   - The bitmask 0xff is applied to the first byte (no effective change for secp256k1).
 *
 * @param ikm        Input keying material (IKM) bytes.
 * @param outPrivKey (output) Derived private key (32 bytes).
 * @param outPubKey  (output) Derived public key (65 bytes, uncompressed format 0x04 || X || Y).
 * @return true on success, false if derivation failed (e.g., after 256 attempts).
 */
bool DeriveKeyPair(std::span<const uint8_t> ikm,
                   std::array<uint8_t, 32>& outPrivKey,
                   std::array<uint8_t, 65>& outPubKey);

/**
 * Encap(pkR): Perform HPKE KEM encapsulation to the recipient's public key (Base mode).
 *
 * Uses an ephemeral key pair provided by the caller to produce:
 *   - enc: the ephemeral public key (65 bytes, uncompressed)
 *   - shared_secret: 32-byte KEM shared secret
 *
 * Implements DHKEM Base mode using secp256k1 ECDH:
 *   dh = x-coordinate of (skE * pkR)^17,
 *   kem_context = enc || pkR,
 *   shared_secret = HKDF-Extract & Expand(dh, "shared secret")^18.
 *
 * Note: Unlike the RFC 9180 interface, the caller must supply the ephemeral private key (`skE`)
 * and corresponding public key (`enc`) rather than the function generating a random ephemeral key.
 *
 * @param pkR  Recipient's public key (65-byte uncompressed).
 * @param skE  Ephemeral private key (32-byte scalar).
 * @param enc  Ephemeral public key corresponding to skE (65-byte uncompressed).
 * @return 32-byte shared secret on success, or std::nullopt on failure.
 */
std::optional<std::array<uint8_t, 32>>
Encap(std::span<const uint8_t> pkR, const std::array<uint8_t, 32>& skE, const std::array<uint8_t, 65>& enc);

/**
 * Decap(enc, skR): Perform HPKE KEM decapsulation using the recipient's private key (Base mode).
 *
 * Given the encapsulated key (ephemeral public key `enc`) and the recipient's private key `skR`,
 * computes the same 32-byte shared secret as Encap(). Returns std::nullopt if input keys are invalid.
 *
 * @param enc  Encapsulated ephemeral public key (65-byte uncompressed).
 * @param skR  Recipient's private key (32-byte scalar).
 * @return 32-byte shared secret on success, or std::nullopt if validation fails.
 */
std::optional<std::array<uint8_t, 32>>
Decap(std::span<const uint8_t> enc, std::span<const uint8_t> skR);

/** Initialize the internal secp256k1 context (must be called before other operations). */
void InitContext();

/**
 * LabeledExpand(prk, label, info, L): HKDF-Expand with a label, per RFC 9180.
 * Constructs the info as: length (2 bytes) || "HPKE-v1" || suite_id || label || info.
 *
 * @param prk   The pseudorandom key (PRK) from a previous HKDF-Extract (32 bytes).
 * @param label A context string label to include (e.g., "info_hash", "key", "base_nonce").
 * @param info  Application-specific info bytes to include (can be empty).
 * @param L     Desired length of output keying material in bytes.
 * @return A vector of length L containing the output keying material.
 */
std::vector<uint8_t> LabeledExpand(const std::vector<uint8_t>& prk,
                                   const std::vector<uint8_t>& label,
                                   const std::vector<uint8_t>& info,
                                   size_t L);

/**
 * LabeledExtract(salt, label, ikm): HKDF-Extract with a label, per RFC 9180.
 * Constructs the input keying material as: "HPKE-v1" || suite_id || label || ikm (using an empty salt if none provided).
 *
 * @param salt  An optional salt value (non-secret), or an empty vector for no salt.
 * @param label A context string label to include in the extract (e.g., "psk_id_hash", "info_hash", "secret").
 * @param ikm   Input keying material bytes.
 * @return A 32-byte pseudorandom key (HKDF-Extract output) as a vector.
 */
std::vector<uint8_t> LabeledExtract(const std::vector<uint8_t>& salt,
                                    const std::vector<uint8_t>& label,
                                    const std::vector<uint8_t>& ikm);

/**
 * AuthEncap(pkR, skS, skE): Authenticated encapsulation using sender's static key.
 *
 * Computes a shared secret using the recipient’s public key (pkR), the sender’s static private key (skS),
 * and an ephemeral key pair (skE with its public key enc). Implements DHKEM in auth mode:
 *   DH1 = x-coordinate of (skE * pkR)
 *   DH2 = x-coordinate of (skS * pkR)
 *   kem_context = enc || pkR || pkS
 *   shared_secret = HKDF-Extract & Expand(DH1 || DH2, "shared secret")
 *
 * @param pkR  Recipient’s public key (65-byte uncompressed).
 * @param skS  Sender’s static private key (32-byte scalar).
 * @param skE  Ephemeral private key (32-byte scalar).
 * @param enc  Ephemeral public key corresponding to skE (65-byte uncompressed).
 * @return 32-byte shared secret on success, or std::nullopt if any input key is invalid.
 */
std::optional<std::array<uint8_t, 32>>
AuthEncap(const std::array<uint8_t, 65>& pkR,
          const std::array<uint8_t, 32>& skS,
          const std::array<uint8_t, 32>& skE,
          const std::array<uint8_t, 65>& enc);

/**
 * AuthDecap(enc, skR, pkS): Authenticated decapsulation using sender’s static public key.
 *
 * Given the encapsulated ephemeral public key (enc), the receiver’s private key (skR),
 * and the sender’s static public key (pkS), computes the 32-byte shared secret matching AuthEncap’s output.
 * Returns std::nullopt if decapsulation fails (e.g., invalid key inputs).
 *
 * @param enc  Encapsulated ephemeral public key from AuthEncap (65-byte uncompressed).
 * @param skR  Receiver’s private key (32-byte scalar).
 * @param pkS  Sender’s static public key (65-byte uncompressed).
 * @return 32-byte shared secret on success, or std::nullopt if decapsulation fails.
 */
std::optional<std::array<uint8_t, 32>>
AuthDecap(const std::array<uint8_t, 65>& enc,
          const std::array<uint8_t, 32>& skR,
          const std::array<uint8_t, 65>& pkS);

/**
 * Seal(key, nonce, aad, plaintext): AEAD encryption with ChaCha20-Poly1305.
 *
 * Encrypts the plaintext using a 32-byte symmetric key, a 96-bit nonce, and optional associated data (AAD).
 * Returns a vector containing the ciphertext followed by the 16-byte authentication tag.
 *
 * @param key       Symmetric key for encryption (32 bytes).
 * @param nonce     Nonce for encryption (96-bit, e.g., 12 bytes).
 * @param aad       Additional authenticated data (AAD) that is authenticated but not encrypted.
 * @param plaintext Plaintext bytes to encrypt.
 * @return A vector containing the ciphertext concatenated with a 16-byte authentication tag.
 */
std::vector<uint8_t> Seal(std::span<const std::byte> key, ChaCha20::Nonce96 nonce,
                          std::span<const std::byte> aad, std::span<const std::byte> plaintext);

/**
 * Open(key, nonce, aad, ciphertext): AEAD decryption with ChaCha20-Poly1305.
 *
 * Decrypts the ciphertext (including its 16-byte authentication tag) using the given key, nonce, and AAD.
 *
 * @param key        Symmetric key for decryption (32 bytes).
 * @param nonce      Nonce used during encryption (96-bit).
 * @param aad        Additional associated data that was provided during encryption.
 * @param ciphertext Ciphertext bytes (including the final 16-byte tag).
 * @return The decrypted plaintext on success, or std::nullopt if authentication fails (tag mismatch or corrupted data).
 */
std::optional<std::vector<uint8_t>> Open(std::span<const std::byte> key, ChaCha20::Nonce96 nonce,
                                        std::span<const std::byte> aad, std::span<const std::byte> ciphertext);

/**
 * Derives a new nonce from a base nonce and a sequence number.
 *
 * The sequence number `seq` is encoded in big-endian form into a buffer of the same length as `base_nonce`,
 * then XORed with `base_nonce` to produce the per-message nonce (per RFC 9180 §5.2).
 *
 * @param base_nonce  The base nonce (e.g., 12-byte IV from KeySchedule). Must be at least sizeof(size_t) bytes long.
 * @param seq         Sequence number (e.g., message number starting from 0).
 * @return A vector of the same length as base_nonce containing the derived nonce for this sequence.
 */
std::vector<uint8_t> ComputeNonce(const std::vector<uint8_t>& base_nonce, size_t seq);

/**
 * Context: HPKE encryption context holding the secrets derived by KeySchedule (RFC 9180).
 *
 * Contains:
 * - `key` : the AEAD encryption key (derived from the shared secret).
 * - `base_nonce` : the base nonce for AEAD (used to derive per-message nonces via ComputeNonce).
 * - `exporter_secret` : the exporter secret (for use with the HPKE exporter interface).
 *
 * *Note:* The sequence number (for encryption operations) is not stored in this struct; it must be managed externally.
 */
struct Context
{
    std::vector<uint8_t> key;
    std::vector<uint8_t> base_nonce;
    std::vector<uint8_t> exporter_secret;

    Context(const std::vector<uint8_t>& _key,
            const std::vector<uint8_t>& _nonce,
            const std::vector<uint8_t>& _exp)
      : key(_key), base_nonce(_nonce), exporter_secret(_exp) {}
};

/**
 * KeySchedule(mode, shared_secret, info): Derive Context (key, base_nonce, exporter_secret).
 *
 * Only Base (0x00) and Auth (0x02) modes are supported.
 * If `mode` is 0x01 or 0x03, this function returns std::nullopt.
 *
 * @param mode          HPKE mode (0x00=Base, 0x02=Auth).
 * @param shared_secret The KEM shared secret (e.g., output of Encap/AuthEncap, 32 bytes).
 * @param info          Application-supplied info bytes (can be empty).
 * @return A Context struct containing the derived `key`, `base_nonce`, and `exporter_secret`,
 *         or std::nullopt on invalid inputs (e.g., unsupported mode).
 */
std::optional<Context> KeySchedule(
    uint8_t mode,
    const std::vector<uint8_t>& shared_secret,
    const std::vector<uint8_t>& info);

} // namespace dhkem_secp256k1

#endif // BITCOIN_DHKEM_SECP256K1_H
